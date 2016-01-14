#include "HPWH.hh"


using std::cout;
using std::endl;
using std::string;


//the HPWH functions
//the publics
HPWH::HPWH() :
  isHeating(false) {
  }

HPWH::~HPWH() {
  delete[] tankTemps_C;
  delete[] setOfSources;  
}


int HPWH::HPWHinit_presets(int presetNum) {
  //resistive with no UA losses for testing
  if (presetNum == 1) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = 50;

    //start tank off at setpoint
    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
    tankVolume_L = 12; 
    tankUA_kJperHrC = 0; //0 to turn off
    
    doTempDepression = false;
    tankMixesOnDraw = false;

    numHeatSources = 2;
    setOfSources = new HeatSource[numHeatSources];

    //set up a resistive element at the bottom, 4500 kW
    HeatSource resistiveElementBottom(this);
    HeatSource resistiveElementTop(this);
    
    resistiveElementBottom.setupAsResistiveElement(0, 4500);
    resistiveElementTop.setupAsResistiveElement(9, 4500);

    //temporary for testing
    resistiveElementBottom.configuration = "external";
    resistiveElementBottom.shutOffLogicSet.push_back(HeatSource::heatingLogicPair("bottomTwelthMaxTemp", 40));
    
    //assign heat sources into array in order of priority
    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = resistiveElementBottom;
  }

  //resistive tank with massive UA loss for testing
  else if (presetNum == 2) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = 50;

    //start tank off at setpoint
    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
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
    
    //assign heat sources into array in order of priority
    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = resistiveElementBottom;


  }

  //realistic resistive tank
  else if(presetNum == 3) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = F_TO_C(127);

    //start tank off at setpoint
    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
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

    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = resistiveElementBottom;
  }

  //basic compressor tank for testing
  else if (presetNum == 4) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = 50;

    //start tank off at setpoint
    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
    tankVolume_L = 120; 
    //tankUA_kJperHrC = 10; //0 to turn off
    tankUA_kJperHrC = 0; //0 to turn off
    
    doTempDepression = false;
    tankMixesOnDraw = false;

    numHeatSources = 3;
    setOfSources = new HeatSource[numHeatSources];

    HeatSource resistiveElementBottom(this);
    HeatSource resistiveElementTop(this);
    HeatSource compressor(this);

    resistiveElementBottom.setupAsResistiveElement(0, 4500);
    resistiveElementTop.setupAsResistiveElement(9, 4500);

    resistiveElementBottom.shutOffLogicSet.push_back(HeatSource::heatingLogicPair("lowTreheat", 5));

    

    compressor.isOn = false;
    compressor.isVIP = false;

    double oneSixth = 1.0/6.0;
    compressor.setCondensity(oneSixth, oneSixth, oneSixth, oneSixth, oneSixth, oneSixth, 0, 0, 0, 0, 0, 0);

    //GE tier 1 values
    compressor.T1_F = 47;
    compressor.T2_F = 67;

    compressor.inputPower_T1_constant_W = 0.290*1000;
    compressor.inputPower_T1_linear_WperF = 0.00159*1000;
    compressor.inputPower_T1_quadratic_WperF2 = 0.00000107*1000;
    compressor.inputPower_T2_constant_W = 0.375*1000;
    compressor.inputPower_T2_linear_WperF = 0.00121*1000;
    compressor.inputPower_T2_quadratic_WperF2 = 0.00000216*1000;
    compressor.COP_T1_constant = 4.49;
    compressor.COP_T1_linear = -0.0187;
    compressor.COP_T1_quadratic = -0.0000133;
    compressor.COP_T2_constant = 5.60;
    compressor.COP_T2_linear = -0.0252;
    compressor.COP_T2_quadratic = 0.00000254;
    compressor.hysteresis = 0;  //no hysteresis
    compressor.configuration = "wrapped"; //wrapped around tank
    
    compressor.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("bottomThird", 20));
    compressor.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("standby", 15));

    //lowT cutoff
    compressor.shutOffLogicSet.push_back(HeatSource::heatingLogicPair("lowT", 0));

    compressor.depressesTemperature = false;  //no temp depression

    //set everything in its places
    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = compressor;
    setOfSources[2] = resistiveElementBottom;

    //and you have to do this after putting them into setOfSources, otherwise
    //you don't get the right pointers
    setOfSources[2].backupHeatSource = &setOfSources[1];
    setOfSources[1].backupHeatSource = &setOfSources[2];
   
  }


  //resistiveElementBottom.backupHeatSource = &compressor;
  //compressor.backupHeatSource = &resistiveElementBottom;
   

  //cout << "heat source 1 " << &setOfSources[0] << endl;
  //cout << "heat source 2 " << &setOfSources[1] << endl;
  //cout << "heat source 3 " << &setOfSources[2] << endl;

  return 0;
}  //end HPWHinit_presets


