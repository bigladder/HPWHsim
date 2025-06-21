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
    const std::string modelName = "TamScalable_SP"; // Just a compressor with R134A
    hpwh.initPreset(modelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 4. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / logicSize, tol);
}

/*
 * Sanco83_SizingFract tests
 */
TEST(SizingFractionsTest, Sanco83_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "Sanco83";
    hpwh.initPreset(modelName);

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
    const std::string modelName = "ColmacCxV_5_SP";
    hpwh.initPreset(modelName);

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
    const std::string modelName = "AOSmithHPTU50";
    hpwh.initPreset(modelName);

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
    const std::string modelName = "GE2012_50";
    hpwh.initPreset(modelName);

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (1. + 2. + 3. + 4.) / 4. / logicSize;

    hpwh.getSizingFractions(aquaFrac, useableFrac);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1., tol);
}

/*
 * Stiebel220e_SizingFract tests
 */
TEST(SizingFractionsTest, Stiebel220E_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string modelName = "Stiebel220E";
    hpwh.initPreset(modelName);

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
    const std::string modelName = "restankRealistic";
    hpwh.initPreset(modelName);

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
    const std::string modelName = "StorageTank";
    hpwh.initPreset(modelName);

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
    const std::string modelName = "TamScalable_SP"; // Just a compressor with R134A
    hpwh.initPreset(modelName);

    double expectedRunTime_min = 10.;
    double expectedRunTime_sec = expectedRunTime_min * 60.;
    double expectedRunTime_hr = expectedRunTime_min / 60.;

    double runTime_min = hpwh.getCompressorMinRuntime();
    EXPECT_EQ(runTime_min, expectedRunTime_min);

    double runTime_sec = hpwh.getCompressorMinRuntime(HPWH::UNITS_SEC);
    EXPECT_EQ(runTime_sec, expectedRunTime_sec);

    double runTime_hr = hpwh.getCompressorMinRuntime(HPWH::UNITS_HR);
    EXPECT_EQ(runTime_hr, expectedRunTime_hr);
}
