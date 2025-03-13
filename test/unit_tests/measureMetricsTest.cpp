/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "HPWHFitter.hh"
#include "unit-test.hh"

struct MeasureMetricsTest : public testing::Test
{
    HPWH::FirstHourRating firstHourRating;
    HPWH::StandardTestOptions standardTestOptions;
    HPWH::StandardTestSummary standardTestSummary;

    MeasureMetricsTest()
    {
        standardTestOptions.saveOutput = false;
        standardTestOptions.outputStream = &std::cout;
        standardTestOptions.changeSetpoint = true;
        standardTestOptions.nTestTCouples = 6;
        standardTestOptions.setpointT_C = 51.7;
    }
};

/*
 * AquaThermAire tests
 */
TEST_F(MeasureMetricsTest, AquaThermAire)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AquaThermAire";
    hpwh.initPreset(sModelName);

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(standardTestSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 272.5659, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::Medium);
    EXPECT_NEAR(standardTestSummary.UEF, 2.6493, 1.e-4);
}

/*
 * AOSmithHPTS50 tests
 */
TEST_F(MeasureMetricsTest, AOSmithHPTS50)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AOSmithHPTS50";
    hpwh.initPreset(sModelName);

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(standardTestSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 188.0432, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::Low);
    EXPECT_NEAR(standardTestSummary.UEF, 4.0018, 1.e-4);
}

/*
 * AOSmithHPTS80 tests
 */
TEST_F(MeasureMetricsTest, AOSmithHPTS80)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AOSmithHPTS80";
    hpwh.initPreset(sModelName);

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(standardTestSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 310.9384, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::High);
    EXPECT_NEAR(standardTestSummary.UEF, 4.3272, 1.e-4);
}

/*
 * make Tier-4
 */
TEST_F(MeasureMetricsTest, MakeGenericTier4)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AWHSTier4Generic50";
    hpwh.initPreset(sModelName);

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    hpwh.customTestOptions.overrideFirstHourRating = true;
    hpwh.customTestOptions.desig = firstHourRating.desig;

    constexpr double E50 = 3.7;
    constexpr double UEF = 4.3;
    constexpr double E95 = 4.9;

    { // E50 (50 degF)
        const double ambientT_C = 10.;
        HPWH::GenericOptions genericOptions;

        HPWH::Fitter::UEF_MeritInput uef_merit(E50, ambientT_C);
        genericOptions.meritInputs.push_back(&uef_merit);

        HPWH::Fitter::COP_CoefInput copCoeffInput0(0, 0);
        genericOptions.paramInputs.push_back(&copCoeffInput0);

        HPWH::Fitter::COP_CoefInput copCoeffInput1(0, 1);
        genericOptions.paramInputs.push_back(&copCoeffInput1);

        EXPECT_NO_THROW(hpwh.makeGeneric(genericOptions, standardTestOptions))
            << "Could not make generic model.";
    }
    { // UEF (67.5 degF)
        const double ambientT_C = 19.7;

        HPWH::GenericOptions genericOptions;

        HPWH::Fitter::UEF_MeritInput uef_merit(UEF, ambientT_C);
        genericOptions.meritInputs.push_back(&uef_merit);

        HPWH::Fitter::COP_CoefInput copCoeffInput0(1, 0);
        genericOptions.paramInputs.push_back(&copCoeffInput0);

        HPWH::Fitter::COP_CoefInput copCoeffInput1(1, 1);
        genericOptions.paramInputs.push_back(&copCoeffInput1);

        EXPECT_NO_THROW(hpwh.makeGeneric(genericOptions, standardTestOptions))
            << "Could not make generic model.";
    }
    { // E95 (95 degF)
        const double ambientT_C = 35.;

        HPWH::GenericOptions genericOptions;

        HPWH::Fitter::UEF_MeritInput uef_merit(E95, ambientT_C);
        genericOptions.meritInputs.push_back(&uef_merit);

        HPWH::Fitter::COP_CoefInput copCoeffInput0(2, 0);
        genericOptions.paramInputs.push_back(&copCoeffInput0);

        HPWH::Fitter::COP_CoefInput copCoeffInput1(2, 1);
        genericOptions.paramInputs.push_back(&copCoeffInput1);

        EXPECT_NO_THROW(hpwh.makeGeneric(genericOptions, standardTestOptions))
            << "Could not make generic model.";

        hpwh.customTestOptions.ambientT_C = 35;
        EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(standardTestSummary.UEF, E95, 1.e-12) << "Did not measure expected E95";
    }
    { // verify E50
        hpwh.customTestOptions.overrideAmbientT = true;
        hpwh.customTestOptions.ambientT_C = 10.;
        EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(standardTestSummary.UEF, E50, 1.e-12) << "Did not measure expected E50";
    }
    { // verify UEF
        hpwh.customTestOptions.ambientT_C = 19.7;
        EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(standardTestSummary.UEF, UEF, 1.e-12) << "Did not measure expected UEF";
    }
    { // verify E95
        hpwh.customTestOptions.ambientT_C = 35;
        EXPECT_NO_THROW(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(standardTestSummary.UEF, E95, 1.e-12) << "Did not measure expected E95";
    }
}
