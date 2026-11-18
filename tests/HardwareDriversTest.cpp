#include "HardwareDrivers.h"

#include <cassert>
#include <map>
#include <vector>

using namespace irrigation;

class TestIo final : public BoardIo {
public:
    std::map<int, bool> levels;
    std::vector<int> writes;
    uint32_t time = 0;
    int failedPin = -1;
    bool input(int pin, bool) override { return pin >= 0; }
    bool output(int pin, bool high) override { return write(pin, high); }
    bool read(int pin, bool& high) override {
        high = levels[pin];
        return pin != failedPin;
    }
    bool write(int pin, bool high) override {
        writes.push_back(pin);
        if (pin < 0 || pin == failedPin) {
            return false;
        }
        levels[pin] = high;
        return true;
    }
    uint32_t nowMs() override { return time; }
};

class Soil final : public NumberSensor {
public:
    bool read(float& value) override { value = 40; return true; }
};
class Power final : public PowerSensor {
public:
    bool ok = true;
    bool read(PowerReadings& value) override {
        value.batteryPct = 50;
        value.batteryAvailableW = 20;
        return ok;
    }
};

int main() {
    TestIo io;
    Soil soil;
    Power power;
    SensorWiring sw = {{1, false, true}, {2, false, true}, {3, false, true}, 100};
    ActuatorWiring relays = {{4, false}, {5, false}, {6, false},
                               {7, false}, {8, false}, 300};
    HardwareSensors sensors(io, sw, soil, power);
    RelayActuators actuators(io, relays);
    Config config;
    config.useTankSwitches = true;
    IrrigationSystem system(sensors, actuators, config);
    io.levels[1] = false;  // Active LOW: low switch wet.
    io.levels[2] = true;   // Full switch dry.
    io.levels[3] = false;  // Source wet.
    assert(sensors.begin() && actuators.begin());
    assert(io.levels[4] && io.levels[7]);  // Active LOW outputs start OFF.
    assert(system.tick().requested.inputFault);
    io.time = 100;
    auto result = system.tick();
    assert(result.requested.p1 && result.requested.p2 && result.requested.vp);
    assert(result.actuation == ActuationStatus::Pending);
    assert(io.levels[7] && !io.levels[6]);  // P2 waits while VP opens.
    io.time = 400;
    assert(system.tick().actuation == ActuationStatus::Applied);
    assert(!io.levels[7]);  // P2 ON after settling.

    io.levels[2] = false;  // Tank becomes full.
    assert(system.tick().requested.inputFault);  // Stop during debounce.
    assert(io.levels[4] && io.levels[7]);
    io.time = 500;
    result = system.tick();
    assert(result.requested.vg && !result.requested.p1 && !result.requested.p2);
    assert(!io.levels[5] && io.levels[6] && io.levels[7]);

    io.levels[1] = true;  // Full wet but low dry: impossible combination.
    system.tick();
    io.time = 600;
    assert(system.tick().requested.inputFault);

    io.levels[2] = true;  // Both switches dry: refill, no watering.
    system.tick();
    io.time = 700;
    result = system.tick();
    assert(result.requested.p1 && !result.requested.p2 && !result.requested.vg);

    io.levels[3] = true;  // Source dries up.
    system.tick();
    io.time = 800;
    result = system.tick();
    assert(result.requested.bothWaterSourcesUnavailable && !result.requested.p1);

    power.ok = false;
    assert(system.tick().requested.inputFault);

    io.failedPin = 5;
    Outputs commands;
    commands.vg = true;
    io.writes.clear();
    assert(!actuators.apply(commands));
    // Emergency stop still attempts every output after one pin fails.
    assert(io.writes.back() == 8);
    assert(io.levels[4] && io.levels[6] && io.levels[7]);
    io.failedPin = -1;

    // Timer subtraction also works across uint32_t rollover.
    io.time = 0xFFFFFFF0u;
    commands = Outputs{};
    commands.vp = true;
    commands.p2 = true;
    assert(actuators.apply(commands) && actuators.pending());
    io.time = 284;
    assert(actuators.apply(commands) && !actuators.pending());

    // A full switch overrides a mass-based decision to refill.
    config.useTankMass = true;
    config.emptyTankMassKg = 2;
    config.tankLitersPerMeter = 8;
    config.tankBottomToOutletM = 0.5f;
    config.minGravityHeadM = 0.75f;
    config.gravityStartHeadM = 1;
    Controller weighed(config);
    Inputs in{40, 0, 0, 50, 0, 20, 0, true, 2.4f};
    in.upperLowWet = true;
    in.upperFullWet = true;
    in.sourceWet = true;
    assert(!weighed.update(in).p1);
    // Low switch dry stops watering even if the mass says plenty of water.
    in.upperFullWet = false;
    in.upperLowWet = false;
    in.upperTankMassKg = 6;
    const Outputs out = weighed.update(in);
    assert(!out.p2 && !out.vg && !out.vp);
}
