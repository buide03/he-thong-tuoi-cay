#include "IrrigationController.h"

#include <cassert>
#include <initializer_list>
#include <limits>

int main() {
    irrigation::Config config;
    config.useTankMass = true;
    config.emptyTankMassKg = 2.0f;
    config.tankLitersPerMeter = 8.0f;
    config.tankBottomToOutletM = 0.5f;
    config.gravityStartHeadM = 1.0f;
    config.minGravityHeadM = 0.75f;
    irrigation::Controller controller(config);

    // Omit the direct volume reading to verify the driver can supply mass alone.
    irrigation::Inputs in{42.0f, -1.0f, 14.0f, 50.0f, 0.0f, 20.0f, 0.0f, true};
    assert(controller.update(in).inputFault);
    in.upperTankMassKg = 5.0f;  // 3 L, 0.875 m: below the gravity start threshold.
    auto out = controller.update(in);
    assert(out.p1 && out.vp && out.p2 && !out.vg && !out.inputFault);

    in.upperTankMassKg = 6.0f;  // 4 L, 1 m: gravity starts even below tankFullL.
    out = controller.update(in);
    assert(out.vg && !out.p1 && !out.vp && !out.p2);
    in.upperTankMassKg = 5.0f;  // 0.875 m: keep gravity active.
    assert(controller.update(in).vg);
    in.upperTankMassKg = 4.0f;  // 2 L, 0.75 m: switch to pumped irrigation.
    out = controller.update(in);
    assert(!out.vg && out.vp && out.p2);

    in.upperTankMassKg = 2.5f;  // 0.5 L: stop watering, refill.
    out = controller.update(in);
    assert(out.p1 && !out.vg && !out.vp && !out.p2);

    for (float invalidMass : {1.5f, 7.5f, std::numeric_limits<float>::quiet_NaN()}) {
        in.upperTankMassKg = invalidMass;
        out = controller.update(in);
        assert(out.inputFault && !out.p1 && !out.p2 && !out.vg && !out.vp);
    }

    irrigation::Controller solarCharge(config);
    in.soilPct = 60.0f;
    in.batteryPct = 100.0f;
    in.pvAvailableW = 13.0f;
    in.upperTankMassKg = 6.25f;  // 4.25 L: PV can power P1 while the tank has room.
    assert(solarCharge.update(in).p1);
    in.upperTankMassKg = 6.5f;  // 4.5 L: stop filling at the working limit.
    assert(!solarCharge.update(in).p1);

    config.tankLitersPerMeter = 0.0f;
    irrigation::Controller uncalibrated(config);
    in.upperTankMassKg = 6.0f;
    assert(uncalibrated.update(in).inputFault);
}
