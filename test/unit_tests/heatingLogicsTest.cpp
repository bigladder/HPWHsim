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
    for (auto& sModelName : sHighShutOffSP_modelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(sModelName);

        { // testHasEnteringWaterShutOff
            int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
            EXPECT_TRUE(hpwh.hasEnteringWaterHighTempShutOff(index));
        }

        { // testSetEnteringWaterShuffOffOutOfBoundsIndex
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff({10., Units::dC}, -1));
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff({10., Units::dC}, 15));
        }

        { // testSetEnteringWaterShuffOffDeadbandToSmall
            HPWH::GenTemp_t value = {HPWH::MINSINGLEPASSLIFT()(Units::dC) - 1., Units::dC};
            EXPECT_ANY_THROW(
                hpwh.setEnteringWaterHighTempShutOff(value, hpwh.getCompressorIndex()));

            value = HPWH::MINSINGLEPASSLIFT();
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(value, hpwh.getCompressorIndex()));

            value = {hpwh.getSetpointT()(Units::C) - (HPWH::MINSINGLEPASSLIFT()(Units::dC) - 1),
                     Units::C};
            EXPECT_ANY_THROW(
                hpwh.setEnteringWaterHighTempShutOff(value, hpwh.getCompressorIndex()));

            value = {hpwh.getSetpointT()(Units::C) - HPWH::MINSINGLEPASSLIFT()(Units::dC),
                     Units::C};
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(value, hpwh.getCompressorIndex()));
        }

        { // testSetEnteringWaterHighTempShutOffAbsolute
            const HPWH::Volume_t drawVolume = 0.;
            const HPWH::Temp_t externalT = {20., Units::C};
            const HPWH::Temp_d_t dT = {2., Units::dC};
            const HPWH::Temp_t highT = {20., Units::C};
            const bool doAbsolute = true;

            // make tank cold to force on
            hpwh.setTankToT(highT);

            // run a step and check we're heating
            hpwh.runOneStep(highT, drawVolume, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // change entering water temp to below temp
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                {highT(Units::C) - dT(Units::dC), Units::C}, hpwh.getCompressorIndex()));

            // run a step and check we're not heating.
            hpwh.runOneStep(highT, drawVolume, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);

            // and reverse it
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                {highT(Units::C) + dT(Units::dC), Units::C}, hpwh.getCompressorIndex()));

            hpwh.runOneStep(highT, drawVolume, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);
        }

        { // testSetEnteringWaterHighTempShutOffRelative
            const HPWH::Volume_t drawVolume = 0.;
            const HPWH::Temp_t externalT = {20., Units::C};
            const HPWH::Temp_d_t dT = {2., Units::dC};
            const HPWH::Temp_t highT = {20., Units::C};

            const HPWH::Temp_d_t relativeHigh_dT = {hpwh.getSetpointT()(Units::C) - highT(Units::C),
                                                    Units::dC}; //?
            // make tank cold to force on
            hpwh.setTankToT(highT);

            // run a step and check we're heating
            hpwh.runOneStep(highT, drawVolume, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // change entering water temp to below temp
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                {relativeHigh_dT(Units::dC) + dT(Units::dC), Units::dC},
                hpwh.getCompressorIndex()));

            // run a step and check we're not heating.
            hpwh.runOneStep(highT, drawVolume, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);

            // and reverse it
            EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                {relativeHigh_dT(Units::dC) - dT(Units::dC), Units::dC},
                hpwh.getCompressorIndex()));

            hpwh.runOneStep(highT, drawVolume, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);
        }
    }
}

/*
 * noHighShutOffMP_external tests
 */