int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
                     double tankAmbientT_C, double heatSourceAmbientT_C,
                     double DRstatus, double minutesPerStep) {

  //reset the output variables
  outletTemp_C = 0;
  energyRemovedFromEnvironment_kWh = 0;
  standbyLosses_kWh = 0;

  for (int i = 0; i < numHeatSources; i++) {
    setOfSources[i].runtime_min = 0;
    setOfSources[i].energyInput_kWh = 0;
    setOfSources[i].energyOutput_kWh = 0;
  }

  // if you are doing temp. depression, set tank and heatSource ambient temps
  // to the tracked locationTemperature
  double temperatureGoal = tankAmbientT_C;
  if (doTempDepression) {
    tankAmbientT_C = locationTemperature;
    heatSourceAmbientT_C = locationTemperature;
  }
  


  //process draws and standby losses
  updateTankTemps(drawVolume_L, inletT_C, tankAmbientT_C, minutesPerStep);


  //do HeatSource choice
  for (int i = 0; i < numHeatSources; i++) {
    //if there's a priority HeatSource (e.g. upper resistor) and it needs to 
    //come on, then turn everything off and start it up
    //cout << "check vip should heat source[i]: " << i << endl;
    if (setOfSources[i].isVIP && setOfSources[i].shouldHeat(heatSourceAmbientT_C)) {
      turnAllHeatSourcesOff();
      setOfSources[i].engageHeatSource(heatSourceAmbientT_C);
      //stop looking when you've found such a HeatSource
      break;
    }
    //is nothing is currently on, then check if something should come on
    else if (isHeating == false) {
    //cout << "check all-off should heat source[i]: " << i << endl;

      if (setOfSources[i].shouldHeat(heatSourceAmbientT_C)) {
        setOfSources[i].engageHeatSource(heatSourceAmbientT_C);
        //engaging a heat source sets isHeating to true, so this will only trigger once
      }
    }
    else {
      //check if anything that is on needs to turn off (generally for lowT cutoffs)
      //even for things that just turned on this step - otherwise they don't stay
      //off when they should
      if (setOfSources[i].isEngaged() && setOfSources[i].shutsOff(heatSourceAmbientT_C)) {
        setOfSources[i].disengageHeatSource();
        //check if the backup heat source would have to shut off too
        if (setOfSources[i].backupHeatSource != NULL && setOfSources[i].backupHeatSource->shutsOff(heatSourceAmbientT_C) != true) {
          //and if not, go ahead and turn it on 
          setOfSources[i].backupHeatSource->engageHeatSource(heatSourceAmbientT_C);
        }
      }
    } 
  }  //end loop over heat sources

  //cout << "after heat source choosing:  heatsource 0: " << setOfSources[0].isEngaged() << " heatsource 1: " << setOfSources[1].isEngaged() << endl;


  //change the things according to DR schedule
  if (DRstatus == 0) {
    //force off
    turnAllHeatSourcesOff();
    isHeating = false;
  }
  else if (DRstatus == 1) {
    //do nothing
  }
  else if (DRstatus == 2) {
    //if nothing else is on, force the first heat source on
    //this may or may not be desired behavior, pending more research (and funding)
    if (areAllHeatSourcesOff() == true) {
      setOfSources[0].engageHeatSource(heatSourceAmbientT_C);
    }
  }





  //do heating logic
  double minutesToRun = minutesPerStep;
  
  for (int i = 0; i < numHeatSources; i++) {
    //going through in order, check if the heat source is on
    if (setOfSources[i].isEngaged()) {
      //and add heat if it is
      setOfSources[i].addHeat(heatSourceAmbientT_C, minutesToRun);
      //if it finished early
      if (setOfSources[i].runtime_min < minutesToRun) {
        cout << "done heating! runtime_min minutesToRun " << setOfSources[i].runtime_min << " " << minutesToRun << endl;
        //subtract time it ran and turn it off
        minutesToRun -= setOfSources[i].runtime_min;
        setOfSources[i].disengageHeatSource();
        //and if there's another heat source in the list, that's able to come on,
        if (numHeatSources > i+1 && setOfSources[i + 1].shutsOff(heatSourceAmbientT_C) == false) {
          //turn it on
          setOfSources[i + 1].engageHeatSource(heatSourceAmbientT_C);
        }
      }
    }
  }

  if (areAllHeatSourcesOff() == true) {
    isHeating = false;
  }


  //track the depressed local temperature
  if (doTempDepression) {
    bool compressorRan = false;
    for (int i = 0; i < numHeatSources; i++) {
      if (setOfSources[i].isEngaged() && setOfSources[i].depressesTemperature) {
        compressorRan = true;
      }
    }
    
    if(compressorRan){
      temperatureGoal -= 4.5;		//hardcoded 4.5 degree total drop - from experimental data
    }
    else{
      //otherwise, do nothing, we're going back to ambient
    }

    // shrink the gap by the same percentage every minute - that gives us
    // exponential behavior the percentage was determined by a fit to
    // experimental data - 9.4 minute half life and 4.5 degree total drop
    //minus-equals is important, and fits with the order of locationTemperature
    //and temperatureGoal, so as to not use fabs() and conditional tests
    locationTemperature -= (locationTemperature - temperatureGoal)*(1 - 0.9289);
  }
  
  


  //settle outputs

  //outletTemp_C and standbyLosses_kWh are taken care of in updateTankTemps

  //sum energyRemovedFromEnvironment_kWh for each heat source;
  for (int i = 0; i < numHeatSources; i++) {
    energyRemovedFromEnvironment_kWh += (setOfSources[i].energyOutput_kWh - setOfSources[i].energyInput_kWh);
  }



  return 0;
} //end runOneStep



