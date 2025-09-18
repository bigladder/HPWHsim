/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "HPWHFitter.hh"
#include "unit-test.hh"

struct MeasureMetricsTest : public testing::Test
{
    HPWH::FirstHourRating firstHourRating;
    HPWH::TestSummary testSummary;
};

/*
 * AquaThermAire tests
 */
TEST_F(MeasureMetricsTest, AquaThermAire)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "AquaThermAire";
    hpwh.initPreset(modelName);

    EXPECT_NO_THROW(firstHourRating = hpwh.findFirstHourRating())
        << "Could not complete first-hour rating test.";

    EXPECT_EQ(firstHourRating.designation, HPWH::FirstHourRating::Designation::Medium);

    EXPECT_NO_THROW(testSummary =
                        hpwh.run24hrTest(HPWH::testConfiguration_UEF, firstHourRating.designation))
        << "Could not complete 24-hr test.";

    EXPECT_TRUE(testSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 272.5659, 1.e-4);

    EXPECT_NEAR(testSummary.EF, 2.6728, 1.e-4);
}

/*
 * AOSmithHPTS50 tests
 */
TEST_F(MeasureMetricsTest, AOSmithHPTS50)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "AOSmithHPTS50";
    hpwh.initPreset(modelName);

    EXPECT_NO_THROW(firstHourRating = hpwh.findFirstHourRating())
        << "Could not complete first-hour rating test.";

    EXPECT_EQ(firstHourRating.designation, HPWH::FirstHourRating::Designation::Low);

    EXPECT_NO_THROW(testSummary =
                        hpwh.run24hrTest(HPWH::testConfiguration_UEF, firstHourRating.designation))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(testSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 188.0302, 1.e-4);

    EXPECT_NEAR(testSummary.EF, 4.0834, 1.e-4);
}

/*
 * AOSmithHPTS80 tests
 */
TEST_F(MeasureMetricsTest, AOSmithHPTS80)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "AOSmithHPTS80";
    hpwh.initPreset(modelName);

    EXPECT_NO_THROW(firstHourRating = hpwh.findFirstHourRating())
        << "Could not complete first-hour rating test.";

    EXPECT_EQ(firstHourRating.designation, HPWH::FirstHourRating::Designation::High);

    EXPECT_NO_THROW(testSummary =
                        hpwh.run24hrTest(HPWH::testConfiguration_UEF, firstHourRating.designation))
        << "Could not complete complete 24-hr test.";

    EXPECT_TRUE(testSummary.qualifies);
    EXPECT_NEAR(firstHourRating.drawVolume_L, 310.8838, 1.e-4);
    EXPECT_NEAR(testSummary.EF, 4.3865, 1.e-4);
}

/*
 * make Tier-4 from UEF only
 */
TEST_F(MeasureMetricsTest, MakeGenericTier4_UEF)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "Rheem2020Prem50";
    hpwh.initPreset(modelName);
    hpwh.makeCondenserPerformance(HPWH::tier4);

    EXPECT_NO_THROW(firstHourRating = hpwh.findFirstHourRating())
        << "Could not complete first-hour rating sequence.";

    constexpr double UEF = 4.3;
    EXPECT_NO_THROW(hpwh.makeGenericUEF(UEF, firstHourRating.designation, HPWH::tier4))
        << "Could not make generic model.";

    { // verify UEF
        EXPECT_NO_THROW(testSummary = hpwh.run24hrTest(HPWH::testConfiguration_UEF,
                                                       firstHourRating.designation))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.EF, UEF, 1.e-12) << "Did not measure expected UEF";
    }
}

/*
 * make Tier-4
 */
TEST_F(MeasureMetricsTest, MakeGenericTier4_E50_UEF_E95)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "Rheem2020Prem50";
    hpwh.initPreset(modelName);
    hpwh.makeCondenserPerformance(HPWH::tier4);

    EXPECT_NO_THROW(firstHourRating = hpwh.findFirstHourRating())
        << "Could not complete first-hour rating test.";

    constexpr double E50 = 3.7;
    constexpr double UEF = 4.3;
    constexpr double E95 = 4.9;

    EXPECT_NO_THROW(
        hpwh.makeGenericE50_UEF_E95(E50, UEF, E95, firstHourRating.designation, HPWH::tier4))
        << "Could not make generic model.";

    { // verify E50
        EXPECT_NO_THROW(testSummary = hpwh.run24hrTest(HPWH::testConfiguration_E50,
                                                       firstHourRating.designation))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.EF, E50, 1.e-12) << "Did not measure expected E50";
    }
    { // verify UEF
        EXPECT_NO_THROW(testSummary = hpwh.run24hrTest(HPWH::testConfiguration_UEF,
                                                       firstHourRating.designation))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.EF, UEF, 1.e-12) << "Did not measure expected UEF";
    }
    { // verify E95
        EXPECT_NO_THROW(testSummary = hpwh.run24hrTest(HPWH::testConfiguration_E95,
                                                       firstHourRating.designation))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.EF, E95, 1.e-12) << "Did not measure expected E95";
    }
}

/*
 * make Tier-4
 */
TEST_F(MeasureMetricsTest, InitGenericFunction)
{
    // get preset model
    HPWH hpwh;
    constexpr double UEF = 1.74;
    hpwh.initGeneric(GAL_TO_L(45.), UEF, 20.);
    EXPECT_NO_THROW(firstHourRating = hpwh.findFirstHourRating())
        << "Could not complete first-hour rating test.";

    { // verify UEF
        EXPECT_NO_THROW(testSummary = hpwh.run24hrTest(HPWH::testConfiguration_UEF,
                                                       firstHourRating.designation))
            << "Could not complete complete 24-hr test.";
        EXPECT_NEAR(testSummary.EF, 1.81, 1.e-12) << "Did not measure expected UEF";
    }
}

