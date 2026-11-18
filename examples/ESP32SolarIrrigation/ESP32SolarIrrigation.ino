#include "ArduinoBoardIo.h"
#include "INA219ElectricalSensor.h"
#include "BoardConfig.h"

using namespace irrigation;

ArduinoBoardIo io;
AnalogSoilSensor soil(board::soilPin, board::soilDryAdc, board::soilWetAdc);
HX711TankSensor mass(io, board::scale());
INA219ElectricalSensor solarMeter(board::solarAddress);
INA219ElectricalSensor batteryMeter(board::batteryAddress);
DualSourcePowerSensor power(solarMeter, batteryMeter, board::sourceLimits());
HardwareSensors sensors(io, board::sensors(), soil, power, &mass);
RelayActuators actuators(io, board::actuators());
IrrigationSystem irrigationSystem(sensors, actuators, board::control());
bool ready = false;

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    const bool outputsOk = actuators.begin();
    const bool busOk = Wire.begin(board::sdaPin, board::sclPin, 100000);
    const bool solarOk = solarMeter.begin(Wire);
    const bool batteryOk = batteryMeter.begin(Wire);
    const bool massOk = mass.begin();
    const bool switchesOk = sensors.begin();
    const bool soilOk = board::soilDryAdc != board::soilWetAdc;
    ready = outputsOk && busOk && solarOk && batteryOk && massOk && switchesOk && soilOk;
    if (!ready) {
        actuators.apply(Outputs{});
        Serial.println("Chua san sang: kiem tra I2C, chan va hieu chuan BoardConfig.h");
    }
}

void loop() {
    static uint32_t lastTickMs = 0;
    static uint32_t lastReportMs = 0;
    const uint32_t now = millis();
    if (!ready || static_cast<uint32_t>(now - lastTickMs) < 20) {
        return;
    }
    lastTickMs = now;
    const CycleResult result = irrigationSystem.tick();
    if (static_cast<uint32_t>(now - lastReportMs) >= 1000) {
        lastReportMs = now;
        const PowerReadings& measured = power.lastReadings();
        Serial.print("PV V/A/W: ");
        Serial.print(measured.solarVoltageV);
        Serial.print(" / ");
        Serial.print(measured.solarCurrentA);
        Serial.print(" / ");
        Serial.println(measured.solarPowerW);
        Serial.print("Battery V/A/W: ");
        Serial.print(measured.batteryVoltageV);
        Serial.print(" / ");
        Serial.print(measured.batteryCurrentA);
        Serial.print(" / ");
        Serial.println(measured.batteryPowerW);
        Serial.print("P1 VG VP P2: ");
        Serial.print(result.requested.p1);
        Serial.print(result.requested.vg);
        Serial.print(result.requested.vp);
        Serial.println(result.requested.p2);
        Serial.print("Input fault: ");
        Serial.println(result.requested.inputFault);
        Serial.print("Actuation: ");
        Serial.println(static_cast<int>(result.actuation));
    }
}
