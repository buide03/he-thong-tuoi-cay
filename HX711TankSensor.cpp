#include "HX711TankSensor.h"
#include <cmath>

namespace irrigation {

HX711TankSensor::HX711TankSensor(BoardIo& io, const HX711Config& config)
    : io_(io), config_(config) {}

bool HX711TankSensor::begin() {
    ready_ = false;
    hasSample_ = false;
    if (config_.dataPin < 0 || config_.dataPin > 255 ||
        config_.clockPin < 0 || config_.clockPin > 255 ||
        config_.dataPin == config_.clockPin ||
        !std::isfinite(config_.countsPerKg) || config_.countsPerKg == 0.0f ||
        config_.maxAgeMs == 0) {
        return false;
    }
    device_.begin(config_.dataPin, config_.clockPin, 128);
    ready_ = true;
    return true;
}

bool HX711TankSensor::read(float& massKg) {
    if (!ready_) {
        return false;
    }
    if (device_.is_ready()) {
        // Một mẫu mỗi lần. Không đợi nhiều mẫu vì sẽ chặn vòng điều khiển.
        const long raw = device_.read();
        if (raw <= -8388608L || raw >= 8388607L) {
            hasSample_ = false;  // ADC chạm biên.
            return false;
        }
        const double netCounts = static_cast<double>(raw) - config_.zeroRaw;
        lastKg_ = static_cast<float>(netCounts / config_.countsPerKg);
        if (!std::isfinite(lastKg_) || lastKg_ < 0.0f) {
            hasSample_ = false;
            return false;
        }
        sampledAtMs_ = io_.nowMs();
        hasSample_ = true;
    }
    if (!hasSample_ ||
        static_cast<uint32_t>(io_.nowMs() - sampledAtMs_) > config_.maxAgeMs) {
        return false;
    }
    massKg = lastKg_;
    return true;
}

}  // namespace irrigation
