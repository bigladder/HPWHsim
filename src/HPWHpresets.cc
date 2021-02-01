/*
File Containing all of the presets available in HPWHsim
*/
#include <algorithm>

int HPWH::HPWHinit_resTank() {
	//a default resistance tank, nominal 50 gallons, 0.95 EF, standard double 4.5 kW elements
	return this->HPWHinit_resTank(GAL_TO_L(47.5), 0.95, 4500, 4500);
}
int HPWH::HPWHinit_resTank(double tankVol_L, double energyFactor, double upperPower_W, double lowerPower_W) {
	//low power element will cause divide by zero/negative UA in EF -> UA conversion
	if (lowerPower_W < 550) {
		if (hpwhVerbosity >= VRB_reluctant) {
			msg("Resistance tank wattage below 550 W.  DOES NOT COMPUTE\n");
		}
		return HPWH_ABORT;
	}
	if (energyFactor <= 0) {
		if (hpwhVerbosity >= VRB_reluctant) {
			msg("Energy Factor less than zero.  DOES NOT COMPUTE\n");
		}
		return HPWH_ABORT;
	}

	//use tank size setting function since it has bounds checking
	tankSizeFixed = false;
	int failure = this->setTankSize(tankVol_L);
	if (failure == HPWH_ABORT) {
		return failure;
	}


	numNodes = 12;
	tankTemps_C = new double[numNodes];
	setpoint_C = F_TO_C(127.0);

	//start tank off at setpoint
	resetTankToSetpoint();

	nextTankTemps_C = new double[numNodes];


	doTempDepression = false;
	tankMixesOnDraw = true;

	numHeatSources = 2;
	setOfSources = new HeatSource[numHeatSources];

	HeatSource resistiveElementBottom(this);
	HeatSource resistiveElementTop(this);
	resistiveElementBottom.setupAsResistiveElement(0, lowerPower_W);
	resistiveElementTop.setupAsResistiveElement(8, upperPower_W);

	//standard logic conditions
	resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
	resistiveElementBottom.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));

	resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
	resistiveElementTop.isVIP = true;

	setOfSources[0] = resistiveElementTop;
	setOfSources[1] = resistiveElementBottom;

	setOfSources[0].followedByHeatSource = &setOfSources[1];

	// (1/EnFac + 1/RecovEff) / (67.5 * ((24/41094) - 1/(RecovEff * Power_btuperHr)))
	double recoveryEfficiency = 0.98;
	double numerator = (1.0 / energyFactor) - (1.0 / recoveryEfficiency);
	double temp = 1.0 / (recoveryEfficiency * lowerPower_W*3.41443);
	double denominator = 67.5 * ((24.0 / 41094.0) - temp);
	tankUA_kJperHrC = UAf_TO_UAc(numerator / denominator);

	if (tankUA_kJperHrC < 0.) {
		if (hpwhVerbosity >= VRB_reluctant && tankUA_kJperHrC < -0.1) {
			msg("Computed tankUA_kJperHrC is less than 0, and is reset to 0.");
		}
		tankUA_kJperHrC = 0.0;
	}

	hpwhModel = MODELS_CustomResTank;

	//calculate oft-used derived values
	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) return HPWH_ABORT;

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}


	if (hpwhVerbosity >= VRB_emetic) {
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %p \n", i, &setOfSources[i]);
		}
		msg("\n\n");
	}

	simHasFailed = false;
	return 0;  //successful init returns 0
}

int HPWH::HPWHinit_genericHPWH(double tankVol_L, double energyFactor, double resUse_C) {
	//return 0 on success, HPWH_ABORT for failure
	simHasFailed = true;  //this gets cleared on successful completion of init

	//clear out old stuff if you're re-initializing
	delete[] tankTemps_C;
	delete[] setOfSources;




	//except where noted, these values are taken from MODELS_GE2014STDMode on 5/17/16
	numNodes = 12;
	tankTemps_C = new double[numNodes];
	setpoint_C = F_TO_C(127.0);

	nextTankTemps_C = new double[numNodes];

	//start tank off at setpoint
	resetTankToSetpoint();

	tankSizeFixed = false;

	//custom settings - these are set later
	//tankVolume_L = GAL_TO_L(45);
	//tankUA_kJperHrC = 6.5;

	doTempDepression = false;
	tankMixesOnDraw = true;

	numHeatSources = 3;
	setOfSources = new HeatSource[numHeatSources];

	HeatSource compressor(this);
	HeatSource resistiveElementBottom(this);
	HeatSource resistiveElementTop(this);

	//compressor values
	compressor.isOn = false;
	compressor.isVIP = false;
	compressor.typeOfHeatSource = TYPE_compressor;

	double split = 1.0 / 4.0;
	compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

	compressor.perfMap.reserve(2);

	compressor.perfMap.push_back({
		50, // Temperature (T_F)
		{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
		{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
		});

	compressor.perfMap.push_back({
		70, // Temperature (T_F)
		{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
		{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
		});

	compressor.minT = F_TO_C(45.);
	compressor.maxT = F_TO_C(120.);
	compressor.hysteresis_dC = dF_TO_dC(2);
	compressor.configuration = HeatSource::CONFIG_WRAPPED;

	//top resistor values
	resistiveElementTop.setupAsResistiveElement(6, 4500);
	resistiveElementTop.isVIP = true;

	//bottom resistor values
	resistiveElementBottom.setupAsResistiveElement(0, 4000);
	resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
	resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

	//logic conditions
	//this is set customly, from input
	//resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(19.6605)));
	resistiveElementTop.addTurnOnLogic(HPWH::topThird(resUse_C));

	resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

	compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
	compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));

	//custom adjustment for poorer performance
	//compressor.addShutOffLogic(HPWH::lowT(F_TO_C(37)));


	//end section of parameters from GE model




	//set tank volume from input
	//use tank size setting function since it has bounds checking
	int failure = this->setTankSize(tankVol_L);
	if (failure == HPWH_ABORT) {
		if (hpwhVerbosity >= VRB_reluctant) {
			msg("Failure to set tank size in generic hpwh init.");
		}
		return failure;
	}


	// derive conservative (high) UA from tank volume
	//   curve fit by Jim Lutz, 5-25-2016
	double tankVol_gal = tankVol_L / GAL_TO_L(1.);
	double v1 = 7.5156316175 * pow(tankVol_gal, 0.33) + 5.9995357658;
	tankUA_kJperHrC = 0.0076183819 * v1 * v1;




	//do a linear interpolation to scale COP curve constant, using measured values
	// Chip's attempt 24-May-2014
	double uefSpan = 3.4 - 2.0;

	//force COP to be 70% of GE at UEF 2 and 95% at UEF 3.4
	//use a fudge factor to scale cop and input power in tandem to maintain constant capacity
	double fUEF = (energyFactor - 2.0) / uefSpan;
	double genericFudge = (1. - fUEF)*.7 + fUEF * .95;

	compressor.perfMap[0].COP_coeffs[0] *= genericFudge;
	compressor.perfMap[0].COP_coeffs[1] *= genericFudge;
	compressor.perfMap[0].COP_coeffs[2] *= genericFudge;

	compressor.perfMap[1].COP_coeffs[0] *= genericFudge;
	compressor.perfMap[1].COP_coeffs[1] *= genericFudge;
	compressor.perfMap[1].COP_coeffs[2] *= genericFudge;

	compressor.perfMap[0].inputPower_coeffs[0] /= genericFudge;
	compressor.perfMap[0].inputPower_coeffs[1] /= genericFudge;
	compressor.perfMap[0].inputPower_coeffs[2] /= genericFudge;

	compressor.perfMap[1].inputPower_coeffs[0] /= genericFudge;
	compressor.perfMap[1].inputPower_coeffs[1] /= genericFudge;
	compressor.perfMap[1].inputPower_coeffs[2] /= genericFudge;




	//set everything in its places
	setOfSources[0] = resistiveElementTop;
	setOfSources[1] = resistiveElementBottom;
	setOfSources[2] = compressor;

	//and you have to do this after putting them into setOfSources, otherwise
	//you don't get the right pointers
	setOfSources[2].backupHeatSource = &setOfSources[1];
	setOfSources[1].backupHeatSource = &setOfSources[2];

	setOfSources[0].followedByHeatSource = &setOfSources[1];
	setOfSources[1].followedByHeatSource = &setOfSources[2];



	//standard finishing up init, borrowed from init function

	hpwhModel = MODELS_genericCustomUEF;

	//calculate oft-used derived values
	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) {
		return HPWH_ABORT;
	}

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}

	if (hpwhVerbosity >= VRB_emetic) {
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %p \n", i, &setOfSources[i]);
		}
		msg("\n\n");
	}

	simHasFailed = false;

	return 0;
}


