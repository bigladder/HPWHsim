/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

constexpr double tol = 1.e-4;

struct Performance
{
    HPWH::Power_t input, output;
    double cop;
};

void getCompressorPerformance(
    HPWH& hpwh, Performance& point, HPWH::Temp_t airT, HPWH::Temp_t waterT, HPWH::Temp_t setpointT)
{
    if (hpwh.isCompressorMultipass() == 1)
    { // Multipass capacity looks at the average of the
      // temperature of the lift
        hpwh.setSetpointT(HPWH::Temp_t((waterT() + setpointT()) / 2.));
    }
    else
    {
        hpwh.setSetpointT(waterT);
    }
    hpwh.resetTankToSetpoint(); // Force tank cold
    hpwh.setSetpointT(setpointT);

    hpwh.setUA(HPWH::UA_t(0.));
    hpwh.setFittingsUA(HPWH::UA_t(0.));

    // Run the step
    hpwh.runOneStep(waterT,         // Inlet water temperature
                    {0., Units::L}, // Flow
                    airT,           // Ambient Temp
                    airT,           // External Temp
                    // HPWH::DR_TOO // DR Status (now an enum. Fixed for now as allow)
                    (HPWH::DR_TOO | HPWH::DR_LOR) // DR Status (now an enum. Fixed for now as allow)
    );

    // Check the heat in and out of the compressor
    double dt = HPWH::Time_t(1., Units::min)(Units::s);
    point.input = {hpwh.getNthHeatSourceEnergyInput(hpwh.getCompressorIndex())(Units::J) / dt,
                   Units::W};
    point.output = {hpwh.getNthHeatSourceEnergyOutput(hpwh.getCompressorIndex())(Units::J) / dt,
                    Units::W};
    point.cop = point.output / point.input;
}

bool scaleCapacityCOP(HPWH& hpwh,
                      double scaleInput,
                      double scaleCOP,
                      Performance& point0,
                      Performance& point1,
                      HPWH::Temp_t airT = {77, Units::F},
                      HPWH::Temp_t waterT = {63, Units::F},
                      HPWH::Temp_t setpointT = {135, Units::F})
{
    // Get peformance unscaled
    getCompressorPerformance(hpwh, point0, airT, waterT, setpointT);

    // Scale the compressor
    hpwh.setScaleCapacityCOP(scaleInput, scaleCOP);

    // Get the scaled performance
    getCompressorPerformance(hpwh, point1, airT, waterT, setpointT);
    return true;
}

/*
 * noScaleOutOfBounds tests
 */
TEST(ScaleTest, noScaleOutOfBounds)
{ // Test that we CANNOT scale the scalable model with scale <= 0

    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    hpwh.initPreset(sModelName);

    double num = 0;
    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(num, 1.));
    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(1., num));

    num = -1;
    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(num, 1.));
    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(1., num));
}

/*
 * nonScalable tests
 */
TEST(ScaleTest, nonScalable)
{ // Test a model that is not scalable

    const std::string sModelName = "AOSmithCAHP120";

    // get preset model
    HPWH hpwh;
    hpwh.initPreset(sModelName);

    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(1., 1.));
}

/*
 * scalableScales tests
 */
TEST(ScaleTest, scalableScales)
{ // Test that scalable hpwh can scale

    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    hpwh.initPreset(sModelName);

    Performance point0, point1;
    double num, anotherNum;

    num = 0.001; // Very low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 0.2; // normal but bad low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 3.; // normal but pretty high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1000; // really high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 10;
    anotherNum = 0.1; // weird high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / anotherNum, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1.5;
    anotherNum = 0.9; // real high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / anotherNum, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);
}

/*
 * scalableMP_scales tests
 */
