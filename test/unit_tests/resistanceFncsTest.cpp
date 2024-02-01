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
#include "unit-test.hh"

/*
 * setResistanceCapacityErrorChecks test
 */
TEST(ResistanceFunctionsTest, setResistanceCapacityErrorChecks)
{
    {
        const std::string sModelName = "ColmacCxA_30_SP";

        // get preset model
        HPWH hpwh;
        EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

        EXPECT_EQ(hpwh.setResistanceCapacity(100.), HPWH::HPWH_ABORT); // Need's to be scalable
    }

    {
        const std::string sModelName = "restankRealistic";

        // get preset model
        HPWH hpwh;
        EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

        EXPECT_EQ(hpwh.setResistanceCapacity(-100.), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., 3), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., 30000), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., -3), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., 0, HPWH::UNITS_F), HPWH::HPWH_ABORT);
    }
}

/*
 * getSetResistanceErrors test
 */
TEST(ResistanceFunctionsTest, getSetResistanceErrors)
{
    HPWH hpwh;
    double lowerElementPower_W = 1000;
    double lowerElementPower = lowerElementPower_W / 1000;
    hpwh.HPWHinit_resTank(100., 0.95, 0., lowerElementPower_W);

    double returnVal;

    returnVal = hpwh.getResistanceCapacity(0, HPWH::UNITS_KW); // lower
    EXPECT_NEAR_REL(returnVal, lowerElementPower);

    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW); // both,
    EXPECT_NEAR_REL(returnVal, lowerElementPower);

    returnVal = hpwh.getResistanceCapacity(1, HPWH::UNITS_KW); // higher doesn't exist
    EXPECT_EQ(returnVal, HPWH::HPWH_ABORT);
}

/*
 * commercialTankInitErrors test
 */
TEST(ResistanceFunctionsTest, commercialTankInitErrors)
{
    HPWH hpwh;

    // init model
    EXPECT_EQ(hpwh.HPWHinit_resTankGeneric(-800., 10., 100., 100.),
              HPWH::HPWH_ABORT); // negative volume
    EXPECT_EQ(hpwh.HPWHinit_resTankGeneric(800., 10., -100., 100.),
              HPWH::HPWH_ABORT); // negative element
    EXPECT_EQ(hpwh.HPWHinit_resTankGeneric(800., 10., 100., -100.),
              HPWH::HPWH_ABORT); // negative element
    EXPECT_EQ(hpwh.HPWHinit_resTankGeneric(800., -10., 100., 100.),
              HPWH::HPWH_ABORT); // negative r value
    EXPECT_EQ(hpwh.HPWHinit_resTankGeneric(800., 0., 100., 100.), HPWH::HPWH_ABORT); // 0 r value
    EXPECT_EQ(hpwh.HPWHinit_resTankGeneric(800., 10., 0., 0.),
              HPWH::HPWH_ABORT); // Check needs one element
}

/*
 * getNumResistanceElements test
 */
TEST(ResistanceFunctionsTest, getNumResistanceElements)
{
    HPWH hpwh;

    hpwh.HPWHinit_resTankGeneric(800., 10., 0., 1000.);
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 0.);
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 1000.);
    EXPECT_EQ(hpwh.getNumResistanceElements(), 2); // Check 2 elements
}

/*
 * getResistancePositionInRETank test
 */
TEST(ResistanceFunctionsTest, getResistancePositionInRETank)
{
    HPWH hpwh;

    hpwh.HPWHinit_resTankGeneric(800., 10., 0., 1000.);
    EXPECT_EQ(hpwh.getResistancePosition(0), 0);                // Check lower element is there
    EXPECT_EQ(hpwh.getResistancePosition(1), HPWH::HPWH_ABORT); // Check no element
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT); // Check no element

    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 0.);
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);                // Check upper element there
    EXPECT_EQ(hpwh.getResistancePosition(1), HPWH::HPWH_ABORT); // Check no elements
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT); // Check no elements

    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 1000.);
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);                // Check upper element there
    EXPECT_EQ(hpwh.getResistancePosition(1), 0);                // Check lower element is there
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT); // Check 0 elements}
}

/*
 * getResistancePositionInCompressorTank test
 */
