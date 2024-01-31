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

#define STRING(x) #x
#define XSTRING(x) STRING(x)

const double Pi = 4. * atan(1.);

struct HPWH_Tests : public testing::Test
{
    const std::vector<std::string> sModelNames =
    {
        "AOSmithHPTS50",
        "AOSmithPHPT60",
        "AOSmithHPTU80",
        "Sanden80",
        "RheemHB50",
        "Stiebel220e",
        "GE502014",
        "Rheem2020Prem40",
        "Rheem2020Prem50",
        "Rheem2020Build50"
    };
};

struct HPWH_HeatingLogicsTests : public HPWH_Tests
{
    const std::vector<std::string> highShutOffSinglePassModelNames = {"Sanden80",
                                                                      "QAHV_N136TAU_HPB_SP"
                                                                      "ColmacCxA_20_SP",
                                                                      "ColmacCxV_5_SP",
                                                                      "NyleC60A_SP",
                                                                      "NyleC60A_C_SP",
                                                                      "NyleC185A_C_SP",
                                                                      "TamScalable_SP"};

    const std::vector<std::string> noHighShuttOffMultiPassExternalModelNames = {"Scalable_MP",
                                                                                "ColmacCxA_20_MP",
                                                                                "ColmacCxA_15_MP",
                                                                                "ColmacCxV_5_MP",
                                                                                "NyleC90A_MP",
                                                                                "NyleC90A_C_MP",
                                                                                "NyleC250A_MP",
                                                                                "NyleC250A_C_MP"};

    const std::vector<std::string> noHighShutOffIntegratedModelNames = {"AOSmithHPTU80",
                                                                        "Rheem2020Build80",
                                                                        "Stiebel220e",
                                                                        "AOSmithCAHP120",
                                                                        "AWHSTier3Generic80",
                                                                        "Generic1",
                                                                        "RheemPlugInDedicated50",
                                                                        "RheemPlugInShared65",
                                                                        "restankRealistic",
                                                                        "StorageTank"};

    const double setSoC_T_C[5][3] = {
        {49, 99, 125}, {65, 110, 129}, {32, 120, 121}, {32, 33, 121}, {80, 81, 132.5}};
};


TEST_F(HPWH_Tests, tankSizeFixed)
{
    const double tol =  1.e-4;

    for (auto& sModelName : sModelNames)
    {

 // get model number
        HPWH::MODELS model;
        EXPECT_TRUE(HPWH::mapStringToPreset(sModelName, model)) << "Could not find model.";


        HPWH hpwh;

        // set preset
        EXPECT_NEAR(hpwh.initPresets(model), 0, tol) << "Could not initialize model.";


        // get the initial tank size
        double intitialTankSize_gal = hpwh.getTankSize(HPWH::UNITS_GAL);

         // get the target tank size
        double targetTankSize_gal = intitialTankSize_gal + 100.;

        // change the tank size
        hpwh.setTankSize(targetTankSize_gal, HPWH::UNITS_GAL);


    // change tank size (unforced)
        if (hpwh.isTankSizeFixed())
        { // tank size should not have changed
            EXPECT_NEAR(intitialTankSize_gal, hpwh.getTankSize(HPWH::UNITS_GAL), tol)
                << "The tank size has changed when it should not.";
        }
        else
        { // tank size should have changed to target value
            EXPECT_NEAR(targetTankSize_gal, hpwh.getTankSize(HPWH::UNITS_GAL), tol)
                << "The tank size did not change to the target value.";
        }

        // change tank size (forced)
        hpwh.setTankSize(targetTankSize_gal, HPWH::UNITS_GAL, true);


// tank size should have changed to target value
        EXPECT_NEAR(targetTankSize_gal, hpwh.getTankSize(HPWH::UNITS_GAL), tol)
            << "The tank size has not changed when forced.";
 
    }

}
