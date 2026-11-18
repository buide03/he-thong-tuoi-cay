#ifndef IRRIGATION_INA219_ELECTRICAL_SENSOR_H
#define IRRIGATION_INA219_ELECTRICAL_SENSOR_H

#include "HardwareDrivers.h"
#include <Adafruit_INA219.h>

namespace irrigation {

class INA219ElectricalSensor final : public ElectricalSensor {
public:
    // Hiệu chuẩn thư viện mặc định 32V/2A dành cho điện trở shunt 0,1 ohm.
    // Giới hạn điện áp bus đầu vào của INA219 là 26V.
    explicit INA219ElectricalSensor(uint8_t address = 0x40);
    bool begin(TwoWire& wire, float maxVoltageV = 26.0f, float maxCurrentA = 2.0f);
    bool read(ElectricalReadings& readings) override;

private:
    Adafruit_INA219 device_;
    bool ready_ = false;
    float maxVoltageV_ = 0.0f;
    float maxCurrentA_ = 0.0f;
};

}  // namespace irrigation
#endif
