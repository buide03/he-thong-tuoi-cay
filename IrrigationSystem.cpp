#include "IrrigationSystem.h"

namespace irrigation {

IrrigationSystem::IrrigationSystem(SensorDriver& sensors, ActuatorDriver& actuators,
                                   const Config& config)
    : sensors_(sensors), actuators_(actuators), controller_(config) {}

CycleResult IrrigationSystem::tick() {
    Inputs readings{};
    readings.sensorsOk = sensors_.read(readings);
    const Outputs requested = controller_.update(readings);

    if (actuators_.apply(requested)) {
        return {requested, ActuationStatus::Applied};
    }

    // Try to stop the equipment after a failed or partial write.
    // Preserve alarm flags for drivers that also report them.
    Outputs safeCommands = requested;
    safeCommands.p1 = false;
    safeCommands.p2 = false;
    safeCommands.vg = false;
    safeCommands.vp = false;
    safeCommands.requestBackup = false;
    const bool safeApplied = actuators_.apply(safeCommands);
    return {requested, safeApplied ? ActuationStatus::SafeStateApplied
                                   : ActuationStatus::OutputFailed};
}

}  // namespace irrigation