int HPWH::runNSteps(int N,  double *inletT_C, double *drawVolume_L, 
                    double *tankAmbientT_C, double *heatSourceAmbientT_C,
                    double *DRstatus, double minutesPerStep) {
  //these are all the accumulating variables we'll need
  double energyRemovedFromEnvironment_kWh_SUM = 0;
  double standbyLosses_kWh_SUM = 0;
  double outletTemp_C_AVG = 0;
  double totalDrawVolume_L = 0;
  std::vector<double> heatSources_runTimes_SUM(numHeatSources);
  std::vector<double> heatSources_energyInputs_SUM(numHeatSources);
  std::vector<double> heatSources_energyOutputs_SUM(numHeatSources);


  //run the sim one step at a time, accumulating the outputs as you go
  for (int i = 0; i < N; i++) {
    runOneStep( inletT_C[i], drawVolume_L[i], tankAmbientT_C[i], heatSourceAmbientT_C[i],
                DRstatus[i], minutesPerStep );

    energyRemovedFromEnvironment_kWh_SUM += energyRemovedFromEnvironment_kWh;
    standbyLosses_kWh_SUM += standbyLosses_kWh;

    totalDrawVolume_L += drawVolume_L[i];
    outletTemp_C_AVG += outletTemp_C * drawVolume_L[i];
    
    for (int j = 0; j < numHeatSources; j++) {
      heatSources_runTimes_SUM[j] += getNthHeatSourceRunTime(j);
      heatSources_energyInputs_SUM[j] += getNthHeatSourceEnergyInput(j);
      heatSources_energyOutputs_SUM[j] += getNthHeatSourceEnergyOutput(j);
    }

  }

  //finish weighted avg. of outlet temp by dividing by the total drawn volume
  outletTemp_C_AVG /= totalDrawVolume_L;


  //now, reassign all of the accumulated values to their original spots
  energyRemovedFromEnvironment_kWh = energyRemovedFromEnvironment_kWh_SUM;
  standbyLosses_kWh = standbyLosses_kWh_SUM;
  outletTemp_C = outletTemp_C_AVG;

  for (int i = 0; i < numHeatSources; i++) {
    setNthHeatSourceRunTime(i, heatSources_runTimes_SUM[i]);
    setNthHeatSourceEnergyInput(i, heatSources_energyInputs_SUM[i]);
    setNthHeatSourceEnergyOutput(i, heatSources_energyOutputs_SUM[i]);
  }

  return 0;
}




int HPWH::getNumNodes() const {
  return numNodes;
  }

double HPWH::getTankNodeTemp(int nodeNum) const {
  if (nodeNum > numNodes || nodeNum < 0) {
    cout << "You have attempted to access the temperature of a tank node that does not exist.  Exiting..." << endl;
    exit(1);
  }
  return tankTemps_C[nodeNum];
}


double HPWH::getNthSimTcouple(int N, string units) const {
  if (N > 6 || N < 1) {
    cout << "You have attempted to access a simulated thermocouple that does not exist.  Exiting..." << endl;
    exit(1);
  }
  
  double averageTemp_C = 0;
  //specify N from 1-6, so use N-1 for node number
  for (int i = (N-1)*(numNodes/6); i < N*(numNodes/6); i++) {
    averageTemp_C += getTankNodeTemp(i);
  }
  averageTemp_C /= (numNodes/6);

  if (units == "C") {
    return averageTemp_C;
  }
  else if (units == "F") {
    return C_TO_F(averageTemp_C);
  }
 else {
    cout << "Incorrect unit specification for getNthSimTcouple" << endl;
    exit(1);
 }
}


int HPWH::getNumHeatSources() const {
  return numHeatSources;
}



double HPWH::getNthHeatSourceEnergyInput(int N, string units) const {
  if (N > numNodes || N < 0) {
    cout << "You have attempted to access the energy input of a heat source that does not exist.  Exiting..." << endl;
    exit(1);
  }
  double returnVal = setOfSources[N].energyInput_kWh;

  if (units == "kWh" || units == "kwh") {
    return returnVal;
  }
  else if (units == "btu") {
    return KWH_TO_BTU(returnVal);
  }
  else if (units == "kJ") {
    return KWH_TO_KJ(returnVal);
  }
 else {
    cout << "Incorrect unit specification for getNthHeatSourceEnergyInput" << endl;
    exit(1);
 }
}

double HPWH::getNthHeatSourceEnergyOutput(int N, string units) const {
  if (N > numNodes || N < 0) {
    cout << "You have attempted to access the energy output of a heat source that does not exist.  Exiting..." << endl;
  }
  double returnVal = setOfSources[N].energyOutput_kWh;

  if (units == "kWh" || units == "kwh") {
    return returnVal;
  }
  else if (units == "btu") {
    return KWH_TO_BTU(returnVal);
  }
  else if (units == "kJ") {
    return KWH_TO_KJ(returnVal);
  }
 else {
    cout << "Incorrect unit specification for getNthHeatSourceEnergyInput" << endl;
    exit(1);
 }
}

double HPWH::getNthHeatSourceRunTime(int N) const {
  if (N > numNodes || N < 0) {
    cout << "You have attempted to access the run time of a heat source that does not exist.  Exiting..." << endl;
    exit(1);
  }
  return setOfSources[N].runtime_min;
}	


bool HPWH::isNthHeatSourceRunning(int N) const{
  if (N > numNodes || N < 0) {
    cout << "You have attempted to access the status of a heat source that does not exist.  Exiting..." << endl;
    exit(1);
  }
  return setOfSources[N].isEngaged();
}


double HPWH::getOutletTemp(string units) const {
  if (units == "C") {
    return outletTemp_C;
  }
  else if (units == "F") {
    return C_TO_F(outletTemp_C);
    }
  else {
    cout << "Incorrect unit specification for getOutletTemp" << endl;
    exit(1);
  }
}

double HPWH::getEnergyRemovedFromEnvironment(string units) const {
  double returnVal = 0;

  if (units == "kWh" || units == "kwh") {
    returnVal = energyRemovedFromEnvironment_kWh;
  }
  else if (units == "btu") {
    returnVal = KWH_TO_BTU(energyRemovedFromEnvironment_kWh);
    }
  else {
    cout << "Incorrect unit specification for getEnergyRemovedFromEnvironment" << endl;
    exit(1);
  }
  return returnVal;
}

