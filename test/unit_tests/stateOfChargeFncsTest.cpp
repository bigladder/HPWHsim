/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

constexpr double tol = 1.e-4;

/*
 * getSoC tests
 */
TEST(StateOfChargeFunctionsTest, getSoC)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "Sanco80";
    hpwh.initPreset(modelName);

    const double mainsT_C = F_TO_C(55.);
    const double minUsefulT_C = F_TO_C(110.);
    double chargeFraction;

    // Check for errors
    EXPECT_NO_THROW(hpwh.calcSoCFraction(F_TO_C(125.), minUsefulT_C));

    EXPECT_NO_THROW(hpwh.calcSoCFraction(mainsT_C, F_TO_C(155.)));

    EXPECT_NO_THROW(hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(100.)));

    // Check state of charge returns 1 at setpoint
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C);
    EXPECT_NEAR(chargeFraction, 1., tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT_C + 5., minUsefulT_C);
    EXPECT_NEAR(chargeFraction, 1., tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C + 5.);
    EXPECT_NEAR(chargeFraction, 1., tol);

    // Varying max temp
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(110.));
    EXPECT_NEAR(chargeFraction, 1.70909, tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(120.));
    EXPECT_NEAR(chargeFraction, 1.4461, tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(135.));
    EXPECT_NEAR(chargeFraction, 1.175, tol);
}

/*
 * chargeBelowSetpoint tests
 */
TEST(StateOfChargeFunctionsTest, chargeBelowSetpoint)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "ColmacCxV_5_SP";
    hpwh.initPreset(modelName);

    const double mainsT_C = F_TO_C(60.);
    double minUsefulT_C = F_TO_C(110.);
    double chargeFraction;

    // Check state of charge returns 1 at setpoint
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C);
    EXPECT_NEAR(chargeFraction, 1., tol);

    // Check state of charge returns 0 when tank below useful
    hpwh.setTankToTemperature(F_TO_C(109.));
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(140.));
    EXPECT_NEAR(chargeFraction, 0., tol);

    // Check some lower values with tank set at constant temperatures
    hpwh.setTankToTemperature(F_TO_C(110.));
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(140.));
    EXPECT_NEAR(chargeFraction, 0.625, tol);

    hpwh.setTankToTemperature(F_TO_C(120.));
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(140.));
    EXPECT_NEAR(chargeFraction, 0.75, tol);

    hpwh.setTankToTemperature(F_TO_C(130.));
    chargeFraction = hpwh.calcSoCFraction(mainsT_C, minUsefulT_C, F_TO_C(140.));
    EXPECT_NEAR(chargeFraction, 0.875, tol);
}
