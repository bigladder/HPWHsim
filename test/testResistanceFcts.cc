

/*unit test for isTankSizeFixed() functionality
 *
 *
 *
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string>

using std::cout;
using std::string;

void testSetResistanceCapacityErrorChecks()
{
    HPWH hpwhHP1, hpwhR;
    string input = "ColmacCxA_30_SP";

    // get preset model
    getHPWHObject(hpwhHP1, input);

    ASSERTTRUE(hpwhHP1.setResistanceCapacity(100.) == HPWH::HPWH_ABORT); // Need's to be scalable

    input = "restankRealistic";
    // get preset model
    getHPWHObject(hpwhR, input);
    ASSERTTRUE(hpwhR.setResistanceCapacity(-100.) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwhR.setResistanceCapacity(100., 3) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwhR.setResistanceCapacity(100., 30000) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwhR.setResistanceCapacity(100., -3) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwhR.setResistanceCapacity(100., 0, HPWH::UNITS_F) == HPWH::HPWH_ABORT);
}

void testGetSetResistanceErrors()
{
    HPWH hpwh;
    double lowerElementPower_W = 1000;
    double lowerElementPower = lowerElementPower_W / 1000;
    hpwh.HPWHinit_resTank(100., 0.95, 0., lowerElementPower_W);

    double returnVal;
    returnVal = hpwh.getResistanceCapacity(0, HPWH::UNITS_KW); // lower
    ASSERTTRUE(relcmpd(returnVal, lowerElementPower));
    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW); // both,
    ASSERTTRUE(relcmpd(returnVal, lowerElementPower));
    returnVal = hpwh.getResistanceCapacity(1, HPWH::UNITS_KW); // higher doesn't exist
    ASSERTTRUE(returnVal == HPWH::HPWH_ABORT);
}

void testCommercialTankInitErrors()
{
    HPWH hpwh;
    // init model
    ASSERTTRUE(hpwh.HPWHinit_resTankGeneric(-800., 10., 100., 100.) ==
               HPWH::HPWH_ABORT); // negative volume
    ASSERTTRUE(hpwh.HPWHinit_resTankGeneric(800., 10., -100., 100.) ==
               HPWH::HPWH_ABORT); // negative element
    ASSERTTRUE(hpwh.HPWHinit_resTankGeneric(800., 10., 100., -100.) ==
               HPWH::HPWH_ABORT); // negative element
    ASSERTTRUE(hpwh.HPWHinit_resTankGeneric(800., -10., 100., 100.) ==
               HPWH::HPWH_ABORT); // negative r value
    ASSERTTRUE(hpwh.HPWHinit_resTankGeneric(800., 0., 100., 100.) == HPWH::HPWH_ABORT); // 0 r value
    ASSERTTRUE(hpwh.HPWHinit_resTankGeneric(800., 10., 0., 0.) ==
               HPWH::HPWH_ABORT); // Check needs one element
}

void testGetNumResistanceElements()
{
    HPWH hpwh;

    hpwh.HPWHinit_resTankGeneric(800., 10., 0., 1000.);
    ASSERTTRUE(hpwh.getNumResistanceElements() == 1); // Check 1 elements
    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 0.);
    ASSERTTRUE(hpwh.getNumResistanceElements() == 1); // Check 1 elements
    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 1000.);
    ASSERTTRUE(hpwh.getNumResistanceElements() == 2); // Check 2 elements
}

void testGetResistancePositionInRETank()
{
    HPWH hpwh;

    hpwh.HPWHinit_resTankGeneric(800., 10., 0., 1000.);
    ASSERTTRUE(hpwh.getResistancePosition(0) == 0);                // Check lower element is there
    ASSERTTRUE(hpwh.getResistancePosition(1) == HPWH::HPWH_ABORT); // Check no element
    ASSERTTRUE(hpwh.getResistancePosition(2) == HPWH::HPWH_ABORT); // Check no element

    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 0.);
    ASSERTTRUE(hpwh.getResistancePosition(0) == 8);                // Check upper element there
    ASSERTTRUE(hpwh.getResistancePosition(1) == HPWH::HPWH_ABORT); // Check no elements
    ASSERTTRUE(hpwh.getResistancePosition(2) == HPWH::HPWH_ABORT); // Check no elements

    hpwh.HPWHinit_resTankGeneric(800., 10., 1000., 1000.);
    ASSERTTRUE(hpwh.getResistancePosition(0) == 8);                // Check upper element there
    ASSERTTRUE(hpwh.getResistancePosition(1) == 0);                // Check lower element is there
    ASSERTTRUE(hpwh.getResistancePosition(2) == HPWH::HPWH_ABORT); // Check 0 elements}
}

void testGetResistancePositionInCompressorTank()
{
    HPWH hpwh;

    string input = "TamScalable_SP";
    // get preset model
    getHPWHObject(hpwh, input);

    ASSERTTRUE(hpwh.getResistancePosition(0) == 9);                   // Check top elements
    ASSERTTRUE(hpwh.getResistancePosition(1) == 0);                   // Check bottom elements
    ASSERTTRUE(hpwh.getResistancePosition(2) == HPWH::HPWH_ABORT);    // Check compressor isn't RE
    ASSERTTRUE(hpwh.getResistancePosition(-1) == HPWH::HPWH_ABORT);   // Check out of bounds
    ASSERTTRUE(hpwh.getResistancePosition(1000) == HPWH::HPWH_ABORT); // Check out of bounds
}

void testCommercialTankErrorsWithBottomElement()
{
    HPWH hpwh;
    double elementPower_kW = 10.; // KW
    // init model
    hpwh.HPWHinit_resTankGeneric(800., 10., 0., elementPower_kW * 1000.);

    // Check only lowest setting works
    double factor = 3.;
    // set both, but really only one
    ASSERTTRUE(hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::UNITS_KW) ==
               0); // Check sets
    ASSERTTRUE(relcmpd(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                       factor * elementPower_kW)); // Check gets just bottom with both
    ASSERTTRUE(relcmpd(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                       factor * elementPower_kW)); // Check gets bottom with bottom
    ASSERTTRUE(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW) ==
               HPWH::HPWH_ABORT); // only have one element

    // set lowest
    factor = 4.;
    ASSERTTRUE(hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::UNITS_KW) ==
               0); // Set just bottom
    ASSERTTRUE(relcmpd(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                       factor * elementPower_kW)); // Check gets just bottom with both
    ASSERTTRUE(relcmpd(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                       factor * elementPower_kW)); // Check gets bottom with bottom
    ASSERTTRUE(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW) ==
               HPWH::HPWH_ABORT); // only have one element

    ASSERTTRUE(hpwh.setResistanceCapacity(factor * elementPower_kW, 1, HPWH::UNITS_KW) ==
               HPWH::HPWH_ABORT); // set top returns error
}
void testCommercialTankErrorsWithTopElement()
{
    HPWH hpwh;
    double elementPower_kW = 10.; // KW
    // init model
    hpwh.HPWHinit_resTankGeneric(800., 10., elementPower_kW * 1000., 0.);

    // Check only bottom setting works
    double factor = 3.;
    // set both, but only bottom really.
    ASSERTTRUE(hpwh.setResistanceCapacity(factor * elementPower_kW, -1, HPWH::UNITS_KW) ==
               0); // Check sets
    ASSERTTRUE(
        relcmpd(hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW),
                factor * elementPower_kW)); // Check gets just bottom which is now top with both
    ASSERTTRUE(relcmpd(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                       factor * elementPower_kW)); // Check the lower and only element
    ASSERTTRUE(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW) ==
               HPWH::HPWH_ABORT); //  error on non existant element

    // set top
    factor = 4.;
    ASSERTTRUE(hpwh.setResistanceCapacity(factor * elementPower_kW, 0, HPWH::UNITS_KW) ==
               0); // only one element to set
    ASSERTTRUE(
        relcmpd(hpwh.getResistanceCapacity(0, HPWH::UNITS_KW),
                factor * elementPower_kW)); // Check gets just bottom which is now top with both
    ASSERTTRUE(hpwh.getResistanceCapacity(1, HPWH::UNITS_KW) ==
               HPWH::HPWH_ABORT); // error on non existant bottom

    // set bottom returns error
    ASSERTTRUE(hpwh.setResistanceCapacity(factor * elementPower_kW, 2, HPWH::UNITS_KW) ==
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
void testCommercialTankInit()
{
    HPWH hpwh;
    double elementPower_kW = 10.; // KW
    double UA;
    const InsulationPoint testPoint800 = {800., 10., 10.500366};
    const InsulationPoint testPoint2 = {2., 6., 0.322364};
    const InsulationPoint testPoint50 = {50., 12., 1.37808};
    const InsulationPoint testPoint200 = {200., 16., 2.604420};
    const InsulationPoint testPoint200B = {200., 6., 6.94512163};
    const InsulationPoint testPoint2000 = {2000., 16., 12.0886496};
    const InsulationPoint testPoint20000 = {20000., 6., 149.628109};

    // Check UA is as expected at 800 gal
    INITGEN(testPoint800);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint800.expectedUA_SI, 0.0001));
    // Check UA independent of elements
    hpwh.HPWHinit_resTankGeneric(
        testPoint800.volume_L, R_TO_RSI(testPoint800.rValue_IP), elementPower_kW, elementPower_kW);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint800.expectedUA_SI, 0.0001));

    // Check UA is as expected at 2 gal
    INITGEN(testPoint2);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint2.expectedUA_SI, 0.0001));

    // Check UA is as expected at 50 gal
    INITGEN(testPoint50);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint50.expectedUA_SI, 0.0001));

    // Check UA is as expected at 200 gal
    INITGEN(testPoint200);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint200.expectedUA_SI, 0.0001));

    INITGEN(testPoint200B);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint200B.expectedUA_SI, 0.0001));

    // Check UA is as expected at 2000 gal
    INITGEN(testPoint2000);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint2000.expectedUA_SI, 0.0001));

    // Check UA is as expected at 20000 gal
    INITGEN(testPoint20000);
    hpwh.getUA(UA);
    ASSERTTRUE(relcmpd(UA, testPoint20000.expectedUA_SI, 0.0001));
}
#undef INITGEN
#undef R_TO_RSI

int main(int, char**)
{
    testSetResistanceCapacityErrorChecks(); // Check the resistance reset throws errors when
                                            // expected.

    testGetSetResistanceErrors(); // Check can make ER residential tank with one lower element, and
                                  // can't set/get upper

    testCommercialTankInitErrors();      // test it inits as expected
    testGetNumResistanceElements();      // unit test on getNumResistanceElements()
    testGetResistancePositionInRETank(); // unit test on getResistancePosition() for an RE tank
    testGetResistancePositionInCompressorTank(); // unit test on getResistancePosition() for a
                                                 // compressor

    testCommercialTankErrorsWithBottomElement(); //
    testCommercialTankErrorsWithTopElement();
    testCommercialTankInit(); //

    // Made it through the gauntlet
    return 0;
}