double HPWH::getStandbyLosses(string units) const {
  double returnVal = 0;

  if (units == "kWh" || units == "kwh") {
    returnVal = standbyLosses_kWh;
  }
  else if (units == "btu") {
    returnVal = KWH_TO_BTU(standbyLosses_kWh);
    }
  else {
    cout << "Incorrect unit specification for getStandbyLosses" << endl;
    exit(1);
  }
  return returnVal;
}



//the privates
void HPWH::updateTankTemps(double drawVolume_L, double inletT_C, double tankAmbientT_C, double minutesPerStep) {
  //set up some useful variables for calculations
  double volPerNode_LperNode = tankVolume_L/numNodes;
  double drawFraction;
  int wholeNodesToDraw;
  outletTemp_C = 0;

  //calculate how many nodes to draw (wholeNodesToDraw), and the remainder (drawFraction)
  drawFraction = drawVolume_L/volPerNode_LperNode;
  wholeNodesToDraw = (int)std::floor(drawFraction);
  drawFraction -= wholeNodesToDraw;

  //move whole nodes
  if (wholeNodesToDraw > 0) {        
    for (int i = 0; i < wholeNodesToDraw; i++) {
      //add temperature of drawn nodes for outletT average
      outletTemp_C += tankTemps_C[numNodes-1 - i];  
    }

    for (int i = numNodes-1; i >= 0; i--) {
      if (i > wholeNodesToDraw-1) {
        //move nodes up
        tankTemps_C[i] = tankTemps_C[i - wholeNodesToDraw];
      }
      else {
        //fill in bottom nodes with inlet water
        tankTemps_C[i] = inletT_C;  
      }
    }
  }
  //move fractional node
  if (drawFraction > 0) {
    //add temperature for outletT average
    outletTemp_C += drawFraction*tankTemps_C[numNodes - 1];
    //move partial nodes up
    for (int i = numNodes-1; i > 0; i--) {
      tankTemps_C[i] = tankTemps_C[i] *(1.0 - drawFraction) + tankTemps_C[i-1] * drawFraction;
    }
    //fill in bottom partial node with inletT
    tankTemps_C[0] = tankTemps_C[0] * (1.0 - drawFraction) + inletT_C*drawFraction;
  }

  //fill in average outlet T
  outletTemp_C /= (wholeNodesToDraw + drawFraction);





  //Account for mixing at the bottom of the tank
  if (tankMixesOnDraw == true && drawVolume_L > 0) {
    int mixedBelowNode = numNodes / 3;
    double ave = 0;
    
    for (int i = 0; i < mixedBelowNode; i++) {
      ave += tankTemps_C[i];
    }
    ave /= mixedBelowNode;
    
    for (int i = 0; i < mixedBelowNode; i++) {
      tankTemps_C[i] += ((ave - tankTemps_C[i]) / 3.0);
    }
  }

  //calculate standby losses
  //get average tank temperature
  double avgTemp = 0;
  for (int i = 0; i < numNodes; i++) avgTemp += tankTemps_C[i];
  avgTemp /= numNodes;

  //kJ's lost as standby in the current time step
  double standbyLosses_kJ = (tankUA_kJperHrC * (avgTemp - tankAmbientT_C) * (minutesPerStep / 60.0));  
  standbyLosses_kWh = standbyLosses_kJ / 3600.0;

  //The effect of standby loss on temperature in each segment
  double lossPerNode_C = (standbyLosses_kJ / numNodes)    /    ((volPerNode_LperNode * DENSITYWATER_kgperL) * CPWATER_kJperkgC);
  for (int i = 0; i < numNodes; i++) tankTemps_C[i] -= lossPerNode_C;
}  //end updateTankTemps


void HPWH::turnAllHeatSourcesOff() {
  for (int i = 0; i < numHeatSources; i++) {
    setOfSources[i].disengageHeatSource();
  }
  isHeating = false;
}


bool HPWH::areAllHeatSourcesOff() const {
  bool allOff = true;
  for (int i = 0; i < numHeatSources; i++) {
    if (setOfSources[i].isEngaged() == true) {
      allOff = false;
    }
  }
  return allOff;
}


double HPWH::topThirdAvg_C() const {
  double sum = 0;
  int num = 0;
  
  for (int i = 2*(numNodes/3); i < numNodes; i++) {
    sum += tankTemps_C[i];
    num++;
  }
  
  return sum/num;
}


double HPWH::bottomThirdAvg_C() const {
  double sum = 0;
  int num = 0;
  
  for (int i = 0; i < numNodes/3; i++) {
    sum += tankTemps_C[i];
    num++;
  }
  
  return sum/num;
}


double HPWH::bottomTwelthAvg_C() const {
  double sum = 0;
  int num = 0;
  
  for (int i = 0; i < numNodes/12; i++) {
    sum += tankTemps_C[i];
    num++;
  }
  
  return sum/num;
}


void HPWH::setNthHeatSourceEnergyInput(int N, double value){
  setOfSources[N].energyInput_kWh = value;
}

void HPWH::setNthHeatSourceEnergyOutput(int N, double value){
  setOfSources[N].energyOutput_kWh = value;
}

void HPWH::setNthHeatSourceRunTime(int N, double value){
  setOfSources[N].runtime_min = value;
}







//these are the HeatSource functions
//the public functions
HPWH::HeatSource::HeatSource(HPWH *parentInput)
  :hpwh(parentInput), isOn(false), backupHeatSource(NULL), companionHeatSource(NULL) {}


