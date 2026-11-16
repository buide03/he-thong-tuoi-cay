#include "IrrigationController.h"

#include <cmath>

namespace irrigation {

Inputs::Inputs(float soilPct, float upperTankL, float lowerSourceL, float batteryPct,
               float pvAvailableW, float batteryAvailableW, float backupAvailableW,
               bool sensorsOk, float upperTankMassKg)
    : soilPct(soilPct), upperTankL(upperTankL), lowerSourceL(lowerSourceL),
      batteryPct(batteryPct), pvAvailableW(pvAvailableW), batteryAvailableW(batteryAvailableW),
      backupAvailableW(backupAvailableW), sensorsOk(sensorsOk), upperTankMassKg(upperTankMassKg) {}

Controller::Controller(const Config& config) : config_(config) {}

Outputs Controller::update(const Inputs& in) {
    // 1. Các bơm mặc định tắt và các van mặc định đóng.
    Outputs out;

    float upperTankL = in.upperTankL;
    // So sánh bằng lít ở chế độ cũ và bằng mét cột nước khi dùng cân.
    float gravityLevel = upperTankL;
    float gravityStart = config_.tankFullL;
    float gravityStop = config_.gravityStopL;
    bool tankCalibrationOk = true;

    if (config_.useTankMass) {
        tankCalibrationOk = validPower(config_.emptyTankMassKg) &&
                            std::isfinite(config_.tankLitersPerMeter) &&
                            config_.tankLitersPerMeter > 0.0f &&
                            std::isfinite(config_.tankBottomToOutletM) &&
                            std::isfinite(config_.gravityStartHeadM) &&
                            std::isfinite(config_.minGravityHeadM) &&
                            config_.minGravityHeadM > 0.0f &&
                            config_.gravityStartHeadM > config_.minGravityHeadM;
        if (tankCalibrationOk) {
            // Với nước, 1 kg xấp xỉ 1 L. Đổi thể tích thành độ sâu nước.
            upperTankL = in.upperTankMassKg - config_.emptyTankMassKg;
            const float waterDepthM = upperTankL / config_.tankLitersPerMeter;
            const float gravityHeadM = config_.tankBottomToOutletM + waterDepthM;
            tankCalibrationOk = std::isfinite(gravityHeadM);
            gravityLevel = gravityHeadM;
            gravityStart = config_.gravityStartHeadM;
            gravityStop = config_.minGravityHeadM;
        }
    }

    if (!tankCalibrationOk || !valid(in, upperTankL)) {
        resetState();
        out.inputFault = true;
        return out;
    }

    // 2. Dùng trạng thái lượt trước để chọn ngưỡng bắt đầu hoặc dừng.
    if (wateringNeeded_) {
        wateringNeeded_ = in.soilPct < config_.soilStopPct;
    } else {
        wateringNeeded_ = in.soilPct <= config_.soilStartPct;
    }

    if (gravityPreferred_) {
        gravityPreferred_ = gravityLevel > gravityStop;
    } else {
        gravityPreferred_ = gravityLevel >= gravityStart;
    }

    if (tankUsable_) {
        tankUsable_ = upperTankL > config_.tankIrrigationStopL;
    } else {
        tankUsable_ = upperTankL >= config_.tankIrrigationStartL;
    }

    if (sourceUsable_) {
        sourceUsable_ = in.lowerSourceL > config_.sourceStopL;
    } else {
        sourceUsable_ = in.lowerSourceL >= config_.sourceStartL;
    }

    if (batteryUsable_) {
        batteryUsable_ = in.batteryPct > config_.batteryStopPct;
    } else {
        batteryUsable_ = in.batteryPct >= config_.batteryResumePct;
    }

    if (tankUsable_) {
        emergencyRefill_ = false;
    } else if (wateringNeeded_) {
        emergencyRefill_ = true;
    }

    out.criticalSoil = in.soilPct <= config_.soilCriticalPct;
    out.bothWaterSourcesUnavailable = wateringNeeded_ && !tankUsable_ && !sourceUsable_;

    float primaryW = in.pvAvailableW;
    if (batteryUsable_) {
        primaryW += in.batteryAvailableW;
    }
    const float fillW = config_.controllerW + config_.fillPumpW;

    // 3. TH4a–TH4c: ưu tiên nạp bể. P2 và hai van giữ lệnh mặc định.
    if (emergencyRefill_) {
        if (!sourceUsable_) {
            return out;  // TH4c: nguồn dưới thiếu nước.
        }

        if (enough(primaryW, fillW)) {
            out.p1 = true;  // TH4a: đủ điện để nạp bể.
        } else {
            out.requestBackup = true;  // TH4b: yêu cầu nguồn dự phòng.
            if (enough(primaryW + in.backupAvailableW, fillW)) {
                out.p1 = true;
            }
        }
        return out;
    }

    // 4. TH1–TH2: chọn đường tưới rồi cộng công suất của đường đã chọn.
    float requiredW = config_.controllerW;
    if (wateringNeeded_ && tankUsable_) {
        if (gravityPreferred_) {
            if (enough(primaryW, requiredW + config_.gravityValveW)) {
                out.vg = true;
                requiredW += config_.gravityValveW;
            }
        } else if (enough(primaryW, requiredW + config_.pressurePathW)) {
            out.vp = true;
            out.p2 = true;
            requiredW += config_.pressurePathW;
        }
    }

    // 5. Xét thêm P1 để nạp bể trong TH2 hoặc tận dụng PV trong TH3.
    const float requiredWithP1W = requiredW + config_.fillPumpW;
    bool wantFill = false;
    if (wateringNeeded_ && !gravityPreferred_) {
        wantFill = true;
    } else if (in.batteryPct >= config_.batteryFullPct &&
               enough(in.pvAvailableW, requiredWithP1W)) {
        wantFill = true;
    }

    const bool tankHasSpace = upperTankL < config_.tankFullL;
    if (wantFill && sourceUsable_ && tankHasSpace && enough(primaryW, requiredWithP1W)) {
        out.p1 = true;
    }
    return out;
}

bool Controller::enough(float availableW, float requiredW) {
    constexpr float roundingToleranceW = 0.0001f;
    return availableW + roundingToleranceW >= requiredW;
}

bool Controller::inRange(float value, float max) {
    return std::isfinite(value) && value >= 0.0f && value <= max;
}

bool Controller::validPower(float value) {
    return std::isfinite(value) && value >= 0.0f;
}

bool Controller::valid(const Inputs& in, float upperTankL) const {
    if (!in.sensorsOk || !inRange(in.soilPct, 100.0f)) {
        return false;
    }
    if (!inRange(upperTankL, config_.tankCapacityL) || !validPower(in.lowerSourceL)) {
        return false;
    }
    if (!inRange(in.batteryPct, 100.0f)) {
        return false;
    }
    return validPower(in.pvAvailableW) && validPower(in.batteryAvailableW) &&
           validPower(in.backupAvailableW);
}

void Controller::resetState() {
    wateringNeeded_ = false;
    gravityPreferred_ = false;
    tankUsable_ = false;
    sourceUsable_ = false;
    batteryUsable_ = false;
    emergencyRefill_ = false;
}

}  // namespace irrigation
