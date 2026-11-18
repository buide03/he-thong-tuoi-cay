#include "DualSourcePowerSensor.h"
#include <cassert>
#include <cmath>
#include <limits>

using namespace irrigation;

class Meter final : public ElectricalSensor {
public:
    ElectricalReadings value;
    bool ok = true;
    bool read(ElectricalReadings& readings) override {
        readings = value;
        return ok;
    }
};

int main() {
    Meter solar;
    Meter battery;
    solar.value.voltageV = 18;
    solar.value.currentA = 0.5f;
    solar.value.powerW = 9;
    battery.value.voltageV = 12.5f;
    battery.value.currentA = -0.3f;
    battery.value.powerW = -3.75f;
    DualSourcePowerSensor power(solar, battery);
    PowerReadings readings;
    assert(power.read(readings));
    assert(readings.hasSourceReadings && readings.batteryCurrentA == -0.3f);
    assert(std::fabs(readings.pvAvailableW - 7.2f) < 0.0001f);
    assert(readings.pvAvailableW + readings.batteryAvailableW <= 15.0001f);
    assert(readings.backupAvailableW == 0 && readings.batteryPct == 0);

    Config config;
    config.useBatteryVoltage = true;
    Controller controller(config);
    Inputs in{40, 1.7f, 14, 0, 0, 15, 0, true};
    in.hasSourceReadings = true;
    in.batteryVoltageV = 12.5f;
    Outputs out = controller.update(in);
    assert(out.p1 && out.p2 && out.vp);
    in.batteryVoltageV = 12.0f;
    assert(controller.update(in).p2);  // Duy trì đến ngưỡng dừng.
    in.batteryVoltageV = 11.8f;
    out = controller.update(in);
    assert(!out.p1 && !out.p2);
    in.batteryVoltageV = 12.3f;
    assert(!controller.update(in).p2);
    in.batteryVoltageV = 12.4f;
    assert(controller.update(in).p2);
    in.hasSourceReadings = false;
    assert(controller.update(in).inputFault);

    Controller solarFill(config);
    in = Inputs{60, 1.7f, 14, 0, 7.2f, 0, 0, true};
    in.hasSourceReadings = true;
    in.batteryVoltageV = 13.5f;
    assert(solarFill.update(in).p1);
    in.batteryVoltageV = 13.4f;
    assert(!solarFill.update(in).p1);

    solar.ok = false;
    assert(!power.read(readings) && !power.lastReadings().hasSourceReadings);
    solar.ok = true;
    battery.ok = false;
    assert(!power.read(readings));
    battery.ok = true;
    solar.value.powerW = -2;
    assert(power.read(readings) && readings.pvAvailableW == 0);
    battery.value.voltageV = std::numeric_limits<float>::quiet_NaN();
    assert(!power.read(readings));
    SourceLimits bad;
    bad.solarEfficiency = 1.1f;
    DualSourcePowerSensor invalid(solar, battery, bad);
    assert(!invalid.read(readings));
}
