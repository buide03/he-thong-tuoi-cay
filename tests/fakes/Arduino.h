#ifndef IRRIGATION_TEST_ARDUINO_H
#define IRRIGATION_TEST_ARDUINO_H
#include <cstdint>
constexpr int HIGH = 1;
constexpr int LOW = 0;
constexpr int INPUT = 0;
constexpr int INPUT_PULLUP = 2;
constexpr int OUTPUT = 1;
extern uint32_t fakeMillis;
extern int fakeAdc;
extern bool fakePins[32];
inline uint32_t millis() { return fakeMillis; }
inline int analogRead(int) { return fakeAdc; }
inline int digitalRead(int pin) { return fakePins[pin] ? HIGH : LOW; }
inline void digitalWrite(int pin, int value) { fakePins[pin] = value == HIGH; }
inline void pinMode(int, int) {}
struct FakeSerial {
    void begin(int) {}
    void println(const char*) {}
};
extern FakeSerial Serial;
#endif
