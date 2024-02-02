/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

constexpr double tol = 1.e-4;

struct Performance
{
    double input, output, cop;
};

void getCompressorPerformance(
    HPWH& hpwh, Performance& point, double waterTempC, double airTempC, double setpointC)
{
    if (hpwh.isCompressorMultipass() == 1)
    { // Multipass capacity looks at the average of the
      // temperature of the lift
        hpwh.setSetpoint((waterTempC + setpointC) / 2.);
    }
    else
    {
        hpwh.setSetpoint(waterTempC);
    }
    hpwh.resetTankToSetpoint(); // Force tank cold
    hpwh.setSetpoint(setpointC);

    // Run the step
    hpwh.runOneStep(waterTempC, // Inlet water temperature (C )
                    0.,         // Flow in gallons
                    airTempC,   // Ambient Temp (C)
                    airTempC,   // External Temp (C)
                    // HPWH::DR_TOO // DR Status (now an enum. Fixed for now as allow)
                    (HPWH::DR_TOO | HPWH::DR_LOR) // DR Status (now an enum. Fixed for now as allow)
    );

    // Check the heat in and out of the compressor
    point.input = hpwh.getNthHeatSourceEnergyInput(hpwh.getCompressorIndex());
    point.output = hpwh.getNthHeatSourceEnergyOutput(hpwh.getCompressorIndex());
    point.cop = point.output / point.input;
}

bool scaleCapacityCOP(HPWH& hpwh,
                      double scaleInput,
                      double scaleCOP,
                      Performance& point0,
                      Performance& point1,
                      double waterTempC = F_TO_C(63),
                      double airTempC = F_TO_C(77),
                      double setpointC = F_TO_C(135))
{
    // Get peformance unscaled
    getCompressorPerformance(hpwh, point0, waterTempC, airTempC, setpointC);

    // Scale the compressor
    int val = hpwh.setScaleCapacityCOP(scaleInput, scaleCOP);
    if (val != 0)
    {
        return false;
    }

    // Get the scaled performance
    getCompressorPerformance(hpwh, point1, waterTempC, airTempC, setpointC);
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
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double num = 0;
    EXPECT_EQ(hpwh.setScaleCapacityCOP(num, 1.), HPWH::HPWH_ABORT);
    EXPECT_EQ(hpwh.setScaleCapacityCOP(1., num), HPWH::HPWH_ABORT);

    num = -1;
    EXPECT_EQ(hpwh.setScaleCapacityCOP(num, 1.), HPWH::HPWH_ABORT);
    EXPECT_EQ(hpwh.setScaleCapacityCOP(1., num), HPWH::HPWH_ABORT);
}

/*
 * nonScalable tests
 */
TEST(ScaleTest, nonScalable)
{ // Test a model that is not scalable

    const std::string sModelName = "AOSmithCAHP120";

    // get preset model
    HPWH hpwh;
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    EXPECT_EQ(hpwh.setScaleCapacityCOP(1., 1.), HPWH::HPWH_ABORT);
}

/*
 * scalableScales tests
 */
TEST(ScaleTest, scalableScales)
{ // Test that scalable hpwh can scale

    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    Performance point0, point1;
    double num, anotherNum;

    num = 0.001; // Very low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 0.2; // normal but bad low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 3.; // normal but pretty high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1000; // really high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 10;
    anotherNum = 0.1; // weird high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / anotherNum, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1.5;
    anotherNum = 0.9; // real high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / anotherNum, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
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
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    Performance point0, point1;
    double num, anotherNum;

    num = 0.001; // Very low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 0.2; // normal but bad low
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 3.; // normal but pretty high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1000; // really high
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, 1., point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, 1., num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input, tol);
    EXPECT_NEAR(point0.output, point1.output / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / num, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 10;
    anotherNum = 0.1; // weird high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / anotherNum, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / num, tol);

    num = 1.5;
    anotherNum = 0.9; // real high and low
    EXPECT_TRUE(scaleCapacityCOP(hpwh, num, anotherNum, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.input, point1.input / num, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
    EXPECT_NEAR(point0.cop, point1.cop / anotherNum, tol);

    EXPECT_TRUE(scaleCapacityCOP(hpwh, anotherNum, num, point0, point1));
    EXPECT_NEAR(point0.input, point1.input / anotherNum, tol);
    EXPECT_NEAR(point0.output, point1.output / num / anotherNum, tol);
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
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    Performance point0;

    double capacity_kWH, capacity_BTU;
    double waterTempC = F_TO_C(63);
    double airTempC = F_TO_C(77);
    double setpointC = F_TO_C(135);

    getCompressorPerformance(hpwh, point0, waterTempC, airTempC, setpointC); // gives kWH
    capacity_kWH = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC) /
                   60; // div 60 to kWh because I know above only runs 1 minute

    EXPECT_NEAR(point0.output, capacity_kWH, tol);

    // Test with IP units
    capacity_BTU = hpwh.getCompressorCapacity(C_TO_F(airTempC),
                                              C_TO_F(waterTempC),
                                              C_TO_F(setpointC),
                                              HPWH::UNITS_BTUperHr,
                                              HPWH::UNITS_F) /
                   60; // div 60 to BTU because I know above only runs 1 minute

    EXPECT_NEAR_REL(KWH_TO_BTU(point0.output),
                    capacity_BTU); // relative cmp since in btu's these will be large numbers
}

/*
 * getCompressorMP_capacity tests
 */
TEST(ScaleTest, getCompressorMP_capacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_20_MP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    Performance point0;

    const double waterTempC = F_TO_C(50); // 50 and 126 to make average 88
    const double airTempC = F_TO_C(61.7);
    const double setpointC = F_TO_C(126);

    double capacity_kWH = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC) /
                          60.; // div 60 to kWh because I know above only runs 1 minute
    getCompressorPerformance(hpwh, point0, waterTempC, airTempC, setpointC); // gives kWH
    EXPECT_NEAR(point0.output, capacity_kWH, tol);

    // Test with IP units
    double capacity_BTU = hpwh.getCompressorCapacity(C_TO_F(airTempC),
                                                     C_TO_F(waterTempC),
                                                     C_TO_F(setpointC),
                                                     HPWH::UNITS_BTUperHr,
                                                     HPWH::UNITS_F) /
                          60; // div 60 to BTU because I know above only runs 1 minute

    EXPECT_NEAR_REL(KWH_TO_BTU(point0.output),
                    capacity_BTU); // relative cmp since in btu's these will be large numbers
}

