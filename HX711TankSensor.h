#ifndef IRRIGATION_HX711_TANK_SENSOR_H
#define IRRIGATION_HX711_TANK_SENSOR_H

#include "HardwareDrivers.h"
#include <HX711.h>

namespace irrigation {

struct HX711Config {
    int dataPin;
    int clockPin;
    long zeroRaw;         // Giá trị ADC khi cân không đỡ bể.
    float countsPerKg;   // (ADC có quả cân - zeroRaw) / khối lượng quả cân (kg).
    uint32_t maxAgeMs;
};

class HX711TankSensor final : public NumberSensor {
public:
    HX711TankSensor(BoardIo& io, const HX711Config& config);
    bool begin();
    bool read(float& massKg) override;

private:
    BoardIo& io_;
    HX711Config config_;
    HX711 device_;
    bool ready_ = false;
    bool hasSample_ = false;
    float lastKg_ = 0.0f;
    uint32_t sampledAtMs_ = 0;
};

}  // namespace irrigation
#endif
