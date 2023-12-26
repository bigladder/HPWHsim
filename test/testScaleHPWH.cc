

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

struct performance
{
    double input, output, cop;
};

void getCompressorPerformance(
    HPWH& hpwh, performance& point, double waterTempC, double airTempC, double setpointC)
{
    if (hpwh.isCompressorMultipass())
    { // Multipass capacity looks at the average of the temperature of the lift
        hpwh.setSetpoint((waterTempC + setpointC) / 2.);
    }
    else
    {
        hpwh.setSetpoint(waterTempC);
    }
    hpwh.resetTankToSetpoint(); // Force tank cold
    hpwh.setSetpoint(setpointC);

    // Run the step
    hpwh.runOneStep(waterTempC, // Inlet water temperature (C )
                    0.,         // Flow in gallons
                    airTempC,   // Ambient Temp (C)
                    airTempC,   // External Temp (C)
                    // HPWH::DR_TOO // DR Status (now an enum. Fixed for now as allow)
                    (HPWH::DR_TOO | HPWH::DR_LOR) // DR Status (now an enum. Fixed for now as allow)
    );

    // Check the heat in and out of the compressor
    point.input = hpwh.getNthHeatSourceEnergyInput(hpwh.getCompressorIndex());
    point.output = hpwh.getNthHeatSourceEnergyOutput(hpwh.getCompressorIndex());
    point.cop = point.output / point.input;
}

void scaleCapacityCOP(HPWH& hpwh,
                      double scaleInput,
                      double scaleCOP,
                      performance& point0,
                      performance& point1,
                      double waterTempC = F_TO_C(63),
                      double airTempC = F_TO_C(77),
                      double setpointC = F_TO_C(135))
{
    // Get peformance unscalled
    getCompressorPerformance(hpwh, point0, waterTempC, airTempC, setpointC);

    // Scale the compressor
    int val = hpwh.setScaleHPWHCapacityCOP(scaleInput, scaleCOP);
    ASSERTTRUE(val == 0);

    // Get the scaled performance
    getCompressorPerformance(hpwh, point1, waterTempC, airTempC, setpointC);
}

void testNoScaleOutOfBounds()
{ // Test that we can NOT scale the scalable model with scale <= 0
    HPWH hpwh;

    string input = "TamScalable_SP";

    double num;

    // get preset model
    getHPWHObject(hpwh, input);

    num = 0;
    ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(num, 1.) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., num) == HPWH::HPWH_ABORT);

    num = -1;
    ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(num, 1.) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., num) == HPWH::HPWH_ABORT);
}

void testNoneScalable()
{ // Test a model that is not scalable
    HPWH hpwh;

    string input = "AOSmithCAHP120";

    // get preset model
    getHPWHObject(hpwh, input);

    ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., 1.) == HPWH::HPWH_ABORT);
}

void testScalableHPWHScales()
{ // Test the scalable hpwh can scale
    HPWH hpwh;

    string input = "TamScalable_SP";

    performance point0, point1;
    double num, anotherNum;

    // get preset model
    getHPWHObject(hpwh, input);

    num = 0.001; // Very low
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 0.2; // normal but bad low
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 3.; // normal but pretty high
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 1000; // really high
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 10;
    anotherNum = 0.1; // weird high and low
    scaleCapacityCOP(hpwh, num, anotherNum, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / anotherNum));

    scaleCapacityCOP(hpwh, anotherNum, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / anotherNum));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 1.5;
    anotherNum = 0.9; // real high and low
    scaleCapacityCOP(hpwh, num, anotherNum, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / anotherNum));

    scaleCapacityCOP(hpwh, anotherNum, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / anotherNum));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));
}

void testScalableMPHPWHScales()
{ // Test the scalable MP hpwh can scale
    HPWH hpwh;

    string input = "Scalable_MP";

    performance point0, point1;
    double num, anotherNum;

    // get preset model
    getHPWHObject(hpwh, input);

    num = 0.001; // Very low
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 0.2; // normal but bad low
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 3.; // normal but pretty high
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 1000; // really high
    scaleCapacityCOP(hpwh, num, 1., point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop));

    scaleCapacityCOP(hpwh, 1., num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input));
    ASSERTTRUE(cmpd(point0.output, point1.output / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    scaleCapacityCOP(hpwh, num, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / num));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 10;
    anotherNum = 0.1; // weird high and low
    scaleCapacityCOP(hpwh, num, anotherNum, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / anotherNum));

    scaleCapacityCOP(hpwh, anotherNum, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / anotherNum));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));

    num = 1.5;
    anotherNum = 0.9; // real high and low
    scaleCapacityCOP(hpwh, num, anotherNum, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / num));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / anotherNum));

    scaleCapacityCOP(hpwh, anotherNum, num, point0, point1);
    ASSERTTRUE(cmpd(point0.input, point1.input / anotherNum));
    ASSERTTRUE(cmpd(point0.output, point1.output / num / anotherNum));
    ASSERTTRUE(cmpd(point0.cop, point1.cop / num));
}

