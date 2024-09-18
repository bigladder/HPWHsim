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

        EXPECT_ANY_THROW(hpwh.setResistanceCapacity({100., Units::W})); // Needs to be scalable
    }

    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "restankRealistic";
        hpwh.initPreset(sModelName);

        EXPECT_ANY_THROW(hpwh.setResistanceCapacity({-100., Units::W}));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity({100., Units::W}, 3));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity({100., Units::W}, 30000));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity({100., Units::W}, -3));
        EXPECT_ANY_THROW(hpwh.setResistanceCapacity({100., Units::W}, 0));
    }
}

/*
 * getSetResistanceErrors tests
 */
TEST(ResistanceFunctionsTest, getSetResistanceErrors)
{
    HPWH hpwh;
    HPWH::Power_t lowerElementPower = {1000, Units::W};
    EXPECT_NO_THROW(hpwh.initResistanceTank({100., Units::L}, {0.95, Units::m2C_per_W}, Units::P0, lowerElementPower))
        << "Could not initialize resistance tank.";

    HPWH::Power_t returnVal;

    returnVal = hpwh.getResistanceCapacity(0); // lower
    EXPECT_NEAR_REL(returnVal(), lowerElementPower());

    returnVal = hpwh.getResistanceCapacity(-1); // both,
    EXPECT_NEAR_REL(returnVal(), lowerElementPower());

    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1)); // higher doesn't exist
}

/*
 * commercialTankInitErrors tests
 */
TEST(ResistanceFunctionsTest, commercialTankInitErrors)
{
    HPWH hpwh;
    // init model
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric({-800., Units::L},
                                                    {10., Units::m2C_per_W},
                                                    {100., Units::W},
                                                    {100., Units::W})); // negative volume
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric({800., Units::L},
                                                    {10., Units::m2C_per_W},
                                                    {-100., Units::W},
                                                    {100., Units::W})); // negative element
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric({800., Units::L},
                                                    {10., Units::m2C_per_W},
                                                    {100., Units::W},
                                                    {-100., Units::W})); // negative element
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric({800., Units::L},
                                                    {-10., Units::m2C_per_W},
                                                    {100., Units::W},
                                                    {100., Units::W})); // negative r value
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(
        {800., Units::L}, {0., Units::m2C_per_W}, {100., Units::W}, {100., Units::W})); // 0 r value
    EXPECT_ANY_THROW(hpwh.initResistanceTankGeneric(
        {800., Units::L}, {10., Units::m2C_per_W}, Units::P0, Units::P0)); // Check needs one element
}

/*
 * getNumResistanceElements tests
 */
TEST(ResistanceFunctionsTest, getNumResistanceElements)
{
    HPWH hpwh;

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(
        {800., Units::L}, {10., Units::m2C_per_W}, Units::P0, {1000., Units::W}));
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(
        {800., Units::L}, {10., Units::m2C_per_W}, {1000., Units::W}, Units::P0));
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(
        {800., Units::L}, {10., Units::m2C_per_W}, {1000., Units::W}, {1000., Units::W}));
    EXPECT_EQ(hpwh.getNumResistanceElements(), 2); // Check 2 elements
}

/*
 * getResistancePositionInRE_tank tests
 */
TEST(ResistanceFunctionsTest, getResistancePositionInRE_tank)
{
    HPWH hpwh;

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric({800., Units::L}, {10., Units::m2C_per_W}, Units::P0, {1000., Units::W}));
    EXPECT_EQ(hpwh.getResistancePosition(0), 0);     // Check lower element is there
    EXPECT_ANY_THROW(hpwh.getResistancePosition(1)); // Check no element
    EXPECT_ANY_THROW(hpwh.getResistancePosition(2)); // Check no element

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric({800., Units::L}, {10., Units::m2C_per_W}, {1000., Units::W}, Units::P0));
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);     // Check upper element there
    EXPECT_ANY_THROW(hpwh.getResistancePosition(1)); // Check no elements
    EXPECT_ANY_THROW(hpwh.getResistancePosition(2)); // Check no elements

    EXPECT_NO_THROW(hpwh.initResistanceTankGeneric(
        {800., Units::L}, {10., Units::m2C_per_W}, {1000., Units::W}, {1000., Units::W}));
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
    const HPWH::Power_t elementPower = {10., Units::kW};

    // init model
    HPWH hpwh;
    hpwh.initResistanceTankGeneric({800., Units::L}, {10., Units::m2C_per_W}, Units::P0, 1000. * elementPower);

    // Check only lowest setting works
    double factor = 3.;

    // set both, but really only one
    EXPECT_NO_THROW(hpwh.setResistanceCapacity(factor * elementPower, -1)); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1)(),
                    factor * elementPower()); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(),
                    factor * elementPower());          // Check gets bottom with bottom
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1)()); // only have one element

    // set lowest
    factor = 4.;
    EXPECT_NO_THROW(hpwh.setResistanceCapacity(factor * elementPower, 0)); // Set just bottom
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1)(),
                    factor * elementPower()); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(),
                    factor * elementPower());          // Check gets bottom with bottom
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1)()); // only have one element

    EXPECT_ANY_THROW(
        hpwh.setResistanceCapacity(factor * elementPower, 1)); // set top returns error
}

