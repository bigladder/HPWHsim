/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct MeasureMetricsTest : public testing::Test
{
    HPWH::FirstHourRating firstHourRating;
    HPWH::StandardTestOptions standardTestOptions;
    HPWH::StandardTestSummary standardTestSummary;

    MeasureMetricsTest()
    {
        standardTestOptions.saveOutput = false;
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
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model.";

    EXPECT_TRUE(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    EXPECT_TRUE(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(standardTestSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 272.5659, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRatingDesig::Medium);
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
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model.";

    EXPECT_TRUE(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    EXPECT_TRUE(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(standardTestSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 188.0432, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRatingDesig::Low);
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
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model.";

    EXPECT_TRUE(hpwh.findFirstHourRating(firstHourRating, standardTestOptions))
        << "Could not complete first-hour rating test.";

    EXPECT_TRUE(hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(standardTestSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 310.9384, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRatingDesig::High);
    EXPECT_NEAR(standardTestSummary.UEF, 4.3272, 1.e-4);
}