void testSPGetCompressorCapacity()
{
    HPWH hpwh;

    string input = "ColmacCxA_20_SP";

    performance point0;

    double capacity_kWH, capacity_BTU;
    double waterTempC = F_TO_C(63);
    double airTempC = F_TO_C(77);
    double setpointC = F_TO_C(135);

    // get preset model
    getHPWHObject(hpwh, input);

    getCompressorPerformance(hpwh, point0, waterTempC, airTempC, setpointC); // gives kWH
    capacity_kWH = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC) /
                   60; // div 60 to kWh because I know above only runs 1 minute

    ASSERTTRUE(cmpd(point0.output, capacity_kWH));

    // Test with IP units
    capacity_BTU = hpwh.getCompressorCapacity(C_TO_F(airTempC),
                                              C_TO_F(waterTempC),
                                              C_TO_F(setpointC),
                                              HPWH::UNITS_BTUperHr,
                                              HPWH::UNITS_F) /
                   60; // div 60 to BTU because I know above only runs 1 minute
    ASSERTTRUE(relcmpd(KWH_TO_BTU(point0.output),
                       capacity_BTU)); // relative cmp since in btu's these will be large numbers
}

void testMPGetCompressorCapacity()
{
    HPWH hpwh;

    string input = "ColmacCxA_20_MP";

    performance point0;

    double capacity_kWH, capacity_BTU;
    double waterTempC = F_TO_C(50); // 50 and 126 to make average 88
    double airTempC = F_TO_C(61.7);
    double setpointC = F_TO_C(126);

    // get preset model
    getHPWHObject(hpwh, input);
    capacity_kWH = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC) /
                   60.; // div 60 to kWh because I know above only runs 1 minute
    getCompressorPerformance(hpwh, point0, waterTempC, airTempC, setpointC); // gives kWH
    ASSERTTRUE(cmpd(point0.output, capacity_kWH));

    // Test with IP units
    capacity_BTU = hpwh.getCompressorCapacity(C_TO_F(airTempC),
                                              C_TO_F(waterTempC),
                                              C_TO_F(setpointC),
                                              HPWH::UNITS_BTUperHr,
                                              HPWH::UNITS_F) /
                   60; // div 60 to BTU because I know above only runs 1 minute
    ASSERTTRUE(relcmpd(KWH_TO_BTU(point0.output),
                       capacity_BTU,
                       0.0001)); // relative cmp since in btu's these will be large numbers
}

void testSetMPCompressorOutputCapacity()
{
    HPWH hpwh;

    string input = "Scalable_MP";

    double newCapacity_kW, num;
    double waterTempC = F_TO_C(44);
    double airTempC = F_TO_C(98);
    double setpointC = F_TO_C(150);

    // get preset model
    getHPWHObject(hpwh, input);

    // Scale output to 1 kW
    num = 1.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTTRUE(cmpd(num, newCapacity_kW));

    // Scale output to .01 kW
    num = .01;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTTRUE(cmpd(num, newCapacity_kW));

    // Scale output to 1000 kW
    num = 1000.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTTRUE(cmpd(num, newCapacity_kW));

    // Check again with changed setpoint, for MP it should affect output capacity since it looks at
    // the mean temperature for the cycle.
    setpointC = F_TO_C(100);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTFALSE(cmpd(num, newCapacity_kW));
}