int HPWH::HPWHinit_presets(MODELS presetNum) {
	//return 0 on success, HPWH_ABORT for failure
	simHasFailed = true;  //this gets cleared on successful completion of init

	//clear out old stuff if you're re-initializing
	delete[] tankTemps_C;
	delete[] setOfSources;


	//resistive with no UA losses for testing
	if (presetNum == MODELS_restankNoUA) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankSizeFixed = false;
		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 0; //0 to turn off


		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(8, 4500);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.isVIP = true;

		//assign heat sources into array in order of priority
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;

		setOfSources[0].followedByHeatSource = &setOfSources[1];
	}

	//resistive tank with massive UA loss for testing
	else if (presetNum == MODELS_restankHugeUA) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankSizeFixed = false;
		tankVolume_L = 120;
		tankUA_kJperHrC = 500; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;


		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		//set up a resistive element at the bottom, 4500 kW
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(9, 4500);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(20));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(15));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(20));
		resistiveElementTop.isVIP = true;


		//assign heat sources into array in order of priority
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;

		setOfSources[0].followedByHeatSource = &setOfSources[1];

	}

	//realistic resistive tank
	else if (presetNum == MODELS_restankRealistic) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankSizeFixed = false;
		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 10; //0 to turn off

		doTempDepression = false;
		//should eventually put tankmixes to true when testing progresses
		tankMixesOnDraw = false;

		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(9, 4500);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(20));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(15));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(20));
		resistiveElementTop.isVIP = true;

		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;

		setOfSources[0].followedByHeatSource = &setOfSources[1];
	}

	else if (presetNum == MODELS_StorageTank) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = 52;

		//start tank off at setpoint
		resetTankToSetpoint();

		setpoint_C = 800;

		tankSizeFixed = false;
		tankVolume_L = GAL_TO_L(80);
		tankUA_kJperHrC = 10; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;

		////////////////////////////////////////////////////
		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];
		
		HeatSource extra(this);
		
		//compressor values
		extra.isOn = false;
		extra.isVIP = false;
		extra.typeOfHeatSource = TYPE_extra;
		extra.configuration = HeatSource::CONFIG_WRAPPED;
		
		extra.addTurnOnLogic(HPWH::topThird_absolute(1));

		//initial guess, will get reset based on the input heat vector
		extra.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		setOfSources[0] = extra;
	}

	//basic compressor tank for testing
	else if (presetNum == MODELS_basicIntegrated) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankSizeFixed = false;
		tankVolume_L = 120;
		tankUA_kJperHrC = 10; //0 to turn off
		//tankUA_kJperHrC = 0; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);
		HeatSource compressor(this);

		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementTop.setupAsResistiveElement(9, 4500);

		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);

		//standard logic conditions
		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(20));
		resistiveElementBottom.addTurnOnLogic(HPWH::standby(15));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(20));
		resistiveElementTop.isVIP = true;


		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double oneSixth = 1.0 / 6.0;
		compressor.setCondensity(oneSixth, oneSixth, oneSixth, oneSixth, oneSixth, oneSixth, 0, 0, 0, 0, 0, 0);

		//GE tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.290 * 1000, 0.00159 * 1000, 0.00000107 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.49, -0.0187, -0.0000133} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.375 * 1000, 0.00121 * 1000, 0.00000216 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{5.60, -0.0252, 0.00000254} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = 0;
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED; //wrapped around tank

		compressor.addTurnOnLogic(HPWH::bottomThird(20));
		compressor.addTurnOnLogic(HPWH::standby(15));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}

	//simple external style for testing
	else if (presetNum == MODELS_externalTest) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankSizeFixed = false;
		tankVolume_L = 120;
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 0; //0 to turn off

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);

		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		//GE tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.290 * 1000, 0.00159 * 1000, 0.00000107 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.49, -0.0187, -0.0000133} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.375 * 1000, 0.00121 * 1000, 0.00000216 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{5.60, -0.0252, 0.00000254} // COP Coefficients (COP_coeffs)
			});

		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = 0;  //no hysteresis
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;

		compressor.addTurnOnLogic(HPWH::bottomThird(20));
		compressor.addTurnOnLogic(HPWH::standby(15));

		//lowT cutoff
		compressor.addShutOffLogic(HPWH::bottomNodeMaxTemp(20));


		//set everything in its places
		setOfSources[0] = compressor;
	}
	//voltex 60 gallon
	else if (presetNum == MODELS_AOSmithPHPT60) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 215.8;
		tankUA_kJperHrC = 7.31;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.467 * 1000, 0.00281 * 1000, 0.0000072 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.86, -0.0222, -0.00001} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.541 * 1000, 0.00147 * 1000, 0.0000176 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{6.58, -0.0392, 0.0000407} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4250);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 2000);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);

		//logic conditions
		double compStart = dF_TO_dC(43.6);
		double standbyT = dF_TO_dC(23.8);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(compStart));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(25.0)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithPHPT80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 283.9;
		tankUA_kJperHrC = 8.8;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.467 * 1000, 0.00281 * 1000, 0.0000072 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.86, -0.0222, -0.00001} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.541 * 1000, 0.00147 * 1000, 0.0000176 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{6.58, -0.0392, 0.0000407} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4250);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 2000);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);


		//logic conditions
		double compStart = dF_TO_dC(43.6);
		double standbyT = dF_TO_dC(23.8);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(compStart));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(25.0)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2012) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 172;
		tankUA_kJperHrC = 6.8;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{0.3 * 1000, 0.00159 * 1000, 0.00000107 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.7, -0.0210, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{0.378 * 1000, 0.00121 * 1000, 0.00000216 * 1000}, // Input Power Coefficients (inputPower_coeffs)
			{4.8, -0.0167, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(4);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4200);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4200);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);


		//logic conditions
		//    double compStart = dF_TO_dC(24.4);
		double compStart = dF_TO_dC(40.0);
		double standbyT = dF_TO_dC(5.2);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));
		// compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(66)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(compStart));
		//resistiveElementBottom.addShutOffLogic(HPWH::lowTreheat(lowTcutoff));
		//GE element never turns off?

		// resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(31.0)));
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(28.0)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	// If a Colmac single pass preset cold weather or not
	else if (MODELS_ColmacCxV_5_SP <= presetNum && presetNum <= MODELS_ColmacCxA_30_SP) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(135.0);
		tankSizeFixed = false;

		doTempDepression = false;
		tankMixesOnDraw = false;
		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 315; // Stolen from Sanden, will adjust 
		tankUA_kJperHrC = 7; // Stolen from Sanden, will adjust to tank size

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		compressor.isOn = false;
		compressor.isVIP = true;
		compressor.typeOfHeatSource = TYPE_compressor;
		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;
		compressor.perfMap.reserve(1);
		compressor.hysteresis_dC = 0;

		//logic conditions
		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(4);
		compressor.addTurnOnLogic(HPWH::HeatingLogic("fourth node", nodeWeights, dF_TO_dC(15), false));

		//lowT cutoff
		std::vector<NodeWeight> nodeWeights1;
		nodeWeights1.emplace_back(1);
		compressor.addShutOffLogic(HPWH::HeatingLogic("bottom node", nodeWeights1, dF_TO_dC(15.), false, std::greater<double>()));
		compressor.depressesTemperature = false;  //no temp depression

		//Defrost Derate 
		compressor.setupDefrostMap();

		if (presetNum == MODELS_ColmacCxV_5_SP) {
			setTankSize_adjustUA(200., UNITS_GAL);
			//logic conditions
			compressor.minT = F_TO_C(-4.0);

			compressor.perfMap.push_back({
				105, // Temperature (T_F)

				{-63.03503461, -0.113969285,1.131886391,-0.395945059,-0.000146565,-0.004548549,
				6.41633E-05,0.000942556,0.00203668,0.002724261,-1.37743E-05}, // Input Power Coefficients (inputPower_coeffs)

				{75.51023893,0.192639579,-1.236500063,0.410918932,-8.30539E-05,0.004998052,1.49907E-05,
				-0.001081378 ,-0.002400876, -0.002843726,1.5949E-05} // COP Coefficients (COP_coeffs)
				});
		}
		else {
			//logic conditions
			compressor.minT = F_TO_C(40.);
			
			if (presetNum == MODELS_ColmacCxA_10_SP) {
				setTankSize_adjustUA(500., UNITS_GAL);

				compressor.perfMap.push_back({
					105, // Temperature (T_F)

					{6.245405475,0.017843061, -0.004880269, -0.005480953, -3.36344E-05,
					0.000153143,0.000131151,1.55439E-05,-0.000207022,1.45476E-05,1.13399E-06}, // Input Power Coefficients (inputPower_coeffs)

					{2.999798697,0.049642812,-0.01948994,-0.004284595,9.38143E-05,
					4.84994E-05,-3.8207E-05,-0.000202575,2.55782E-05,5.76961E-05,-4.67458E-07} // COP Coefficients (COP_coeffs)
				});

			}
			else if (presetNum == MODELS_ColmacCxA_15_SP) {

				setTankSize_adjustUA(600., UNITS_GAL);
				compressor.perfMap.push_back({
					105, // Temperature (T_F)

					{13.66353179,0.00995298,-0.034178789 ,-0.013976793 ,-0.000110695,0.000260479,0.000232498,
					0.000194561, -0.000339296,5.31754E-06,2.3653E-06}, // Input Power Coefficients (inputPower_coeffs

					{1.945158936,0.041271369,-0.011142406,-0.001605424,4.92958E-05,3.48959E-05,-3.22831E-05,
					-0.000165898,1.12053E-05,3.92771E-05,-3.52623E-07} // COP Coefficients (COP_coeffs)
					});

			}
			else if (presetNum == MODELS_ColmacCxA_20_SP) {
				setTankSize_adjustUA(800., UNITS_GAL);

				compressor.perfMap.push_back({
					105, // Temperature (T_F)

					{18.72568714,0.027659996,-0.086369893,-0.007301256, -0.000514866,0.000442547,0.000231776,0.000564562,
					-0.000453523,-0.000106934,4.17353E-06}, // Input Power Coefficients (inputPower_coeffs)

					{1.731863198,0.057728361,-0.00874979,0.002559259,0.000133259,4.50948E-05,-3.21827E-05,-0.000317704,
					-6.47424E-05,1.90658E-05,3.35539E-08 } // COP Coefficients (COP_coeffs)
					});

			}
			else if (presetNum == MODELS_ColmacCxA_25_SP) {
				setTankSize_adjustUA(1000., UNITS_GAL);

				compressor.perfMap.push_back({
					105, // Temperature (T_F)

					{10.14179201,0.015981041,0.043484303,0.038453377,-0.000555971,0.000135448,0.000263362,
					0.000755368,-0.000740822,-0.000395114,5.98699E-06}, // Input Power Coefficients (inputPower_coeffs)
					

					{3.102088551,0.061014745,-0.025923335,-0.001847663,0.000101461,9.75336E-05,-2.82794E-05,
					-0.000334181 ,-6.88623E-05,4.09758E-05,1.45752E-07} // COP Coefficients (COP_coeffs)
					});

			}
			else if (presetNum == MODELS_ColmacCxA_30_SP) {
				setTankSize_adjustUA(1200., UNITS_GAL);

				compressor.perfMap.push_back({
					105, // Temperature (T_F)

					{13.36929355,-0.055689603,0.034403032,-0.03850619,-0.00053802,0.00010938,0.000356058,0.001429926,
					0.00019538, 0.000138577, -9.59205E-07}, // Input Power Coefficients (inputPower_coeffs)

					{4.433780491,0.061659096,-0.041408782,-0.0012498,7.91626E-05,0.000150242,-3.00005E-05,-0.000342003,
					-9.42052E-05,3.39684E-05,3.60339E-07} // COP Coefficients (COP_coeffs)
					});
			}
		} //End if MODELS_ColmacCxV_5_SP

		//set everything in its places
		setOfSources[0] = compressor;
	}
	
	// If Nyle single pass preset
	else if (MODELS_NyleC25A_SP <= presetNum && presetNum <= MODELS_NyleC250A_C_SP) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(135.0);
		tankSizeFixed = false;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 315;
		tankUA_kJperHrC = 7; // Stolen from Sanden, will adjust to 800 gallon tank
		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		//HeatSource resistiveElement(this);

		compressor.isOn = false;
		compressor.isVIP = true;
		compressor.typeOfHeatSource = TYPE_compressor;
		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		compressor.extrapolationMethod = EXTRAP_NEAREST;
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;
		compressor.perfMap.reserve(1);

		//logic conditions

		//logic conditions
		if (MODELS_NyleC25A_SP <= presetNum && presetNum <= MODELS_NyleC250A_SP) {// If not cold weather package
			compressor.minT = F_TO_C(40.); // Min air temperature sans Cold Weather Package
		}
		else {
			compressor.minT = F_TO_C(35.);// Min air temperature WITH Cold Weather Package
		}
		compressor.maxT = F_TO_C(120.0); // Max air temperature
		compressor.hysteresis_dC = 0;

		// Defines the maximum outlet temperature at the a low air temperature
		compressor.maxOut_at_LowT.outT_C = F_TO_C(140.);  
		compressor.maxOut_at_LowT.airT_C = F_TO_C(40.);

		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(4);
		compressor.addTurnOnLogic(HPWH::HeatingLogic("fourth node", nodeWeights, dF_TO_dC(15), false));

		//lowT cutoff
		std::vector<NodeWeight> nodeWeights1;
		nodeWeights1.emplace_back(1);
		compressor.addShutOffLogic(HPWH::HeatingLogic("bottom node", nodeWeights1, dF_TO_dC(15.), false, std::greater<double>()));
		compressor.depressesTemperature = false;  //no temp depression

		//Defrost Derate 
		compressor.setupDefrostMap();

		//Perfmaps for each compressor size
		if (presetNum == MODELS_NyleC25A_SP) {
			setTankSize_adjustUA(200., UNITS_GAL);
			compressor.perfMap.push_back({
					90, // Temperature (T_F)

					{4.060120364 ,-0.020584279,-0.024201054,-0.007023945,0.000017461,0.000110366,0.000060338,
					0.000120015,0.000111068,0.000138907,-0.000001569}, // Input Power Coefficients (inputPower_coeffs)

					{0.462979529,0.065656840,0.001077377,0.003428059,0.000243692,0.000021522,0.000005143,
					-0.000384778,-0.000404744,-0.000036277, 0.000001900 } // COP Coefficients (COP_coeffs)
				});
		}
		else if (presetNum == MODELS_NyleC60A_SP || presetNum == MODELS_NyleC60A_C_SP) {
			setTankSize_adjustUA(300., UNITS_GAL);
			compressor.perfMap.push_back({
					90, // Temperature (T_F)

					{-0.1180905709,0.0045354306,0.0314990479,-0.0406839757,0.0002355294,0.0000818684,0.0001943834,
					-0.0002160871,0.0003053633,0.0003612413,-0.0000035912}, // Input Power Coefficients (inputPower_coeffs)

					{6.8205043418,0.0860385185,-0.0748330699,-0.0172447955,0.0000510842,0.0002187441,-0.0000321036,
					-0.0003311463,-0.0002154270,0.0001307922,0.0000005568} // COP Coefficients (COP_coeffs)
				});
		}
		else if (presetNum == MODELS_NyleC90A_SP || presetNum == MODELS_NyleC90A_C_SP) {
			setTankSize_adjustUA(400., UNITS_GAL);
			compressor.perfMap.push_back({
					90, // Temperature (T_F)

					{13.27612215047,-0.01014009337,-0.13401028549,-0.02325705976,-0.00032515646,0.00040270625,
					0.00001988733,0.00069451670,0.00069067890,0.00071091372,-0.00000854352}, // Input Power Coefficients (inputPower_coeffs)

					{1.49112327987,0.06616282153,0.00715307252,-0.01269458185,0.00031448571,0.00001765313,
					0.00006002498,-0.00045661397,-0.00034003896,-0.00004327766,0.00000176015} // COP Coefficients (COP_coeffs)
				});
		}
		else if (presetNum == MODELS_NyleC125A_SP || presetNum == MODELS_NyleC125A_C_SP) {
			setTankSize_adjustUA(500., UNITS_GAL);
			compressor.perfMap.push_back({
					90, // Temperature (T_F)

					{-3.558277209, -0.038590968,	0.136307181, -0.016945699,0.000983753, -5.18201E-05,	
					0.000476904, -0.000514211, -0.000359172,	0.000266509, -1.58646E-07}, // Input Power Coefficients (inputPower_coeffs)

					{4.889555031,	0.117102769, -0.060005795, -0.011871234, -1.79926E-05,	0.000207293, 
					-1.4452E-05, -0.000492486, -0.000376814,	7.85911E-05,	1.47884E-06}// COP Coefficients (COP_coeffs)
				});
		}
		else if (presetNum == MODELS_NyleC185A_SP || presetNum == MODELS_NyleC185A_C_SP) {
			setTankSize_adjustUA(800., UNITS_GAL);
			compressor.perfMap.push_back({
					90, // Temperature (T_F)

					{18.58007733, -0.215324777, -0.089782421,	0.01503161,	0.000332503,	0.000274216,
					2.70498E-05,	0.001387914,	0.000449199,	0.000829578, -5.28641E-06}, // Input Power Coefficients (inputPower_coeffs)

					{-0.629432348,	0.181466663,	0.00044047,	0.012104957, -6.61515E-05,	9.29975E-05,	
						9.78042E-05, -0.000872708,-0.001013945, -0.00021852,	5.55444E-06}// COP Coefficients (COP_coeffs)
				});
		}
		else if (presetNum == MODELS_NyleC250A_SP || presetNum == MODELS_NyleC250A_C_SP) {
			setTankSize_adjustUA(800., UNITS_GAL);

			compressor.perfMap.push_back({
				90, // Temperature (T_F)

				{-13.89057656,0.025902417,0.304250541,0.061695153,-0.001474249,-0.001126845,-0.000220192,
				0.001241026,0.000571009,-0.000479282,9.04063E-06},// Input Power Coefficients (inputPower_coeffs)

				{7.443904067,0.185978755,-0.098481635,-0.002500073,0.000127658,0.000444321,0.000139547,
				-0.001000195,-0.001140199,-8.77557E-05,4.87405E-06} // COP Coefficients (COP_coeffs)
				});
		}

		//set everything in its places
		setOfSources[0] = compressor;
	}

	else if (presetNum == MODELS_Sanden80 || presetNum == MODELS_Sanden_GS3_45HPA_US_SP || presetNum == MODELS_Sanden120) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = 65;
		setpointFixed = true;

		//start tank off at setpoint
		resetTankToSetpoint();

		if (presetNum == MODELS_Sanden120) {
			tankVolume_L = GAL_TO_L(119);
			tankUA_kJperHrC = 9;
		}
		else {
			tankVolume_L = 315;
			tankUA_kJperHrC = 7;
			if (presetNum == MODELS_Sanden_GS3_45HPA_US_SP) {
				tankSizeFixed = false;
			}
		}

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);

		compressor.isOn = false;
		compressor.isVIP = true;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(5);

		compressor.perfMap.push_back({
			17, // Temperature (T_F)
			{1650, 5.5, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.2, -0.015, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			35, // Temperature (T_F)
			{1100, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.7, -0.015, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{880, 3.1, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.25, -0.025, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{740, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.2, -0.03, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{790, 2, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.15, -0.04, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.hysteresis_dC = 4;
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;


		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(8);
		compressor.addTurnOnLogic(HPWH::HeatingLogic("eighth node absolute", nodeWeights, F_TO_C(113), true));
		if (presetNum == MODELS_Sanden80 || presetNum == MODELS_Sanden120) {
			compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(8.2639)));
		}
		//lowT cutoff
		std::vector<NodeWeight> nodeWeights1;
		nodeWeights1.emplace_back(1);
		compressor.addShutOffLogic(HPWH::HeatingLogic("bottom node absolute", nodeWeights1, F_TO_C(135), true, std::greater<double>()));
		compressor.depressesTemperature = false;  //no temp depression

		//set everything in its places
		setOfSources[0] = compressor;
	}
	else if (presetNum == MODELS_Sanden40) {
		numNodes = 96;
		tankTemps_C = new double[numNodes];
		setpoint_C = 65;
		setpointFixed = true;

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 160;
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 5;

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 1;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);

		compressor.isOn = false;
		compressor.isVIP = true;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(5);

		compressor.perfMap.push_back({
			17, // Temperature (T_F)
			{1650, 5.5, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.2, -0.015, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			35, // Temperature (T_F)
			{1100, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.7, -0.015, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{880, 3.1, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.25, -0.025, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{740, 4.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.2, -0.03, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{790, 2, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.15, -0.04, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.hysteresis_dC = 4;
		compressor.configuration = HeatSource::CONFIG_EXTERNAL;


		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(4);
		compressor.addTurnOnLogic(HPWH::HeatingLogic("fourth node absolute", nodeWeights, F_TO_C(113), true));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(8.2639)));

		//lowT cutoff
		std::vector<NodeWeight> nodeWeights1;
		nodeWeights1.emplace_back(1);
		compressor.addShutOffLogic(HPWH::HeatingLogic("bottom node absolute", nodeWeights1, F_TO_C(135), true, std::greater<double>()));
		compressor.depressesTemperature = false;  //no temp depression

		//set everything in its places
		setOfSources[0] = compressor;
	}
	else if (presetNum == MODELS_AOSmithHPTU50 || presetNum == MODELS_RheemHBDR2250 || presetNum == MODELS_RheemHBDR4550) {
		numNodes = 24;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 171;
		tankUA_kJperHrC = 6;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 5.0;
		compressor.setCondensity(split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0);

		// performance map
		compressor.perfMap.reserve(3);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{170, 2.02, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.93, -0.027, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{144.5, 2.42, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.67, -0.037, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{94.1, 3.15, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{11.1, -0.056, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		if (presetNum == MODELS_RheemHBDR2250) {
			resistiveElementTop.setupAsResistiveElement(8, 2250);
		}
		else {
			resistiveElementTop.setupAsResistiveElement(8, 4500);
		}
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		if (presetNum == MODELS_RheemHBDR2250) {
			resistiveElementBottom.setupAsResistiveElement(0, 2250);
		}
		else {
			resistiveElementBottom.setupAsResistiveElement(0, 4500);
		}
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(35);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));


		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(11); nodeWeights.emplace_back(12);
		resistiveElementTop.addTurnOnLogic(HPWH::HeatingLogic("top sixth absolute", nodeWeights, F_TO_C(105), true));
		//		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(28)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

		setOfSources[0].companionHeatSource = &setOfSources[2];


	}
	else if (presetNum == MODELS_AOSmithHPTU66 || presetNum == MODELS_RheemHBDR2265 || presetNum == MODELS_RheemHBDR4565) {
		numNodes = 24;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		if (presetNum == MODELS_AOSmithHPTU66) {
			tankVolume_L = 244.6;
		}
		else {
			tankVolume_L = 221.4;
		}
		tankUA_kJperHrC = 8;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		// performance map
		compressor.perfMap.reserve(3);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{170, 2.02, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.93, -0.027, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{144.5, 2.42, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.67, -0.037, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{94.1, 3.15, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{11.1, -0.056, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		if (presetNum == MODELS_RheemHBDR2265) {
			resistiveElementTop.setupAsResistiveElement(8, 2250);
		}
		else {
			resistiveElementTop.setupAsResistiveElement(8, 4500);
		}
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		if (presetNum == MODELS_RheemHBDR2265) {
			resistiveElementBottom.setupAsResistiveElement(0, 2250);
		}
		else {
			resistiveElementBottom.setupAsResistiveElement(0, 4500);
		}
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(35);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		std::vector<NodeWeight> nodeWeights;
		nodeWeights.emplace_back(11); nodeWeights.emplace_back(12);
		resistiveElementTop.addTurnOnLogic(HPWH::HeatingLogic("top sixth absolute", nodeWeights, F_TO_C(105), true));
		//		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(31)));


				//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

		setOfSources[0].companionHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithHPTU80 || presetNum == MODELS_RheemHBDR2280 || presetNum == MODELS_RheemHBDR4580) {
		numNodes = 24;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 299.5;
		tankUA_kJperHrC = 9;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;
		compressor.configuration = HPWH::HeatSource::CONFIG_WRAPPED;

		double split = 1.0 / 3.0;
		compressor.setCondensity(split, split, split, 0, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(3);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{170, 2.02, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.93, -0.027, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{144.5, 2.42, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.67, -0.037, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			95, // Temperature (T_F)
			{94.1, 3.15, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{11.1, -0.056, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.0);
		compressor.hysteresis_dC = dF_TO_dC(1);

		//top resistor values
		if (presetNum == MODELS_RheemHBDR2280) {
			resistiveElementTop.setupAsResistiveElement(8, 2250);
		}
		else {
			resistiveElementTop.setupAsResistiveElement(8, 4500);
		}
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		if (presetNum == MODELS_RheemHBDR2280) {
			resistiveElementBottom.setupAsResistiveElement(0, 2250);
		}
		else {
			resistiveElementBottom.setupAsResistiveElement(0, 4500);
		}
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(35);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		std::vector<NodeWeight> nodeWeights;
		//		nodeWeights.emplace_back(9); nodeWeights.emplace_back(10);
		nodeWeights.emplace_back(11); nodeWeights.emplace_back(12);
		resistiveElementTop.addTurnOnLogic(HPWH::HeatingLogic("top sixth absolute", nodeWeights, F_TO_C(105), true));
		//		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(35)));


				//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

		setOfSources[0].companionHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithHPTU80_DR) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = 283.9;
		tankUA_kJperHrC = 9;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{142.6, 2.152, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.989258, -0.038320, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{120.14, 2.513, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{8.188, -0.0432, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(42.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(34.1636);
		double standbyT = dF_TO_dC(7.1528);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80.108)));

		// resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(39.9691)));
		resistiveElementTop.addTurnOnLogic(HPWH::topThird_absolute(F_TO_C(87)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_AOSmithCAHP120) {
	numNodes = 24;
	tankTemps_C = new double[numNodes];
	setpoint_C = F_TO_C(150.0);

	//start tank off at setpoint
	resetTankToSetpoint();

	tankVolume_L = GAL_TO_L(111.76); // AOSmith docs say 111.76
	tankUA_kJperHrC = UAf_TO_UAc(11.8);

	doTempDepression = false;
	tankMixesOnDraw = false;

	numHeatSources = 3;
	setOfSources = new HeatSource[numHeatSources];

	HeatSource compressor(this);
	HeatSource resistiveElementBottom(this);
	HeatSource resistiveElementTop(this);

	//compressor values
	compressor.isOn = false;
	compressor.isVIP = false;
	compressor.typeOfHeatSource = TYPE_compressor;
	compressor.setCondensity(0.3, 0.3, 0.2, 0.1, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0);

	//From CAHP 120 COP Tests
	compressor.perfMap.reserve(3);
	
	// Tuned on the multiple K167 tests
	compressor.perfMap.push_back({
		50., // Temperature (T_F)
		{2010.49966, -4.20966, 0.085395}, // Input Power Coefficients (inputPower_coeffs)
		{5.91, -0.026299, 0.0} // COP Coefficients (COP_coeffs)
		});

	compressor.perfMap.push_back({
		67.5, // Temperature (T_F)
		{2171.012, -6.936571, 0.1094962}, // Input Power Coefficients (inputPower_coeffs)
		{7.26272, -0.034135, 0.0} // COP Coefficients (COP_coeffs)
		});

	compressor.perfMap.push_back({
		95., // Temperature (T_F)
		{2276.0625, -7.106608, 0.119911}, // Input Power Coefficients (inputPower_coeffs)
		{8.821262, -0.042059, 0.0} // COP Coefficients (COP_coeffs)
		});

	compressor.minT = F_TO_C(47.0); //Product documentation says 45F doesn't look like it in CMP-T test//
	compressor.maxT = F_TO_C(110.0);
	compressor.hysteresis_dC = dF_TO_dC(2);
	compressor.configuration = HeatSource::CONFIG_WRAPPED;

	//top resistor values
	double wattRE = 6000;//5650.; 
	resistiveElementTop.setupAsResistiveElement(7, wattRE);
	resistiveElementTop.isVIP = true; // VIP is the only source that turns on independently when something else is already heating.

	//bottom resistor values
	resistiveElementBottom.setupAsResistiveElement(0, wattRE);
	resistiveElementBottom.hysteresis_dC = dF_TO_dC(2); 
	resistiveElementBottom.setCondensity(0.2, 0.8, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.); // Based of CMP test

	//logic conditions for 	
	double compStart = dF_TO_dC(5.25); 
	double standbyT = dF_TO_dC(5.); //Given CMP_T test 
	compressor.addTurnOnLogic(HPWH::secondSixth(compStart));
	compressor.addTurnOnLogic(HPWH::standby(standbyT));

	double resistanceStart = 12.;
	resistiveElementTop.addTurnOnLogic(HPWH::topThird(resistanceStart));
	resistiveElementBottom.addTurnOnLogic(HPWH::topThird(resistanceStart));

	resistiveElementTop.addShutOffLogic(HPWH::fifthSixthMaxTemp(F_TO_C(117.)));
	resistiveElementBottom.addShutOffLogic(HPWH::secondSixthMaxTemp(F_TO_C(109.)));

	//set everything in its places
	setOfSources[0] = resistiveElementTop;
	setOfSources[1] = resistiveElementBottom;
	setOfSources[2] = compressor;

	//and you have to do this after putting them into setOfSources, otherwise
	//you don't get the right pointers
	setOfSources[2].backupHeatSource = &setOfSources[1];
	setOfSources[1].backupHeatSource = &setOfSources[2];

	//setOfSources[0].followedByHeatSource = &setOfSources[2];
	//setOfSources[1].followedByHeatSource = &setOfSources[2];

	setOfSources[0].companionHeatSource = &setOfSources[1];
	setOfSources[1].companionHeatSource = &setOfSources[2];

	}
		else if (presetNum == MODELS_GE2014STDMode) {
			numNodes = 12;
			tankTemps_C = new double[numNodes];
			setpoint_C = F_TO_C(127.0);

			//start tank off at setpoint
			resetTankToSetpoint();

			tankVolume_L = GAL_TO_L(45);
			tankUA_kJperHrC = 6.5;

			doTempDepression = false;
			tankMixesOnDraw = true;

			numHeatSources = 3;
			setOfSources = new HeatSource[numHeatSources];

			HeatSource compressor(this);
			HeatSource resistiveElementBottom(this);
			HeatSource resistiveElementTop(this);

			//compressor values
			compressor.isOn = false;
			compressor.isVIP = false;
			compressor.typeOfHeatSource = TYPE_compressor;

			double split = 1.0 / 4.0;
			compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

			compressor.perfMap.reserve(2);

			compressor.perfMap.push_back({
				50, // Temperature (T_F)
				{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
				{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
				});

			compressor.perfMap.push_back({
				70, // Temperature (T_F)
				{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
				{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
				});

			compressor.minT = F_TO_C(37.0);
			compressor.maxT = F_TO_C(120.);
			compressor.hysteresis_dC = dF_TO_dC(2);
			compressor.configuration = HeatSource::CONFIG_WRAPPED;

			//top resistor values
			resistiveElementTop.setupAsResistiveElement(6, 4500);
			resistiveElementTop.isVIP = true;

			//bottom resistor values
			resistiveElementBottom.setupAsResistiveElement(0, 4000);
			resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
			resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

			//logic conditions
			resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(19.6605)));

			resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

			compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
			compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));
			//    compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

			//set everything in its places
			setOfSources[0] = resistiveElementTop;
			setOfSources[1] = resistiveElementBottom;
			setOfSources[2] = compressor;

			//and you have to do this after putting them into setOfSources, otherwise
			//you don't get the right pointers
			setOfSources[2].backupHeatSource = &setOfSources[1];
			setOfSources[1].backupHeatSource = &setOfSources[2];

			setOfSources[0].followedByHeatSource = &setOfSources[1];
			setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014STDMode_80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(75.4);
		tankUA_kJperHrC = 10.;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
			});

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(19.6605)));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));
		compressor.minT = F_TO_C(37);
		//    compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 6.5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014_80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(75.4);
		tankUA_kJperHrC = 10.;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_GE2014_80DR) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(75.4);
		tankUA_kJperHrC = 10.;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		// resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addTurnOnLogic(HPWH::topThird_absolute(F_TO_C(87)));
		// resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		// resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	// PRESET USING GE2014 DATA 
	else if (presetNum == MODELS_BWC2020_65) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(64);
		tankUA_kJperHrC = 7.6;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(20)));
		resistiveElementTop.addShutOffLogic(HPWH::topNodeMaxTemp(F_TO_C(116.6358)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(11.0648)));
		// compressor.addShutOffLogic(HPWH::largerDraw(F_TO_C(62.4074)));

		resistiveElementBottom.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(60)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(80)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;


		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

	}
	// If Rheem Premium
	else if (MODELS_Rheem2020Prem40 <= presetNum && presetNum <= MODELS_Rheem2020Prem80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		if (presetNum == MODELS_Rheem2020Prem40) {
			tankVolume_L = GAL_TO_L(36.1);
			tankUA_kJperHrC = 9.5;
		}
		else if (presetNum == MODELS_Rheem2020Prem50) {
			tankVolume_L = GAL_TO_L(45.1);
			tankUA_kJperHrC = 8.55;
		}
		else if (presetNum == MODELS_Rheem2020Prem65) {
			tankVolume_L = GAL_TO_L(58.5);
			tankUA_kJperHrC = 10.64;
		}
		else if (presetNum == MODELS_Rheem2020Prem80) {
			tankVolume_L = GAL_TO_L(72.0);
			tankUA_kJperHrC = 10.83;
		}

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(0.2, 0.2, 0.2, 0.2, 0.2, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{250, -1.0883, 0.0176}, // Input Power Coefficients (inputPower_coeffs)
			{6.7, -0.0087, -0.0002} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{275.0, -0.6631, 0.01571}, // Input Power Coefficients (inputPower_coeffs)
			{7.0, -0.0168, -0.0001} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.0);
		compressor.hysteresis_dC = dF_TO_dC(1);
		compressor.configuration = HPWH::HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);

		//logic conditions
		double compStart = dF_TO_dC(32);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));
		resistiveElementTop.addTurnOnLogic(HPWH::topSixth(dF_TO_dC(20.4167)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[1].backupHeatSource = &setOfSources[2];
		setOfSources[2].backupHeatSource = &setOfSources[1];

		setOfSources[0].followedByHeatSource = &setOfSources[2];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

		setOfSources[0].companionHeatSource = &setOfSources[2];
	}

	// If Rheem Build
	else if (MODELS_Rheem2020Build40 <= presetNum && presetNum <= MODELS_Rheem2020Build80) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		if (presetNum == MODELS_Rheem2020Build40) {
			tankVolume_L = GAL_TO_L(36.1);
			tankUA_kJperHrC = 9.5;
		}
		else if (presetNum == MODELS_Rheem2020Build50) {
			tankVolume_L = GAL_TO_L(45.1);
			tankUA_kJperHrC = 8.55;
		}
		else if (presetNum == MODELS_Rheem2020Build65) {
			tankVolume_L = GAL_TO_L(58.5);
			tankUA_kJperHrC = 10.64;
		}
		else if (presetNum == MODELS_Rheem2020Build80) {
			tankVolume_L = GAL_TO_L(72.0);
			tankUA_kJperHrC = 10.83;
		}

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		compressor.setCondensity(0.2, 0.2, 0.2, 0.2, 0.2, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);
		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{220.0, 0.8743, 0.00454}, // Input Power Coefficients (inputPower_coeffs)
			{ 7.96064, -0.0448, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{275.0, -0.6631, 0.01571}, // Input Power Coefficients (inputPower_coeffs)
			{8.45936, -0.04539, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.hysteresis_dC = dF_TO_dC(1);
		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.0);

		compressor.configuration = HPWH::HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(4);

		//logic conditions
		double compStart = dF_TO_dC(30);
		double standbyT = dF_TO_dC(9);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));
		resistiveElementTop.addTurnOnLogic(HPWH::topSixth(dF_TO_dC(20.4167)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[1].backupHeatSource = &setOfSources[2];
		setOfSources[2].backupHeatSource = &setOfSources[1];

		setOfSources[0].followedByHeatSource = &setOfSources[2];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

		setOfSources[0].companionHeatSource = &setOfSources[2];
	}

	else if (presetNum == MODELS_RheemHB50) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 7;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			47, // Temperature (T_F)
			{280, 4.97342, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.634009, -0.029485, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{280, 5.35992, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.3, -0.03, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.hysteresis_dC = dF_TO_dC(1);
		compressor.minT = F_TO_C(40.0);
		compressor.maxT = F_TO_C(120.0);

		compressor.configuration = HPWH::HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4200);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 2250);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		double compStart = dF_TO_dC(38);
		double standbyT = dF_TO_dC(13.2639);
		compressor.addTurnOnLogic(HPWH::bottomThird(compStart));
		compressor.addTurnOnLogic(HPWH::standby(standbyT));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(76.7747)));

		resistiveElementTop.addTurnOnLogic(HPWH::topSixth(dF_TO_dC(20.4167)));


		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Stiebel220E) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(56);
		//tankUA_kJperHrC = 10; //0 to turn off
		tankUA_kJperHrC = 9;

		doTempDepression = false;
		tankMixesOnDraw = false;

		numHeatSources = 2;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElement(this);

		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		resistiveElement.setupAsResistiveElement(0, 1500);
		resistiveElement.hysteresis_dC = dF_TO_dC(0);

		compressor.setCondensity(0, 0.12, 0.22, 0.22, 0.22, 0.22, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{295.55337, 2.28518, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.744118, -0.025946, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{282.2126, 2.82001, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{8.012112, -0.039394, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(32.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = 0;  //no hysteresis
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		compressor.addTurnOnLogic(HPWH::thirdSixth(dF_TO_dC(6.5509)));
		compressor.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		compressor.depressesTemperature = false;  //no temp depression

		//set everything in its places
		setOfSources[0] = compressor;
		setOfSources[1] = resistiveElement;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[0].backupHeatSource = &setOfSources[1];

	}
	else if (presetNum == MODELS_Generic1) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();
		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 9;
		doTempDepression = false;
		tankMixesOnDraw = true;
		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{472.58616, 2.09340, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{2.942642, -0.0125954, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{439.5615, 2.62997, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{3.95076, -0.01638033, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(45.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(8, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40.0)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(65)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(80)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(110)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(35)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Generic2) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();
		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 7.5;
		doTempDepression = false;
		tankMixesOnDraw = true;
		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{272.58616, 2.09340, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{4.042642, -0.0205954, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{239.5615, 2.62997, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.25076, -0.02638033, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(40.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(60)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(80)));
		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(100)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(40)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_Generic3) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(50);
		tankUA_kJperHrC = 5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		//voltex60 tier 1 values
		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{172.58616, 2.09340, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.242642, -0.0285954, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			67, // Temperature (T_F)
			{139.5615, 2.62997, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{6.75076, -0.03638033, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(35.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4500);
		resistiveElementBottom.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(40)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(10)));
		compressor.addShutOffLogic(HPWH::largeDraw(F_TO_C(55)));

		resistiveElementBottom.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(60)));

		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(40)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = compressor;
		setOfSources[2] = resistiveElementBottom;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}
	else if (presetNum == MODELS_UEF2generic) {
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = F_TO_C(127.0);

		//start tank off at setpoint
		resetTankToSetpoint();

		tankVolume_L = GAL_TO_L(45);
		tankUA_kJperHrC = 6.5;

		doTempDepression = false;
		tankMixesOnDraw = true;

		numHeatSources = 3;
		setOfSources = new HeatSource[numHeatSources];

		HeatSource compressor(this);
		HeatSource resistiveElementBottom(this);
		HeatSource resistiveElementTop(this);

		//compressor values
		compressor.isOn = false;
		compressor.isVIP = false;
		compressor.typeOfHeatSource = TYPE_compressor;

		double split = 1.0 / 4.0;
		compressor.setCondensity(split, split, split, split, 0, 0, 0, 0, 0, 0, 0, 0);

		compressor.perfMap.reserve(2);

		compressor.perfMap.push_back({
			50, // Temperature (T_F)
			{187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{4.29, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.perfMap.push_back({
			70, // Temperature (T_F)
			{148.0418, 2.553291, 0.0}, // Input Power Coefficients (inputPower_coeffs)
			{5.61, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
			});

		compressor.minT = F_TO_C(37.0);
		compressor.maxT = F_TO_C(120.);
		compressor.hysteresis_dC = dF_TO_dC(2);
		compressor.configuration = HeatSource::CONFIG_WRAPPED;

		//top resistor values
		resistiveElementTop.setupAsResistiveElement(6, 4500);
		resistiveElementTop.isVIP = true;

		//bottom resistor values
		resistiveElementBottom.setupAsResistiveElement(0, 4000);
		resistiveElementBottom.setCondensity(0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0);
		resistiveElementBottom.hysteresis_dC = dF_TO_dC(2);

		//logic conditions
		resistiveElementTop.addTurnOnLogic(HPWH::topThird(dF_TO_dC(18.6605)));

		resistiveElementBottom.addShutOffLogic(HPWH::bottomTwelthMaxTemp(F_TO_C(86.1111)));

		compressor.addTurnOnLogic(HPWH::bottomThird(dF_TO_dC(33.6883)));
		compressor.addTurnOnLogic(HPWH::standby(dF_TO_dC(12.392)));

		//set everything in its places
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		setOfSources[2] = compressor;

		//and you have to do this after putting them into setOfSources, otherwise
		//you don't get the right pointers
		setOfSources[2].backupHeatSource = &setOfSources[1];
		setOfSources[1].backupHeatSource = &setOfSources[2];

		setOfSources[0].followedByHeatSource = &setOfSources[1];
		setOfSources[1].followedByHeatSource = &setOfSources[2];

	}

	else {
		if (hpwhVerbosity >= VRB_reluctant) {
			msg("You have tried to select a preset model which does not exist.  \n");
		}
		return HPWH_ABORT;
	}

	// initialize nextTankTemps_C 
	nextTankTemps_C = new double[numNodes];

	hpwhModel = presetNum;

	//calculate oft-used derived values
	calcDerivedValues();

	if (checkInputs() == HPWH_ABORT) {
		return HPWH_ABORT;
	}

	isHeating = false;
	for (int i = 0; i < numHeatSources; i++) {
		if (setOfSources[i].isOn) {
			isHeating = true;
		}
		setOfSources[i].sortPerformanceMap();
	}

	if (hpwhVerbosity >= VRB_emetic) {
		for (int i = 0; i < numHeatSources; i++) {
			msg("heat source %d: %p \n", i, &setOfSources[i]);
		}
		msg("\n\n");
	}

	simHasFailed = false;
	return 0;  //successful init returns 0
}  //end HPWHinit_presets