TEST_F(HeatingLogicsTest, noShutOffMP_external)
{
    for (auto& sModelName : sNoHighShutOffMP_externalModelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(sModelName);

        { // testDoesNotHaveEnteringWaterShutOff
            int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
            EXPECT_FALSE(hpwh.hasEnteringWaterHighTempShutOff(index));
        }

        { // testCanNotSetEnteringWaterShutOff
            int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff({10., Units::dC}, index));
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

    HPWH::Temp_t externalT = {20., Units::C};
    HPWH::Temp_t setpointT = {149., Units::F};

    for (auto& sModelName : sCombinedModelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(sModelName);

        if (!hpwh.isSetpointFixed())
        {
            HPWH::Temp_t temp;
            std::string tempStr;
            if (!hpwh.isNewSetpointPossible(setpointT, temp, tempStr))
            {
                continue; // Numbers don't align for this type
            }
            hpwh.setSetpointT(setpointT);
        }

        { // testChangeToStateofChargeControlled
            externalT = {20., Units::C};
            setpointT = {149., Units::F};

            const bool originalHasHighTempShutOff =
                hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex());
            if (!hpwh.isSetpointFixed())
            {
                HPWH::Temp_t temp;
                std::string tempStr;
                if (!hpwh.isNewSetpointPossible(setpointT, temp, tempStr))
                {
                    continue; // Numbers don't align for this type
                }
                hpwh.setSetpointT(setpointT);
            }

            // Just created so should be fasle
            EXPECT_FALSE(hpwh.isSoCControlled());

            // change to SOC control;
            hpwh.switchToSoCControls(.76, .05, {99, Units::F}, true, {49, Units::F});
            EXPECT_TRUE(hpwh.isSoCControlled());

            // check entering water high temp shut off controll unchanged
            EXPECT_EQ(hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex()),
                      originalHasHighTempShutOff);

            // Test we can change the SoC and run a step and check we're heating
            if (hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex()))
            {
                EXPECT_NO_THROW(hpwh.setEnteringWaterHighTempShutOff(
                    {setpointT(Units::C) - HPWH::MINSINGLEPASSLIFT()(Units::dC), Units::C},
                    hpwh.getCompressorIndex())); // Force to ignore this part.
            }
            EXPECT_NO_THROW(hpwh.setTankToT({100., Units::F})); // .51
            hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // Test if we're on and in band stay on
            hpwh.setTankToT({125, Units::F}); // .76 (current target)
            hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 1);

            // Test we can change the SoC and turn off
            hpwh.setTankToT({133, Units::F}); // .84
            hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);

            // Test if off and in band stay off
            hpwh.setTankToT({125, Units::F}); // .76  (current target)
            hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
            EXPECT_EQ(hpwh.isCompressorRunning(), 0);
        }

        { // testSetStateOfCharge
            const double tempsForSetSoC[5][3] = {
                {49, 99, 125}, {65, 110, 129}, {32, 120, 121}, {32, 33, 121}, {80, 81, 132.5}};

            for (auto i = 0; i < 5; ++i)
            {
                HPWH::Temp_t coldWaterT = {tempsForSetSoC[i][0], Units::F};
                HPWH::Temp_t minUseT = {tempsForSetSoC[i][1], Units::F};
                HPWH::Temp_t tankAt76SoC_T = {tempsForSetSoC[i][2], Units::F};

                // change to SOC control;
                hpwh.switchToSoCControls(.85, .05, minUseT, true, coldWaterT);
                EXPECT_TRUE(hpwh.isSoCControlled());

                // Test if we're on and in band stay on
                hpwh.setTankToT(tankAt76SoC_T); // .76
                hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 1);

                // Test we can change the target SoC and turn off
                hpwh.setTargetSoCFraction(0.5);
                hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 0);

                // Change back in the band and stay turned off
                hpwh.setTargetSoCFraction(0.72);
                hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
                EXPECT_EQ(hpwh.isCompressorRunning(), 0);

                // Drop below the band and turn on
                hpwh.setTargetSoCFraction(0.70);
                hpwh.runOneStep(0, externalT, externalT, HPWH::DR_ALLOW);
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
    for (auto& sModelName : sNoHighShutOffIntegratedModelNames)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(sModelName);

        int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
        { // testDoesNotHaveEnteringWaterShutOff
            EXPECT_FALSE(hpwh.hasEnteringWaterHighTempShutOff(index));
        }

        { // testCannotSetEnteringWaterShutOff
            EXPECT_ANY_THROW(hpwh.setEnteringWaterHighTempShutOff({10., Units::dC}, index));
        }
    }
}

/*
 * extraHeat test
 */
TEST(ExtraHeatTest, extraHeat)
{
    constexpr double tol = 1.e-4;

    const std::string sModelName = "StorageTank";

    // get preset model
    HPWH hpwh;
    hpwh.initPreset(sModelName);

    const HPWH::Temp_t ambientT = {20., Units::C};
    const HPWH::Temp_t externalT = {20., Units::C};
    const HPWH::Volume_t inletVol2 = 0;
    const HPWH::Temp_t inletT2 = {0., Units::C};

    HPWH::Power_t extraPower = {1., Units::kW};
    HPWH::PowerVect_t nodePowerExtra = {{extraPower(Units::kW)}, Units::kW};

    // Test adding extra heat to a tank for one minute
    hpwh.setUA(0.);
    hpwh.setTankToT({20., Units::C});

    HPWH::Energy_t Q_init = hpwh.getTankHeatContent();
    hpwh.runOneStep(0, ambientT, externalT, HPWH::DR_LOC, inletVol2, inletT2, &nodePowerExtra);

    HPWH::Energy_t Q_final = hpwh.getTankHeatContent();

    HPWH::Energy_t dQ_actual = Q_final - Q_init;

    HPWH::Energy_t dQ_expected = {extraPower(Units::W) * HPWH::Time_t(1, Units::min)(Units::s),
                                  Units::J}; // 1 min

    EXPECT_NEAR(dQ_actual(), dQ_expected(), tol);
}
