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

bool runTest(
	const HPWH::TestDesc testDesc,
	HPWH::TestResults &testResults,
	double airT_C = 0.,
	bool doTempDepress = false)
{
	HPWH hpwh;

	getHPWHObject(hpwh, testDesc.modelName);

	std::string sOutputDirectory = "../build/test/output";

	// Parse the model
	if(testDesc.presetOrFile == "Preset") {  
		if (getHPWHObject(hpwh, testDesc.modelName) == HPWH::HPWH_ABORT) {
			cout << "Error, preset model did not initialize.\n";
			exit(1);
		}
	} else if (testDesc.presetOrFile == "File") {
		std::string inputFile = testDesc.modelName + ".txt";
		if (hpwh.HPWHinit_file(inputFile) != 0) exit(1);
	}
	else {
		cout << "Invalid argument, received '"<< testDesc.presetOrFile << "', expected 'Preset' or 'File'.\n";
		exit(1);
	}

	// Use the built-in temperature depression for the lockout test. Set the temp depression of 4C to better
	// try and trigger the lockout and hysteresis conditions
	hpwh.setMaxTempDepression(4.);
	hpwh.setDoTempDepression(doTempDepress);

	HPWH::ControlInfo controlInfo;
	if(!hpwh.readControlInfo(testDesc.testName,controlInfo)){
		cout << "Control file testInfo.txt has unsettable specifics in it. \n";
		exit(1);
	}

	std::vector<HPWH::Schedule> allSchedules;
	if (!(hpwh.readSchedules(testDesc.testName,controlInfo,allSchedules))) {
		exit(1);
	}

	return hpwh.runSimulation(testDesc,sOutputDirectory,controlInfo,allSchedules,airT_C,doTempDepress,testResults);

}

/* Test energy balance for storage tank with extra heat (solar)*/
void runTestSuite(const std::string &sModelName,const std::string &sPresetOrFile) {

	HPWH::TestDesc testDesc;
	testDesc.modelName = sModelName;
	testDesc.presetOrFile = sPresetOrFile;

	double totalEnergyConsumed_kJ = 0.;
	double totalVolumeRemoved_L = 0.;

	for (auto &sProfileName: sProfileNames) {
		HPWH::TestResults testResults;
		testDesc.testName = sProfileName;
		ASSERTTRUE(runTest(testDesc,testResults));

		totalEnergyConsumed_kJ += testResults.totalEnergyConsumed_kJ;
		totalVolumeRemoved_L += testResults.totalVolumeRemoved_L;
	}
	double totalMassRemoved_kg = HPWH::DENSITYWATER_kgperL * totalVolumeRemoved_L;
	double totalHeatCapacity_kJperC = HPWH::CPWATER_kJperkgC * totalMassRemoved_kg;
	double refEnergy_kJ = totalHeatCapacity_kJperC * (51.7 - 14.4);

	double UEF = refEnergy_kJ / totalEnergyConsumed_kJ;
	std::cout << "UEF: " << UEF << "\n";
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

	runTestSuite(sModelName,sPresetOrFile);

	return 0;
}
