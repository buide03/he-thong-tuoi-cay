#ifndef IRRIGATION_TEST_INA219_H
#define IRRIGATION_TEST_INA219_H
#include <cstdint>
class TwoWire {
public:
    void begin() {}
    bool begin(int, int, uint32_t) { return true; }
};
extern TwoWire Wire;
extern float fakeVoltage;
extern float fakeCurrentMa;
extern bool fakeBeginOk;
extern bool fakeVoltageOk;
extern bool fakeCurrentOk;
class Adafruit_INA219 {
public:
    explicit Adafruit_INA219(uint8_t) {}
    bool begin(TwoWire*) { ok_ = fakeBeginOk; return ok_; }
    bool success() { return ok_; }
    float getBusVoltage_V() { ok_ = fakeVoltageOk; return fakeVoltage; }
    float getCurrent_mA() { ok_ = fakeCurrentOk; return fakeCurrentMa; }
private:
    bool ok_ = false;
};
#endif