void testSetCompressorOutputCapacity()
{
    HPWH hpwh;

    string input = "TamScalable_SP";

    double newCapacity_kW, num;
    double waterTempC = F_TO_C(44);
    double airTempC = F_TO_C(98);
    double setpointC = F_TO_C(145);

    // get preset model
    getHPWHObject(hpwh, input);

    // Scale output to 1 kW
    num = 1.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTTRUE(cmpd(num, newCapacity_kW));

    // Scale output to .01 kW
    num = .01;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTTRUE(cmpd(num, newCapacity_kW));

    // Scale output to 1000 kW
    num = 1000.;
    hpwh.setCompressorOutputCapacity(num, airTempC, waterTempC, setpointC);
    newCapacity_kW = hpwh.getCompressorCapacity(airTempC, waterTempC, setpointC);
    ASSERTTRUE(cmpd(num, newCapacity_kW));

    // Scale output to 1000 kW but let's use do the calc in other units
    num = KW_TO_BTUperHR(num);
    hpwh.setCompressorOutputCapacity(num,
                                     C_TO_F(airTempC),
                                     C_TO_F(waterTempC),
                                     C_TO_F(setpointC),
                                     HPWH::UNITS_BTUperHr,
                                     HPWH::UNITS_F);
    double newCapacity_BTUperHr = hpwh.getCompressorCapacity(C_TO_F(airTempC),
                                                             C_TO_F(waterTempC),
                                                             C_TO_F(setpointC),
                                                             HPWH::UNITS_BTUperHr,
                                                             HPWH::UNITS_F);
    ASSERTTRUE(relcmpd(num, newCapacity_BTUperHr));
}

void testChipsCaseWithIPUnits()
{
    HPWH hpwh;

    string input = "TamScalable_SP";

    double waterTempF = 50;
    double airTempF = 50;
    double setpointF = 120;
    double wh_heatingCap = 20000.;

    // get preset model
    getHPWHObject(hpwh, input);

    // Scale output to 20000 btu/hr but let's use do the calc in other units
    hpwh.setCompressorOutputCapacity(
        wh_heatingCap, airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    double newCapacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    ASSERTTRUE(relcmpd(wh_heatingCap, newCapacity_BTUperHr));
}

void testScaleRestank()
{
    HPWH hpwh;

    string input = "restankRealistic";
    // get preset model
    getHPWHObject(hpwh, input);

    // Scale COP for restank fails.
    int val = hpwh.setScaleHPWHCapacityCOP(2., 2.);
    ASSERTTRUE(val == HPWH::HPWH_ABORT);
}

void testResistanceScales()
{
    HPWH hpwh;

    string input = "TamScalable_SP";
    double elementPower = 30.0; // KW

    // hpwh.HPWHinit_resTank();
    getHPWHObject(hpwh, input);

    double returnVal;
    returnVal = hpwh.getResistanceCapacity(0, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, elementPower));
    returnVal = hpwh.getResistanceCapacity(1, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, elementPower));
    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, 2. * elementPower));

    // check units convert
    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_BTUperHr);
    ASSERTTRUE(relcmpd(returnVal, 2. * elementPower * 3412.14));

    // Check setting bottom works
    double factor = 2.0;
    hpwh.setResistanceCapacity(factor * elementPower, 0);
    returnVal = hpwh.getResistanceCapacity(0, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, factor * elementPower));
    returnVal = hpwh.getResistanceCapacity(1, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, elementPower));
    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, factor * elementPower + elementPower));

    // Check setting both works
    factor = 3.;
    hpwh.setResistanceCapacity(factor * elementPower, -1);
    returnVal = hpwh.getResistanceCapacity(0, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, factor * elementPower));
    returnVal = hpwh.getResistanceCapacity(1, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, factor * elementPower));
    returnVal = hpwh.getResistanceCapacity(-1, HPWH::UNITS_KW);
    ASSERTTRUE(relcmpd(returnVal, 2. * factor * elementPower));
}

void testStorageTankErrors()
{
    HPWH hpwh;
    string input = "StorageTank";
    // get preset model
    getHPWHObject(hpwh, input);

    ASSERTTRUE(hpwh.setResistanceCapacity(1000.) == HPWH::HPWH_ABORT);
    ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., 1.) == HPWH::HPWH_ABORT);
}

int main(int, char*)
{

    testSetMPCompressorOutputCapacity();

    testScalableHPWHScales(); // Test the scalable model scales properly

    testNoScaleOutOfBounds(); // Test that models can't scale with invalid inputs

    testNoneScalable(); // Test that models can't scale that are non scalable presets

    testScaleRestank(); // Test the resistance tank can't scale the compressor

    testResistanceScales(); // Test the resistance tank scales the resistance elements

    testSPGetCompressorCapacity(); // Test we can get the capacity

    testSetCompressorOutputCapacity(); // Test we can set the capacity with a specific number

    testChipsCaseWithIPUnits(); // Debuging Chip's case

    testStorageTankErrors(); // Make sure we can't scale the storage tank.

    testScalableMPHPWHScales(); // Test for proper scaling in the MP scalable model

    testMPGetCompressorCapacity(); // Test MP capacity in and out correct.

    // testSetMPCompressorOutputCapacity(); // Tets MP capacity can be set correctly.

    // Made it through the gauntlet
    return 0;
}
