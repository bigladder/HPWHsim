

/*unit test for isTankSizeFixed() functionality
 *
 *
 *
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 
//#include <assert.h>     /* assert */



using std::cout;
using std::string;

struct performance {
	double input, output, cop;
};


void getCompressorCapacity(HPWH &hpwh, performance &point, double waterTempC, double airTempC, double setpointC) {
	//Force tank cold
	hpwh.setSetpoint(waterTempC);
	hpwh.resetTankToSetpoint();
	hpwh.setSetpoint(setpointC);

	// Run the step
	hpwh.runOneStep(waterTempC, // Inlet water temperature (C)
		0., // Flow in gallons
		airTempC,  // Ambient Temp (C)
		airTempC,  // External Temp (C)
		HPWH::DR_TOO // DR Status (now an enum. Fixed for now as allow)
	);
	

	// Check the heat in and out of the compressor
	point.input = hpwh.getNthHeatSourceEnergyInput(hpwh.getCompressorIndex());
	point.output = hpwh.getNthHeatSourceEnergyOutput(hpwh.getCompressorIndex());
	point.cop = point.output / point.input ;
}

void scaleCapacityCOP(HPWH &hpwh, double scaleInput, double scaleCOP, performance &point0, performance &point1,
	double waterTempC = F_TO_C(63), double airTempC = F_TO_C(77), double setpointC = F_TO_C(135)) {

	getCompressorCapacity(hpwh, point0, waterTempC, airTempC, setpointC);

	int val = hpwh.setScaleHPWHCapacityCOP(scaleInput, scaleCOP);

	getCompressorCapacity(hpwh, point1, waterTempC, airTempC, setpointC);
}

void testNoScaleOutOfBounds() { // Test that we can scale the scalable model <= 0
	HPWH::MODELS presetModel;
	HPWH hpwh;

	string input = "Scalable_SP";

	double num;

	// get model number
	presetModel = mapStringToPreset(input);
	hpwh.HPWHinit_presets(presetModel); // set preset

	num = 0;
	ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(num, 1.) == HPWH::HPWH_ABORT);
	ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., num) == HPWH::HPWH_ABORT);

	num = -1;
	ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(num, 1.) == HPWH::HPWH_ABORT);
	ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., num) == HPWH::HPWH_ABORT);
}

void testNoneScalable() { // Test a model that is not scalable
	HPWH::MODELS presetModel;
	HPWH hpwh;

	string input = "AOSmithCAHP120";

	// get model number
	presetModel = mapStringToPreset(input);
	hpwh.HPWHinit_presets(presetModel); // set preset

	ASSERTTRUE(hpwh.setScaleHPWHCapacityCOP(1., 1.) == HPWH::HPWH_ABORT);
}

void testScalableHPWHScales() { // Test the scalable hpwh can be put through it's passes

	HPWH::MODELS presetModel;
	HPWH hpwh;

	string input = "Scalable_SP";

	performance point0, point1;
	double num, anotherNum;

	// get model number
	presetModel = mapStringToPreset(input);
	hpwh.HPWHinit_presets(presetModel); // set preset

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
}

int main(int argc, char *argv[])
{
	testScalableHPWHScales();
	testNoScaleOutOfBounds();
	testNoneScalable(); 

	//Made it through the gauntlet
	return 0;
}
