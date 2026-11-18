#ifndef IRRIGATION_ESP32_BOARD_CONFIG_H
#define IRRIGATION_ESP32_BOARD_CONFIG_H

#include "HardwareDrivers.h"
#include "HX711TankSensor.h"
#include "DualSourcePowerSensor.h"

namespace board {

// ESP32 DevKit dùng ESP32-WROOM-32, không dùng sơ đồ này cho mọi ESP32.
constexpr int soilPin = 34;
constexpr int sdaPin = 21;
constexpr int sclPin = 22;
constexpr uint8_t solarAddress = 0x40;
constexpr uint8_t batteryAddress = 0x41;

// Điền hai số ADC khi đo đất khô và đủ ẩm.
constexpr int soilDryAdc = 0;
constexpr int soilWetAdc = 0;

inline irrigation::SensorWiring sensors() {
    return {{16, false, true}, {17, false, true}, {18, false, true}, 100};
}
inline irrigation::ActuatorWiring actuators() {
    // P1, VG, VP, P2, yêu cầu dự phòng. Relay tương thích logic 3,3V, kích LOW.
    return {{19, false}, {23, false}, {25, false}, {26, false}, {27, false}, 300};
}
inline irrigation::HX711Config scale() {
    // DT=32, SCK=33, zeroRaw và countsPerKg cần hiệu chuẩn trên cân thật.
    return {32, 33, 0, 0.0f, 500};
}
inline irrigation::Config control() {
    irrigation::Config config;
    config.useTankSwitches = true;
    config.useTankMass = true;
    config.useBatteryVoltage = true;
    // Profile mẫu ắc quy chì 12V, cần hiệu chỉnh theo tải và tài liệu ắc quy.
    config.batteryStopV = 11.8f;
    config.batteryResumeV = 12.4f;
    config.solarFillVoltageV = 13.5f;
    // Điền các giá trị đo trên bể thực tế.
    config.emptyTankMassKg = 0;
    config.tankLitersPerMeter = 0;
    config.tankBottomToOutletM = 0;
    config.gravityStartHeadM = 0;
    config.minGravityHeadM = 0;
    return config;
}

inline irrigation::SourceLimits sourceLimits() {
    irrigation::SourceLimits limits;
    limits.batteryDischargeA = 1.2f;
    limits.maxBusPowerW = 15.0f;
    limits.maxSolarPowerW = 10.0f;
    limits.solarEfficiency = 0.8f;
    return limits;
}

}  // namespace board
#endif