void HPWH::HeatSource::setCondensity(double cnd1, double cnd2, double cnd3, double cnd4, 
                  double cnd5, double cnd6, double cnd7, double cnd8, 
                  double cnd9, double cnd10, double cnd11, double cnd12) {
  condensity[0] = cnd1;  
  condensity[1] = cnd2;  
  condensity[2] = cnd3;  
  condensity[3] = cnd4;  
  condensity[4] = cnd5;  
  condensity[5] = cnd6;  
  condensity[6] = cnd7;  
  condensity[7] = cnd8;  
  condensity[8] = cnd9;  
  condensity[9] = cnd10;  
  condensity[10] = cnd11;  
  condensity[11] = cnd12;  
}
                  

bool HPWH::HeatSource::isEngaged() const {
  return isOn;
}


void HPWH::HeatSource::engageHeatSource(double heatSourceAmbientT_C) {
  isOn = true;
  hpwh->isHeating = true;
  if (companionHeatSource != NULL &&
        companionHeatSource->shutsOff(heatSourceAmbientT_C) != true &&
        companionHeatSource->isEngaged() == false) {
    companionHeatSource->engageHeatSource(heatSourceAmbientT_C);
  }
  
}              

                  
void HPWH::HeatSource::disengageHeatSource() {
  isOn = false;
}              

                  
bool HPWH::HeatSource::shouldHeat(double heatSourceAmbientT_C) const {
  bool shouldEngage = false;
  int selection = 0;


  for (int i = 0; i < (int)turnOnLogicSet.size(); i++) {

    if (turnOnLogicSet[i].selector == "topThird") {
      selection = 1;
    }
    else if (turnOnLogicSet[i].selector == "bottomThird") {
      selection = 2;
    }
    else if (turnOnLogicSet[i].selector == "standby") {
      selection = 3;
    }

    //cout << "selection: " << selection << endl;

    switch (selection) {
      case 1:
        //when the top third is too cold - typically used for upper resistance/VIP heat sources
        if (hpwh->topThirdAvg_C() < hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint_C) {
          //cout << "engage 1\n";
          shouldEngage = true;
        }
        break;
      
      case 2:
        //when the bottom third is too cold - typically used for compressors
        if (hpwh->bottomThirdAvg_C() < hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint_C) {
          //cout << "engage 2\n";
          shouldEngage = true;
        }    
        break;
        
      case 3:
        //when the top node is too cold - typically used for standby heating
        if (hpwh->tankTemps_C[hpwh->numNodes - 1] < hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint_C) {
          //cout << "engage 3\n";
          //cout << "tanktoptemp:  setpoint:  decisionPoint:  " << hpwh->tankTemps_C[hpwh->numNodes - 1] << " " << hpwh->setpoint_C << " " << turnOnLogicSet[i].decisionPoint_C << endl;
          shouldEngage = true;
        }
        break;
        
      default:
        cout << "You have input an incorrect turnOn logic choice specifier: "
             << selection << "  Exiting now" << endl;
        exit(1);
        break;
    }
  }  //end loop over set of logic conditions

  //if everything else wants it to come on, if it would shut off anyways don't turn it on
  if (shouldEngage == true && shutsOff(heatSourceAmbientT_C) == true ) {
    shouldEngage = false;
  }
  

  return shouldEngage;
}


bool HPWH::HeatSource::shutsOff(double heatSourceAmbientT_C) const {
  bool shutOff = false;
  int selection = 0;

  for (int i = 0; i < (int)shutOffLogicSet.size(); i++) {
    if (shutOffLogicSet[i].selector == "lowT") {
      selection = 1;
    }
    else if (shutOffLogicSet[i].selector == "lowTreheat") {
      selection = 2;
    }
    else if (shutOffLogicSet[i].selector == "bottomNodeMaxTemp") {
      selection = 3;
    }
    else if (shutOffLogicSet[i].selector == "bottomTwelthMaxTemp") {
      selection = 4;
    }


    switch (selection) {
      case 1:
        //when the "external" temperature is too cold - typically used for compressor low temp. cutoffs
        if (heatSourceAmbientT_C < shutOffLogicSet[i].decisionPoint_C) {
          cout << "shut down lowT" << endl << endl;
          shutOff = true;
        }
        break;

      case 2:
        //don't run is the temperature is too warm
        if (heatSourceAmbientT_C > shutOffLogicSet[i].decisionPoint_C) {
          cout << "shut down lowTreheat" << endl << endl;
          shutOff = true;
        }
        break;
      
      case 3:
        //don't run if the bottom node is too hot - typically for "external" configuration
        if (hpwh->tankTemps_C[0] > shutOffLogicSet[i].decisionPoint_C) {
          cout << "shut down bottom node temp" << endl << endl;
          shutOff = true;
        }
        break;

      case 4:
        //don't run if the bottom twelth of the tank is too hot
        //typically for "external" configuration
        if (hpwh->bottomTwelthAvg_C() > shutOffLogicSet[i].decisionPoint_C) {
          cout << "shut down bottom twelth temp" << endl << endl;
          shutOff = true;
        }
        break;
      
      default:
        cout << "You have input an incorrect shutOff logic choice specifier, exiting now" << endl;
        exit(1);
        break;
    }
  }

  return shutOff;
}


