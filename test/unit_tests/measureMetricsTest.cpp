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
        standardTestOptions.setpointT = {51.7, Units::C};
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
    EXPECT_NEAR(firstHourRating.drawVolume(Units::L), 272.5660, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::Medium);
    EXPECT_NEAR(standardTestSummary.UEF, 2.6780, 1.e-4);
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
    EXPECT_NEAR(firstHourRating.drawVolume(Units::L), 188.2549, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::Low);
    EXPECT_NEAR(standardTestSummary.UEF, 4.0357, 1.e-4);
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
    EXPECT_NEAR(firstHourRating.drawVolume(Units::L), 311.0505, 1.e-4);
    EXPECT_EQ(firstHourRating.desig, HPWH::FirstHourRating::Desig::High);
    EXPECT_NEAR(standardTestSummary.UEF, 4.4136, 1.e-4);
}
