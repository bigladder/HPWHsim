/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct HeatingLogicsTest : public testing::Test
{
    const std::vector<std::string> sHighShutOffSP_modelNames = {"Sanden80",
                                                                "QAHV_N136TAU_HPB_SP",
                                                                "ColmacCxA_20_SP",
                                                                "ColmacCxV_5_SP",
                                                                "NyleC60A_SP",
                                                                "NyleC60A_C_SP",
                                                                "NyleC185A_C_SP",
                                                                "TamScalable_SP"};

    const std::vector<std::string> sNoHighShutOffMP_externalModelNames = {"Scalable_MP",
                                                                          "ColmacCxA_20_MP",
                                                                          "ColmacCxA_15_MP",
                                                                          "ColmacCxV_5_MP",
                                                                          "NyleC90A_MP",
                                                                          "NyleC90A_C_MP",
                                                                          "NyleC250A_MP",
                                                                          "NyleC250A_C_MP"};

    const std::vector<std::string> sNoHighShutOffIntegratedModelNames = {"AOSmithHPTU80",
                                                                         "Rheem2020Build80",
                                                                         "Stiebel220e",
                                                                         "AOSmithCAHP120",
                                                                         "AWHSTier3Generic80",
                                                                         "Generic1",
                                                                         "RheemPlugInDedicated50",
                                                                         "RheemPlugInShared65",
                                                                         "restankRealistic",
                                                                         "StorageTank"};
};

/*
 * highShutOffSP tests
 */
TEST_F(HeatingLogicsTest, highShutOffSP)
{
    for (auto& modelName : sHighShutOffSP_modelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(modelName);

        { // testHasEnteringWaterShutOff
            int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
            EXPECT_TRUE(hpwh.hasEnteringWaterHighTempShutOff(index));
        }

        { // testSetEnteringWaterShuffOffOutOfBoundsIndex
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff(10., false, -1));
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff(10., false, 15));
        }

        { // testSetEnteringWaterShuffOffDeadbandToSmall
            double value = HPWH::MINSINGLEPASSLIFT - 1.;
            EXPECT_ANY_THROW(
                hpwh.setEnteringWaterHighTempShutOff(value, false, hpwh.getCompressorIndex()));

            value = HPWH::MINSINGLEPASSLIFT;
            EXPECT_NO_THROW(
                hpwh.setEnteringWaterHighTempShutOff(value, false, hpwh.getCompressorIndex()));

            value = hpwh.getSetpoint() - (HPWH::MINSINGLEPASSLIFT - 1);
            EXPECT_ANY_THROW(
                hpwh.setEnteringWaterHighTempShutOff(value, true, hpwh.getCompressorIndex()));

            value = hpwh.getSetpoint() - HPWH::MINSINGLEPASSLIFT;
            EXPECT_NO_THROW(
                hpwh.setEnteringWaterHighTempShutOff(value, true, hpwh.getCompressorIndex()));
        }

        { // testSetEnteringWaterHighTempShutOffAbsolute
            const double drawVolume_L = 0.;
            const double externalT_C = 20.;
            const double delta = 2.;
            const double highT_C = 20.;
            const bool doAbsolute = true;

            // make tank cold to force on
            hpwh.setTankToTemperature(highT_C);

            // run a step and check we're heating
            hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // change entering water temp to below temp
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                highT_C - delta, doAbsolute, hpwh.getCompressorIndex()));

            // run a step and check we're not heating.
            hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);

            // and reverse it
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                highT_C + delta, doAbsolute, hpwh.getCompressorIndex()));

            hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);
        }

        { // testSetEnteringWaterHighTempShutOffRelative
            const double drawVolume_L = 0.;
            const double externalT_C = 20.;
            const double delta = 2.;
            const double highT_C = 20.;
            const bool doAbsolute = false;

            const double relativeHighT_C = hpwh.getSetpoint() - highT_C;
            // make tank cold to force on
            hpwh.setTankToTemperature(highT_C);

            // run a step and check we're heating
            hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // change entering water temp to below temp
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                relativeHighT_C + delta, doAbsolute, hpwh.getCompressorIndex()));

            // run a step and check we're not heating.
            hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);

            // and reverse it
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                relativeHighT_C - delta, doAbsolute, hpwh.getCompressorIndex()));

            hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);
        }
    }
}