void HPWH::HeatSource::addHeat_temp(double heatSourceAmbientT_C, double minutesPerStep) {
  //cout << "heat source 0: " << hpwh->setOfSources[0].isEngaged() <<  "\theat source 1: " << hpwh->setOfSources[1].isEngaged() << endl;
  //a temporary function, for testing
  int lowerBound = 0;
  for (int i = 0; i < hpwh->numNodes; i++) {
    if (condensity[i] == 1) {
      lowerBound = i;
      }
  }


  for (int i = lowerBound; i < hpwh->numNodes; i++) {
    if (hpwh->tankTemps_C[i] < hpwh->setpoint_C) {
      if (hpwh->tankTemps_C[i] + 2.0 > hpwh->setpoint_C) {
        hpwh->tankTemps_C[i] = hpwh->setpoint_C;
        if (i == 0) {
          runtime_min = 0.5*minutesPerStep;
        }
        
      
      }
      else {
        hpwh->tankTemps_C[i] += 2.0;
        runtime_min = minutesPerStep;
      }
    }
  }
}  //end addheat_temp function


void HPWH::HeatSource::addHeat(double externalT_C, double minutesToRun) {
  double input_BTUperHr, cap_BTUperHr, cop, captmp_kJ, leftoverCap_kJ;
  static std::vector<double> heatDistribution(hpwh->numNodes);

  //clear the heatDistribution vector, since it's static it is still holding the
  //distribution from the last go around
  heatDistribution.clear();
  
  // Reset the runtime of the Heat Source
  this->runtime_min = 0.0;

  // calculate capacity btu/hr, input btu/hr, and cop
  getCapacity(externalT_C, input_BTUperHr, cap_BTUperHr, cop);
  leftoverCap_kJ = 0.0;

  cout << "capacity_kJ " << BTU_TO_KJ(cap_BTUperHr)*(minutesToRun)/60.0 << endl;

  
  if((configuration == "submerged") || (configuration == "wrapped")) {
    calcHeatDist(heatDistribution);
    //calcHeatDist takes care of the swooping for wrapped configurations
    cout << "heatDistribution: " << heatDistribution[0] << " "<< heatDistribution[1] << " "<< heatDistribution[2] << " "<< heatDistribution[3] << " "<< heatDistribution[4] << " "<< heatDistribution[5] << " "<< heatDistribution[6] << " "<< heatDistribution[7] << " "<< heatDistribution[8] << " "<< heatDistribution[9] << " "<< heatDistribution[10] << " "<< heatDistribution[11] << endl;

    //the loop over nodes here is intentional - essentially each node that has
    //some amount of heatDistribution acts as a separate resistive element
//maybe start from the top and go down?  test this with graphs
    for(int i = hpwh->numNodes -1; i >= 0; i--){
    //for(int i = 0; i < hpwh->numNodes; i++){
      captmp_kJ = BTU_TO_KJ(cap_BTUperHr * minutesToRun / 60.0 * heatDistribution[i]);
      if(captmp_kJ != 0){
        //add leftoverCap to the next run, and keep passing it on
        leftoverCap_kJ = addHeatAboveNode(captmp_kJ + leftoverCap_kJ, i, minutesToRun);
      }
    }
   
  }
  else if(configuration == "external"){
    // Else the heat source is external. Sanden thingy
    leftoverCap_kJ = addHeatExternal(externalT_C, cap_BTUperHr, minutesToRun);
  }
  else{
    cout << "Invalid heat source configuration chosen: " << configuration << endl;
    cout << "Ending program!" << endl;
    exit(1);
  }

  //after you've done everything, any leftover capacity is time that didn't run
  this->runtime_min = (1.0 - (leftoverCap_kJ / BTU_TO_KJ(cap_BTUperHr * minutesToRun / 60.0))) * minutesToRun;
 
  // Write the input & output energy
  energyInput_kWh = BTU_TO_KWH(input_BTUperHr * runtime_min / 60.0);
  energyOutput_kWh = BTU_TO_KWH(cap_BTUperHr * runtime_min / 60.0);
}



//the private functions
double HPWH::HeatSource::expitFunc(double x, double offset) {
  double val;
  val = 1 / (1 + exp(x - offset));
  return val;
}


void HPWH::HeatSource::normalize(std::vector<double> &distribution, int n) {
  double sum_tmp = 0.0;

  for(int i = 0; i < n; i++) {
    sum_tmp += distribution[i];
  }

  for(int i = 0; i < n; i++) {
    distribution[i] /= sum_tmp;
  }
}


int HPWH::HeatSource::lowestNode() {
  int lowest = 0;
  for(int i = 0; i < hpwh->numNodes; i++) {
    if(condensity[i] > 0) {
      lowest = i;
      break;
    }
  }
  return lowest;
}


double HPWH::HeatSource::getCondenserTemp() {
  double condenserTemp_C = 0.0;
  int i;

  for(i = 0; i < hpwh->numNodes; i++) {
    condenserTemp_C += condensity[i] * hpwh->tankTemps_C[i];
    //cout << "condenserTemp_C i condensity tankTemps_C " << condenserTemp_C << " " << i << " " << condensity[i] << " " << hpwh->tankTemps_C[i] << endl;
  }
  
  return condenserTemp_C;
}


