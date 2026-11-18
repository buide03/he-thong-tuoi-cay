#include "HardwareDrivers.h"

namespace irrigation {

HardwareSensors::HardwareSensors(BoardIo& io, const SensorWiring& wiring,
                                 NumberSensor& soil, PowerSensor& power,
                                 NumberSensor* tankMass, ElectricalSensor* electrical)
    : io_(io), wiring_(wiring), soil_(soil), power_(power), tankMass_(tankMass),
      electrical_(electrical) {}

bool HardwareSensors::begin() {
    low_ = SwitchState{};
    full_ = SwitchState{};
    source_ = SwitchState{};
    const bool lowOk = io_.input(wiring_.upperLow.pin, wiring_.upperLow.pullup);
    const bool fullOk = io_.input(wiring_.upperFull.pin, wiring_.upperFull.pullup);
    const bool sourceOk = io_.input(wiring_.source.pin, wiring_.source.pullup);
    ready_ = lowOk && fullOk && sourceOk;
    return ready_;
}

bool HardwareSensors::readSwitch(const SwitchConfig& config, SwitchState& state,
                                 bool& wet) {
    bool high = false;
    if (!io_.read(config.pin, high)) {
        state.initialized = false;
        return false;
    }
    const bool rawWet = high == config.wetHigh;
    const uint32_t now = io_.nowMs();
    if (!state.initialized || rawWet != state.candidate) {
        state.candidate = rawWet;
        state.sinceMs = now;
        state.initialized = true;
    }
    // Trong lúc tín hiệu đang đổi hoặc lúc khởi động, báo chưa có dữ liệu ổn định.
    if (static_cast<uint32_t>(now - state.sinceMs) < wiring_.debounceMs) {
        return false;
    }
    state.stable = state.candidate;
    wet = state.stable;
    return true;
}

bool HardwareSensors::read(Inputs& readings) {
    readings = Inputs{};
    if (!ready_) {
        return false;
    }
    // Đọc cả ba công tắc dù một công tắc đang chờ ổn định.
    const bool lowOk = readSwitch(wiring_.upperLow, low_, readings.upperLowWet);
    const bool fullOk = readSwitch(wiring_.upperFull, full_, readings.upperFullWet);
    const bool sourceOk = readSwitch(wiring_.source, source_, readings.sourceWet);
    if (!lowOk || !fullOk || !sourceOk) {
        return false;
    }
    if (readings.upperFullWet && !readings.upperLowWet) {
        return false;
    }
    PowerReadings power;
    if (!soil_.read(readings.soilPct) || !power_.read(power)) {
        return false;
    }
    if (tankMass_ != nullptr && !tankMass_->read(readings.upperTankMassKg)) {
        return false;
    }
    if (electrical_ != nullptr) {
        ElectricalReadings measured;
        if (!electrical_->read(measured)) {
            return false;
        }
        readings.busVoltageV = measured.voltageV;
        readings.busCurrentA = measured.currentA;
        readings.busPowerW = measured.powerW;
        readings.hasElectricalReading = true;
    }
    readings.batteryPct = power.batteryPct;
    readings.pvAvailableW = power.pvAvailableW;
    readings.batteryAvailableW = power.batteryAvailableW;
    readings.backupAvailableW = power.backupAvailableW;
    readings.hasSourceReadings = power.hasSourceReadings;
    readings.solarVoltageV = power.solarVoltageV;
    readings.solarCurrentA = power.solarCurrentA;
    readings.solarPowerW = power.solarPowerW;
    readings.batteryVoltageV = power.batteryVoltageV;
    readings.batteryCurrentA = power.batteryCurrentA;
    readings.batteryPowerW = power.batteryPowerW;
    readings.sensorsOk = true;
    return true;
}

RelayActuators::RelayActuators(BoardIo& io, const ActuatorWiring& wiring)
    : io_(io), wiring_(wiring) {}

bool RelayActuators::begin() {
    // Cấu hình mọi chân về OFF, kể cả khi một chân khác báo lỗi.
    const bool p1Ok = io_.output(wiring_.p1.pin, !wiring_.p1.activeHigh);
    const bool p2Ok = io_.output(wiring_.p2.pin, !wiring_.p2.activeHigh);
    const bool vgOk = io_.output(wiring_.vg.pin, !wiring_.vg.activeHigh);
    const bool vpOk = io_.output(wiring_.vp.pin, !wiring_.vp.activeHigh);
    const bool backupOk = io_.output(wiring_.backup.pin, !wiring_.backup.activeHigh);
    pressurePathOpen_ = false;
    pending_ = false;
    ready_ = p1Ok && p2Ok && vgOk && vpOk && backupOk;
    if (!ready_) {
        stopAll();
    }
    return ready_;
}

bool RelayActuators::set(const RelayPin& pin, bool active) {
    return io_.write(pin.pin, active == pin.activeHigh);
}

bool RelayActuators::stopAll() {
    // Dừng bơm trước khi đóng van. Thử ghi tất cả đầu ra khi có lỗi.
    const bool p2Ok = set(wiring_.p2, false);
    const bool p1Ok = set(wiring_.p1, false);
    const bool vgOk = set(wiring_.vg, false);
    const bool vpOk = set(wiring_.vp, false);
    const bool backupOk = set(wiring_.backup, false);
    pressurePathOpen_ = false;
    pending_ = false;
    return p2Ok && p1Ok && vgOk && vpOk && backupOk;
}

bool RelayActuators::apply(const Outputs& commands) {
    pending_ = false;
    if (!ready_) {
        stopAll();
        return false;
    }
    if ((commands.vg && commands.vp) || (commands.p2 && !commands.vp)) {
        stopAll();
        return false;
    }
    if (!commands.vp) {
        if (!set(wiring_.p2, false)) {
            stopAll();
            return false;
        }
        pressurePathOpen_ = false;
    } else if (!pressurePathOpen_) {
        // Dừng P2, đóng VG, mở VP, sau đó đợi van mở đủ thời gian.
        if (!set(wiring_.p2, false) || !set(wiring_.vg, false) ||
            !set(wiring_.vp, true)) {
            stopAll();
            return false;
        }
        pressurePathOpen_ = true;
        openedAtMs_ = io_.nowMs();
    }

    bool runP2 = false;
    if (commands.p2 &&
        static_cast<uint32_t>(io_.nowMs() - openedAtMs_) >= wiring_.valveSettleMs) {
        runP2 = true;
    }
    pending_ = commands.p2 && !runP2;
    if (!set(wiring_.vg, commands.vg) || !set(wiring_.vp, commands.vp) ||
        !set(wiring_.backup, commands.requestBackup) ||
        !set(wiring_.p1, commands.p1) || !set(wiring_.p2, runP2)) {
        stopAll();
        return false;
    }
    return true;
}

}  // namespace irrigation
