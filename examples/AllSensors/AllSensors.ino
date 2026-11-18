#include "ArduinoBoardIo.h"
#include "HX711TankSensor.h"
#include "INA219ElectricalSensor.h"

using namespace irrigation;

// Điền chân, cực tính và hiệu chuẩn đúng với bo ESP32/STM32 đang dùng.
constexpr int soilPin = -1;
SensorWiring sensorWiring = {{-1, false, true}, {-1, false, true},
                              {-1, false, true}, 100};
ActuatorWiring actuatorWiring = {{-1, false}, {-1, false}, {-1, false},
                                  {-1, false}, {-1, false}, 300};
HX711Config scaleConfig = {-1, -1, 0, 0.0f, 500};

ArduinoBoardIo io;
AnalogSoilSensor soil(soilPin, 0, 0);  // ADC khô và ADC đủ ẩm sau khi hiệu chuẩn.
HX711TankSensor mass(io, scaleConfig);
INA219ElectricalSensor electrical(0x40);
BufferedPowerSensor power(io, 2000);
HardwareSensors sensors(io, sensorWiring, soil, power, &mass, &electrical);
RelayActuators actuators(io, actuatorWiring);

Config makeConfig() {
    Config config;
    config.useTankSwitches = true;
    config.useTankMass = true;
    // Điền theo bể thật. Các giá trị 0 sẽ làm control báo lỗi hiệu chuẩn.
    config.emptyTankMassKg = 0;
    config.tankLitersPerMeter = 0;
    config.tankBottomToOutletM = 0;
    config.gravityStartHeadM = 0;
    config.minGravityHeadM = 0;
    return config;
}

IrrigationSystem irrigationSystem(sensors, actuators, makeConfig());
bool ready = false;

void setup() {
    Serial.begin(115200);
    Wire.begin();  // Chọn bus/chân I2C theo bo trước bước này khi cần.
    const bool outputOk = actuators.begin();
    const bool scaleOk = mass.begin();
    const bool electricalOk = electrical.begin(Wire);
    const bool switchesOk = sensors.begin();
    ready = outputOk && scaleOk && electricalOk && switchesOk;
}

void loop() {
    // Cập nhật PowerReadings từ BMS/bộ quản lý nguồn và gọi power.publish().
    // Tham khảo SENSOR_MODULES.md. U * I trong Inputs là số đo tải đang chạy.
    static uint32_t lastTickMs = 0;
    const uint32_t now = millis();
    if (ready && static_cast<uint32_t>(now - lastTickMs) >= 20) {
        lastTickMs = now;
        const CycleResult result = irrigationSystem.tick();
        if (result.requested.inputFault) {
            Serial.println("Loi cam bien, hieu chuan hoac du lieu nguon qua han");
        }
    }
}