/*
 * noHighShutOffMP_external tests
 */
TEST_F(HeatingLogicsTest, noShutOffMP_external)
{
    for (auto& modelName : sNoHighShutOffMP_externalModelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(modelName);

        { // testDoesNotHaveEnteringWaterShutOff
            int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
            EXPECT_FALSE(hpwh.hasEnteringWaterHighTempShutOff(index));
        }

        { // testCanNotSetEnteringWaterShutOff
            int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff(10., false, index));
        }
    }
}

/*
 * state-of-charge logics tests
 */
TEST_F(HeatingLogicsTest, stateOfChargeLogics)
{
    std::vector<std::string> sCombinedModelNames = sHighShutOffSP_modelNames;
    sCombinedModelNames.insert(sCombinedModelNames.end(),
                               sNoHighShutOffMP_externalModelNames.begin(),
                               sNoHighShutOffMP_externalModelNames.end());

    const double externalT_C = 20.;
    const double setpointT_C = F_TO_C(149.);

    for (auto& modelName : sCombinedModelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(modelName);

        if (!hpwh.isSetpointFixed())
        {
            double temp;
            std::string tempStr;
            if (!hpwh.isNewSetpointPossible(setpointT_C, temp, tempStr))
            {
                continue; // Numbers don't align for this type
            }
            hpwh.setSetpoint(setpointT_C);
        }

        { // testChangeToStateofChargeControlled
            double value = HPWH::MINSINGLEPASSLIFT - 1.;
            const double externalT_C = 20.;
            const double setpointT_C = F_TO_C(149.);

            const bool originalHasHighTempShutOff =
                hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex());
            if (!hpwh.isSetpointFixed())
            {
                double temp;
                std::string tempStr;
                if (!hpwh.isNewSetpointPossible(setpointT_C, temp, tempStr))
                {
                    continue; // Numbers don't align for this type
                }
                hpwh.setSetpoint(setpointT_C);
            }

            // Just created so should be fasle
            EXPECT_FALSE(hpwh.isSoCControlled());

            // change to SOC control;
            hpwh.switchToSoCControls(.76, .05, 99, true, 49, HPWH::UNITS_F);
            EXPECT_TRUE(hpwh.isSoCControlled());

            // check entering water high temp shut off controll unchanged
            EXPECT_EQ(hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex()),
                      originalHasHighTempShutOff);

            // Test we can change the SoC and run a step and check we're heating
            if (hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex()))
            {
                EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                    setpointT_C - HPWH::MINSINGLEPASSLIFT,
                    true,
                    hpwh.getCompressorIndex())); // Force to ignore this part.
            }
            EXPECT_NO_THROW(hpwh.setTankToTemperature(F_TO_C(100.))); // .51
            hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // Test if we're on and in band stay on
            hpwh.setTankToTemperature(F_TO_C(125)); // .76 (current target)
            hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // Test we can change the SoC and turn off
            hpwh.setTankToTemperature(F_TO_C(133)); // .84
            hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);

            // Test if off and in band stay off
            hpwh.setTankToTemperature(F_TO_C(125)); // .76  (current target)
            hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);
        }

        { // testSetStateOfCharge
            const double tempsForSetSoC[5][3] = {
                {49, 99, 125}, {65, 110, 129}, {32, 120, 121}, {32, 33, 121}, {80, 81, 132.5}};

            for (auto i = 0; i < 5; ++i)
            {
                double coldWaterT_F = tempsForSetSoC[i][0];
                double minUseT_F = tempsForSetSoC[i][1];
                double tankAt76SoC_T_F = tempsForSetSoC[i][2];

                // change to SOC control;
                hpwh.switchToSoCControls(.85, .05, minUseT_F, true, coldWaterT_F, HPWH::UNITS_F);
                EXPECT_TRUE(hpwh.isSoCControlled());

                // Test if we're on and in band stay on
                hpwh.setTankToTemperature(F_TO_C(tankAt76SoC_T_F)); // .76
                hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 1);

                // Test we can change the target SoC and turn off
                hpwh.setTargetSoCFraction(0.5);
                hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 0);

                // Change back in the band and stay turned off
                hpwh.setTargetSoCFraction(0.72);
                hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 0);

                // Drop below the band and turn on
                hpwh.setTargetSoCFraction(0.70);
                hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 0);
            }
        }
    }
}

