#include "DualSourcePowerSensor.h"
#include <algorithm>
#include <cmath>

namespace irrigation {

DualSourcePowerSensor::DualSourcePowerSensor(ElectricalSensor& solar,
                                             ElectricalSensor& battery,
                                             const SourceLimits& limits)
    : solar_(solar), battery_(battery), limits_(limits) {}

bool DualSourcePowerSensor::read(PowerReadings& readings) {
    readings = PowerReadings{};
    last_ = PowerReadings{};
    if (!std::isfinite(limits_.batteryDischargeA) || limits_.batteryDischargeA <= 0 ||
        !std::isfinite(limits_.maxBusPowerW) || limits_.maxBusPowerW <= 0 ||
        !std::isfinite(limits_.maxSolarPowerW) || limits_.maxSolarPowerW < 0 ||
        !std::isfinite(limits_.solarEfficiency) || limits_.solarEfficiency <= 0 ||
        limits_.solarEfficiency > 1) {
        return false;
    }
    ElectricalReadings solar;
    ElectricalReadings battery;
    if (!solar_.read(solar) || !battery_.read(battery)) {
        return false;
    }
    if (!std::isfinite(solar.voltageV) || solar.voltageV < 0 ||
        !std::isfinite(solar.currentA) || !std::isfinite(solar.powerW) ||
        !std::isfinite(battery.voltageV) || battery.voltageV <= 0 ||
        !std::isfinite(battery.currentA) || !std::isfinite(battery.powerW)) {
        return false;
    }
    readings.hasSourceReadings = true;
    readings.solarVoltageV = solar.voltageV;
    readings.solarCurrentA = solar.currentA;
    readings.solarPowerW = solar.powerW;
    readings.batteryVoltageV = battery.voltageV;
    readings.batteryCurrentA = battery.currentA;
    readings.batteryPowerW = battery.powerW;
    // Ngân sách nguồn có giới hạn thiết kế; đo PV là phần đóng góp quan sát được.
    const float observedSolarW = std::max(0.0f, solar.powerW) * limits_.solarEfficiency;
    readings.pvAvailableW = std::min(observedSolarW,
        std::min(limits_.maxSolarPowerW, limits_.maxBusPowerW));
    const float batteryBudgetW = battery.voltageV * limits_.batteryDischargeA;
    readings.batteryAvailableW = std::min(batteryBudgetW,
        limits_.maxBusPowerW - readings.pvAvailableW);
    // batteryPct không được ước tính từ INA219. Profile dùng useBatteryVoltage.
    readings.batteryPct = 0;
    readings.backupAvailableW = 0;
    last_ = readings;
    return true;
}

}  // namespace irrigation
