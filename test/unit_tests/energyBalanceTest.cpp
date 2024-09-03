/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

/*
 * energyBalance tests
 */
TEST(EnergyBalanceTest, energyBalance)
{
    const double Pi = 4. * atan(1.);

    /* no extra heat */
    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "AOSmithHPTS50";
        hpwh.initPreset(sModelName);

        const HPWH::Volume_t maxDrawVol = {1., Units::L};
        const HPWH::Temp_t ambientT = {20., Units::C};
        const HPWH::Temp_t externalT = {20., Units::C};

        //
        hpwh.setTankToT({20., Units::C});
        hpwh.setInletT({5., Units::C});

        const HPWH::Time_t testDuration = {60., Units::min};
        const HPWH::Time_t stepTime = {1., Units::min};
        HPWH::Time_t time = 0;
        bool result = true;
        do
        {
            double flowFac = sin(Pi * time / testDuration) - 0.5;
            flowFac += fabs(flowFac); // semi-sinusoidal flow profile
            HPWH::Volume_t drawVol = flowFac * maxDrawVol;

            HPWH::Energy_t prevHeatContent = hpwh.getTankHeatContent();
            EXPECT_NO_THROW(hpwh.runOneStep(drawVol, ambientT, externalT, HPWH::DR_ALLOW))
                << "Failure in hpwh.runOneStep.";
            result &= hpwh.isEnergyBalanced(drawVol, prevHeatContent, 1.e-6);

            time += stepTime;
        } while (result && (time < testDuration));

        EXPECT_TRUE(result) << "Energy balance failed for model " << sModelName;
    }

    /* storage tank with extra heat (solar) */
    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "StorageTank";
        hpwh.initPreset(sModelName);

        const HPWH::Volume_t maxDrawVol = {1., Units::L};
        const HPWH::Temp_t ambientT = {20., Units::C};
        const HPWH::Temp_t externalT = {20., Units::C};

        //
        HPWH::PowerVect_t nodePowerExtra = {{1000.}, Units::W};
        hpwh.setTankToT({20., Units::C});
        hpwh.setInletT({5., Units::C});
        hpwh.setUA(0.);

        constexpr double testDuration_min = 60.;
        bool result = true;
        const HPWH::Time_t testDuration = {60., Units::min};
        const HPWH::Time_t stepTime = {1., Units::min};
        HPWH::Time_t time = 0;
        do
        {
            double flowFac = sin(Pi * time / testDuration) - 0.5;
            flowFac += fabs(flowFac); // semi-sinusoidal flow profile
            HPWH::Volume_t drawVol = flowFac * maxDrawVol;

            HPWH::Energy_t prevHeatContent = hpwh.getTankHeatContent();
            EXPECT_NO_THROW(hpwh.runOneStep(
                drawVol, ambientT, externalT, HPWH::DR_ALLOW, 0., 0., &nodePowerExtra))
                << "Failure in hpwh.runOneStep.";
            result &= hpwh.isEnergyBalanced(drawVol, prevHeatContent, 1.e-6);

            time += stepTime;
        } while (result && (time < testDuration));

        EXPECT_TRUE(result) << "Energy balance failed for model " << sModelName;
    }
}
