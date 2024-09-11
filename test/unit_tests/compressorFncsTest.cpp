/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct CompressorFncsTest : public testing::Test
{
    const int intAbort = -1;
    const double dblAbort = -1;

    struct ModelSpecs
    {
        std::string sModelName;
        bool hasCompressor;
        int coilConfig; // 1: Wrapped, 2: External
        int isMultipass;
        int isExternalMultipass;
        HPWH::Temp_t maxSetpointT;
        HPWH::Temp_t minT;
    };

    const std::vector<ModelSpecs> modelSpecs = {
        {"AOSmithHPTU50", true, 1, true, false, HPWH::MAXOUTLET_R134A(), {42., Units::F}},
        {"Stiebel220e", true, 1, true, false, HPWH::MAXOUTLET_R134A(), {32., Units::F}},
        {"AOSmithCAHP120", true, 1, true, false, HPWH::MAXOUTLET_R134A(), {47., Units::F}},
        {"Sanden80", true, 2, false, false, HPWH::MAXOUTLET_R744(), {-25., Units::F}},
        {"ColmacCxV_5_SP", true, 2, false, false, HPWH::MAXOUTLET_R410A(), {-4., Units::F}},
        {"ColmacCxA_20_SP", true, 2, false, false, HPWH::MAXOUTLET_R134A(), {40., Units::F}},
        {"TamScalable_SP", true, 2, false, false, HPWH::MAXOUTLET_R134A(), {40., Units::F}},
        {"ColmacCxA_20_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A(), {40., Units::F}},
        {"Scalable_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A(), {40., Units::F}},
        {"NyleC90A_MP", true, 2, true, true, {160., Units::F}, {40., Units::F}},
        {"NyleC90A_C_MP", true, 2, true, true, {160., Units::F}, {35., Units::F}},
        {"QAHV_N136TAU_HPB_SP", true, 2, false, false, {176.1, Units::F}, {-13., Units::F}},
        {"restankRealistic", false, intAbort, intAbort, intAbort, dblAbort, dblAbort},
        {"StorageTank", false, intAbort, intAbort, intAbort, dblAbort, dblAbort}};
};

/*
 * compressorSpecs tests
 */
TEST_F(CompressorFncsTest, compressorSpecs)
{
    for (auto& modelSpec : modelSpecs)
    {
        // get preset model
        HPWH hpwh;
        hpwh.initPreset(modelSpec.sModelName);

        EXPECT_EQ(hpwh.hasACompressor(), modelSpec.hasCompressor) << modelSpec.sModelName;
        if (modelSpec.hasCompressor)
        {
            EXPECT_EQ(hpwh.getCompressorCoilConfig(), modelSpec.coilConfig) << modelSpec.sModelName;
            EXPECT_EQ(hpwh.isCompressorMultipass(), modelSpec.isMultipass) << modelSpec.sModelName;
            EXPECT_EQ(hpwh.isCompressorExternalMultipass(), modelSpec.isExternalMultipass)
                << modelSpec.sModelName;
            EXPECT_EQ(hpwh.getMaxCompressorSetpointT(), modelSpec.maxSetpointT)
                << modelSpec.sModelName;
            EXPECT_EQ(hpwh.getMinOperatingT(), modelSpec.minT) << modelSpec.sModelName;
        }
        else
        {
            EXPECT_ANY_THROW(hpwh.getCompressorCoilConfig()) << modelSpec.sModelName;
            EXPECT_ANY_THROW(hpwh.isCompressorMultipass()) << modelSpec.sModelName;
            EXPECT_ANY_THROW(hpwh.isCompressorExternalMultipass()) << modelSpec.sModelName;
            EXPECT_ANY_THROW(hpwh.getMaxCompressorSetpointT()) << modelSpec.sModelName;
            EXPECT_ANY_THROW(hpwh.getMinOperatingT()) << modelSpec.sModelName;
        }
    }
}
