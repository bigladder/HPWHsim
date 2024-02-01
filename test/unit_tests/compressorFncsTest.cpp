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

struct CompressorFncsTest : public testing::Test
{

    struct ModelSpecs
    {
        std::string sModelName;
        bool hasCompressor;
        int coilConfig; // 1: Wrapped, 2: External
        int isMultipass;
        int isExternalMultipass;
        double maxSetpointT_C;
        double minT_F;
    };

    const std::vector<ModelSpecs> modelSpecs = {
        {"AOSmithHPTU50", true, 1, true, false, HPWH::MAXOUTLET_R134A, 42.},
        {"Stiebel220e", true, 1, true, false, HPWH::MAXOUTLET_R134A, 32.},
        {"AOSmithCAHP120", true, 1, true, false, HPWH::MAXOUTLET_R134A, 47.},
        {"Sanden80", true, 2, false, false, HPWH::MAXOUTLET_R744, -25.},
        {"ColmacCxV_5_SP", true, 2, false, false, HPWH::MAXOUTLET_R410A, -4.},
        {"ColmacCxA_20_SP", true, 2, false, false, HPWH::MAXOUTLET_R134A, 40.},
        {"TamScalable_SP", true, 2, false, false, HPWH::MAXOUTLET_R134A, 40.},
        {"restankRealistic",
         false,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT},
        {"StorageTank",
         false,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT,
         HPWH::HPWH_ABORT},
        {"ColmacCxA_20_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A, 40.},
        {"Scalable_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A, 40.},
        {"NyleC90A_MP", true, 2, true, true, F_TO_C(160.), 40.},
        {"NyleC90A_C_MP", true, 2, true, true, F_TO_C(160.), 35.},
        {"QAHV_N136TAU_HPB_SP", true, 2, false, false, F_TO_C(176.1), -13.}};
};

/*
 * compressorSpecs
 */
TEST_F(CompressorFncsTest, compressorSpecs)
{
    for (auto& modelSpec : modelSpecs)
    {
        // get preset model
        HPWH hpwh;
        EXPECT_TRUE(hpwh.getObject(modelSpec.sModelName))
            << "Could not initialize model " << modelSpec.sModelName;

        EXPECT_EQ(hpwh.hasACompressor(), modelSpec.hasCompressor) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.getCompressorCoilConfig(), modelSpec.coilConfig) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.isCompressorMultipass(), modelSpec.isMultipass) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.isCompressorExternalMultipass(), modelSpec.isExternalMultipass) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.getMaxCompressorSetpoint(), modelSpec.maxSetpointT_C) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.getMinOperatingTemp(HPWH::UNITS_F), modelSpec.minT_F) << modelSpec.sModelName;
    }
}
