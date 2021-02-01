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
#define DEBUG 0

using std::cout;
using std::endl;
using std::string;
using std::ifstream;
//using std::ofstream;

#define F_TO_C(T) ((T-32.0)*5.0/9.0)
#define GAL_TO_L(GAL) (GAL*3.78541)

typedef std::vector<double> schedule;

int readSchedule(schedule &scheduleArray, string scheduleFileName, long minutesOfTest);

int main(int argc, char *argv[])
{
  HPWH hpwh;

  HPWH::DRMODES drStatus = HPWH::DR_ALLOW;
  HPWH::MODELS model;
  HPWH::CSVOPTIONS IP = HPWH::CSVOPT_IPUNITS; //  CSVOPT_NONE or  CSVOPT_IPUNITS
  // HPWH::UNITS units = HPWH::UNITS_F;

  const double EBALTHRESHOLD = 0.005;

  const int nTestTCouples = 6;
  // Schedule stuff
  std::vector<string> scheduleNames;
  std::vector<schedule> allSchedules(6);

  string testDirectory, fileToOpen, fileToOpen2, scheduleName, var1, input1, input2, input3, inputFile, outputDirectory;
  string inputVariableName, firstCol;
  double testVal, newSetpoint, airTemp, airTemp2, tempDepressThresh, inletH, newTankSize, tot_limit;
  int i, outputCode;
  long minutesToRun;

  double cumHeatIn[3] = { 0,0,0 };
  double cumHeatOut[3] = { 0,0,0 };

  bool HPWH_doTempDepress;
  int doInvMix, doCondu;

  FILE * outputFile;
  FILE * yearOutFile;
  ifstream controlFile;

  string strPreamble;
  string strHead = "minutes,Ta,Tsetpoint,inletT,draw,";
  
  if (DEBUG) {
	 hpwh.setVerbosity(HPWH::VRB_reluctant);
  }

  //.......................................
  //process command line arguments
  //.......................................

  cout << "Testing HPWHsim version " << HPWH::getVersion() << endl;

  //Obvious wrong number of command line arguments
  if ((argc > 6)) {
    cout << "Invalid input. This program takes FOUR arguments: model specification type (ie. Preset or File), model specification (ie. Sanden80),  test name (ie. test50) and output directory\n";
    exit(1);
  }
  //Help message
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

  if(argc == 6) {
    airTemp = std::stoi(argv[5]);
    HPWH_doTempDepress = true;
  } else {
    airTemp = 0;
    HPWH_doTempDepress = false;
  }

  //Only input file specified -- don't suffix with .csv
  testDirectory = input3;

  // Parse the model
  newSetpoint = 0;
  if(input1 == "Preset") {
    inputFile = "";
    
	model = mapStringToPreset(input2);

    if (hpwh.HPWHinit_presets(model) != 0) exit(1);
    if(model == HPWH::MODELS_Sanden80 || model == HPWH::MODELS_Sanden40) {
      newSetpoint = (149 - 32) / 1.8;
    }
  } else {
    inputFile = input2 + ".txt";
    if (hpwh.HPWHinit_file(inputFile) != 0) exit(1);
  }

  // Use the built-in temperature depression for the lockout test. Set the temp depression of 4C to better
  // try and trigger the lockout and hysteresis conditions
  tempDepressThresh = 4;
  hpwh.setMaxTempDepression(tempDepressThresh);
  hpwh.setDoTempDepression(HPWH_doTempDepress);

  // Read the test control file
  fileToOpen = testDirectory + "/" + "testInfo.txt";
  controlFile.open(fileToOpen.c_str());
  if(!controlFile.is_open()) {
    cout << "Could not open control file " << fileToOpen << "\n";
    exit(1);
  }
  minutesToRun = 0;
  newSetpoint = 0.;
  doCondu = 1;
  doInvMix = 1;
  inletH = 0.;
  newTankSize = 0.;
  tot_limit = 0.;
  cout << "Running: " << input2 << ", " << input1 << ", " << input3 << endl;

  while(controlFile >> var1 >> testVal) {
	if(var1 == "setpoint") { // If a setpoint was specified then override the default
		newSetpoint = testVal;
    }
	else if(var1 == "length_of_test") {
		minutesToRun = (int) testVal;
    }
	else if(var1 == "doInversionMixing") {
		doInvMix = (testVal > 0.0) ? 1 : 0;
	}
	else if(var1 == "doConduction") {
		doCondu = (testVal > 0.0) ? 1 : 0;
	}
	else if(var1 == "inletH") {
		inletH = testVal;
	}
	else if (var1 == "tanksize") {
		newTankSize = testVal;
	}
	else if(var1 == "tot_limit") {
		tot_limit = testVal;
	}
	else {
		cout << var1 << " in testInfo.txt is an unrecogized key.\n";
	}
  }

  if(minutesToRun == 0) {
    cout << "Error, must record length_of_test in testInfo.txt file\n";
    exit(1);
  }

  // ------------------------------------- Read Schedules--------------------------------------- //
  scheduleNames.push_back("inletT");
  scheduleNames.push_back("draw");
  scheduleNames.push_back("ambientT");
  scheduleNames.push_back("evaporatorT");
  scheduleNames.push_back("DR");
  scheduleNames.push_back("setpoint");

  for(i = 0; (unsigned)i < scheduleNames.size(); i++) {
    fileToOpen = testDirectory + "/" + scheduleNames[i] + "schedule.csv";
    outputCode = readSchedule(allSchedules[i], fileToOpen, minutesToRun);
    if(outputCode != 0) {
		if (scheduleNames[i] != "setpoint") {
			cout << "readSchedule returns an error on " << scheduleNames[i] << " schedule!\n";
			exit(1);
		}
		else {
			outputCode = 0;
		}
    }
  }
  
  if (doInvMix == 0) {
	  outputCode += hpwh.setDoInversionMixing(false);
  }

  if (doCondu == 0) {
	  outputCode += hpwh.setDoConduction(false);
  }
  if (newSetpoint > 0) {
	  if (!allSchedules[5].empty()) {
		  hpwh.setSetpoint(allSchedules[5][0]); //expect this to fail sometimes
		  hpwh.resetTankToSetpoint();
	  }
	  else {
		  hpwh.setSetpoint(newSetpoint);
		  hpwh.resetTankToSetpoint();
	  }
  }
  if (inletH > 0) {
	  outputCode += hpwh.setInletByFraction(inletH);
  }
  if (newTankSize > 0) {
	  hpwh.setTankSize(newTankSize, HPWH::UNITS_GAL);
  }
  if (tot_limit > 0) {
	  outputCode += hpwh.setTimerLimitTOT(tot_limit);
  }

  if (outputCode != 0) {
	  cout << "Control file testInfo.txt has unsettable specifics in it. \n";
	  exit(1);
  }

  // ----------------------Open the Output Files and Print the Header---------------------------- //
 
  if (minutesToRun > 500000.) {
	  fileToOpen = outputDirectory + "/DHW_YRLY.csv";

	  if (fopen_s(&yearOutFile, fileToOpen.c_str(), "a+") != 0) {
		  cout << "Could not open output file " << fileToOpen << "\n";
		  exit(1);
	  }
  }
  else {
	  fileToOpen = outputDirectory + "/" + input3 + "_" + input1 + "_" + input2 + ".csv";

	  if (fopen_s(&outputFile, fileToOpen.c_str(), "w+") != 0) {
		  cout << "Could not open output file " << fileToOpen << "\n";
		  exit(1);
	  }
	  hpwh.WriteCSVHeading(outputFile, strHead.c_str(), nTestTCouples, 0);
  }
  // ------------------------------------- Simulate --------------------------------------- //
  cout << "Now Simulating " << minutesToRun << " Minutes of the Test\n";

  std::vector<double> nodeExtraHeat_W;
  std::vector<double>* vectptr = NULL;
  // Loop over the minutes in the test
  for (i = 0; i < minutesToRun; i++) {

	  if (DEBUG) {
		  cout << "Now on minute: " << i << "\n";
	  }

	  if (HPWH_doTempDepress) {
		  airTemp2 = F_TO_C(airTemp);
	  }
	  else {
		  airTemp2 = allSchedules[2][i];
	  }

	  double tankHCStart = hpwh.getTankHeatContent_kJ();

	  // Process the dr status
	  drStatus = static_cast<HPWH::DRMODES>(int(allSchedules[4][i]));

	  // Change setpoint if there is a setpoint schedule. 
	  if (!allSchedules[5].empty() && !hpwh.isSetpointFixed()) {
		  hpwh.setSetpoint(allSchedules[5][i]); //expect this to fail sometimes
	  }

	  // Mix down for yearly tests with large compressors
	  if (hpwh.getHPWHModel() >= 210 && minutesToRun > 500000.) {
		  //Do a simple mix down of the draw for the cold water temperature
		  if (hpwh.getSetpoint() <= 125.) {
			  allSchedules[1][i] *= (125. - allSchedules[0][i]) / (hpwh.getTankNodeTemp(hpwh.getNumNodes() - 1, HPWH::UNITS_F) - allSchedules[0][i]);
		  }
	  }

	  // Run the step
	  hpwh.runOneStep(allSchedules[0][i], // Inlet water temperature (C)
		  GAL_TO_L(allSchedules[1][i]), // Flow in gallons
		  airTemp2,  // Ambient Temp (C)
		  allSchedules[3][i],  // External Temp (C)
		  drStatus, // DDR Status (now an enum. Fixed for now as allow)
		  1. * GAL_TO_L(allSchedules[1][i]), allSchedules[0][i],
		  vectptr);

	  // Check energy balance accounting. 
	  double hpwhElect = 0;
	  for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++) {
		  hpwhElect += hpwh.getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KJ);
	  }
	  double hpwhqHW = GAL_TO_L(allSchedules[1][i]) * (hpwh.getOutletTemp() - allSchedules[0][i]) 
				* HPWH::DENSITYWATER_kgperL
				* HPWH::CPWATER_kJperkgC;
	  double hpwhqEnv = hpwh.getEnergyRemovedFromEnvironment(HPWH::UNITS_KJ);
	  double hpwhqLoss = hpwh.getStandbyLosses(HPWH::UNITS_KJ);
	  double deltaHC = hpwh.getTankHeatContent_kJ() - tankHCStart;
	  double qBal = hpwhqEnv// HP energy extracted from surround
		  - hpwhqLoss		// tank loss to environment
		  + hpwhElect		// electricity in from heat sources
		  - hpwhqHW			// hot water energy from flows in and out
		  - deltaHC;		// change in tank stored energy

	  double fBal = fabs(qBal) / std::max(tankHCStart, 1.);
	  if (fBal > EBALTHRESHOLD){
		  cout << "On minute " << i << " HPWH has an energy balance error " << qBal << "kJ, " << 100*fBal << "%"<< "\n";
	  }

	  // Recording
	  if (minutesToRun < 500000.) {
	  		// Copy current status into the output file
	  		if (HPWH_doTempDepress) {
	  			airTemp2 = hpwh.getLocationTemp_C();
	  		}
	  		strPreamble = std::to_string(i) + ", " + std::to_string(airTemp2) + ", " + std::to_string(hpwh.getSetpoint()) + ", " +
	  			std::to_string(allSchedules[0][i]) + ", " + std::to_string(allSchedules[1][i]) + ", ";// +
	  			//std::to_string(hpwh.getOutletTemp()) + ",";
	  		hpwh.WriteCSVRow(outputFile, strPreamble.c_str(), nTestTCouples, 0);
	  }
	  else {
	  		for (int iHS = 0; iHS < hpwh.getNumHeatSources(); iHS++) {
	  			cumHeatIn[iHS] += hpwh.getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH)*1000.;
	  			cumHeatOut[iHS] += hpwh.getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH)*1000.;
	  			//cout << "Now on minute: " << i << ", heat source" << iHS << ", cumulative input:"<< cumHeatIn[iHS] << "\n";
	  		}
	  }
  }

  if (minutesToRun > 500000.) {
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



// this function reads the named schedule into the provided array
int readSchedule(schedule &scheduleArray, string scheduleFileName, long minutesOfTest) {
  int minuteHrTmp;
  bool hourInput;
  string line, snippet, s, minORhr;
  double valTmp;
  ifstream inputFile(scheduleFileName.c_str());
  //open the schedule file provided
  cout << "Opening " << scheduleFileName << '\n';

  if(!inputFile.is_open()) {
    return 1;
  }

  inputFile >> snippet >> valTmp;
  // cout << "snippet " << snippet << " valTmp"<< valTmp<<'\n';

  if(snippet != "default") {
    cout << "First line of " << scheduleFileName << " must specify default\n";
    return 1;
  }
  // cout << valTmp << " minutes = " << minutesOfTest << "\n";

  // Fill with the default value
  scheduleArray.assign(minutesOfTest, valTmp);

  // Burn the first two lines
  std::getline(inputFile, line);
  std::getline(inputFile, line);

  std::stringstream ss(line); // Will parse with a stringstream
  // Grab the first token, which is the minute or hour marker
  ss >> minORhr;
  if (minORhr.empty() ) { // If nothing left in the file
	  return 0;
  }
  hourInput = tolower(minORhr.at(0)) == 'h';
  char c; // to eat the commas nom nom
  // Read all the exceptions to the default value
  while (inputFile >> minuteHrTmp >> c >> valTmp) {

		if (minuteHrTmp >= (int)scheduleArray.size()) {
			cout << "In " << scheduleFileName << " the input file has more minutes than the test was defined with\n";
			return 1;
		}
		// Update the value
		if (!hourInput) {
			scheduleArray[minuteHrTmp] = valTmp;
		}
		else if (hourInput) {
			for (int j = minuteHrTmp * 60; j < (minuteHrTmp+1) * 60; j++) {
				scheduleArray[j] = valTmp;
				//cout << "minute " << j-(minuteHrTmp) * 60 << " of hour" << (minuteHrTmp)<<"\n";
			}
		}
  }
  
  //print out the whole schedule
// if(DEBUG == 1){
//   for(i = 0; (unsigned)i < scheduleArray.size(); i++){
//     cout << "scheduleArray[" << i << "] = " << scheduleArray[i] << "\n";
//   }
// }

  inputFile.close();

  return 0;

}