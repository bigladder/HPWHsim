/*
 * unit test for energy balancing
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 

const std::vector<std::string> sProfileNames({	"24hr67_vsmall",
												"24hr67_low",
												"24hr67_medium",
                                                "24hr67_high"	});

static bool runTest(
	const HPWH::TestDesc testDesc,
	HPWH::TestResults &testResults,
	double airT_C = 0.,
	bool doTempDepress = false)
{
	HPWH hpwh;

#if defined _DEBUG
	hpwh.setVerbosity(HPWH::VRB_reluctant);
#endif

	getHPWHObject(hpwh, testDesc.modelName);

	std::string sOutputDirectory = "../build/test/output";

	// create the model
	bool result = true;
	if(testDesc.presetOrFile == "Preset") {  
		result = (getHPWHObject(hpwh, testDesc.modelName) != HPWH::HPWH_ABORT);
	} else if (testDesc.presetOrFile == "File") {
		std::string inputFile = testDesc.modelName + ".txt";
		result = (hpwh.HPWHinit_file(inputFile) != HPWH::HPWH_ABORT);
	}
	ASSERTTRUE(result);

	// Use the built-in temperature depression for the lockout test. Set the temp depression of 4C to better
	// try and trigger the lockout and hysteresis conditions
	hpwh.setMaxTempDepression(4.);
	hpwh.setDoTempDepression(doTempDepress);

	HPWH::ControlInfo controlInfo;
	result = hpwh.readControlInfo(testDesc.testName,controlInfo);
	ASSERTTRUE(result);

	std::vector<HPWH::Schedule> allSchedules;
	result = hpwh.readSchedules(testDesc.testName,controlInfo,allSchedules);
	ASSERTTRUE(result);

	controlInfo.recordMinuteData = false;
	controlInfo.recordYearData = false;
	result = hpwh.runSimulation(testDesc,sOutputDirectory,controlInfo,allSchedules,airT_C,doTempDepress,testResults);
	ASSERTTRUE(result);

	return result;
}

/* Evaluate UEF based on simulations using standard profiles */
static bool runUEFTestSuite(const std::string &sModelName,const std::string &sPresetOrFile, double &UEF) {

	HPWH::TestDesc testDesc;
	testDesc.modelName = sModelName;
	testDesc.presetOrFile = sPresetOrFile;

	double totalEnergyConsumed_kJ = 0.;
	double totalVolumeRemoved_L = 0.;

	bool result = true;
	for (auto &sProfileName: sProfileNames) {
		HPWH::TestResults testResults;
		testDesc.testName = sProfileName;
		result &= runTest(testDesc,testResults);

		totalEnergyConsumed_kJ += testResults.totalEnergyConsumed_kJ;
		totalVolumeRemoved_L += testResults.totalVolumeRemoved_L;
	}
	double totalMassRemoved_kg = HPWH::DENSITYWATER_kgperL * totalVolumeRemoved_L;
	double totalHeatCapacity_kJperC = HPWH::CPWATER_kJperkgC * totalMassRemoved_kg;
	double refEnergy_kJ = totalHeatCapacity_kJperC * (51.7 - 14.4);

	UEF = refEnergy_kJ / totalEnergyConsumed_kJ;
	return result;
}

int main(int argc, char *argv[])
{
	// process command line arguments
	if ((argc != 3)) {
		cout << "Invalid input. This program takes TWO arguments: model type (i.e., Preset or File), model name (i.e., Sanden80)\n";
		exit(1);
	}

	std::string sPresetOrFile = argv[1];
	std::string sModelName = argv[2];

	double UEF = 0.;
	if (runUEFTestSuite(sModelName, sPresetOrFile, UEF)) {
		std::cout << "UEF: " << UEF << "\n";
	}

	return 0;
}
