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
        EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

        EXPECT_EQ(hpwh.setResistanceCapacity(100.), HPWH::HPWH_ABORT); // Need's to be scalable
    }

    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "restankRealistic";
        EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

        EXPECT_EQ(hpwh.setResistanceCapacity(-100.), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., 3), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., 30000), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., -3), HPWH::HPWH_ABORT);
        EXPECT_EQ(hpwh.setResistanceCapacity(100., 0), HPWH::HPWH_ABORT);
    }
}

/*
 * getSetResistanceErrors tests
 */
TEST(ResistanceFunctionsTest, getSetResistanceErrors)
{
    HPWH hpwh;
    double lowerElementPower_W = 1000;
    EXPECT_EQ(
        hpwh.initResistanceTank(
            100., 0.95, 0., lowerElementPower_W, HPWH::Units::Volume::L, HPWH::Units::Power::W),
        0)
        << "Could not initialize resistance tank.";

    double returnVal;

    returnVal = hpwh.getResistanceCapacity(0, HPWH::Units::Power::W); // lower
    EXPECT_NEAR_REL(returnVal, lowerElementPower_W);

    returnVal = hpwh.getResistanceCapacity(-1, HPWH::Units::Power::W); // both,
    EXPECT_NEAR_REL(returnVal, lowerElementPower_W);

    returnVal = hpwh.getResistanceCapacity(1, HPWH::Units::Power::W); // higher doesn't exist
    EXPECT_EQ(returnVal, HPWH::HPWH_ABORT);
}

/*
 * commercialTankInitErrors tests
 */
TEST(ResistanceFunctionsTest, commercialTankInitErrors)
{
    HPWH hpwh;

    // init model
    EXPECT_EQ(hpwh.initResistanceTankGeneric(-800.,
                                             10.,
                                             100.,
                                             100.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              HPWH::HPWH_ABORT); // negative volume

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             -100.,
                                             100.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              HPWH::HPWH_ABORT); // negative element

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             100.,
                                             -100.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              HPWH::HPWH_ABORT); // negative element

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             -10.,
                                             100.,
                                             100.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              HPWH::HPWH_ABORT); // negative r value

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             0.,
                                             100.,
                                             100.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              HPWH::HPWH_ABORT); // 0 r value

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             0.,
                                             0.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              HPWH::HPWH_ABORT); // Check needs one element
}

/*
 * getNumResistanceElements tests
 */
TEST(ResistanceFunctionsTest, getNumResistanceElements)
{
    HPWH hpwh;

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             0.,
                                             1000.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0);
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             1000.,
                                             0.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0);
    EXPECT_EQ(hpwh.getNumResistanceElements(), 1); // Check 1 elements

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             1000.,
                                             1000.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0);
    EXPECT_EQ(hpwh.getNumResistanceElements(), 2); // Check 2 elements
}

/*
 * getResistancePositionInRE_tank tests
 */
TEST(ResistanceFunctionsTest, getResistancePositionInRE_tank)
{
    HPWH hpwh;

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             0.,
                                             1000.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0);
    EXPECT_EQ(hpwh.getResistancePosition(0), 0);                // Check lower element is there
    EXPECT_EQ(hpwh.getResistancePosition(1), HPWH::HPWH_ABORT); // Check no element
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT); // Check no element

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             1000.,
                                             0.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0);
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);                // Check upper element there
    EXPECT_EQ(hpwh.getResistancePosition(1), HPWH::HPWH_ABORT); // Check no elements
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT); // Check no elements

    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             1000.,
                                             1000.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0);
    EXPECT_EQ(hpwh.getResistancePosition(0), 8);                // Check upper element there
    EXPECT_EQ(hpwh.getResistancePosition(1), 0);                // Check lower element is there
    EXPECT_EQ(hpwh.getResistancePosition(2), HPWH::HPWH_ABORT); // Check 0 elements}
}

/*
 * getResistancePositionInCompressorTank test
 */
