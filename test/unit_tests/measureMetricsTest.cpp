/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "HPWHFitter.hh"
#include "unit-test.hh"

struct MeasureMetricsTest : public testing::Test
{
    HPWH::FirstHourRating firstHourRating;
    HPWH::TestOptions testOptions;
    HPWH::TestSummary testSummary;

    MeasureMetricsTest()
    {
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

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, testOptions))
        << "Could not complete first-hour rating test.";

    testOptions.testConfiguration = HPWH::testConfiguration_UEF;
    EXPECT_NO_THROW(hpwh.run24hrTest(testOptions, testSummary))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(testSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 272.5659, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::Medium);
    EXPECT_NEAR(testSummary.UEF, 2.6493, 1.e-4);
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

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, testOptions))
        << "Could not complete first-hour rating test.";

    testOptions.testConfiguration = HPWH::testConfiguration_UEF;
    EXPECT_NO_THROW(hpwh.run24hrTest(testOptions, testSummary))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(testSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 188.0432, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::Low);
    EXPECT_NEAR(testSummary.UEF, 4.0018, 1.e-4);
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

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, testOptions))
        << "Could not complete first-hour rating test.";

    testOptions.testConfiguration = HPWH::testConfiguration_UEF;
    EXPECT_NO_THROW(hpwh.run24hrTest(testOptions, testSummary))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(testSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 310.9384, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::High);
    EXPECT_NEAR(testSummary.UEF, 4.3272, 1.e-4);
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

    EXPECT_NO_THROW(hpwh.findFirstHourRating(firstHourRating, testOptions))
        << "Could not complete first-hour rating test.";

    constexpr double E50 = 3.7;
    constexpr double UEF = 4.3;
    constexpr double E95 = 4.9;

    EXPECT_NO_THROW(hpwh.makeGenericE50_UEF_E95(E50, UEF, E95)) << "Could not make generic model.";

   HPWH::TestOptions testOptions;

    { // verify E50
        testOptions.testConfiguration = HPWH::testConfiguration_E50;
        EXPECT_NO_THROW(hpwh.run24hrTest(testOptions, testSummary))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.UEF, E50, 1.e-12) << "Did not measure expected E50";
    }
    { // verify UEF
        testOptions.testConfiguration = HPWH::testConfiguration_UEF;
        EXPECT_NO_THROW(hpwh.run24hrTest(testOptions, testSummary))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.UEF, UEF, 1.e-12) << "Did not measure expected UEF";
    }
    { // verify E95
        testOptions.testConfiguration = HPWH::testConfiguration_E95;
        EXPECT_NO_THROW(hpwh.run24hrTest(testOptions, testSummary))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.UEF, E95, 1.e-12) << "Did not measure expected E95";
    }
}
