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

const std::vector<string> hasHighShuttOffVect = { "Sanden80", "QAHV_N136TAU_HPB_SP", \
	"ColmacCxA_20_SP", "ColmacCxV_5_SP", "NyleC60A_SP", "NyleC60A_C_SP", "NyleC185A_C_SP", "TamScalable_SP" };

const std::vector<string> noHighShuttOffVect = { "AOSmithHPTU80", "Rheem2020Build80", \
	"Stiebel220e", "AOSmithCAHP120", "AWHSTier3Generic80", "Generic1", "RheemPlugInDedicated50", \
	"RheemPlugInShared65", "restankRealistic", "StorageTank", "ColmacCxA_20_MP", "Scalable_MP", \
	"NyleC90A_MP", "NyleC90A_C_MP" };

int main(int, char*) {
	for (string hpwhStr : hasHighShuttOffVect) {
		testHasEnteringWaterShutOff(hpwhStr);
		testSetEnteringWaterShuffOffOutOfBoundsIndex(hpwhStr);
		testSetEnteringWaterShuffOffDeadbandToSmall(hpwhStr);
		testSetEnteringWaterHighTempShutOffAbsolute(hpwhStr);
		testSetEnteringWaterHighTempShutOffRelative(hpwhStr);
	}

	for (string hpwhStr : noHighShuttOffVect) {
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
	ASSERTTRUE(hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex()) == 1);

	// change entering water temp to below temp
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(highT_C - delta, doAbsolute, hpwh.getCompressorIndex()) == 0);

	// run a step and check we're not heating. 
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex()) == 0);

	// and reverse it
	ASSERTTRUE(hpwh.setEnteringWaterHighTempShutOff(highT_C + delta, doAbsolute, hpwh.getCompressorIndex()) == 0);
	hpwh.runOneStep(highT_C, drawVolume_L, externalT_C, externalT_C, HPWH::DR_ALLOW);
	ASSERTTRUE(hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex()) == 1);
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