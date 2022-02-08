

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



void testHasACompressor(string input, bool expected) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);	// get preset model 
	ASSERTTRUE(hpwh.hasACompressor() == expected); 
}

void testGetCompCoilConfig(string input, int expected) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);	// get preset model 
	ASSERTTRUE(hpwh.getCompressorCoilConfig() == expected);
}

void testIsCompressorMultipass(string input, bool expected) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);	// get preset model 
	ASSERTTRUE(hpwh.isCompressorMultipass() == expected);
}

void testIsCompressoExternalMultipass(string input, bool expected) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);	// get preset model 
	ASSERTTRUE(hpwh.isCompressoExternalMultipass() == expected);
}

void testGetMaxCompressorSetpoint(string input, double expected) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);	// get preset model 
	ASSERTTRUE(hpwh.getMaxCompressorSetpoint() == expected);
}

void testGetMinOperatingTemp(string input, double expected) {
	HPWH hpwh;
	getHPWHObject(hpwh, input);	// get preset model 
	ASSERTTRUE(hpwh.getMinOperatingTemp(HPWH::UNITS_F) == expected);
}

int main(int, char*)
{

	const int length = 13;
	const string models [length] = {"AOSmithHPTU50", "Stiebel220e","AOSmithCAHP120", \
						"Sanden80", "ColmacCxV_5_SP", "ColmacCxA_20_SP", \
						"TamScalable_SP", "restankRealistic", "StorageTank", \
						"ColmacCxA_20_MP", "Scalable_MP", "NyleC90A_MP", "NyleC90A_C_MP"};
	const int hasComp[length] = { true, true, true, true, true, true, true, false, false, true, true, true, true };
	const int coilConfig[length] = { 1, 1, 1, 2, 2, 2, 2, HPWH::HPWH_ABORT, HPWH::HPWH_ABORT, 2, 2, 2, 2 };
	const int heatCycle[length] = { true, true, true, false, false, false, false, HPWH::HPWH_ABORT, HPWH::HPWH_ABORT, true, true, true, true };
	const int isExtMP[length] = { false, false, false, false, false, false, false, HPWH::HPWH_ABORT, HPWH::HPWH_ABORT, true, true, true, true };

	const double maxStpt[length] = { HPWH::MAXOUTLET_R134A, HPWH::MAXOUTLET_R134A,HPWH::MAXOUTLET_R134A,
									 HPWH::MAXOUTLET_R744, HPWH::MAXOUTLET_R410A, HPWH::MAXOUTLET_R134A,
									 HPWH::MAXOUTLET_R134A, HPWH::HPWH_ABORT, HPWH::HPWH_ABORT, 
									 HPWH::MAXOUTLET_R134A, HPWH::MAXOUTLET_R134A, 160., 160. };

	const double minTemp[length] = { 42., 32., 47., -25, -4., 40., 40., HPWH::HPWH_ABORT, HPWH::HPWH_ABORT, 40., 40., 40., 35. };

	for (int i = 0; i < length; i++) {
		testHasACompressor(models[i], hasComp[i]);
		testGetCompCoilConfig(models[i], coilConfig[i]);
		testIsCompressorMultipass(models[i], heatCycle[i]);
		testIsCompressoExternalMultipass(models[i], isExtMP[i]);

		testGetMaxCompressorSetpoint(models[i], maxStpt[i]);
		testGetMinOperatingTemp(models[i], minTemp[i]);
	}

	//Made it through the gauntlet
	return 0;
}