/*
 * noHighShutOffIntegrated tests
 */
TEST_F(HeatingLogicsTest, noHighShutOffIntegrated)
{
    for (auto& modelName : sNoHighShutOffIntegratedModelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(modelName);

        int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
        { // testDoesNotHaveEnteringWaterShutOff
            EXPECT_FALSE(hpwh.hasEnteringWaterHighTempShutOff(index));
        }

        { // testCannotSetEnteringWaterShutOff
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff(10., false, index));
        }
    }
}

/*
 * extraHeat test
 */
TEST(ExtraHeatTest, extraHeat)
{
    constexpr double tol = 1.e-4;

    const std::string modelName = "StorageTank";

    // get preset model
    HPWH hpwh;
    hpwh.initPreset(modelName);

    const double ambientT_C = 20.;
    const double externalT_C = 20.;
    const double inletVol2_L = 0.;
    const double inletT2_C = 0.;

    double extraPower_W = 1000.;
    std::vector<double> nodePowerExtra_W = {extraPower_W};

    // Test adding extra heat to a tank for one minute
    hpwh.setUA(0.);
    hpwh.setTankToTemperature(20.);

    double Q_init = hpwh.getTankHeatContent_kJ();
    hpwh.runOneStep(
        0, ambientT_C, externalT_C, HPWH::DR_LOC, inletVol2_L, inletT2_C, &nodePowerExtra_W);
    double Q_final = hpwh.getTankHeatContent_kJ();

    double dQ_actual_kJ = (Q_final - Q_init);

    double dQ_expected_kJ = extraPower_W * 60. / 1.e3; // 1 min

    EXPECT_NEAR(dQ_actual_kJ, dQ_expected_kJ, tol);
}

/*
 * construct weighted distribution from height, weight vectors
 */
TEST(WeightedDistributionTest, construct_from_heights_widths)
{
    const std::vector<double> heights = {1., 2., 3., 4., 5.};
    const std::vector<double> weights = {0., 2., 1.5, 1., 0.};
    HPWH::WeightedDistribution weightedDistribution(heights, weights);

    EXPECT_TRUE(weightedDistribution.isValid());
    EXPECT_EQ(weightedDistribution.maximumHeight(), 5.);
    EXPECT_EQ(weightedDistribution.totalWeight(), 0.9);
    EXPECT_EQ(weightedDistribution.maximumWeight(), 2.);
    EXPECT_EQ(weightedDistribution.fractionalHeight(3), 4. / 5.);
    EXPECT_EQ(weightedDistribution.fractionalWeight(3), 1. / 2.);
    EXPECT_EQ(weightedDistribution.normalizedWeight(0.4, 0.8), (1.5 + 1.) / 4.5);
    EXPECT_EQ(weightedDistribution.lowestNormalizedHeight(), 0.2);
}

/*
 * construct weighted distribution from node distribution
 */
TEST(WeightedDistributionTest, construct_from_node_distribution)
{
    const std::vector<double> nodeDistribution = {
        0.2, 0.4, 0.2, 0.1, 0.1, 0., 0., 0., 0., 0., 0., 0.};
    HPWH::WeightedDistribution weightedDistribution(nodeDistribution);

    EXPECT_TRUE(weightedDistribution.isValid());
    EXPECT_EQ(weightedDistribution.maximumHeight(), 1.);
    EXPECT_NEAR_REL_TOL(weightedDistribution.totalWeight(), 1., 1.e-12);
    EXPECT_EQ(weightedDistribution.maximumWeight(), 12. * 0.4);
    EXPECT_NEAR_REL_TOL(
        weightedDistribution.normalizedWeight(2. / 12., 4. / 12.), 0.2 + 0.1, 1.e-12);
    EXPECT_EQ(weightedDistribution.lowestNormalizedHeight(), 0.);
}
