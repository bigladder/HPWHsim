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
    const std::string sModelName = "Sanden80";
    hpwh.initPreset(sModelName);

    const HPWH::Temp_t mainsT = {55., Units::F};
    const HPWH::Temp_t minUsefulT = {110., Units::F};
    double chargeFraction;

    // Check for errors
    EXPECT_NO_THROW(hpwh.calcSoCFraction({125., Units::F}, minUsefulT));

    EXPECT_NO_THROW(hpwh.calcSoCFraction(mainsT, {155., Units::F}));

    EXPECT_NO_THROW(hpwh.calcSoCFraction(mainsT, minUsefulT, {100., Units::F}));

    // Check state of charge returns 1 at setpoint
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT);
    EXPECT_NEAR(chargeFraction, 1., tol);

    chargeFraction = hpwh.calcSoCFraction({mainsT(Units::C) + 5., Units::C}, minUsefulT);
    EXPECT_NEAR(chargeFraction, 1., tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT, {minUsefulT(Units::C) + 5, Units::C});
    EXPECT_NEAR(chargeFraction, 1., tol);

    // Varying max temp
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {110., Units::F});
    EXPECT_NEAR(chargeFraction, 1.70909, tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {120., Units::F});
    EXPECT_NEAR(chargeFraction, 1.4461, tol);

    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {135., Units::F});
    EXPECT_NEAR(chargeFraction, 1.175, tol);
}

/*
 * chargeBelowSetpoint tests
 */
TEST(StateOfChargeFunctionsTest, chargeBelowSetpoint)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_SP";
    hpwh.initPreset(sModelName);

    const HPWH::Temp_t mainsT = {60., Units::F};
    HPWH::Temp_t minUsefulT = {110., Units::F};
    double chargeFraction;

    // Check state of charge returns 1 at setpoint
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT);
    EXPECT_NEAR(chargeFraction, 1., tol);

    // Check state of charge returns 0 when tank below useful
    hpwh.setTankToT({109., Units::F});
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {140., Units::F});
    EXPECT_NEAR(chargeFraction, 0., tol);

    // Check some lower values with tank set at constant temperatures
    hpwh.setTankToT({110., Units::F});
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {140., Units::F});
    EXPECT_NEAR(chargeFraction, 0.625, tol);

    hpwh.setTankToT({120., Units::F});
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {140., Units::F});
    EXPECT_NEAR(chargeFraction, 0.75, tol);

    hpwh.setTankToT({130., Units::F});
    chargeFraction = hpwh.calcSoCFraction(mainsT, minUsefulT, {140., Units::F});
    EXPECT_NEAR(chargeFraction, 0.875, tol);
}
