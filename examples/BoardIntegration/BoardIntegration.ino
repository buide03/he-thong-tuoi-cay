#include "ArduinoBoardIo.h"

using namespace irrigation;

// Điền chân theo đúng bo ESP32 hoặc STM32 đang dùng. -1 báo chưa cấu hình.
constexpr int soilPin = -1;
SensorWiring sensorWiring = {{-1, false, true}, {-1, false, true},
                              {-1, false, true}, 100};
ActuatorWiring actuatorWiring = {{-1, false}, {-1, false}, {-1, false},
                                  {-1, false}, {-1, false}, 300};

ArduinoBoardIo io;
// Thay hai giá trị ADC bằng số đo thực tế khi đất khô và đủ ẩm.
AnalogSoilSensor soil(soilPin, 0, 0);
BufferedPowerSensor power(io, 2000);
HardwareSensors sensors(io, sensorWiring, soil, power);
RelayActuators actuators(io, actuatorWiring);

Config makeConfig() {
    Config config;
    config.useTankSwitches = true;
    return config;
}
IrrigationSystem irrigationSystem(sensors, actuators, makeConfig());
bool ready = false;

void setup() {
    Serial.begin(115200);
    const bool actuatorsOk = actuators.begin();
    const bool sensorsOk = sensors.begin();
    ready = actuatorsOk && sensorsOk;
}

void loop() {
    // Cắm module đo điện/BMS ở đây. Khi có dữ liệu mới, gọi:
    // power.publish(PowerReadings{...}) theo mẫu trong HARDWARE.md.
    // Chưa có dữ liệu hoặc quá 2 giây không cập nhật thì hệ thống gửi lệnh tắt.
    static uint32_t lastTickMs = 0;
    const uint32_t now = millis();
    if (ready && static_cast<uint32_t>(now - lastTickMs) >= 20) {
        lastTickMs = now;
        const CycleResult result = irrigationSystem.tick();
        if (result.actuation == ActuationStatus::OutputFailed) {
            Serial.println("Loi ghi dau ra");
        }
    }
}
