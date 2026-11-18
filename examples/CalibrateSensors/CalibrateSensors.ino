#include <Arduino.h>
#include <HX711.h>

// ESP32-WROOM-32: cùng chân với profile ESP32SolarIrrigation.
HX711 scale;

void setup() {
    Serial.begin(115200);
    analogReadResolution(12);
    scale.begin(32, 33, 128);
    Serial.println("Ghi ADC dat kho/du am. Ghi HX711 khi khong tai va khi co tai chuan.");
}

void loop() {
    static uint32_t lastReportMs = 0;
    const uint32_t now = millis();
    if (static_cast<uint32_t>(now - lastReportMs) < 200) {
        return;
    }
    lastReportMs = now;
    Serial.print("Soil ADC: ");
    Serial.print(analogRead(34));
    Serial.print(" | HX711 raw: ");
    if (scale.is_ready()) {
        Serial.println(scale.read());
    } else {
        Serial.println("chua san sang");
    }
}
