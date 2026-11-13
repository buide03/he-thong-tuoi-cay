#ifndef IRRIGATION_CONTROLLER_H
#define IRRIGATION_CONTROLLER_H

#include <cmath>

namespace irrigation {

struct Config {
    // Gia tri khoi dau de chay thu. Hieu chinh theo cam bien va tai thuc.
    float soilStartPct = 50.0f;
    float soilStopPct = 55.0f;
    float soilCriticalPct = 25.0f;
    float tankCapacityL = 5.0f;
    float tankFullL = 4.5f;
    float gravityStopL = 4.0f;
    float tankIrrigationStartL = 1.2f;
    float tankIrrigationStopL = 0.5f;
    float sourceStartL = 0.2f;
    float sourceStopL = 0.1f;
    float batteryFullPct = 99.5f;
    float batteryStopPct = 20.0f;
    float batteryResumePct = 22.0f;
    float controllerW = 0.5f;
    float fillPumpW = 5.6f;
    float gravityValveW = 1.0f;
    float pressurePathW = 6.0f;  // P2 + VP
};

struct Inputs {
    // Doc tat ca gia tri truoc khi tinh lenh cho chu ky hien tai.
    float soilPct;
    float upperTankL;
    float lowerSourceL;
    float batteryPct;
    float pvAvailableW;
    float batteryAvailableW;  // Cong suat xa toi da BMS cho phep tai bus DC.
    float backupAvailableW;   // Bang 0 cho den khi nguon du phong san sang.
    bool sensorsOk;           // Adapter dat false khi mat loi doc/du lieu qua han.
};

struct Outputs {
    bool p1 = false;  // Nap be cao.
    bool vg = false;  // Tuoi trong luc.
    bool vp = false;  // Van nhanh tuoi bang bom.
    bool p2 = false;  // Bom tuoi.
    bool requestBackup = false;
    bool criticalSoil = false;
    bool bothWaterSourcesUnavailable = false;
    bool inputFault = false;
};

class Controller {
public:
    explicit Controller(const Config& config = Config{}) : config_(config) {}

    Outputs update(const Inputs& in) {
        Outputs out;
        if (!valid(in)) {
            resetState();
            out.inputFault = true;
            return out;
        }

        wateringNeeded_ = wateringNeeded_ ? in.soilPct < config_.soilStopPct
                                          : in.soilPct <= config_.soilStartPct;
        gravityPreferred_ = gravityPreferred_ ? in.upperTankL > config_.gravityStopL
                                               : in.upperTankL >= config_.tankFullL;
        tankUsable_ = tankUsable_ ? in.upperTankL > config_.tankIrrigationStopL
                                  : in.upperTankL >= config_.tankIrrigationStartL;
        sourceUsable_ = sourceUsable_ ? in.lowerSourceL > config_.sourceStopL
                                      : in.lowerSourceL >= config_.sourceStartL;
        batteryUsable_ = batteryUsable_ ? in.batteryPct > config_.batteryStopPct
                                        : in.batteryPct >= config_.batteryResumePct;
        emergencyRefill_ = !tankUsable_ && (emergencyRefill_ || wateringNeeded_);

        out.criticalSoil = in.soilPct <= config_.soilCriticalPct;
        out.bothWaterSourcesUnavailable = wateringNeeded_ && !tankUsable_ && !sourceUsable_;

        const float primaryW = in.pvAvailableW + (batteryUsable_ ? in.batteryAvailableW : 0.0f);
        const float fillW = config_.controllerW + config_.fillPumpW;

        if (emergencyRefill_) {
            // Khoa P2 cho den khi be cao dat nguong tuoi lai.
            if (sourceUsable_) {
                out.requestBackup = !enough(primaryW, fillW);
                const float availableW = primaryW +
                    (out.requestBackup ? in.backupAvailableW : 0.0f);
                out.p1 = enough(availableW, fillW);
            }
            return out;
        }

        float requiredW = config_.controllerW;
        if (wateringNeeded_ && tankUsable_) {
            if (gravityPreferred_ && enough(primaryW, requiredW + config_.gravityValveW)) {
                out.vg = true;
                requiredW += config_.gravityValveW;
            } else if (!gravityPreferred_ && enough(primaryW, requiredW + config_.pressurePathW)) {
                out.vp = true;
                out.p2 = true;
                requiredW += config_.pressurePathW;
            }
        }

        // Pin day va PV du cho tai hien tai cung P1 thi nap bang PV.
        const bool solarSurplus = in.batteryPct >= config_.batteryFullPct &&
                                  enough(in.pvAvailableW, requiredW + config_.fillPumpW);
        const bool wantFill = (wateringNeeded_ && !gravityPreferred_) || solarSurplus;
        out.p1 = wantFill && sourceUsable_ && in.upperTankL < config_.tankFullL &&
                 enough(primaryW, requiredW + config_.fillPumpW);
        return out;
    }

private:
    static bool enough(float availableW, float requiredW) {
        constexpr float roundingToleranceW = 0.0001f;
        return availableW + roundingToleranceW >= requiredW;
    }

    static bool inRange(float value, float max) {
        return std::isfinite(value) && value >= 0.0f && value <= max;
    }

    static bool validPower(float value) {
        return std::isfinite(value) && value >= 0.0f;
    }

    bool valid(const Inputs& in) const {
        return in.sensorsOk && inRange(in.soilPct, 100.0f) &&
               inRange(in.upperTankL, config_.tankCapacityL) &&
               validPower(in.lowerSourceL) && inRange(in.batteryPct, 100.0f) &&
               validPower(in.pvAvailableW) && validPower(in.batteryAvailableW) &&
               validPower(in.backupAvailableW);
    }

    void resetState() {
        wateringNeeded_ = false;
        gravityPreferred_ = false;
        tankUsable_ = false;
        sourceUsable_ = false;
        batteryUsable_ = false;
        emergencyRefill_ = false;
    }

    Config config_;
    bool wateringNeeded_ = false;
    bool gravityPreferred_ = false;
    bool tankUsable_ = false;
    bool sourceUsable_ = false;
    bool batteryUsable_ = false;
    bool emergencyRefill_ = false;
};

}  // namespace irrigation

#endif
