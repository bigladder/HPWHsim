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
        std::string modelName;
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
        {"ColmacCxA_20_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A, 40.},
        {"Scalable_MP", true, 2, true, true, HPWH::MAXOUTLET_R134A, 40.},
        {"NyleC90A_MP", true, 2, true, true, F_TO_C(160.), 40.},
        {"NyleC90A_C_MP", true, 2, true, true, F_TO_C(160.), 35.},
        {"QAHV_N136TAU_HPB_SP", true, 2, false, false, F_TO_C(176.1), -13.},
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
        hpwh.initPreset(modelSpec.modelName);

        EXPECT_EQ(hpwh.hasACompressor(), modelSpec.hasCompressor) << modelSpec.modelName;
        if (modelSpec.hasCompressor)
        {
            EXPECT_EQ(hpwh.getCompressorCoilConfig(), modelSpec.coilConfig) << modelSpec.modelName;
            EXPECT_EQ(hpwh.isCompressorMultipass(), modelSpec.isMultipass) << modelSpec.modelName;
            EXPECT_EQ(hpwh.isCompressorExternalMultipass(), modelSpec.isExternalMultipass)
                << modelSpec.modelName;
            EXPECT_NEAR(hpwh.getMaxCompressorSetpoint(), modelSpec.maxSetpointT_C, 1.e-6)
                << modelSpec.modelName;
            EXPECT_NEAR(hpwh.getMinOperatingTemp(HPWH::UNITS_F), modelSpec.minT_F, 1.e-6)
                << modelSpec.modelName;
        }
        else
        {
            EXPECT_ANY_THROW(hpwh.getCompressorCoilConfig()) << modelSpec.modelName;
            EXPECT_ANY_THROW(hpwh.isCompressorMultipass()) << modelSpec.modelName;
            EXPECT_ANY_THROW(hpwh.isCompressorExternalMultipass()) << modelSpec.modelName;
            EXPECT_ANY_THROW(hpwh.getMaxCompressorSetpoint()) << modelSpec.modelName;
            EXPECT_ANY_THROW(hpwh.getMinOperatingTemp(HPWH::UNITS_F)) << modelSpec.modelName;
        }
    }
}