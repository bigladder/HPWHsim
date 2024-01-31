/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// Standard
#include <iostream>
#include <fstream>
#include <vector>
#include <filesystem>

// vendor
#include <fmt/format.h>
#include <gmock/gmock.h>
#include <gtest/gtest.h>

// HPWHsim
#include "HPWH.hh"

#define SIZE static_cast<double>(HPWH::CONDENSITY_SIZE)

constexpr double tol = 1.e-4;

TEST(SizingFractionsTest, TamScalable_SP_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP"; // Just a compressor with R134A
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 4. / SIZE;

    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), 0);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / SIZE, tol);
}

TEST(SizingFractionsTest, Sanden80_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Sanden80";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 8. / SIZE;

    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), 0);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / SIZE, tol);
}

TEST(SizingFractionsTest, ColmacCxV_5_SP_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_SP";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = 4. / SIZE;

    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), 0);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / SIZE, tol);
}

TEST(SizingFractionsTest, AOSmithHPTU50_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AOSmithHPTU50";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (1. + 2. + 3. + 4.) / 4. / SIZE;

    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), 0);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1., tol);
}

TEST(SizingFractionsTest, GE_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "GE";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (1. + 2. + 3. + 4.) / 4. / SIZE;

    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), 0);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1., tol);
}

TEST(SizingFractionsTest, Stiebel220e_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Stiebel220e";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    const double aquaFrac_answer = (5. + 6.) / 2. / SIZE;

    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), 0);
    EXPECT_NEAR(aquaFrac, aquaFrac_answer, tol);
    EXPECT_NEAR(useableFrac, 1. - 1. / SIZE, tol);
}

TEST(SizingFractionsTest, resTankRealistic_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "restankRealistic";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), HPWH::HPWH_ABORT);
}

TEST(SizingFractionsTest, storageTank_SizingFract)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "StorageTank";
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    double aquaFrac, useableFrac;
    EXPECT_EQ(hpwh.getSizingFractions(aquaFrac, useableFrac), HPWH::HPWH_ABORT);
}

TEST(SizingFractionsTest, getCompressorMinRuntime)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP"; // Just a compressor with R134A
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

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
