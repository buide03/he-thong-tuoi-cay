#include "IrrigationSystem.h"

#include <iostream>

class SimulatedSensors final : public irrigation::SensorDriver {
public:
    bool read(irrigation::Inputs& readings) override {
        if (!online) {
            return false;
        }
        readings.soilPct = soilPct;
        readings.upperTankL = upperTankL;
        readings.lowerSourceL = 14.0f;
        readings.batteryPct = 50.0f;
        readings.pvAvailableW = 0.0f;
        readings.batteryAvailableW = 20.0f;
        readings.backupAvailableW = 0.0f;
        return true;
    }

    float soilPct = 42.0f;
    float upperTankL = 4.5f;
    bool online = true;
};

class ConsoleActuators final : public irrigation::ActuatorDriver {
public:
    bool apply(const irrigation::Outputs& commands) override {
        std::cout << "P1=" << commands.p1
                  << " VG=" << commands.vg
                  << " VP=" << commands.vp
                  << " P2=" << commands.p2
                  << " backup=" << commands.requestBackup
                  << " inputFault=" << commands.inputFault << '\n';
        return static_cast<bool>(std::cout);
    }
};

int main() {
    SimulatedSensors sensors;
    ConsoleActuators actuators;
    irrigation::IrrigationSystem system(sensors, actuators);

    const irrigation::CycleResult gravity = system.tick();
    sensors.upperTankL = 1.7f;
    const irrigation::CycleResult pressure = system.tick();
    sensors.online = false;
    const irrigation::CycleResult fault = system.tick();

    using irrigation::ActuationStatus;
    return gravity.actuation == ActuationStatus::Applied &&
           pressure.actuation == ActuationStatus::Applied &&
           fault.actuation == ActuationStatus::Applied ? 0 : 1;
}