TEST(ResistanceFunctionsTest, getResistancePositionInCompressorTank)
{
    const std::string sModelName = "TamScalable_SP";

    // get preset model
    HPWH hpwh;
    EXPECT_TRUE(hpwh.getObject(sModelName)) << "Could not initialize model " << sModelName;

    EXPECT_EQ(hpwh.getResistancePosition(0), 9);                   // Check top elements
    EXPECT_EQ(hpwh.getResistancePosition(1), 0);                   // Check bottom elements
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT);    // Check compressor isn't RE
    EXPECT_EQ(hpwh.getResistancePosition(-1), HPWH::HPWH_ABORT);   // Check out of bounds
    EXPECT_EQ(hpwh.getResistancePosition(1000), HPWH::HPWH_ABORT); // Check out of bounds
}

/*
 * commercialTankErrorsWithBottomElement test
 */
TEST(ResistanceFunctionsTest, commercialTankErrorsWithBottomElement)
{
    const double elementPower_kW = 10.; // KW

    // init model
    HPWH hpwh;
    hpwh.HPWHinit_resTankGeneric(800., 10., 0., elementPower_kW * 1000.);

    // Check only lowest setting works
    double factor = 3.;

    // set both, but really only one
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::UNITS_KW),
              0); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets bottom with bottom
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW),
              HPWH::HPWH_ABORT); // only have one element

    // set lowest
    factor = 4.;
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::UNITS_KW),
              0); // Set just bottom
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets bottom with bottom
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW),
              HPWH::HPWH_ABORT); // only have one element

    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 1, HPWH::UNITS_KW),
              HPWH::HPWH_ABORT); // set top returns error
}

/*
 * commercialTankErrorsWithTopElement test
 */
TEST(ResistanceFunctionsTest, commercialTankErrorsWithTopElement)
{
    const double elementPower_kW = 10.; // KW

    // init model
    HPWH hpwh;
    hpwh.HPWHinit_resTankGeneric(800., 10., elementPower_kW * 1000., 0.);

    // Check only bottom setting works
    double factor = 3.;

    // set both, but only bottom really.
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::UNITS_KW),
              0); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom which is now top with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check the lower and only element
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW),
              HPWH::HPWH_ABORT); //  error on non existant element

    // set top
    factor = 4.;
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::UNITS_KW),
              0); // only one element to set
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom which is now top with both
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW),
              HPWH::HPWH_ABORT); // error on non existant bottom

    // set bottom returns error
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 2, HPWH::UNITS_KW),
              HPWH::HPWH_ABORT);
}

struct InsulationPoint
{
    double volume_L;
    double rValue_IP;
    double expectedUA_SI;
};

#define R_TO_RSI(rvalue) rvalue * 0.176110
#define INITGEN(point)                                                                             \
    hpwh.HPWHinit_resTankGeneric(point.volume_L,                                                   \
                                 R_TO_RSI(point.rValue_IP),                                        \
                                 elementPower_kW * 1000.,                                          \
                                 elementPower_kW * 1000.)

/*
 * commercialTankInit test
 */
TEST(ResistanceFunctionsTest, commercialTankInit)
{
    const double elementPower_kW = 10.; // KW

    double UA;
    const InsulationPoint testPoint800 = {800., 10., 10.500366};
    const InsulationPoint testPoint2 = {2., 6., 0.322364};
    const InsulationPoint testPoint50 = {50., 12., 1.37808};
    const InsulationPoint testPoint200 = {200., 16., 2.604420};
    const InsulationPoint testPoint200B = {200., 6., 6.94512163};
    const InsulationPoint testPoint2000 = {2000., 16., 12.0886496};
    const InsulationPoint testPoint20000 = {20000., 6., 149.628109};

    HPWH hpwh;

    // Check UA is as expected at 800 gal
    INITGEN(testPoint800);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint800.expectedUA_SI);

    // Check UA independent of elements
    hpwh.HPWHinit_resTankGeneric(
        testPoint800.volume_L, R_TO_RSI(testPoint800.rValue_IP), elementPower_kW, elementPower_kW);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint800.expectedUA_SI);

    // Check UA is as expected at 2 gal
    INITGEN(testPoint2);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint2.expectedUA_SI);

    // Check UA is as expected at 50 gal
    INITGEN(testPoint50);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint50.expectedUA_SI);

    // Check UA is as expected at 200 gal
    INITGEN(testPoint200);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint200.expectedUA_SI);

    INITGEN(testPoint200B);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint200B.expectedUA_SI);

    // Check UA is as expected at 2000 gal
    INITGEN(testPoint2000);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint2000.expectedUA_SI);

    // Check UA is as expected at 20000 gal
    INITGEN(testPoint20000);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint20000.expectedUA_SI);
}

#undef INITGEN
#undef R_TO_RSI
