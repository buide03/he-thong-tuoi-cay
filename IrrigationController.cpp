#include "IrrigationController.h"

#include <cmath>

namespace irrigation {

Controller::Controller(const Config& config) : config_(config) {}

Outputs Controller::update(const Inputs& in) {
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

bool Controller::valid(const Inputs& in) const {
    return in.sensorsOk && inRange(in.soilPct, 100.0f) &&
           inRange(in.upperTankL, config_.tankCapacityL) &&
           validPower(in.lowerSourceL) && inRange(in.batteryPct, 100.0f) &&
           validPower(in.pvAvailableW) && validPower(in.batteryAvailableW) &&
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
