#ifndef IRRIGATION_ARDUINO_BOARD_IO_H
#define IRRIGATION_ARDUINO_BOARD_IO_H

// Dùng với Arduino core cho ESP32 hoặc STM32duino.
#include <Arduino.h>
#include "HardwareDrivers.h"
#include <cmath>

namespace irrigation {

class ArduinoBoardIo final : public BoardIo {
public:
    bool input(int pin, bool pullup) override {
        if (pin < 0) {
            return false;
        }
        pinMode(pin, pullup ? INPUT_PULLUP : INPUT);
        return true;
    }
    bool output(int pin, bool initialHigh) override {
        if (pin < 0) {
            return false;
        }
        digitalWrite(pin, initialHigh ? HIGH : LOW);
        pinMode(pin, OUTPUT);
        digitalWrite(pin, initialHigh ? HIGH : LOW);
        return true;
    }
    bool read(int pin, bool& high) override {
        if (pin < 0) {
            return false;
        }
        high = digitalRead(pin) == HIGH;
        return true;
    }
    bool write(int pin, bool high) override {
        if (pin < 0) {
            return false;
        }
        digitalWrite(pin, high ? HIGH : LOW);
        return true;
    }
    uint32_t nowMs() override { return millis(); }
};

class AnalogSoilSensor final : public NumberSensor {
public:
    AnalogSoilSensor(int pin, int dryAdc, int wetAdc)
        : pin_(pin), dryAdc_(dryAdc), wetAdc_(wetAdc) {}

    bool read(float& percent) override {
        if (pin_ < 0 || dryAdc_ < 0 || wetAdc_ < 0 || wetAdc_ == dryAdc_) {
            return false;
        }
        const float raw = analogRead(pin_);
        if (raw < 0) {
            return false;
        }
        percent = (raw - dryAdc_) * 100.0f / (wetAdc_ - dryAdc_);
        if (percent < 0.0f) {
            percent = 0.0f;
        } else if (percent > 100.0f) {
            percent = 100.0f;
        }
        return true;
    }

private:
    int pin_;
    int dryAdc_;
    int wetAdc_;
};

// Ghép hàm đọc cân đã hiệu chuẩn của thư viện thiết bị vào NumberSensor.
class CallbackNumberSensor final : public NumberSensor {
public:
    using ReadFunction = bool (*)(float&);
    explicit CallbackNumberSensor(ReadFunction read) : read_(read) {}
    bool read(float& value) override {
        return read_ != nullptr && read_(value) && std::isfinite(value);
    }
private:
    ReadFunction read_;
};

// Module BMS/đo điện gọi publish() khi có bộ số đo mới.
class BufferedPowerSensor final : public PowerSensor {
public:
    BufferedPowerSensor(BoardIo& io, uint32_t maxAgeMs)
        : io_(io), maxAgeMs_(maxAgeMs) {}
    void publish(const PowerReadings& readings) {
        readings_ = readings;
        atMs_ = io_.nowMs();
        hasReading_ = true;
    }
    bool read(PowerReadings& readings) override {
        if (!hasReading_ ||
            static_cast<uint32_t>(io_.nowMs() - atMs_) > maxAgeMs_) {
            return false;
        }
        readings = readings_;
        return true;
    }
private:
    BoardIo& io_;
    uint32_t maxAgeMs_;
    PowerReadings readings_;
    uint32_t atMs_ = 0;
    bool hasReading_ = false;
};

}  // namespace irrigation

#endif
