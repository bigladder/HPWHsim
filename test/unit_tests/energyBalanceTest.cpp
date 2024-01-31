/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// Standard
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

// vendor
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// HPWHsim
#include "HPWH.hh"

/*
 * energyBalance tests
 */
TEST(EnergyBalanceTest, energyBalance)
{
    const double Pi = 4. * atan(1.);

    {
        std::string sModelName = "AOSmithHPTS50";

        // get preset model
        HPWH hpwh;
        EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model.";

        double maxDrawVol_L = 1.;
        const double ambientT_C = 20.;
        const double externalT_C = 20.;

        //
        hpwh.setTankToTemperature(20.);
        hpwh.setInletT(5.);

        constexpr double testDuration_min = 60.;
        bool result = true;
        int i_min = 0;
        do
        {
            double t_min = static_cast<double>(i_min);

            double flowFac = sin(Pi * t_min / testDuration_min) - 0.5;
            flowFac += fabs(flowFac); // semi-sinusoidal flow profile
            double drawVol_L = flowFac * maxDrawVol_L;

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            EXPECT_EQ(hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW), 0);
            result &= hpwh.isEnergyBalanced(drawVol_L, prevHeatContent_kJ, 1.e-6);

            ++i_min;
        } while (result && (i_min < testDuration_min));

        EXPECT_TRUE(result) << "Energy balance failed for model " << sModelName;
    }

    /* storage tank with extra heat (solar)*/
    {
        std::string sModelName = "StorageTank";

        // get preset model
        HPWH hpwh;
        EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model.";

        double maxDrawVol_L = 1.;
        const double ambientT_C = 20.;
        const double externalT_C = 20.;

        std::vector<double> nodePowerExtra_W = {1000.};
        //
        hpwh.setUA(0.);
        hpwh.setTankToTemperature(20.);
        hpwh.setInletT(5.);

        constexpr double testDuration_min = 60.;
        bool result = true;
        int i_min = 0;
        do
        {
            double t_min = static_cast<double>(i_min);

            double flowFac = sin(Pi * t_min / testDuration_min) - 0.5;
            flowFac += fabs(flowFac); // semi-sinusoidal flow profile
            double drawVol_L = flowFac * maxDrawVol_L;

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            EXPECT_EQ(
                hpwh.runOneStep(
                    drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW, 0., 0., &nodePowerExtra_W),
                0);
            result &= hpwh.isEnergyBalanced(drawVol_L, prevHeatContent_kJ, 1.e-6);

            ++i_min;
        } while (result && (i_min < testDuration_min));

        EXPECT_TRUE(result) << "Energy balance failed for model " << sModelName;
    }
}
