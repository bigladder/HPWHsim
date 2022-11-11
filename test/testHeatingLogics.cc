#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 
#include <vector>

using std::cout;
using std::string;

void testHasEnteringWaterShutOff(string& input);
void testDoesNotHaveEnteringWaterShutOff(string& input);
void testSetEnteringWaterShuffOffOutOfBoundsIndex(string& input);
void testSetEnteringWaterShuffOffDeadbandToSmall(string& input);
void testCanNotSetEnteringWaterShutOff(string& input);
void testSetEnteringWaterHighTempShutOffAbsolute(string& input);
void testSetEnteringWaterHighTempShutOffRelative(string& input);

void testChangeToStateofChargeControlled(string& input);
void testSetStateOfCharge(string& input);

const std::vector<string> hasHighShuttOffVectSP = { "Sanden80", "QAHV_N136TAU_HPB_SP", \
	"ColmacCxA_20_SP", "ColmacCxV_5_SP", "NyleC60A_SP", "NyleC60A_C_SP", "NyleC185A_C_SP", "TamScalable_SP" };

const std::vector<string> noHighShuttOffVectMPExternal = {"Scalable_MP", "ColmacCxA_20_MP", "ColmacCxA_15_MP", \
	"ColmacCxV_5_MP", "NyleC90A_MP", "NyleC90A_C_MP" , "NyleC250A_MP", "NyleC250A_C_MP" };

const std::vector<string> noHighShuttOffVectIntegrated = { "AOSmithHPTU80", "Rheem2020Build80", \
	"Stiebel220e", "AOSmithCAHP120", "AWHSTier3Generic80", "Generic1", "RheemPlugInDedicated50", \
	"RheemPlugInShared65", "restankRealistic", "StorageTank"};

int main(int, char*) {
	for (string hpwhStr : hasHighShuttOffVectSP) {
		testHasEnteringWaterShutOff(hpwhStr);
		testSetEnteringWaterShuffOffOutOfBoundsIndex(hpwhStr);
		testSetEnteringWaterShuffOffDeadbandToSmall(hpwhStr);
		testSetEnteringWaterHighTempShutOffAbsolute(hpwhStr);
		testSetEnteringWaterHighTempShutOffRelative(hpwhStr);

		testChangeToStateofChargeControlled(hpwhStr);
		testSetStateOfCharge(hpwhStr);
	}

	for (string hpwhStr : noHighShuttOffVectMPExternal) {
		testDoesNotHaveEnteringWaterShutOff(hpwhStr);
		testCanNotSetEnteringWaterShutOff(hpwhStr);

		testChangeToStateofChargeControlled(hpwhStr);
		testSetStateOfCharge(hpwhStr);
	}

	for (string hpwhStr : noHighShuttOffVectIntegrated) {
		testDoesNotHaveEnteringWaterShutOff(hpwhStr);
		testCanNotSetEnteringWaterShutOff(hpwhStr);
	}

	return 0;
}

void testHasEnteringWaterShutOff(string& input) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);
	int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
	ASSERTTRUE(hpwh.hasEnteringWaterHighTempShutOff(index) == true);
}

void testDoesNotHaveEnteringWaterShutOff(string& input) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);
	int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
	ASSERTTRUE(hpwh.hasEnteringWaterHighTempShutOff(index) == false);
}

void testCanNotSetEnteringWaterShutOff(string& input) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);
	int index = hpwh.getCompressorIndex() == -1 ? 0 : hpwh.getCompressorIndex();
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(10., false, index) == HPWH::HPWH_ABORT);
}

void testSetEnteringWaterShuffOffOutOfBoundsIndex(string& input) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);
	int index = -1;
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(10., false, index) == HPWH::HPWH_ABORT);
	index = 15;
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(10., false, index) == HPWH::HPWH_ABORT);
}

void testSetEnteringWaterShuffOffDeadbandToSmall(string& input) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);

	double value = HPWH::MINSINGLEPASSLIFT - 1.;
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(value, false, hpwh.getCompressorIndex()) == HPWH::HPWH_ABORT);
	
	value = HPWH::MINSINGLEPASSLIFT;
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(value, false, hpwh.getCompressorIndex()) == 0);

	value = hpwh.getSetpoint() - (HPWH::MINSINGLEPASSLIFT-1);
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(value, true, hpwh.getCompressorIndex()) == HPWH::HPWH_ABORT);

	value = hpwh.getSetpoint() - HPWH::MINSINGLEPASSLIFT;
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(value, true, hpwh.getCompressorIndex()) == 0);
}

