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
        hpwh.initPreset(sModelName);

        // get the initial tank size
        HPWH::Volume_t intitialTankSize = hpwh.getTankSize();

        HPWH::Volume_t dV = {100., Units::gal};

        // get the target tank size
        HPWH::Volume_t targetTankSize = intitialTankSize + dV;

        // change the tank size

        // change tank size (unforced)
        if (hpwh.isTankSizeFixed())
        { // tank size should not have changed
            EXPECT_ANY_THROW(hpwh.setTankSize(targetTankSize));
            EXPECT_NEAR(intitialTankSize, hpwh.getTankSize(), tol)
                << "The tank size has changed when it should not.";
        }
        else
        { // tank size should have changed to target value
            EXPECT_NO_THROW(hpwh.setTankSize(targetTankSize));
            EXPECT_NEAR(targetTankSize, hpwh.getTankSize(), tol)
                << "The tank size did not change to the target value.";
        }

        // change tank size (forced)
        hpwh.setTankSize(targetTankSize, true);

        // tank size should have changed to target value
        EXPECT_NEAR(targetTankSize, hpwh.getTankSize(), tol)
            << "The tank size has not changed when forced.";
    }
}
