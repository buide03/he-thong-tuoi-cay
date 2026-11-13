#ifndef IRRIGATION_CONTROLLER_H
#define IRRIGATION_CONTROLLER_H

namespace irrigation {

struct Config {
    // Gia tri khoi dau de chay thu. Hieu chinh theo cam bien va tai thuc.
    float soilStartPct = 50.0f;
    float soilStopPct = 55.0f;
    float soilCriticalPct = 25.0f;
    float tankCapacityL = 5.0f;
    float tankFullL = 4.5f;
    float gravityStopL = 4.0f;
    float tankIrrigationStartL = 1.2f;
    float tankIrrigationStopL = 0.5f;
    float sourceStartL = 0.2f;
    float sourceStopL = 0.1f;
    float batteryFullPct = 99.5f;
    float batteryStopPct = 20.0f;
    float batteryResumePct = 22.0f;
    float controllerW = 0.5f;
    float fillPumpW = 5.6f;
    float gravityValveW = 1.0f;
    float pressurePathW = 6.0f;  // P2 + VP
};

struct Inputs {
    // Doc tat ca gia tri truoc khi tinh lenh cho chu ky hien tai.
    float soilPct;
    float upperTankL;
    float lowerSourceL;
    float batteryPct;
    float pvAvailableW;
    float batteryAvailableW;  // Cong suat xa toi da BMS cho phep tai bus DC.
    float backupAvailableW;   // Bang 0 cho den khi nguon du phong san sang.
    bool sensorsOk;           // Adapter dat false khi mat loi doc/du lieu qua han.
};

struct Outputs {
    bool p1 = false;  // Nap be cao.
    bool vg = false;  // Tuoi trong luc.
    bool vp = false;  // Van nhanh tuoi bang bom.
    bool p2 = false;  // Bom tuoi.
    bool requestBackup = false;
    bool criticalSoil = false;
    bool bothWaterSourcesUnavailable = false;
    bool inputFault = false;
};

class Controller {
public:
    explicit Controller(const Config& config = Config{});
    Outputs update(const Inputs& in);

private:
    static bool enough(float availableW, float requiredW);
    static bool inRange(float value, float max);
    static bool validPower(float value);
    bool valid(const Inputs& in) const;
    void resetState();

    Config config_;
    bool wateringNeeded_ = false;
    bool gravityPreferred_ = false;
    bool tankUsable_ = false;
    bool sourceUsable_ = false;
    bool batteryUsable_ = false;
    bool emergencyRefill_ = false;
};

}  // namespace irrigation

#endif