/* Tests we can set the entering water high temp shutt off with aboslute values and it works turns off and on as expected. */
void testSetEnteringWaterHighTempShutOffAbsolute(string& input) {
	const double drawVolume_L = 0.;
	const double externalT_C = 20.;
	const double delta = 2.;
	const double highT_C = 20.;
	const bool doAbsolute = true;
	HPWH hpwh;
	getHPWHObject(hpwh, input);

	// make tank cold to force on
	hpwh.setTankToTemperature(highT_C);

	// run a step and check we're heating
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(compressorIsRunning(hpwh));

	// change entering water temp to below temp
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(highT_C - delta, doAbsolute, hpwh.getCompressorIndex()) == 0);

	// run a step and check we're not heating. 
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTFALSE(compressorIsRunning(hpwh));

	// and reverse it
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(highT_C + delta, doAbsolute, hpwh.getCompressorIndex()) == 0);
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(compressorIsRunning(hpwh));
}

/* Tests we can set the entering water high temp shutt off with relative values and it works turns off and on as expected. */
void testSetEnteringWaterHighTempShutOffRelative(string& input) {
	const double drawVolume_L = 0.;
	const double externalT_C = 20.;
	const double delta = 2.;
	const double highT_C = 20.;
	const bool doAbsolute = false;
	HPWH hpwh;
	getHPWHObject(hpwh, input);

	const double relativeHighT_C = hpwh.getSetpoint() - highT_C;
	// make tank cold to force on
	hpwh.setTankToTemperature(highT_C);

	// run a step and check we're heating
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex()) == 1);

	// change entering water temp to below temp
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(relativeHighT_C + delta, doAbsolute, hpwh.getCompressorIndex()) == 0);

	// run a step and check we're not heating. 
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex()) == 0);

	// and reverse it
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(relativeHighT_C - delta, doAbsolute, hpwh.getCompressorIndex()) == 0);
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex()) == 1);
}

/* Test we can switch and the controls change as expected*/
void testChangeToStateofChargeControlled(string& input) {
	HPWH hpwh;
	const double externalT_C = 20.;
	const double setpointT_C = F_TO_C(149.);

	getHPWHObject(hpwh, input);
	const bool originalHasHighTempShutOff = hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex());
	if (!hpwh.isSetpointFixed()) {
		double temp;
		string tempStr;
		if (!hpwh.isNewSetpointPossible(setpointT_C, temp, tempStr)) {
			return; // Numbers don't aline for this type
		}
		hpwh.setSetpoint(setpointT_C);
	}

	//Just created so should be fasle
	ASSERTFALSE(hpwh.isSoCControlled());
	
	// change to SOC control;
	hpwh.switchToSoCControls(.76, .05, 99, true, 49, HPWH::UNITS_F);
	ASSERTTRUE(hpwh.isSoCControlled());

	// check entering water high temp shut off controll unchanged
	ASSERTTRUE(hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex()) == originalHasHighTempShutOff);
	
	// Test we can change the SoC and run a step and check we're heating
	if (hpwh.hasEnteringWaterHighTempShutOff(hpwh.getCompressorIndex())) {
		ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(setpointT_C-HPWH::MINSINGLEPASSLIFT, true, hpwh.getCompressorIndex()) == 0); // Force to ignore this part.
	}
	ASSERTTRUE(hpwh.setTankToTemperature(F_TO_C(100.)) == 0); // .51
	hpwh.runOneStep( 0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(compressorIsRunning(hpwh));

	// Test if we're on and in band stay on
	hpwh.setTankToTemperature(F_TO_C(125)); // .76 (current target)
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(compressorIsRunning(hpwh));

	// Test we can change the SoC and turn off
	hpwh.setTankToTemperature(F_TO_C(133)); // .84 
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTFALSE(compressorIsRunning(hpwh));

	// Test if off and in band stay off
	hpwh.setTankToTemperature(F_TO_C(125)); // .76  (current target)
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTFALSE(compressorIsRunning(hpwh));
}

/*Test we can change the target and turn off and on*/
void testSetStateOfCharge(string& input) {
	HPWH hpwh;
	const double externalT_C = 20.;
	const double setpointT_C = F_TO_C(149.);

	getHPWHObject(hpwh, input);
	if (!hpwh.isSetpointFixed()) {
		double temp;
		string tempStr;
		if (!hpwh.isNewSetpointPossible(setpointT_C, temp, tempStr)) {
			return; // Numbers don't aline for this type
		}
		hpwh.setSetpoint(setpointT_C);
	}

	// change to SOC control;
	hpwh.switchToSoCControls(.85, .05, 99, true, 49, HPWH::UNITS_F);
	ASSERTTRUE(hpwh.isSoCControlled());

	// Test if we're on and in band stay on
	hpwh.setTankToTemperature(F_TO_C(125)); // .76 
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(compressorIsRunning(hpwh));

	// Test we can change the target SoC and turn off
	hpwh.setTargetSoCFraction(0.5);
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTFALSE(compressorIsRunning(hpwh));

	// Change back in the band and stay turned off
	hpwh.setTargetSoCFraction(0.72);
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTFALSE(compressorIsRunning(hpwh));

	// Drop below the band and turn on
	hpwh.setTargetSoCFraction(0.70);
	hpwh.runOneStep(0, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTFALSE(compressorIsRunning(hpwh));
}