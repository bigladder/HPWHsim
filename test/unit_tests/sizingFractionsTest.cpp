/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

constexpr double logicSize = static_cast<double>(HPWH::LOGIC_SIZE);

constexpr double tol = 1.e-4;

/*
 * TamScalable_SP_SizingFract tests
 */
TEST(SizingFractionsTest, TamScalable_SP_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP"; // Just a compressor with R134A
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 4. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / logicSize, tol);
}

/*
 * Sanden80_SizingFract tests
 */
TEST(SizingFractionsTest, Sanden80_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Sanden80";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 8. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / logicSize, tol);
}

/*
 * ColmacCxV_5_SP_SizingFract tests
 */
TEST(SizingFractionsTest, ColmacCxV_5_SP_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_SP";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 4. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / logicSize, tol);
}

/*
 * AOSmithHPTU50_SizingFract tests
 */
TEST(SizingFractionsTest, AOSmithHPTU50_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AOSmithHPTU50";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (1. + 2. + 3. + 4.) / 4. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1., tol);
}

/*
 * GE_SizingFract tests
 */
TEST(SizingFractionsTest, GE_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "GE";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (1. + 2. + 3. + 4.) / 4. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1., tol);
}

/*
 * Stiebel220e_SizingFract tests
 */
TEST(SizingFractionsTest, Stiebel220e_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Stiebel220e";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (5. + 6.) / 2. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / logicSize, tol);
}

/*
 * resTankRealistic_SizingFract tests
 */
TEST(SizingFractionsTest, resTankRealistic_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "restankRealistic";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    EXPECT_ANY_THROW(hpwh.getSizingFractions(aquaFrac, useableFrac));
}

/*
 * storageTank_SizingFract tests
 */
TEST(SizingFractionsTest, storageTank_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "StorageTank";
    hpwh.initPreset(sModelName);

    double aquaFrac, useableFrac;
    EXPECT_ANY_THROW(hpwh.getSizingFractions(aquaFrac, useableFrac));
}

/*
 * getCompressorMinRuntime tests
 */
TEST(SizingFractionsTest, getCompressorMinRuntime)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP"; // Just a compressor with R134A
    hpwh.initPreset(sModelName);

    HPWH::Time_t expectedRunTime = {10., Units::min};

    auto runTime = hpwh.getCompressorMinRuntime();
    EXPECT_EQ(runTime, expectedRunTime);
}
