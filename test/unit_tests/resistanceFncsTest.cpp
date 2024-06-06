/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

/*
 * setResistanceCapacityErrorChecks tests
 */
TEST(ResistanceFunctionsTest, setResistanceCapacityErrorChecks)
{
    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "ColmacCxA_30_SP";
        hpwh.initPreset(sModelName);

        EXPECT_ANY_THROW(hpwh.setResistanceCapacity(100.)); // Needs to be scalable
    }

    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "restankRealistic";
        hpwh.initPreset(sModelName);

        EXPECT_ANY_THROW(hpwh.setResistanceCapacity(-100.));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity(100., 3));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity(100., 30000));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity(100., -3));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity(100., 0, HPWH::UNITS_F));
    }
}

/*
 * getSetResistanceErrors tests
 */
TEST(ResistanceFunctionsTest, getSetResistanceErrors)
{
    HPWH hpwh;
    double lowerElementPower_W = 1000;
    double lowerElementPower = lowerElementPower_W / 1000;
    EXPECT_NO_THROW(hpwh.initResistanceTank(100., 0.95, 0., lowerElementPower_W))
        << "Could not initialize resistance tank.";

    double returnVal;

    returnVal = hpwh.getResistanceCapacity(0, HPWH::UNITS_KW); // lower
    EXPECT_NEAR_REL(returnVal, lowerElementPower);

    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW); // both,
    EXPECT_NEAR_REL(returnVal, lowerElementPower);

    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW)); // higher doesn't exist
}

/*
 * commercialTankInitErrors tests
 */
TEST(ResistanceFunctionsTest, commercialTankInitErrors)
{
    HPWH hpwh;
    // init model
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(-800., 10., 100., 100.)); // negative volume
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(800., 10., -100., 100.)); // negative element
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(800., 10., 100., -100.)); // negative element
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(800., -10., 100., 100.)); // negative r value
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(800., 0., 100., 100.));   // 0 r value
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(800., 10., 0., 0.)); // Check needs one element
}

/*
 * getNumResistanceElements tests
 */
TEST(ResistanceFunctionsTest, getNumResistanceElements)
{
    HPWH hpwh;

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(800., 10., 0., 1000.));
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(800., 10., 1000., 0.));
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(800., 10., 1000., 1000.));
    EXPECT_EQ(hpwh.getNumResistanceElements(), 2); // Check 2 elements
}

/*
 * getResistancePositionInRE_tank tests
 */
TEST(ResistanceFunctionsTest, getResistancePositionInRE_tank)
{
    HPWH hpwh;

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(800., 10., 0., 1000.));
    EXPECT_EQ(hpwh.getResistancePosition(0), 0);     // Check lower element is there
    EXPECT_ANY_THROW(hpwh.getResistancePosition(1)); // Check no element
    EXPECT_ANY_THROW(hpwh.getResistancePosition(2)); // Check no element

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(800., 10., 1000., 0.));
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);     // Check upper element there
    EXPECT_ANY_THROW(hpwh.getResistancePosition(1)); // Check no elements
    EXPECT_ANY_THROW(hpwh.getResistancePosition(2)); // Check no elements

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(800., 10., 1000., 1000.));
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);     // Check upper element there
    EXPECT_EQ(hpwh.getResistancePosition(1), 0);     // Check lower element is there
    EXPECT_ANY_THROW(hpwh.getResistancePosition(2)); // Check 0 elements}
}

/*
 * getResistancePositionInCompressorTank test
 */
TEST(ResistanceFunctionsTest, getResistancePositionInCompressorTank)
{

    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    hpwh.initPreset(sModelName);

    EXPECT_EQ(hpwh.getResistancePosition(0), 9);        // Check top elements
    EXPECT_EQ(hpwh.getResistancePosition(1), 0);        // Check bottom elements
    EXPECT_ANY_THROW(hpwh.getResistancePosition(2));    // Check compressor isn't RE
    EXPECT_ANY_THROW(hpwh.getResistancePosition(-1));   // Check out of bounds
    EXPECT_ANY_THROW(hpwh.getResistancePosition(1000)); // Check out of bounds
}

/*
 * commercialTankErrorsWithBottomElement test
 */