/*
 * commercialTankErrorsWithTopElement tests
 */
TEST(ResistanceFunctionsTest, commercialTankErrorsWithTopElement)
{
    const HPWH::Power_t elementPower(10., Units::kW);

    // init model
    HPWH hpwh;
    hpwh.initResistanceTankGeneric({800., Units::L}, {10., Units::ft2hF_per_Btu}, elementPower, Units::P0);

    // Check only bottom setting works
    double factor = 3.;

    // set both, but only bottom really.
    EXPECT_NO_THROW(hpwh.setResistanceCapacity(factor * elementPower, -1)); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1)(),
                    factor * elementPower()); // Check gets just bottom which is now top with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(),
                    factor * elementPower());        // Check the lower and only element
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1)); //  error on non existent element

    // set top
    factor = 4.;
    EXPECT_NO_THROW(
        hpwh.setResistanceCapacity(factor * elementPower, 0)); // only one element to set
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0)(),
                    factor * elementPower()); // Check gets just bottom which is now top with both
    EXPECT_ANY_THROW(hpwh.getResistanceCapacity(1)); // error on non existant bottom

    // set bottom returns error
    EXPECT_ANY_THROW(hpwh.setResistanceCapacity(factor * elementPower, 2));
}

struct InsulationPoint
{
    HPWH::Volume_t volume;
    HPWH::RFactor_t rFactor; // ft^2.degF/(BTU/h)
    HPWH::UA_t expectedUA;
};

#define TEST_INIT_RESISTANCE_TANK_GENERIC(point, elementPower)                                     \
    EXPECT_NO_THROW(                                                                               \
        hpwh.initResistanceTankGeneric(point.volume, point.rFactor, elementPower, elementPower))   \
        << "Could not initialize generic resistance tank.";

/*
 * commercialTankInit tests
 */
TEST(ResistanceFunctionsTest, commercialTankInit)
{
    const InsulationPoint testPoint800 = {
        {800., Units::L}, {10., Units::ft2hF_per_Btu}, {10.500366, Units::kJ_per_hC}};
    const InsulationPoint testPoint2 = {
        {2., Units::L}, {6., Units::ft2hF_per_Btu}, {0.322364, Units::kJ_per_hC}};
    const InsulationPoint testPoint50 = {
        {50., Units::L}, {12., Units::ft2hF_per_Btu}, {1.37808, Units::kJ_per_hC}};
    const InsulationPoint testPoint200 = {
        {200., Units::L}, {16., Units::ft2hF_per_Btu}, {2.604420, Units::kJ_per_hC}};
    const InsulationPoint testPoint200B = {
        {200., Units::L}, {6., Units::ft2hF_per_Btu}, {6.94512163, Units::kJ_per_hC}};
    const InsulationPoint testPoint2000 = {
        {2000., Units::L}, {16., Units::ft2hF_per_Btu}, {12.0886496, Units::kJ_per_hC}};
    const InsulationPoint testPoint20000 = {
        {20000., Units::L}, {6., Units::ft2hF_per_Btu}, {149.628109, Units::kJ_per_hC}};

    const HPWH::Power_t elementPower(1.e4, Units::W);
    HPWH::UA_t UA;

    HPWH hpwh;

    // Check UA is as expected at 800 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint800, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(Units::kJ_per_hC), testPoint800.expectedUA(Units::kJ_per_hC));

    // Check UA independent of elements
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint800, elementPower / 1000.);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint800.expectedUA());

    // Check UA is as expected at 2 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint2, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint2.expectedUA());

    // Check UA is as expected at 50 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint50, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint50.expectedUA());

    // Check UA is as expected at 200 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint200, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint200.expectedUA());

    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint200B, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint200B.expectedUA());

    // Check UA is as expected at 2000 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint2000, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint2000.expectedUA());

    // Check UA is as expected at 20000 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint20000, elementPower);
    hpwh.getUA(UA);
    EXPECT_NEAR_REL(UA(), testPoint20000.expectedUA());
}

#undef TEST_INIT_RESISTANCE_TANK_GENERIC
#undef R_TO_RSI
