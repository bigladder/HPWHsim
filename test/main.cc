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

	HPWH::DRMODES drStatus = HPWH::DR_ALLOW;
	HPWH::MODELS model;

	const int nTestTCouples = 6;

	string var1, inputFile;
	string inputVariableName, firstCol;

	double cumHeatIn[3] = { 0,0,0 };
	double cumHeatOut[3] = { 0,0,0 };

	FILE * outputFile = NULL;
	FILE * yearOutFile = NULL;
	ifstream controlFile;

	string strPreamble;
	string strHead = "minutes,Ta,Tsetpoint,inletT,draw,";
	string strHeadMP = "condenserInletT,condenserOutletT,externalVolGPM,";
	string strHeadSoC = "targetSoCFract,soCFract,";

	// process command line arguments
	cout << "Testing HPWHsim version " << HPWH::getVersion() << endl;

	// Obvious wrong number of command line arguments
	if ((argc > 6)) {
		cout << "Invalid input. This program takes FOUR arguments: model specification type (ie. Preset or File), model specification (ie. Sanden80),  test name (ie. test50) and output directory\n";
		exit(1);
	}
	// Help message
	std::string input1, input2, input3;
	std::string outputDirectory;
	if(argc > 1) {
		input1 = argv[1];
		input2 = argv[2];
		input3 = argv[3];
		outputDirectory = argv[4];
	} else {
		input1 = "asdf"; // Makes the next conditional not crash... a little clumsy but whatever
		input2 = "def";
		input3 = "ghi";
		outputDirectory = ".";
	}
	if (argc < 5 || (argc > 6) || (input1 == "?") || (input1 == "help")) {
		cout << "Standard usage: \"hpwhTestTool.x [model spec type Preset/File] [model spec Name] [testName] [airtemp override F (optional)]\"\n";
		cout << "All input files should be located in the test directory, with these names:\n";
		cout << "drawschedule.csv DRschedule.csv ambientTschedule.csv evaporatorTschedule.csv inletTschedule.csv hpwhProperties.csv\n";
		cout << "An output file, `modelname'Output.csv, will be written in the test directory\n";
		exit(1);
	}

	double airTemp;
	bool HPWH_doTempDepress;
	if(argc == 6) {
		airTemp = std::stoi(argv[5]);
		HPWH_doTempDepress = true;
	} else {
		airTemp = 0;
		HPWH_doTempDepress = false;
	}

	// Only input file specified -- don't suffix with .csv
	std::string testDirectory = input3;

	// Parse the model
	HPWH::ControlInfo controlInfo;
	controlInfo.newSetpoint = 0.;
	if(input1 == "Preset") {
		inputFile = "";   
		if (getHPWHObject(hpwh, input2) == HPWH::HPWH_ABORT) {
			cout << "Error, preset model did not initialize.\n";
			exit(1);
		}

		model = static_cast<HPWH::MODELS> (hpwh.getHPWHModel());
		if(model == HPWH::MODELS_Sanden80 || model == HPWH::MODELS_Sanden40) {
			controlInfo.newSetpoint = F_TO_C(149.);
		}
	} else if (input1 == "File") {
		inputFile = input2 + ".txt";
		if (hpwh.HPWHinit_file(inputFile) != 0) exit(1);
	}
	else {
		cout << "Invalid argument, received '"<< input1 << "', expected 'Preset' or 'File'.\n";
		exit(1);
	}

	// Use the built-in temperature depression for the lockout test. Set the temp depression of 4C to better
	// try and trigger the lockout and hysteresis conditions
	double tempDepressThresh = 4.;
	hpwh.setMaxTempDepression(tempDepressThresh);
	hpwh.setDoTempDepression(HPWH_doTempDepress);

	if(!hpwh.readControlInfo(testDirectory,controlInfo)){
		cout << "Control file testInfo.txt has unsettable specifics in it. \n";
		exit(1);
	}

	std::vector<HPWH::Schedule> allSchedules;
	if (!(hpwh.readSchedules(testDirectory,controlInfo,allSchedules))) {
		exit(1);
	}

   // ----------------------Open the Output Files and Print the Header---------------------------- //
	if (controlInfo.minutesToRun > 500000.) {
		std::string fileToOpen = outputDirectory + "/DHW_YRLY.csv";

		if (fopen_s(&yearOutFile, fileToOpen.c_str(), "a+") != 0) {
			cout << "Could not open output file " << fileToOpen << "\n";
			exit(1);
		}
	}
	else {
		std::string fileToOpen = outputDirectory + "/" + input3 + "_" + input1 + "_" + input2 + ".csv";

		if (fopen_s(&outputFile, fileToOpen.c_str(), "w+") != 0) {
			cout << "Could not open output file " << fileToOpen << "\n";
			exit(1);
		}
	  
		string header = strHead;
		if (hpwh.isCompressoExternalMultipass()) {
			header += strHeadMP;
		}
		if(controlInfo.useSoC){ 
			header += strHeadSoC;
		}
		int csvOptions = HPWH::CSVOPT_NONE;
		hpwh.WriteCSVHeading(outputFile, header.c_str(), nTestTCouples, csvOptions);
	}

	// ------------------------------------- Simulate --------------------------------------- //
	cout << "Now Simulating " << controlInfo.minutesToRun << " Minutes of the Test\n";

	// Loop over the minutes in the test
	for (int i = 0; i < controlInfo.minutesToRun; i++) {

		double airTemp2;
		if (HPWH_doTempDepress) {
			airTemp2 = F_TO_C(airTemp);
		}
		else {
			airTemp2 = allSchedules[2][i];
		}

		// Process the dr status
		drStatus = static_cast<HPWH::DRMODES>(int(allSchedules[4][i]));

		// Change setpoint if there is a setpoint schedule.
		bool doSetSetpoint = (!allSchedules[5].empty()) && (!hpwh.isSetpointFixed());
		if (doSetSetpoint) {
			hpwh.setSetpoint(allSchedules[5][i]); //expect this to fail sometimes
		}

		// Change SoC schedule	
		if (controlInfo.useSoC) {
			if (hpwh.setTargetSoCFraction(allSchedules[6][i]) != 0) {
				cout << "ERROR: Can not set the target state of charge fraction. \n";
				exit(1);
			}
		}
		
		// Mix down for yearly tests with large compressors
		if (hpwh.getHPWHModel() >= 210 && controlInfo.minutesToRun > 500000.) {
			//Do a simple mix down of the draw for the cold water temperature
			if (hpwh.getSetpoint() <= 125.) {
				allSchedules[1][i] *= (125. - allSchedules[0][i]) / (hpwh.getTankNodeTemp(hpwh.getNumNodes() - 1, HPWH::UNITS_F) - allSchedules[0][i]);
			}
		}

		double tankHCStart = hpwh.getTankHeatContent_kJ();

		// Run the step
		hpwh.runOneStep(
			allSchedules[0][i],					// inlet water temperature (C)
			GAL_TO_L(allSchedules[1][i]),		// draw (gallons)
			airTemp2,							// ambient Temp (C)
			allSchedules[3][i],					// external Temp (C)
			drStatus,							// DDR Status (now an enum. Fixed for now as allow)
			GAL_TO_L(allSchedules[1][i]),		// inlet volume 2 (gallons)
			allSchedules[0][i],					// inlet Temp 2 (C)
			NULL);								// no extra heat

		const double EBALTHRESHOLD = 0.005;
		if (!hpwh.isEnergyBalanced(GAL_TO_L(allSchedules[1][i]),allSchedules[0][i],tankHCStart,EBALTHRESHOLD)) {
			cout << "WARNING: On minute " << i << " HPWH has an energy balance error.\n";
		}

		// Check timing
		for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++) {
			if (hpwh.getNthHeatSourceRunTime(iHS) > 1) {
				cout << "ERROR: On minute " << i << " heat source " << iHS << " ran for " << hpwh.getNthHeatSourceRunTime(iHS) << "minutes" << "\n";
				exit(1); 
			}
		}

		// Check flow for external MP
		if (hpwh.isCompressoExternalMultipass()) {
			double volumeHeated_Gal = hpwh.getExternalVolumeHeated(HPWH::UNITS_GAL);
			double mpFlowVolume_Gal = hpwh.getExternalMPFlowRate(HPWH::UNITS_GPM)*hpwh.getNthHeatSourceRunTime(hpwh.getCompressorIndex());
			if (fabs(volumeHeated_Gal - mpFlowVolume_Gal) > 0.000001) {
				cout << "ERROR: Externally heated volumes are inconsistent! Volume Heated [Gal]: " << volumeHeated_Gal << ", mpFlowRate in 1 minute [Gal]: "
					<< mpFlowVolume_Gal << "\n";
				exit(1);
			}
		}

		// Recording
		if (controlInfo.minutesToRun < 500000.) {
			// Copy current status into the output file
			if (HPWH_doTempDepress) {
				airTemp2 = hpwh.getLocationTemp_C();
			}
			strPreamble = std::to_string(i) + ", " + std::to_string(airTemp2) + ", " + std::to_string(hpwh.getSetpoint()) + ", " +
				std::to_string(allSchedules[0][i]) + ", " + std::to_string(allSchedules[1][i]) + ", ";
			// Add some more outputs for mp tests
			if (hpwh.isCompressoExternalMultipass()) {
				strPreamble += std::to_string(hpwh.getCondenserWaterInletTemp()) + ", " + std::to_string(hpwh.getCondenserWaterOutletTemp()) + ", " +
					std::to_string(hpwh.getExternalVolumeHeated(HPWH::UNITS_GAL)) + ", ";
			}
			if (controlInfo.useSoC) {
				strPreamble += std::to_string(allSchedules[6][i]) + ", " + std::to_string(hpwh.getSoCFraction()) + ", ";
			}
			int csvOptions = HPWH::CSVOPT_NONE;
			if (allSchedules[1][i] > 0.) {
			csvOptions |= HPWH::CSVOPT_IS_DRAWING;
			}
			hpwh.WriteCSVRow(outputFile, strPreamble.c_str(), nTestTCouples, csvOptions);
		}
		else {
			for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++) {
	  			cumHeatIn[iHS] += hpwh.getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH)*1000.;
	  			cumHeatOut[iHS] += hpwh.getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH)*1000.;
			}
		}
	}

	if (controlInfo.minutesToRun > 500000.) {
		firstCol = input3 + "," + input1 + "," + input2;
		fprintf(yearOutFile, "%s", firstCol.c_str());
		double totalIn = 0, totalOut = 0;
		for (int iHS = 0; iHS < 3; iHS++) {
			fprintf(yearOutFile, ",%0.0f,%0.0f", cumHeatIn[iHS], cumHeatOut[iHS]);
			totalIn += cumHeatIn[iHS];
			totalOut += cumHeatOut[iHS];
		}
		fprintf(yearOutFile, ",%0.0f,%0.0f", totalIn, totalOut);
		for (int iHS = 0; iHS < 3; iHS++) {
			fprintf(yearOutFile, ",%0.2f", cumHeatOut[iHS] /cumHeatIn[iHS]);
		}
		fprintf(yearOutFile, ",%0.2f", totalOut/totalIn);
		fprintf(yearOutFile, "\n");
		fclose(yearOutFile);
	}
	else {
		fclose(outputFile);
	}
  controlFile.close();

  return 0;
}
