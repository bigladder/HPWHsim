/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct FixedSizeTest : public testing::Test
{
    const std::vector<std::string> sModelNames = {"AOSmithHPTS50",
                                                  "AOSmithPHPT60",
                                                  "AOSmithHPTU80",
                                                  "Sanden80",
                                                  "RheemHB50",
                                                  "Stiebel220e",
                                                  "GE502014",
                                                  "Rheem2020Prem40",
                                                  "Rheem2020Prem50",
                                                  "Rheem2020Build50"};
};

/*
 * tankSizeFixed tests
 */
TEST_F(FixedSizeTest, tankSizeFixed)
{
    constexpr double tol = 1.e-4;

    for (auto& sModelName : sModelNames)
    {
        // get preset model
        HPWH hpwh;
        EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

        // get the initial tank size
        double intitialTankSize_gal = hpwh.getTankSize(HPWH::V_UNITS::GAL);

        // get the target tank size
        double targetTankSize_gal = intitialTankSize_gal + 100.;

        // change the tank size
        hpwh.setTankSize(targetTankSize_gal, HPWH::V_UNITS::GAL);

        // change tank size (unforced)
        if (hpwh.isTankSizeFixed())
        { // tank size should not have changed
            EXPECT_NEAR(intitialTankSize_gal, hpwh.getTankSize(HPWH::V_UNITS::GAL), tol)
                << "The tank size has changed when it should not.";
        }
        else
        { // tank size should have changed to target value
            EXPECT_NEAR(targetTankSize_gal, hpwh.getTankSize(HPWH::V_UNITS::GAL), tol)
                << "The tank size did not change to the target value.";
        }

        // change tank size (forced)
        hpwh.setTankSize(targetTankSize_gal, HPWH::V_UNITS::GAL, true);

        // tank size should have changed to target value
        EXPECT_NEAR(targetTankSize_gal, hpwh.getTankSize(HPWH::V_UNITS::GAL), tol)
            << "The tank size has not changed when forced.";
    }
}