void HPWH::HeatSource::getCapacity(double externalT_C, double &input_BTUperHr, double &cap_BTUperHr, double &cop) {
  double COP_T1, COP_T2;    			   //cop at ambient temperatures T1 and T2
  double inputPower_T1_Watts, inputPower_T2_Watts; //input power at ambient temperatures T1 and T2	
  double condenserTemp_C, externalT_F, condenserTemp_F;

  // Calculate the current water temp at the "condenser"
  condenserTemp_C = getCondenserTemp();

  //cout << "condenserTemp_C " << condenserTemp_C << endl;
  // Convert Celsius to Fahrenheit for the curve fits
  condenserTemp_F = C_TO_F(condenserTemp_C);
  externalT_F = C_TO_F(externalT_C);

  // Calculate COP and Input Power at each of the two reference temepratures
  COP_T1 = COP_T1_constant;
  COP_T1 += COP_T1_linear * condenserTemp_F ;
  COP_T1 += COP_T1_quadratic * condenserTemp_F * condenserTemp_F;

  COP_T2 = COP_T2_constant;
  COP_T2 += COP_T2_linear * condenserTemp_F;
  COP_T2 += COP_T2_quadratic * condenserTemp_F * condenserTemp_F;

  inputPower_T1_Watts = inputPower_T1_constant_W;
  inputPower_T1_Watts += inputPower_T1_linear_WperF * condenserTemp_F;
  inputPower_T1_Watts += inputPower_T1_quadratic_WperF2 * condenserTemp_F * condenserTemp_F;

//cout << "inputPower_T1_constant_W inputPower_T1_linear_WperF inputPower_T1_quadratic_WperF2 " << inputPower_T1_constant_W << " " << inputPower_T1_linear_WperF << " " << inputPower_T1_quadratic_WperF2 << endl;


  inputPower_T2_Watts = inputPower_T2_constant_W;
  inputPower_T2_Watts += inputPower_T2_linear_WperF * condenserTemp_F;
  inputPower_T2_Watts += inputPower_T2_quadratic_WperF2 * condenserTemp_F * condenserTemp_F;

//cout << "inputPower_T2_constant_W inputPower_T2_linear_WperF inputPower_T2_quadratic_WperF2 " << inputPower_T2_constant_W << " " << inputPower_T2_linear_WperF << " " << inputPower_T2_quadratic_WperF2 << endl;

//cout << "inputPower_T1_Watts inputPower_T2_Watts " << inputPower_T1_Watts << " " << inputPower_T2_Watts << endl;

  // Interpolate to get COP and input power at the current ambient temperature
  cop = COP_T1 + (externalT_F - T1_F) * ((COP_T2 - COP_T1) / (T2_F - T1_F));
  input_BTUperHr = KWH_TO_BTU((inputPower_T1_Watts + (externalT_F - T1_F) * ((inputPower_T2_Watts - inputPower_T1_Watts) / (T2_F - T1_F))) / 1000.0);  //1000 converts w to kw
  cap_BTUperHr = cop * input_BTUperHr;

  //cout << "cop input_BTUperHr cap_BTUperHr " << cop << " " << input_BTUperHr << " " << cap_BTUperHr << endl;

/*
  //here is where the scaling for flow restriction goes
  //the input power doesn't change, we just scale the cop by a small percentage that is based on the ducted flow rate
  //the equation is a fit to three points, measured experimentally - 12 percent reduction at 150 cfm, 10 percent at 200, and 0 at 375
  //it's slightly adjust to be equal to 1 at 375
  if(hpwh->ductingType != 0){
    cop_interpolated *= 0.00056*hpwh->fanFlow + 0.79;
  }
*/
}


void HPWH::HeatSource::calcHeatDist(std::vector<double> &heatDistribution) {
  double condentropy, s; // Should probably have shrinkage (by way of condensity) be a property of the HeatSource class
  double alpha = 1;
  double beta = 2; // Mapping from condentropy to shrinkage
  double offset = 5;
  int k;

  // Calculate condentropy and ==> shrinkage. Again this could/should be a property of the HeatSource.
  condentropy = 0;
  for(int i = 0; i < hpwh->numNodes; i++) {
    if(condensity[i] > 0) {
      condentropy -= condensity[i] * log(condensity[i]);
    }
  }
  s = alpha + condentropy * beta;

  // Populate the vector of heat distribution
  for(int i = 0; i < hpwh->numNodes; i++) {
    if(i < lowestNode()) {
      heatDistribution[i] = 0;
    }
    else {
      if(configuration == "submerged") { // Inside the tank, no swoopiness required
        k = floor(i / 12.0 * hpwh->numNodes);
        heatDistribution[i] = condensity[k];
      }
      else if(configuration == "wrapped") { // Wrapped around the tank, send through the logistic function
        heatDistribution[i] = expitFunc( (hpwh->tankTemps_C[i] - hpwh->tankTemps_C[lowestNode()]) / s , offset);
        heatDistribution[i] *= (hpwh->setpoint_C - hpwh->tankTemps_C[i]);
      }
    }
  }
  normalize(heatDistribution, hpwh->numNodes);

}


double HPWH::HeatSource::addHeatAboveNode(double cap_kJ, int node, double minutesToRun) {
  double Q_kJ, deltaT_C, targetTemp_C;
  int setPointNodeNum;

  double volumePerNode_L = hpwh->tankVolume_L / hpwh->numNodes;
  
  //cout << "node cap_kwh " << node << " " << KJ_TO_KWH(cap_kJ) << endl;
  // find the first node (from the bottom) that does not have the same temperature as the one above it
  // if they all have the same temp., use the top node, hpwh->numNodes-1
  setPointNodeNum = node;
  for(int i = node; i < hpwh->numNodes-1; i++){
    if(hpwh->tankTemps_C[i] != hpwh->tankTemps_C[i+1]) {
      break;
    }
    else{
      setPointNodeNum = i+1;
    }
  }

  // maximum heat deliverable in this timestep
  while(cap_kJ > 0 && setPointNodeNum < hpwh->numNodes) {
    // if the whole tank is at the same temp, the target temp is the setpoint
    if(setPointNodeNum == (hpwh->numNodes-1)) {
      targetTemp_C = hpwh->setpoint_C;
    }
    //otherwise the target temp is the first non-equal-temp node
    else {
      targetTemp_C = hpwh->tankTemps_C[setPointNodeNum+1];
    }

    deltaT_C = targetTemp_C - hpwh->tankTemps_C[setPointNodeNum];
    
    //heat needed to bring all equal temp. nodes up to the temp of the next node. kJ
    Q_kJ = CPWATER_kJperkgC * volumePerNode_L * DENSITYWATER_kgperL * (setPointNodeNum+1 - node) * deltaT_C;

    //Running the rest of the time won't recover
    if(Q_kJ > cap_kJ){
      for(int j = node; j <= setPointNodeNum; j++) {
        hpwh->tankTemps_C[j] += cap_kJ / CPWATER_kJperkgC / volumePerNode_L / DENSITYWATER_kgperL / (setPointNodeNum + 1 - node);
      }
      cap_kJ = 0;
    }
    //temp will recover by/before end of timestep
    else{
      for(int j = node; j <= setPointNodeNum; j++){
        hpwh->tankTemps_C[j] = targetTemp_C;
      }
      setPointNodeNum++;
      cap_kJ -= Q_kJ;
    }
  }

  //return the unused capacity
  return cap_kJ;
}


