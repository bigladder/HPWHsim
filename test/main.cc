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
#include <iostream>
#include <fstream>
#include <sstream>
#include <string> 

#define MAX_DIR_LENGTH 255
#define DEBUG 0

using std::cout;
using std::endl;
using std::string;
using std::ifstream;
using std::ofstream;


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

  const int nTestTCouples = 6;

  // Schedule stuff
  std::vector<string> scheduleNames;
  std::vector<schedule> allSchedules(5);

  string testDirectory, fileToOpen, scheduleName, var1, input1, input2, input3, inputFile, outputDirectory;
  string inputVariableName;
  double testVal, newSetpoint, airTemp, airTemp2, tempDepressThresh, inletH;
  int i, outputCode;
  long minutesToRun;

  bool HPWH_doTempDepress;
  int doInvMix, doCondu;

  FILE * outputFile;
  ifstream controlFile;

  string strPreamble;
  // string strHead = "minutes,Ta,inletT,draw,outletT,";
  string strHead = "minutes,Ta,inletT,draw,";

  //.......................................
  //process command line arguments
  //.......................................

  cout << "Testing HPWHsim version " << HPWH::getVersion() << endl;

  //Obvious wrong number of command line arguments
  if ((argc > 6)) {
    // printf("Invalid input.  This program takes a single argument.  Help is on the way:\n\n");
    cout << "Invalid input. This program takes three arguments: model specification type, model specification, and test name\n";
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
    if(input2 == "Voltex60" || input2 == "AOSmithPHPT60") {
      model = HPWH::MODELS_AOSmithPHPT60;
    } else if(input2 == "Voltex80" || input2 == "AOSmith80") {
      model = HPWH::MODELS_AOSmithPHPT80;
    } else if(input2 == "GEred" || input2 == "GE") {
      model = HPWH::MODELS_GE2012;
    } else if(input2 == "SandenGAU" || input2 == "Sanden80" || input2 == "SandenGen3") {
      model = HPWH::MODELS_Sanden80;
    } else if(input2 == "SandenGES" || input2 == "Sanden40") {
      model = HPWH::MODELS_Sanden40;
    } else if(input2 == "AOSmithHPTU50") {
      model = HPWH::MODELS_AOSmithHPTU50;
    } else if(input2 == "AOSmithHPTU66") {
      model = HPWH::MODELS_AOSmithHPTU66;
    } else if(input2 == "AOSmithHPTU80") {
      model = HPWH::MODELS_AOSmithHPTU80;
    } else if(input2 == "AOSmithHPTU80DR") {
      model = HPWH::MODELS_AOSmithHPTU80_DR;
    } else if(input2 == "GE502014STDMode" || input2 == "GE2014STDMode") {
      model = HPWH::MODELS_GE2014STDMode;
    } else if(input2 == "GE502014" || input2 == "GE2014") {
      model = HPWH::MODELS_GE2014;
    } else if(input2 == "GE802014") {
      model = HPWH::MODELS_GE2014_80DR;
    } else if(input2 == "RheemHB50") {
      model = HPWH::MODELS_RheemHB50;
    } else if(input2 == "Stiebel220e" || input2 == "Stiebel220E") {
      model = HPWH::MODELS_Stiebel220E;
    } else if(input2 == "Generic1") {
      model = HPWH::MODELS_Generic1;
    } else if(input2 == "Generic2") {
      model = HPWH::MODELS_Generic2;
    } else if(input2 == "Generic3") {
      model = HPWH::MODELS_Generic3;
    } else if(input2 == "custom") {
      model = HPWH::MODELS_CustomFile;
	} else if (input2 == "restankRealistic") {
		model = HPWH::MODELS_restankRealistic;
	} else if(input2 == "StorageTank") {
      model = HPWH::MODELS_StorageTank;
	} else if (input2 == "CxA_20") {
		model = HPWH::MODELS_CxA_20;
      //do nothin, use custom-compiled input specified later
    } else {
      model = HPWH::MODELS_basicIntegrated;
      cout << "Couldn't find model " << input2 << ".  Exiting...\n";
      exit(1);
    }
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
  newSetpoint = 0.0;
  doCondu = 1;
  doInvMix = 1;
  inletH = 0.0;
  cout << "Running: " << input2 << ", " << input1 << ", " << input3 << endl;

  while(controlFile >> var1 >> testVal) {
	if(var1 == "setpoint") { // If a setpoint was specified then override the default
		newSetpoint = testVal;
    }
    if(var1 == "length_of_test") {
		minutesToRun = (int) testVal;
    }
	if (var1 == "doInversionMixing") {
		doInvMix = (testVal > 0.0) ? 1 : 0;
	}
	if (var1 == "doConduction") {
		doCondu = (testVal > 0.0) ? 1 : 0;
	}
	if (var1 == "inletH") {
		inletH = testVal;
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
  for(i = 0; (unsigned)i < scheduleNames.size(); i++) {
    fileToOpen = testDirectory + "/" + scheduleNames[i] + "schedule.csv";
    outputCode = readSchedule(allSchedules[i], fileToOpen, minutesToRun);
    if(outputCode != 0) {
      cout << "readSchedule returns an error on " << scheduleNames[i] << " schedule!\n";
      exit(1);
    }
  }

  if (doInvMix == 0) {
	  hpwh.setDoInversionMixing(false);
  }

  if (doCondu == 0) {
	  hpwh.setDoConduction(false);
  }

  if(newSetpoint > 0) {
	hpwh.setSetpoint(newSetpoint);
    hpwh.resetTankToSetpoint();
  }
 
  if (inletH > 0) {
 	  hpwh.setInletByFraction(inletH);
  }

  // ----------------------Open the Output File and Print the Header---------------------------- //
  fileToOpen = outputDirectory + "/" + input3 + "_" + input1 + "_" + input2 + ".csv";
 
  if (fopen_s(&outputFile, fileToOpen.c_str(), "w+") != 0) {
  cout << "Could not open output file " << fileToOpen << "\n";
  exit(1);
 }

  hpwh.WriteCSVHeading(outputFile, strHead.c_str(), nTestTCouples, 0);
  
  // ------------------------------------- Simulate --------------------------------------- //
  cout << "Now Simulating " << minutesToRun << " Minutes of the Test\n";

  std::vector<double> nodeExtraHeat_W;
  std::vector<double>* vectptr = NULL;
  // Loop over the minutes in the test
  for(i = 0; i < minutesToRun; i++) {

	if(DEBUG) {
      cout << "Now on minute: " << i << "\n";
    }

    if(HPWH_doTempDepress) {
      airTemp2 = F_TO_C(airTemp);
    } else {
      airTemp2 = allSchedules[2][i];
    }

    // Process the dr status
    if(allSchedules[4][i] == 0) {
      drStatus = HPWH::DR_BLOCK;
    } else if(allSchedules[4][i] == 1) {
      drStatus = HPWH::DR_ALLOW;
    } else if(allSchedules[4][i] == 2) {
      drStatus = HPWH::DR_ENGAGE;
    }

    // Run the step
	hpwh.runOneStep(allSchedules[0][i], // Inlet water temperature (C)
		GAL_TO_L(allSchedules[1][i]), // Flow in gallons
		airTemp2,  // Ambient Temp (C)
		allSchedules[3][i],  // External Temp (C)
		drStatus, // DDR Status (now an enum. Fixed for now as allow)
		1.0,    // Minutes per step
		1. * GAL_TO_L(allSchedules[1][i]), allSchedules[0][i],
		vectptr);

    // Copy current status into the output file
    if(HPWH_doTempDepress) {
      airTemp2 = hpwh.getLocationTemp_C();
    }

	strPreamble = std::to_string(i) + ", " + std::to_string(airTemp2) + ", " +
		std::to_string(allSchedules[0][i]) + ", " + std::to_string(allSchedules[1][i]) + ", ";// +
		//std::to_string(hpwh.getOutletTemp()) + ",";
	hpwh.WriteCSVRow(outputFile, strPreamble.c_str(), nTestTCouples, 0);
  }

  controlFile.close();
  fclose(outputFile);

  return 0;

}







// this function reads the named schedule into the provided array
int readSchedule(schedule &scheduleArray, string scheduleFileName, long minutesOfTest) {
  long i, minuteTmp;
  string line, snippet, s;
  double valTmp;
  ifstream inputFile(scheduleFileName.c_str());
  //open the schedule file provided
  cout << "Opening " << scheduleFileName << '\n';

  if(!inputFile.is_open()) {
    cout << "Unable to open file" << '\n';
    return 1;
  }

  inputFile >> snippet >> valTmp;
  if(snippet != "default") {
    cout << "First line of " << scheduleFileName << " must specify default\n";
    return 1;
  }
  // cout << valTmp << " minutes = " << minutesOfTest << "\n";
  // cout << "size " << scheduleArray.size() << "\n";
  // Fill with the default value
  for(i = 0; i < minutesOfTest; i++) {
    scheduleArray.push_back(valTmp);
    // scheduleArray[i] = valTmp;
  }

  // Burn the first two lines
  std::getline(inputFile, line);
  std::getline(inputFile, line);

  // Read all the exceptions
  while(std::getline(inputFile, line)) {
    std::stringstream ss(line); // Will parse with a stringstream

    // Grab the first token, which is the minute
    std::getline(ss, s, ',');
    std::istringstream(s) >> minuteTmp;

    // Grab the second token, which is the value
    std::getline(ss, s, ',');
    std::istringstream(s) >> valTmp;

    // Update the value
    scheduleArray[minuteTmp] = valTmp;
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
