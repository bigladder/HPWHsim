/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct EnergyBalanceTest : public testing::Test
{
    const double Pi = 4. * atan(1.);
};

/*
 * energyBalance tests
 */
TEST_F(EnergyBalanceTest, AOSmithHPTS50)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "AOSmithHPTS50";
    hpwh.initPreset(modelName);

    const double maxDrawVol_L = 1.;
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
        EXPECT_NO_THROW(hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW))
            << "Failure in hpwh.runOneStep.";
        result &= hpwh.isEnergyBalanced(drawVol_L, prevHeatContent_kJ, 1.e-6);

        ++i_min;
    } while (result && (i_min < testDuration_min));

    EXPECT_TRUE(result) << "Energy balance failed for model " << modelName;
}

/* storage tank with extra heat (solar) */
TEST_F(EnergyBalanceTest, StorageTankWithSolar)
{
    HPWH hpwh;
    const std::string modelName = "StorageTank";
    hpwh.initPreset(modelName);

    const double maxDrawVol_L = 1.;
    const double ambientT_C = 20.;
    const double externalT_C = 20.;

    //
    std::vector<double> nodePowerExtra_W = {1000.};
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
        EXPECT_NO_THROW(hpwh.runOneStep(
            drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW, 0., 0., &nodePowerExtra_W))
            << "Failure in hpwh.runOneStep.";
        result &= hpwh.isEnergyBalanced(drawVol_L, prevHeatContent_kJ, 1.e-6);

        ++i_min;
    } while (result && (i_min < testDuration_min));

    EXPECT_TRUE(result) << "Energy balance failed for model " << modelName;
}

/* high draw */
TEST_F(EnergyBalanceTest, highDraw)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "AOSmithHPTS50";
    hpwh.initPreset(modelName);

    hpwh.setInletT(5.);
    const double ambientT_C = 20.;
    const double externalT_C = 20.;

    bool result = true;

    //
    hpwh.setTankToTemperature(20.);
    double drawVol_L = 1.01 * hpwh.getTankVolume_L(); // > tank volume
    double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();

    EXPECT_NO_THROW(hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW))
        << "Failure in hpwh.runOneStep.";
    result &= hpwh.isEnergyBalanced(drawVol_L, prevHeatContent_kJ, 1.e-6);

    //
    hpwh.setTankToTemperature(20.);
    drawVol_L = 2. * hpwh.getTankVolume_L(); // > tank volume
    prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();

    EXPECT_NO_THROW(hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW))
        << "Failure in hpwh.runOneStep.";
    result &= hpwh.isEnergyBalanced(drawVol_L, prevHeatContent_kJ, 1.e-6);

    EXPECT_TRUE(result) << "Energy balance failed for model " << modelName;
}

/* two inlets */
TEST_F(EnergyBalanceTest, twoInlets)
{
    {
        // get preset model
        HPWH hpwh;
        const std::string modelName = "AOSmithHPTS50";
        hpwh.initPreset(modelName);

        double drawVol_L = 0.4 * hpwh.getTankVolume_L(); // < tank volume
        double inlet1Vol_L = 0.6 * drawVol_L;            // split between two inlets
        double inlet2Vol_L = drawVol_L - inlet1Vol_L;

        const double ambientT_C = 20.;
        const double externalT_C = 20.;

        { // inlets at bottom and top of tank
            hpwh.setInletByFraction(0.);
            hpwh.setInlet2ByFraction(1.);
            hpwh.setTankToTemperature(53.);
            double inlet1T_C = 5.;
            double inlet2T_C = 60.;

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            hpwh.runOneStep(inlet1T_C,
                            drawVol_L,
                            ambientT_C,
                            externalT_C,
                            HPWH::DR_ALLOW,
                            inlet2Vol_L,
                            inlet2T_C);

            EXPECT_TRUE(hpwh.isEnergyBalanced(
                inlet1Vol_L, inlet1T_C, inlet2Vol_L, inlet2T_C, prevHeatContent_kJ, 1.e-6))
                << "Failure in two-inlet test:" << modelName;
        }
        {                                   // inlets at 1/4 and 3/4 heights
            hpwh.setInletByFraction(0.25);  // node 3
            hpwh.setInlet2ByFraction(0.75); // node 9
            hpwh.setTankToTemperature(53.);
            double inlet1T_C = 5.;
            double inlet2T_C = 60.;

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            hpwh.runOneStep(inlet1T_C,
                            drawVol_L,
                            ambientT_C,
                            externalT_C,
                            HPWH::DR_ALLOW,
                            inlet2Vol_L,
                            inlet2T_C);

            EXPECT_TRUE(hpwh.isEnergyBalanced(
                inlet1Vol_L, inlet1T_C, inlet2Vol_L, inlet2T_C, prevHeatContent_kJ, 1.e-6))
                << "Failure in two-inlet test:" << modelName;
        }
        { // invert inlets
            hpwh.setInletByFraction(1.);
            hpwh.setInlet2ByFraction(0.);
            hpwh.setTankToTemperature(53.);
            double inlet1T_C = 5.;
            double inlet2T_C = 60.;

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            hpwh.runOneStep(inlet1T_C,
                            drawVol_L,
                            ambientT_C,
                            externalT_C,
                            HPWH::DR_ALLOW,
                            inlet2Vol_L,
                            inlet2T_C);

            EXPECT_TRUE(hpwh.isEnergyBalanced(
                inlet1Vol_L, inlet1T_C, inlet2Vol_L, inlet2T_C, prevHeatContent_kJ, 1.e-6))
                << "Failure in two-inlet test:" << modelName;
        }
        { // swap temps
            hpwh.setInletByFraction(0.);
            hpwh.setInlet2ByFraction(1.);
            hpwh.setTankToTemperature(53.);
            double inlet1T_C = 60.;
            double inlet2T_C = 5.;

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            hpwh.runOneStep(inlet1T_C,
                            drawVol_L,
                            ambientT_C,
                            externalT_C,
                            HPWH::DR_ALLOW,
                            inlet2Vol_L,
                            inlet2T_C);

            EXPECT_TRUE(hpwh.isEnergyBalanced(
                inlet1Vol_L, inlet1T_C, inlet2Vol_L, inlet2T_C, prevHeatContent_kJ, 1.e-6))
                << "Failure in two-inlet test:" << modelName;
        }
    }
}
