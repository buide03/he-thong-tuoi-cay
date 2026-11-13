#ifndef IRRIGATION_SYSTEM_H
#define IRRIGATION_SYSTEM_H

#include "IrrigationController.h"

namespace irrigation {

class SensorDriver {
public:
    virtual ~SensorDriver() = default;

    // Return true only after filling in a complete, current snapshot.
    // IrrigationSystem manages Inputs::sensorsOk.
    virtual bool read(Inputs& readings) = 0;
};

class ActuatorDriver {
public:
    virtual ~ActuatorDriver() = default;

    // Apply the complete requested state. Return false on a write failure.
    // A default Outputs{} requests pumps off, valves closed and backup off.
    virtual bool apply(const Outputs& commands) = 0;
};

enum class ActuationStatus {
    Applied,
    SafeStateApplied,
    OutputFailed
};

struct CycleResult {
    Outputs requested;
    ActuationStatus actuation;
};

class IrrigationSystem {
public:
    // The two drivers must outlive this object.
    IrrigationSystem(SensorDriver& sensors, ActuatorDriver& actuators,
                     const Config& config = Config{});

    CycleResult tick();

private:
    SensorDriver& sensors_;
    ActuatorDriver& actuators_;
    Controller controller_;
};

}  // namespace irrigation

#endif
