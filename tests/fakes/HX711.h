#ifndef IRRIGATION_TEST_HX711_H
#define IRRIGATION_TEST_HX711_H
#include <cstdint>
extern bool fakeHxReady;
extern long fakeHxRaw;
class HX711 {
public:
    void begin(uint8_t, uint8_t, uint8_t = 128) {}
    bool is_ready() { return fakeHxReady; }
    long read() { fakeHxReady = false; return fakeHxRaw; }
};
#endif
