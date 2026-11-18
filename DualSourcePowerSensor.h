#ifndef IRRIGATION_DUAL_SOURCE_POWER_SENSOR_H
#define IRRIGATION_DUAL_SOURCE_POWER_SENSOR_H

#include "HardwareDrivers.h"

namespace irrigation {

struct SourceLimits {
    float batteryDischargeA = 1.2f;
    float maxBusPowerW = 15.0f;
    float maxSolarPowerW = 10.0f;
    float solarEfficiency = 0.8f;
};

class DualSourcePowerSensor final : public PowerSensor {
public:
    DualSourcePowerSensor(ElectricalSensor& solar, ElectricalSensor& battery,
                          const SourceLimits& limits = SourceLimits{});
    bool read(PowerReadings& readings) override;
    const PowerReadings& lastReadings() const { return last_; }
private:
    ElectricalSensor& solar_;
    ElectricalSensor& battery_;
    SourceLimits limits_;
    PowerReadings last_;
};

}  // namespace irrigation
#endif