TEST(ScaleTest, scalableMP_scales)
{ // Test that scalable MP hpwh can scale

    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Scalable_MP";
    hpwh.initPreset(sModelName);

    Performance point0, point1;
    double num, anotherNum;

    num = 0.001; // Very low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 0.2; // normal but bad low
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 3.; // normal but pretty high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1000; // really high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input(), tol);
    EXPECT_NEAR(point0.output(), point1.output() / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 10;
    anotherNum = 0.1; // weird high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / anotherNum, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1.5;
    anotherNum = 0.9; // real high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.input(), point1.input() / num, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input(), point1.input() / anotherNum, tol);
    EXPECT_NEAR(point0.output(), point1.output() / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);
}

/*
 * getCompressorSP_capacity tests
 */
TEST(ScaleTest, getCompressorSP_capacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_20_SP";
    hpwh.initPreset(sModelName);

    HPWH::Temp_t waterT = {63, Units::F};
    HPWH::Temp_t airT = {77, Units::F};
    HPWH::Temp_t setpointT = {135, Units::F};

    Performance point0;
    getCompressorPerformance(hpwh, point0, airT, waterT, setpointT);
    auto capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR(point0.output(), capacity(), tol);
}

/*
 * getCompressorMP_capacity tests
 */
TEST(ScaleTest, getCompressorMP_capacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_20_MP";
    hpwh.initPreset(sModelName);

    Performance point0;

    const HPWH::Temp_t waterT = {50, Units::F}; // 50 and 126 to make average 88
    const HPWH::Temp_t airT = {61.7, Units::F};
    const HPWH::Temp_t setpointT = {126, Units::F};

    auto capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    getCompressorPerformance(hpwh, point0, airT, waterT, setpointT);
    EXPECT_NEAR_REL_TOL(point0.output(), capacity(), tol);
}

/*
 * getCompressorMP_outputCapacity tests
 */
TEST(ScaleTest, getCompressorMP_outputCapacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Scalable_MP";
    hpwh.initPreset(sModelName);

    const HPWH::Temp_t waterT = {44, Units::F};
    const HPWH::Temp_t airT = {98, Units::F};
    HPWH::Temp_t setpointT = {150, Units::F};

    // Scale output to 1 kW
    HPWH::Power_t num = {1., Units::kW};
    hpwh.setCompressorOutputCapacity(num, airT, waterT, setpointT);
    auto newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_NEAR(num(), newCapacity(), tol);

    // Scale output to .01 kW
    num = {.01, Units::kW};
    hpwh.setCompressorOutputCapacity(num, airT, waterT, setpointT);
    newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_NEAR(num(), newCapacity(), tol);

    // Scale output to 1000 kW
    num = {1000., Units::kW};
    hpwh.setCompressorOutputCapacity(num, airT, waterT, setpointT);
    newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_NEAR(num(), newCapacity(), tol);

    // Check again with changed setpoint. For MP it should affect output capacity since it looks at
    // the mean temperature for the cycle.
    setpointT = {100, Units::F};
    newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_FAR(num(), newCapacity(), tol);
}

/*
 * setCompressorSP_outputCapacity tests
 */
TEST(ScaleTest, setCompressorSP_outputCapacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    hpwh.initPreset(sModelName);

    HPWH::Temp_t waterT = {44, Units::F};
    HPWH::Temp_t airT = {98, Units::F};
    HPWH::Temp_t setpointT = {145, Units::F};

    // Scale output to 1 kW
    HPWH::Power_t num = {1., Units::kW};
    hpwh.setCompressorOutputCapacity(num, airT, waterT, setpointT);
    auto newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_NEAR(num(), newCapacity(), tol);

    // Scale output to .01 kW
    num = {.01, Units::kW};
    hpwh.setCompressorOutputCapacity(num, airT, waterT, setpointT);
    newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_NEAR(num(), newCapacity(), tol);

    // Scale output to 1000 kW
    num = {1000., Units::kW};
    hpwh.setCompressorOutputCapacity(num, airT, waterT, setpointT);
    newCapacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);
    EXPECT_NEAR(num(), newCapacity(), tol);
}

/*
 * scaleRestank tests
 */
TEST(ScaleTest, scaleRestank)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "restankRealistic";
    hpwh.initPreset(sModelName);

    // Scale COP for restank fails.
    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(2., 2.));
}

/*
 * resistanceScales tests
 */
TEST(ScaleTest, resistanceScales)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    hpwh.initPreset(sModelName);

    HPWH::Power_t elementPower = {30., Units::kW};

    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(), elementPower());
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(1)(), elementPower());
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1)(), 2. * elementPower());

    // Check setting bottom works
    double factor = 2.;
    hpwh.setResistanceCapacity(factor * elementPower, 0);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(), factor * elementPower());
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(1)(), elementPower());
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1)(), factor * elementPower() + elementPower());

    // Check setting both works
    factor = 3.;
    hpwh.setResistanceCapacity(factor * elementPower, -1);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(), factor * elementPower());
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(1)(), factor * elementPower());
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1)(), 2. * factor * elementPower());
}

/*
 * storageTankErrors tests
 */
TEST(ScaleTest, storageTankErrors)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "StorageTank";
    hpwh.initPreset(sModelName);

    EXPECT_ANY_THROW(hpwh.setResistanceCapacity({1000., Units::W}));
    EXPECT_ANY_THROW(hpwh.setScaleCapacityCOP(1., 1.));
}