TEST(ResistanceFunctionsTest, getResistancePositionInCompressorTank)
{

    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

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
    const double elementPower_kW = 10.; // kW

    // init model
    HPWH hpwh;
    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             0.,
                                             elementPower_kW * 1000.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0)
        << "Could not initialize generic resistance tank.";

    // Check only lowest setting works
    double factor = 3.;

    // set both, but really only one
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::Units::Power::kW),
              0); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check gets bottom with bottom
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::Units::Power::kW),
              HPWH::HPWH_ABORT); // only have one element

    // set lowest
    factor = 4.;
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::Units::Power::kW),
              0); // Set just bottom
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check gets just bottom with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check gets bottom with bottom
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::Units::Power::kW),
              HPWH::HPWH_ABORT); // only have one element

    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 1, HPWH::Units::Power::kW),
              HPWH::HPWH_ABORT); // set top returns error
}

/*
 * commercialTankErrorsWithTopElement tests
 */
TEST(ResistanceFunctionsTest, commercialTankErrorsWithTopElement)
{
    const double elementPower_kW = 10.; // KW

    // init model
    HPWH hpwh;
    EXPECT_EQ(hpwh.initResistanceTankGeneric(800.,
                                             10.,
                                             elementPower_kW * 1000.,
                                             0.,
                                             HPWH::Units::Volume::L,
                                             HPWH::Units::Area::m2,
                                             HPWH::Units::Temp::C,
                                             HPWH::Units::Power::W),
              0)
        << "Could not initialize resistance tank.";

    // Check only bottom setting works
    double factor = 3.;

    // set both, but only bottom really.
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::Units::Power::kW),
              0); // Check sets
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(-1, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check gets just bottom which is now top with both
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check the lower and only element
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::Units::Power::kW),
              HPWH::HPWH_ABORT); //  error on non existant element

    // set top
    factor = 4.;
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::Units::Power::kW),
              0); // only one element to set
    EXPECT_NEAR_REL(hpwh.getResistanceCapacity(0, HPWH::Units::Power::kW),
                    factor * elementPower_kW); // Check gets just bottom which is now top with both
    EXPECT_EQ(hpwh.getResistanceCapacity(1, HPWH::Units::Power::kW),
              HPWH::HPWH_ABORT); // error on non existant bottom

    // set bottom returns error
    EXPECT_EQ(hpwh.setResistanceCapacity(factor * elementPower_kW, 2, HPWH::Units::Power::kW),
              HPWH::HPWH_ABORT);
}

struct InsulationPoint
{
    double volume_L;
    double r_ft2hFperBTU; // ft^2.degF/(BTU/h)
    double expectedUA_kJperhC;
};

#define FT2HFperBTU_TO_M2CperW(r_ft2hFperBTU)                                                      \
    W_TO_KW(KWH_TO_BTU(dF_TO_dC(FT2_TO_M2(r_ft2hFperBTU))))

#define TEST_INIT_RESISTANCE_TANK_GENERIC(point, elementPower_W)                                   \
    EXPECT_EQ(hpwh.initResistanceTankGeneric(point.volume_L,                                       \
                                             FT2HFperBTU_TO_M2CperW(point.r_ft2hFperBTU),          \
                                             elementPower_W,                                       \
                                             elementPower_W,                                       \
                                             HPWH::Units::Volume::L,                               \
                                             HPWH::Units::Area::m2,                                \
                                             HPWH::Units::Temp::C,                                 \
                                             HPWH::Units::Power::W),                               \
              0)                                                                                   \
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
    double UA_kJperhC;

    HPWH hpwh;

    // Check UA is as expected at 800 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint800, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint800.expectedUA_kJperhC);

    // Check UA independent of elements
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint800, elementPower_W / 1000.);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint800.expectedUA_kJperhC);

    // Check UA is as expected at 2 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint2, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint2.expectedUA_kJperhC);

    // Check UA is as expected at 50 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint50, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint50.expectedUA_kJperhC);

    // Check UA is as expected at 200 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint200, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint200.expectedUA_kJperhC);

    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint200B, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint200B.expectedUA_kJperhC);

    // Check UA is as expected at 2000 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint2000, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint2000.expectedUA_kJperhC);

    // Check UA is as expected at 20000 gal
    TEST_INIT_RESISTANCE_TANK_GENERIC(testPoint20000, elementPower_W);
    hpwh.getUA(UA_kJperhC);
    EXPECT_NEAR_REL(UA_kJperhC, testPoint20000.expectedUA_kJperhC);
}

#undef TEST_INIT_RESISTANCE_TANK_GENERIC
#undef R_TO_RSI