TEST(ResistanceFunctionsTest, commercialTankErrorsWithBottomElement)
{
    const double elementPower_kW = 10.; // kW

    // init model
    HPWH hpwh;
    hpwh.initResistanceTankGeneric(800., 10., 0., elementPower_kW * 1000.);

    // Check only lowest setting works
    double factor = 3.;

    // set both, but really only one
    EXPECT_NO_THROW(
        hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::UNITS_KW)); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets bottom with bottom
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW)); // only have one element

    // set lowest
    factor = 4.;
    EXPECT_NO_THROW(
        hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::UNITS_KW)); // Set just bottom
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets bottom with bottom
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW)); // only have one element

    EXPECT_ANY_THROW(hpwh.setResistanceCapacity(
        factor * elementPower_kW, 1, HPWH::UNITS_KW)); // set top returns error
}

/*
 * commercialTankErrorsWithTopElement tests
 */
TEST(ResistanceFunctionsTest, commercialTankErrorsWithTopElement)
{
    const double elementPower_kW = 10.; // KW

    // init model
    HPWH hpwh;
    hpwh.initResistanceTankGeneric(800., 10., elementPower_kW * 1000., 0.);

    // Check only bottom setting works
    double factor = 3.;

    // set both, but only bottom really.
    EXPECT_NO_THROW(
        hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::UNITS_KW)); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom which is now top with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check the lower and only element
    EXPECT_ANY_THROW(
        hpwh.getResistanceCapacity(1, HPWH::UNITS_KW)); //  error on non existent element

    // set top
    factor = 4.;
    EXPECT_NO_THROW(hpwh.setResistanceCapacity(
        factor * elementPower_kW, 0, HPWH::UNITS_KW)); // only one element to set
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                    factor * elementPower_kW); // Check gets just bottom which is now top with both
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW)); // error on non existant bottom

    // set bottom returns error
    EXPECT_ANY_THROW(hpwh.setResistanceCapacity(factor * elementPower_kW, 2, HPWH::UNITS_KW));
}

struct InsulationPoint
{
    double volume_L;
    double r_ft2hFperBTU; // ft^2.degF/(BTU/h)
    double expectedUA_SI;
};

#define FT2HFperBTU_TO_M2CperW(r_ft2hFperBTU) BTUm2C_per_kWhft2F* r_ft2hFperBTU / 1000.

// #define R_TO_RSI(rvalue) rvalue * 0.176110
#define TEST_INIT_RESISTANCE_TANK_GENERIC(point, elementPower_W)                                   \
    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(point.volume_L,                                 \
                                                   FT2HFperBTU_TO_M2CperW(point.r_ft2hFperBTU),    \
                                                   elementPower_W,                                 \
                                                   elementPower_W))                                \
        << "Could not initialize generic resistance tank.";

/*
 * commercialTankInit tests
 */
TEST(ResistanceFunctionsTest, commercialTankInit)
{
    const InsulationPoint testPoint800 = {800., 10., 10.500366};
    const InsulationPoint testPoint2 = {2., 6., 0.322364};
    const InsulationPoint testPoint50 = {50., 12., 1.37808};
    const InsulationPoint testPoint200 = {200., 16., 2.604420};
    const InsulationPoint testPoint200B = {200., 6., 6.94512163};
    const InsulationPoint testPoint2000 = {2000., 16., 12.0886496};
    const InsulationPoint testPoint20000 = {20000., 6., 149.628109};

    const double elementPower_W = 1.e4;
    double UA;

    HPWH hpwh;

    // Check UA is as expected at 800 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint800, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint800.expectedUA_SI);

    // Check UA independent of elements
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint800, elementPower_W / 1000.);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint800.expectedUA_SI);

    // Check UA is as expected at 2 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint2, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint2.expectedUA_SI);

    // Check UA is as expected at 50 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint50, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint50.expectedUA_SI);

    // Check UA is as expected at 200 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint200, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint200.expectedUA_SI);

    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint200B, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint200B.expectedUA_SI);

    // Check UA is as expected at 2000 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint2000, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint2000.expectedUA_SI);

    // Check UA is as expected at 20000 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint20000, elementPower_W);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA, testPoint20000.expectedUA_SI);
}

#undef TEST_INIT_RESISTANCE_TANK_GENERIC
#undef R_TO_RSI
