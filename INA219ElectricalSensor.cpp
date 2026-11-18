#include "INA219ElectricalSensor.h"
#include <cmath>

namespace irrigation {

INA219ElectricalSensor::INA219ElectricalSensor(uint8_t address) : device_(address) {}

bool INA219ElectricalSensor::begin(TwoWire& wire, float maxVoltageV, float maxCurrentA) {
    ready_ = false;
    if (!std::isfinite(maxVoltageV) || maxVoltageV <= 0 || maxVoltageV > 26 ||
        !std::isfinite(maxCurrentA) || maxCurrentA <= 0 || maxCurrentA > 2) {
        return false;
    }
    if (!device_.begin(&wire)) {
        return false;
    }
    maxVoltageV_ = maxVoltageV;
    maxCurrentA_ = maxCurrentA;
    ready_ = device_.success();
    return ready_;
}

bool INA219ElectricalSensor::read(ElectricalReadings& readings) {
    readings = ElectricalReadings{};
    if (!ready_) {
        return false;
    }
    ElectricalReadings measured;
    measured.voltageV = device_.getBusVoltage_V();
    if (!device_.success()) {
        return false;
    }
    measured.currentA = device_.getCurrent_mA() / 1000.0f;
    if (!device_.success()) {
        return false;
    }
    if (!std::isfinite(measured.voltageV) || measured.voltageV < 0 ||
        measured.voltageV > maxVoltageV_ || !std::isfinite(measured.currentA) ||
        std::fabs(measured.currentA) > maxCurrentA_) {
        return false;
    }
    measured.powerW = measured.voltageV * measured.currentA;
    readings = measured;
    return true;
}

}  // namespace irrigation
