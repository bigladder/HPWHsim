/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct CompressorFncsTest : public testing::Test
{
    const int intAbort = HPWH::HPWH_ABORT;
    const double dblAbort = static_cast<double>(HPWH::HPWH_ABORT);

    struct ModelSpecs
    {
        std::string sModelName;
        bool hasCompressor;
        int coilConfig; // 1: Wrapped, 2: External
        bool isCompressorMultipass;
        bool isCompressorExternalMultipass;
        double maxCompressorSetpointT_C;
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
        {"restankRealistic", false, intAbort, false, false, dblAbort, dblAbort},
        {"StorageTank", false, intAbort, false, false, dblAbort, dblAbort},
        {"ColmacCxA_20_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A, 40.},
        {"Scalable_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A, 40.},
        {"NyleC90A_MP", true, 2, true, true, F_TO_C(160.), 40.},
        {"NyleC90A_C_MP", true, 2, true, true, F_TO_C(160.), 35.},
        {"QAHV_N136TAU_HPB_SP", true, 2, false, false, F_TO_C(176.1), -13.}};
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
        EXPECT_EQ(hpwh.initPreset(modelSpec.sModelName), 0)
            << "Could not initialize model " << modelSpec.sModelName;

        EXPECT_EQ(hpwh.hasACompressor(), modelSpec.hasCompressor) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.getCompressorCoilConfig(), modelSpec.coilConfig) << modelSpec.sModelName;
        EXPECT_EQ(hpwh.isCompressorMultipass(), modelSpec.isCompressorMultipass)
            << modelSpec.sModelName;
        EXPECT_EQ(hpwh.isCompressorExternalMultipass(), modelSpec.isCompressorExternalMultipass)
            << modelSpec.sModelName;
        EXPECT_EQ(hpwh.getMaxCompressorSetpointT_C(), modelSpec.maxCompressorSetpointT_C)
            << modelSpec.sModelName;
        EXPECT_EQ(hpwh.getMinOperatingT(HPWH::Units::Temp::F), modelSpec.minT_F)
            << modelSpec.sModelName;
    }
}
