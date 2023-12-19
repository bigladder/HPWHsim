/*This is not a substitute for a proper HPWH Test Tool, it is merely a short program
 * to aid in the testing of the new HPWH.cc as it is being written.
 *
 * -NDK
 *
 * Bring on the HPWH Test Tool!!! -MJL
 *
 *
 *
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string> 
#include <algorithm>    // std::max

#define MAX_DIR_LENGTH 255

using std::cout;
using std::endl;
using std::string;
using std::ifstream;

int main(int argc, char *argv[]){
	HPWH hpwh;
	bool failed = false;

	const long maximumDurationNormalTest_min = 500000;

	// process command line arguments
	cout << "Testing HPWHsim version " << HPWH::getVersion() << endl;

	// Obvious wrong number of command line arguments
	if ((argc > 6)) {
		cout << "Invalid input. This program takes FOUR arguments: model specification type (ie. Preset or File), model specification (ie. Sanden80),  test name (ie. test50) and output directory\n";
		failed = true;
	}
	ASSERTFALSE(failed);

	// Help message
	std::string input1, input2, input3, input4;
	if(argc > 1) {
		input1 = argv[1];
		input2 = argv[2];
		input3 = argv[3];
		input4 = argv[4];
	} else {
		input1 = "asdf"; // Makes the next conditional not crash... a little clumsy but whatever
		input2 = "def";
		input3 = "ghi";
		input4 = ".";
	}
	if (argc < 5 || (argc > 6) || (input1 == "?") || (input1 == "help")) {
		cout << "Standard usage: \"hpwhTestTool.x [model spec type Preset/File] [model spec Name] [testName] [airtemp override F (optional)]\"\n";
		cout << "All input files should be located in the test directory, with these names:\n";
		cout << "drawschedule.csv DRschedule.csv ambientTschedule.csv evaporatorTschedule.csv inletTschedule.csv hpwhProperties.csv\n";
		cout << "An output file, `modelname'Output.csv, will be written in the test directory\n";
		failed = true;
	}
	ASSERTFALSE(failed);

	HPWH::TestDesc testDesc;
	testDesc.presetOrFile = input1;
	testDesc.modelName = input2;
	testDesc.testName = input3;
	std::string outputDirectory = input4;

	// Parse the model
	if(testDesc.presetOrFile == "Preset") {  
		if (getHPWHObject(hpwh, testDesc.modelName) == HPWH::HPWH_ABORT) {
			cout << "Error, preset model did not initialize.\n";
			failed = true;
		}
	} else if (testDesc.presetOrFile == "File") {
		std::string inputFile = testDesc.modelName + ".txt";
		if (hpwh.HPWHinit_file(inputFile) != 0) exit(1);
	}
	else {
		cout << "Invalid argument, received '"<< testDesc.presetOrFile << "', expected 'Preset' or 'File'.\n";
		failed = true;
	}
	ASSERTFALSE(failed);

	double airT_C = 0.;
	bool doTempDepress = false;
	if(argc == 6) { // air temperature specified on command line
		airT_C = F_TO_C(std::stoi(argv[5]));
		doTempDepress = true;
	}

	// Use the built-in temperature depression for the lockout test. Set the temp depression of 4C to better
	// try and trigger the lockout and hysteresis conditions
	hpwh.setMaxTempDepression(4.);
	hpwh.setDoTempDepression(doTempDepress);

	HPWH::ControlInfo controlInfo;		
	if(!hpwh.readControlInfo(testDesc.testName,controlInfo)){
		cout << "Control file testInfo.txt has unsettable specifics in it. \n";
		failed = true;
	}
	ASSERTFALSE(failed);

	std::vector<HPWH::Schedule> allSchedules;
	failed = !hpwh.readSchedules(testDesc.testName,controlInfo,allSchedules);
	ASSERTFALSE(failed);

	controlInfo.extendedTest = (controlInfo.timeToRun_min > maximumDurationNormalTest_min);

	HPWH::TestResults testResults;
	ASSERTTRUE(hpwh.runSimulation(testDesc,outputDirectory,controlInfo,allSchedules,airT_C,doTempDepress,testResults));

  return 0;
}