double HPWH::HeatSource::addHeatExternal(double externalT_C, double cap_BTUperHr, double minutesToRun) {
  double nodeHeat_kJperNode, heatingCapacity_kJ, remainingCapacity_kJ, deltaT_C, nodeFrac;
  double heatingCutoff_C = 20.0; // When to turn off the compressor. Should be a property of the HeatSource?

  double volumePerNode_LperNode = hpwh->tankVolume_L / hpwh->numNodes;

  //how much heat is available this timestep
  heatingCapacity_kJ = BTU_TO_KJ(cap_BTUperHr * (minutesToRun / 60.0));
  remainingCapacity_kJ = heatingCapacity_kJ;
	
  //if there's still remaining capacity and you haven't heated to "setpoint", keep heating
  while(remainingCapacity_kJ > 0 && shutsOff(externalT_C) != true) {
    cout << "condenserTemp: " << getCondenserTemp();
    cout << "\tsetpoint - heatingCutff: " << hpwh->setpoint_C - heatingCutoff_C << endl;
    cout << "bottom tank temp: " << getCondenserTemp();
    cout << "\tremainingCapacity_kJ: " << remainingCapacity_kJ << endl;
    //calculate what percentage of the bottom node can be heated to setpoint
    //with amount of heat available this timestep
    deltaT_C = hpwh->setpoint_C - getCondenserTemp();
    nodeHeat_kJperNode = volumePerNode_LperNode * DENSITYWATER_kgperL * CPWATER_kJperkgC * deltaT_C;
    nodeFrac = remainingCapacity_kJ / nodeHeat_kJperNode;
    cout << "nodeHeat_kJperNode: " << nodeHeat_kJperNode << " nodeFrac: " << nodeFrac << endl << endl;
  //if more than one, round down to 1 and subtract that amount of heat energy
  // from the capacity
    if(nodeFrac > 1){
      nodeFrac = 1;
      remainingCapacity_kJ -= nodeHeat_kJperNode;
    }
    //substract the amount of heat used - a full node's heat if nodeFrac == 1,
    //otherwise just the fraction available 
    //this should make heatingCapacity == 0  if nodeFrac < 1
    else{
      remainingCapacity_kJ = 0;
    }
	
    //move all nodes down, mixing if less than a full node
    for(int n = 0; n < hpwh->numNodes - 1; n++) {
      hpwh->tankTemps_C[n] = hpwh->tankTemps_C[n] * (1 - nodeFrac) + hpwh->tankTemps_C[n + 1] * nodeFrac;
    }
    //add water to top node, heated to setpoint
    hpwh->tankTemps_C[hpwh->numNodes - 1] = hpwh->tankTemps_C[hpwh->numNodes - 1] * (1 - nodeFrac) + hpwh->setpoint_C * nodeFrac;
		
  }
		
  //This is runtime - is equal to timestep if compressor ran the whole time
  cout << "final remaining capacity: " << remainingCapacity_kJ << endl;
  return remainingCapacity_kJ;
}


void HPWH::HeatSource::setupAsResistiveElement(int node, double Watts) {
    int i;

    isOn = false;
    isVIP = false;
    for(i = 0; i < 12; i++) {
      condensity[i] = 0;
    }
    condensity[node] = 1;
    T1_F = 50;
    T2_F = 67;
    inputPower_T1_constant_W = Watts;
    inputPower_T1_linear_WperF = 0;
    inputPower_T1_quadratic_WperF2 = 0;
    inputPower_T2_constant_W = Watts;
    inputPower_T2_linear_WperF = 0;
    inputPower_T2_quadratic_WperF2 = 0;
    COP_T1_constant = 1;
    COP_T1_linear = 0;
    COP_T1_quadratic = 0;
    COP_T2_constant = 1;
    COP_T2_linear = 0;
    COP_T2_quadratic = 0;
    hysteresis = 0;  //no hysteresis
    configuration = "submerged"; //immersed in tank
    
    //standard logic conditions
    if(node < 3) {
      turnOnLogicSet.push_back(HeatSource::heatingLogicPair("bottomThird", 20));
      turnOnLogicSet.push_back(HeatSource::heatingLogicPair("standby", 15));
    } else {
      turnOnLogicSet.push_back(HeatSource::heatingLogicPair("topThird", 20));
      isVIP = true;
    }

    ////lowT cutoff
    //shutOffLogicSet.push_back(HeatSource::heatingLogicPair("lowT", 0));
    depressesTemperature = false;  //no temp depression
}


