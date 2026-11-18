#include "ArduinoBoardIo.h"
#include "HX711TankSensor.h"
#include "INA219ElectricalSensor.h"
#include <cassert>
#include <cmath>
#include <limits>

uint32_t fakeMillis = 0;
int fakeAdc = 2000;
bool fakePins[32] = {};
FakeSerial Serial;
TwoWire Wire;
bool fakeHxReady = false;
long fakeHxRaw = 0;
float fakeVoltage = 12;
float fakeCurrentMa = 500;
bool fakeBeginOk = true;
bool fakeVoltageOk = true;
bool fakeCurrentOk = true;

int main() {
    using namespace irrigation;
    ArduinoBoardIo io;
    AnalogSoilSensor soil(0, 3000, 1000);
    float value = 0;
    assert(soil.read(value) && value == 50);
    fakeAdc = 4000;
    assert(soil.read(value) && value == 0);
    fakeAdc = -1;
    assert(!soil.read(value));
    fakeAdc = 2000;

    HX711Config calibration = {9, 10, 1000, 100.0f, 500};
    HX711TankSensor mass(io, calibration);
    assert(mass.begin());
    assert(!mass.read(value));  // Chưa có mẫu.
    fakeHxRaw = 1600;
    fakeHxReady = true;
    assert(mass.read(value) && value == 6);  // Tổng khối lượng, chưa trừ bể.
    fakeMillis = 500;
    assert(mass.read(value) && value == 6);
    fakeMillis = 501;
    assert(!mass.read(value));
    fakeHxReady = true;
    fakeHxRaw = 8388607;
    assert(!mass.read(value));
    fakeHxReady = true;
    fakeHxRaw = 900;
    assert(!mass.read(value));

    calibration.countsPerKg = -100;  // Chiều tín hiệu đảo vẫn hiệu chuẩn được.
    HX711TankSensor reversed(io, calibration);
    assert(reversed.begin());
    fakeHxReady = true;
    fakeHxRaw = 400;
    assert(reversed.read(value) && value == 6);
    calibration.countsPerKg = 0;
    HX711TankSensor uncalibrated(io, calibration);
    assert(!uncalibrated.begin());

    INA219ElectricalSensor electrical;
    ElectricalReadings measured;
    assert(!electrical.read(measured));
    assert(electrical.begin(Wire));
    assert(electrical.read(measured));
    assert(measured.voltageV == 12 && measured.currentA == 0.5f && measured.powerW == 6);
    fakeCurrentMa = -500;
    assert(electrical.read(measured) && measured.powerW == -6);
    fakeCurrentOk = false;
    assert(!electrical.read(measured));
    fakeCurrentOk = true;
    fakeVoltageOk = false;
    assert(!electrical.read(measured));
    fakeVoltageOk = true;
    fakeVoltage = 27;
    assert(!electrical.read(measured));
    fakeVoltage = 12;
    fakeCurrentMa = 2500;
    assert(!electrical.read(measured));
    fakeCurrentMa = 500;
    fakeBeginOk = false;
    INA219ElectricalSensor missing;
    assert(!missing.begin(Wire));
    fakeBeginOk = true;

    // Cắm driver thật vào bộ gom dữ liệu và kiểm tra các trường Inputs.
    fakePins[1] = true;
    fakePins[2] = false;
    fakePins[3] = true;
    SensorWiring wiring = {{1, true, false}, {2, true, false}, {3, true, false}, 0};
    BufferedPowerSensor budget(io, 2000);
    PowerReadings power;
    power.batteryPct = 50;
    power.batteryAvailableW = 20;
    budget.publish(power);
    HardwareSensors sensors(io, wiring, soil, budget, &mass, &electrical);
    assert(sensors.begin());
    fakeHxReady = true;
    fakeHxRaw = 1600;
    Inputs readings;
    assert(sensors.read(readings));
    assert(readings.upperTankMassKg == 6 && readings.soilPct == 50);
    assert(readings.hasElectricalReading && readings.busCurrentA == 0.5f);
    assert(readings.busPowerW == 6 && readings.batteryAvailableW == 20);
    fakeCurrentOk = false;
    assert(!sensors.read(readings));

    // Hỏng một trường đo điện có thể được phát hiện ở Controller.
    Inputs in{40, 4.5f, 14, 50, 0, 20, 0, true};
    in.hasElectricalReading = true;
    in.busCurrentA = std::numeric_limits<float>::quiet_NaN();
    Controller controller;
    assert(controller.update(in).inputFault);
}
