  
  typedef std::vector<double> schedule;

  bool readTestSchedules(const std::string &testDirectory, std::vector<schedule> &allSchedules)
  {
 
	  // Read the test control file
	  fileToOpen = testDirectory + "/" + "testInfo.txt";
	  controlFile.open(fileToOpen.c_str());
	  if(!controlFile.is_open()) {
		cout << "Could not open control file " << fileToOpen << "\n";
		exit(1);
	  }
	  outputCode = 0;
	  minutesToRun = 0;
	  newSetpoint = 0.;
	  initialTankT_C = 0.;
	  doCondu = 1;
	  doInvMix = 1;
	  inletH = 0.;
	  newTankSize = 0.;
	  tot_limit = 0.;
	  useSoC = false;
	  bool hasInitialTankTemp = false;
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
		else if(var1 == "tanksize") {
			newTankSize = testVal;
		}
		else if(var1 == "tot_limit") {
			tot_limit = testVal;
		}
		else if(var1 == "useSoC") {
			useSoC = (bool)testVal;
		}
		if(var1 == "initialTankT_C") { // Initialize at this temperature instead of setpoint
			initialTankT_C = testVal;
			hasInitialTankTemp = true;
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
	  scheduleNames.push_back("SoC");

	  for(i = 0; (unsigned)i < scheduleNames.size(); i++) {
		fileToOpen = testDirectory + "/" + scheduleNames[i] + "schedule.csv";
		outputCode = readSchedule(allSchedules[i], fileToOpen, minutesToRun);
		if(outputCode != 0) {
			if (scheduleNames[i] != "setpoint" && scheduleNames[i] != "SoC") {
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
			  if (hasInitialTankTemp)
				  hpwh.setTankToTemperature(initialTankT_C);
			  else
				hpwh.resetTankToSetpoint();
		  }
		  else {
			  hpwh.setSetpoint(newSetpoint);
			  if (hasInitialTankTemp)
				  hpwh.setTankToTemperature(initialTankT_C);
			  else
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
	  if (useSoC) {
		  if (allSchedules[6].empty()) {
			  cout << "If useSoC is true need an SoCschedule.csv file \n";
		  }
		  outputCode += hpwh.switchToSoCControls(1., .05, soCMinTUse_C, true, soCMains_C);
	  }
		
	  if (outputCode != 0) {
		  cout << "Control file testInfo.txt has unsettable specifics in it. \n";
		  return false;
	  }
	  return true;
}