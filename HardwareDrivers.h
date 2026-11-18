#ifndef IRRIGATION_HARDWARE_DRIVERS_H
#define IRRIGATION_HARDWARE_DRIVERS_H

#include "IrrigationSystem.h"
#include <cstdint>

namespace irrigation {

// Adapter của bo mạch cài đặt các hàm GPIO và đồng hồ này.
class BoardIo {
public:
    virtual ~BoardIo() = default;
    virtual bool input(int pin, bool pullup) = 0;
    virtual bool output(int pin, bool initialHigh) = 0;
    virtual bool read(int pin, bool& high) = 0;
    virtual bool write(int pin, bool high) = 0;
    virtual uint32_t nowMs() = 0;
};

class NumberSensor {
public:
    virtual ~NumberSensor() = default;
    virtual bool read(float& value) = 0;
};

struct PowerReadings {
    float batteryPct = 0.0f;
    float pvAvailableW = 0.0f;
    float batteryAvailableW = 0.0f;
    float backupAvailableW = 0.0f;
};

class PowerSensor {
public:
    virtual ~PowerSensor() = default;
    virtual bool read(PowerReadings& readings) = 0;
};

struct ElectricalReadings {
    float voltageV = 0.0f;
    float currentA = 0.0f;
    float powerW = 0.0f;
};

class ElectricalSensor {
public:
    virtual ~ElectricalSensor() = default;
    virtual bool read(ElectricalReadings& readings) = 0;
};

struct SwitchConfig {
    int pin;
    bool wetHigh;  // HIGH có nghĩa là nước chạm cảm biến khi true.
    bool pullup;
};

struct SensorWiring {
    SwitchConfig upperLow;
    SwitchConfig upperFull;
    SwitchConfig source;
    uint32_t debounceMs;
};

class HardwareSensors final : public SensorDriver {
public:
    // soil đọc %, tankMass đọc tổng khối lượng kg. Con trỏ null khi không dùng cân.
    HardwareSensors(BoardIo& io, const SensorWiring& wiring, NumberSensor& soil,
                    PowerSensor& power, NumberSensor* tankMass = nullptr,
                    ElectricalSensor* electrical = nullptr);
    bool begin();
    bool read(Inputs& readings) override;

private:
    struct SwitchState {
        bool initialized = false;
        bool candidate = false;
        bool stable = false;
        uint32_t sinceMs = 0;
    };
    bool readSwitch(const SwitchConfig& config, SwitchState& state, bool& wet);

    BoardIo& io_;
    SensorWiring wiring_;
    NumberSensor& soil_;
    PowerSensor& power_;
    NumberSensor* tankMass_;
    ElectricalSensor* electrical_;
    SwitchState low_;
    SwitchState full_;
    SwitchState source_;
    bool ready_ = false;
};

struct RelayPin {
    int pin;
    bool activeHigh;
};

struct ActuatorWiring {
    RelayPin p1;
    RelayPin vg;
    RelayPin vp;
    RelayPin p2;
    RelayPin backup;
    uint32_t valveSettleMs;
};

class RelayActuators final : public ActuatorDriver {
public:
    RelayActuators(BoardIo& io, const ActuatorWiring& wiring);
    bool begin();
    bool apply(const Outputs& commands) override;
    bool pending() const override { return pending_; }

private:
    bool set(const RelayPin& pin, bool active);
    bool stopAll();
    BoardIo& io_;
    ActuatorWiring wiring_;
    bool ready_ = false;
    bool pressurePathOpen_ = false;
    bool pending_ = false;
    uint32_t openedAtMs_ = 0;
};

}  // namespace irrigation

#endif