/*
 * getCompressorMP_outputCapacity tests
 */
TEST(ScaleTest, getCompressorMP_outputCapacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Scalable_MP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double newCapacity_kW, num;
    double waterTempC = F_TO_C(44);
    double airTempC = F_TO_C(98);
    double setpointC = F_TO_C(150);

    // Scale output to 1 kW
    num = 1.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_NEAR(num, newCapacity_kW, tol);

    // Scale output to .01 kW
    num = .01;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_NEAR(num, newCapacity_kW, tol);

    // Scale output to 1000 kW
    num = 1000.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_NEAR(num, newCapacity_kW, tol);

    // Check again with changed setpoint. For MP it should affect output capacity since it looks at
    // the mean temperature for the cycle.
    setpointC = F_TO_C(100);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_FAR(num, newCapacity_kW, tol);
}

/*
 * setCompressorSP_outputCapacity tests
 */
TEST(ScaleTest, setCompressorSP_outputCapacity)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double newCapacity_kW, num;
    double waterTempC = F_TO_C(44);
    double airTempC = F_TO_C(98);
    double setpointC = F_TO_C(145);

    // Scale output to 1 kW
    num = 1.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_NEAR(num, newCapacity_kW, tol);

    // Scale output to .01 kW
    num = .01;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_NEAR(num, newCapacity_kW, tol);

    // Scale output to 1000 kW
    num = 1000.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    EXPECT_NEAR(num, newCapacity_kW, tol);

    // Scale output to 1000 kW but let's do the calc in other units
    num = KW_TO_BTUperH(num);
    hpwh.setCompressorOutputCapacity(num,
                                     C_TO_F(airTempC),
                                     C_TO_F(waterTempC),
                                     C_TO_F(setpointC),
                                     HPWH::UNITS_BTUperHr,
                                     HPWH::UNITS_F);
    double newCapacity_BTUperHr = hpwh.getCompressorCapacity(C_TO_F(airTempC),
                                                             C_TO_F(waterTempC),
                                                             C_TO_F(setpointC),
                                                             HPWH::UNITS_BTUperHr,
                                                             HPWH::UNITS_F);
    EXPECT_NEAR_REL(num, newCapacity_BTUperHr);
}

/*
 * chipsCaseWithIP_units tests
 */
TEST(ScaleTest, chipsCaseWithIP_units)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    const double waterT_F = 50;
    const double airT_F = 50;
    const double setpointT_F = 120;
    const double wh_heatingCap = 20000.;

    // Scale output to 20000 btu/hr but let's use do the calc in other units
    hpwh.setCompressorOutputCapacity(
        wh_heatingCap, airT_F, waterT_F, setpointT_F, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);

    double newCapacity_BTUperHr = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);

    EXPECT_NEAR_REL(wh_heatingCap, newCapacity_BTUperHr);
}

/*
 * scaleRestank tests
 */
TEST(ScaleTest, scaleRestank)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "restankRealistic";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    // Scale COP for restank fails.
    EXPECT_EQ(hpwh.setScaleCapacityCOP(2., 2.), HPWH::HPWH_ABORT);
}

/*
 * resistanceScales tests
 */
TEST(ScaleTest, resistanceScales)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double elementPower = 30.; // KW

    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW), elementPower);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW), elementPower);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW), 2. * elementPower);

    // check units convert
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_BTUperHr),
                    2. * KW_TO_BTUperH(elementPower));

    // Check setting bottom works
    double factor = 2.;
    hpwh.setResistanceCapacity(factor * elementPower, 0);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW), factor * elementPower);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW), elementPower);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower + elementPower);

    // Check setting both works
    factor = 3.;
    hpwh.setResistanceCapacity(factor * elementPower, -1);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW), factor * elementPower);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW), factor * elementPower);
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW), 2. * factor * elementPower);
}

/*
 * storageTankErrors tests
 */
TEST(ScaleTest, storageTankErrors)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "StorageTank";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    EXPECT_EQ(hpwh.setResistanceCapacity(1000.), HPWH::HPWH_ABORT);
    EXPECT_EQ(hpwh.setScaleCapacityCOP(1., 1.), HPWH::HPWH_ABORT);
}
