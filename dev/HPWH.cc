#include "HPWH.hh"


using std::cout;
using std::endl;
using std::string;

HPWH::HPWH() :
  isHeating(false) {
  }

HPWH::~HPWH() {
  delete[] tankTemps_C;
  delete[] setOfSources;  
}


int HPWH::HPWHinit_presets(int presetNum) {
  if (presetNum == 1) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = 50;

    //start tank off at setpoint
    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
    tankVolume_L = 120; 
    //tankUA_kJperHrC = 500; //0 to turn off
    tankUA_kJperHrC = 0; //0 to turn off
    
    doTempDepression = false;
    tankMixing = false;

    numHeatSources = 2;
    setOfSources = new HeatSource[numHeatSources];

    //set up a resistive element at the bottom, 4500 kW
    HeatSource resistiveElementBottom(this);
    
    resistiveElementBottom.isOn = false;
    resistiveElementBottom.isVIP = false;

    resistiveElementBottom.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        
    resistiveElementBottom.T1_F = 50;
    resistiveElementBottom.T2_F = 67;
    
    resistiveElementBottom.inputPower_T1_constant = 4500;
    resistiveElementBottom.inputPower_T1_linear = 0;
    resistiveElementBottom.inputPower_T1_quadratic = 0;
    
    resistiveElementBottom.inputPower_T2_constant = 4500;
    resistiveElementBottom.inputPower_T2_linear = 0;
    resistiveElementBottom.inputPower_T2_quadratic = 0;
    
    resistiveElementBottom.COP_T1_constant = 1;
    resistiveElementBottom.COP_T1_linear = 0;
    resistiveElementBottom.COP_T1_quadratic = 0;
    
    resistiveElementBottom.COP_T2_constant = 1;
    resistiveElementBottom.COP_T2_linear = 0;
    resistiveElementBottom.COP_T2_quadratic = 0;
    
    resistiveElementBottom.hysteresis = 0;  //no hysteresis

    //standard logic conditions
    resistiveElementBottom.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("bottomThird", 20));
    resistiveElementBottom.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("standby", 15));

    
    //lowT cutoff
    resistiveElementBottom.shutOffLogicSet.push_back(HeatSource::heatingLogicPair("lowT", 0));
    
        
    resistiveElementBottom.depressesTemperature = false;  //no temp depression



    //set up a resistive element at the top, 4500 kW
    HeatSource resistiveElementTop(this);
    
    resistiveElementTop.isOn = false;
    resistiveElementTop.isVIP = true;

    resistiveElementTop.setCondensity(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
        
    resistiveElementTop.T1_F = 50;
    resistiveElementTop.T2_F = 67;
    
    resistiveElementTop.inputPower_T1_constant = 4500;
    resistiveElementTop.inputPower_T1_linear = 0;
    resistiveElementTop.inputPower_T1_quadratic = 0;
    
    resistiveElementTop.inputPower_T2_constant = 4500;
    resistiveElementTop.inputPower_T2_linear = 0;
    resistiveElementTop.inputPower_T2_quadratic = 0;
    
    resistiveElementTop.COP_T1_constant = 1;
    resistiveElementTop.COP_T1_linear = 0;
    resistiveElementTop.COP_T1_quadratic = 0;
    
    resistiveElementTop.COP_T2_constant = 1;
    resistiveElementTop.COP_T2_linear = 0;
    resistiveElementTop.COP_T2_quadratic = 0;
    
    resistiveElementTop.hysteresis = 0;  //no hysteresis

    resistiveElementTop.depressesTemperature = false;  //no temp depression

    //standard logic conditions
    resistiveElementTop.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("topThird", 20));
    
    resistiveElementTop.configuration = 1;
    resistiveElementBottom.configuration = 1;
    
    //assign heat sources into array in order of priority
    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = resistiveElementBottom;
  }
  
  else if (presetNum == 2) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = 50;

    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
    tankVolume_L = 120; 
    tankUA_kJperHrC = 500; //0 to turn off
    //tankUA_kJperHrC = 0; //0 to turn off
    
    doTempDepression = false;
    tankMixing = false;


    numHeatSources = 2;
    setOfSources = new HeatSource[numHeatSources];

    //set up a resistive element at the bottom, 4500 kW
    HeatSource resistiveElementBottom(this);
    
    resistiveElementBottom.isOn = false;
    resistiveElementBottom.isVIP = false;

    resistiveElementBottom.setCondensity(1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);
        
    resistiveElementBottom.T1_F = 50;
    resistiveElementBottom.T2_F = 67;
    
    resistiveElementBottom.inputPower_T1_constant = 4500;
    resistiveElementBottom.inputPower_T1_linear = 0;
    resistiveElementBottom.inputPower_T1_quadratic = 0;
    
    resistiveElementBottom.inputPower_T2_constant = 4500;
    resistiveElementBottom.inputPower_T2_linear = 0;
    resistiveElementBottom.inputPower_T2_quadratic = 0;
    
    resistiveElementBottom.COP_T1_constant = 1;
    resistiveElementBottom.COP_T1_linear = 0;
    resistiveElementBottom.COP_T1_quadratic = 0;
    
    resistiveElementBottom.COP_T2_constant = 1;
    resistiveElementBottom.COP_T2_linear = 0;
    resistiveElementBottom.COP_T2_quadratic = 0;
    
    resistiveElementBottom.hysteresis = 0;  //no hysteresis

    //standard logic conditions
    resistiveElementBottom.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("bottomThird", 20));
    resistiveElementBottom.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("standby", 15));

    //lowT cutoff
    resistiveElementBottom.shutOffLogicSet.push_back(HeatSource::heatingLogicPair("lowT", 0));
  
    
    resistiveElementBottom.depressesTemperature = false;  //no temp depression



    //set up a resistive element at the top, 4500 kW
    HeatSource resistiveElementTop(this);
    
    resistiveElementTop.isOn = false;
    resistiveElementTop.isVIP = true;

    resistiveElementTop.setCondensity(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
        
    resistiveElementTop.T1_F = 50;
    resistiveElementTop.T2_F = 67;
    
    resistiveElementTop.inputPower_T1_constant = 4500;
    resistiveElementTop.inputPower_T1_linear = 0;
    resistiveElementTop.inputPower_T1_quadratic = 0;
    
    resistiveElementTop.inputPower_T2_constant = 4500;
    resistiveElementTop.inputPower_T2_linear = 0;
    resistiveElementTop.inputPower_T2_quadratic = 0;
    
    resistiveElementTop.COP_T1_constant = 1;
    resistiveElementTop.COP_T1_linear = 0;
    resistiveElementTop.COP_T1_quadratic = 0;
    
    resistiveElementTop.COP_T2_constant = 1;
    resistiveElementTop.COP_T2_linear = 0;
    resistiveElementTop.COP_T2_quadratic = 0;
    
    resistiveElementTop.hysteresis = 0;  //no hysteresis

    resistiveElementTop.depressesTemperature = false;  //no temp depression

    //standard logic conditions
    resistiveElementTop.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("topThird", 20));
    
    resistiveElementTop.configuration = 1;
    resistiveElementBottom.configuration = 1;
    
    //assign heat sources into array in order of priority
    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = resistiveElementBottom;
  } else if(presetNum == 3) {
    numNodes = 12;
    tankTemps_C = new double[numNodes];
    setpoint_C = 50;

    for (int i = 0; i < numNodes; i++) {
      tankTemps_C[i] = setpoint_C;
    }
    
    tankVolume_L = 120; 
    tankUA_kJperHrC = 500; //0 to turn off
    //tankUA_kJperHrC = 0; //0 to turn off
    
    doTempDepression = false;
    tankMixing = false;

    numHeatSources = 2;
    setOfSources = new HeatSource[numHeatSources];

    HeatSource resistiveElementBottom(this);
    HeatSource resistiveElementTop(this);
    resistiveElementBottom.setupAsResistiveElement(0, 4500);
    resistiveElementTop.setupAsResistiveElement(9, 4500);

    setOfSources[0] = resistiveElementTop;
    setOfSources[1] = resistiveElementBottom;
  }

  return 0;
}  //end HPWHinit_presets


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
    inputPower_T1_constant = Watts;
    inputPower_T1_linear = 0;
    inputPower_T1_quadratic = 0;
    inputPower_T2_constant = Watts;
    inputPower_T2_linear = 0;
    inputPower_T2_quadratic = 0;
    COP_T1_constant = 1;
    COP_T1_linear = 0;
    COP_T1_quadratic = 0;
    COP_T2_constant = 1;
    COP_T2_linear = 0;
    COP_T2_quadratic = 0;
    hysteresis = 0;  //no hysteresis

    //standard logic conditions
    if(node < 3) {
      turnOnLogicSet.push_back(HeatSource::heatingLogicPair("bottomThird", 20));
      turnOnLogicSet.push_back(HeatSource::heatingLogicPair("standby", 15));
    } else {
      turnOnLogicSet.push_back(HeatSource::heatingLogicPair("topThird", 20));
      isVIP = true;
    }

    //lowT cutoff
    shutOffLogicSet.push_back(HeatSource::heatingLogicPair("lowT", 0));
    depressesTemperature = false;  //no temp depression
}


void HPWH::printTankTemps() const {
  cout << std::setw(7) << std::left << tankTemps_C[0] << ", " << std::setw(7) << tankTemps_C[1] << ", " << std::setw(7) << tankTemps_C[2] << ", " << std::setw(7) << tankTemps_C[3] << ", " << std::setw(7) << tankTemps_C[4] << ", " << std::setw(7) << tankTemps_C[5] << ", " << std::setw(7) << tankTemps_C[6] << ", " << std::setw(7) << tankTemps_C[7] << ", " << std::setw(7) << tankTemps_C[8] << ", " << std::setw(7) << tankTemps_C[9] << ", " << std::setw(7) << tankTemps_C[10] << ", " << std::setw(7) << tankTemps_C[11] << " ";
  cout << "heat source 0: " << setOfSources[0].isEngaged() <<  "\theat source 1: " << setOfSources[1].isEngaged() << endl;
}



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
    if (setOfSources[i].isVIP && setOfSources[i].shouldHeat()) {
      turnAllHeatSourcesOff();
      setOfSources[i].engageHeatSource();
      //stop looking when you've found such a HeatSource
      break;
    }
    //is nothing is currently on, then check if something should come on
    else if (isHeating == false) {
    //cout << "check all-off should heat source[i]: " << i << endl;

      if (setOfSources[i].shouldHeat()) {
        setOfSources[i].engageHeatSource();
        //engaging a heat source sets isHeating to true, so this will only trigger once
      }
    }
    //check if anything that is on needs to turn off (generally for lowT cutoffs)
    else {
      if (setOfSources[i].isEngaged() && setOfSources[i].shutsOff(heatSourceAmbientT_C)) {
        setOfSources[i].disengageHeatSource();
        //check if the backup heat source would have to shut off too
        if (setOfSources[i].backupHeatSource != NULL && setOfSources[i].backupHeatSource->shutsOff(heatSourceAmbientT_C) != true) {
          //and if not, go ahead and turn it on 
          setOfSources[i].backupHeatSource->engageHeatSource();
        }
      }
    }

  }  //end loop over heat sources

  cout << "after heat source choosing:  heatsource 0: " << setOfSources[0].isEngaged() << " heatsource 1: " << setOfSources[1].isEngaged() << endl;


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
    if (areAllHeatSourcesOff() == true) {
      setOfSources[0].engageHeatSource();
    }
  }





  //do heating logic
  for (int i = 0; i < numHeatSources; i++) {
    //going through in order, check if the heat source is on
    if (setOfSources[i].isEngaged()) {
      //add heat
      // setOfSources[i].addHeat_temp(heatSourceAmbientT_C, minutesPerStep);
      setOfSources[i].addHeat(heatSourceAmbientT_C, minutesPerStep);
      cout << "input output " << setOfSources[i].energyInput_kWh << " " << setOfSources[i].energyOutput_kWh << endl;
      //if it finished early
      if (setOfSources[i].runtime_min < minutesPerStep) {
        //turn it off
        setOfSources[i].disengageHeatSource();
        //and if there's another heat source in the list, that's able to come on,
        if (numHeatSources > i+1 && setOfSources[i + 1].shutsOff(heatSourceAmbientT_C) == false) {
          //turn it on
          setOfSources[i + 1].engageHeatSource();
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


  //energyRemovedFromEnvironment_kWh;

    









  return 0;
} //end runOneStep


void HPWH::updateTankTemps(double drawVolume_L, double inletT_C, double tankAmbientT_C, double minutesPerStep) {
  //set up some useful variables for calculations
  double volPerNode_L = tankVolume_L/numNodes;
  double drawFraction;
  int wholeNodesToDraw;
  outletTemp_C = 0;

  //calculate how many nodes to draw (wholeNodesToDraw), and the remainder (drawFraction)
  drawFraction = drawVolume_L/volPerNode_L;
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
  if (tankMixing == true && drawVolume_L > 0) {
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
  double lossPerNode_C = (standbyLosses_kJ / numNodes)    /    ((volPerNode_L * DENSITYWATER_kgperL) * CPWATER_kJperkgC);
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


double HPWH::getOutletTemp(string units) const {
  double returnVal = 0;

  if (units == "C") {
    returnVal = outletTemp_C;
  }
  else if (units == "F") {
    returnVal = C_TO_F(outletTemp_C);
    }
  else {
    cout << "Incorrect unit specification for getOutletTemp" << endl;
    exit(1);
  }
  return returnVal;
}

double HPWH::getEnergyRemovedFromEnvironment(string units) const {
  double returnVal = 0;

  if (units == "kWh") {
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

  if (units == "kWh") {
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



//these are the HeatSource functions
//the public functions
HPWH::HeatSource::HeatSource(HPWH *parentInput)
  :hpwh(parentInput), isOn(false), backupHeatSource(NULL) {}


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


void HPWH::HeatSource::engageHeatSource() {
  isOn = true;
  hpwh->isHeating = true;
}              

                  
void HPWH::HeatSource::disengageHeatSource() {
  isOn = false;
}              

                  
bool HPWH::HeatSource::shouldHeat() const {
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
        cout << "You have input an incorrect turnOn logic choice specifier, exiting now" << endl;
        exit(1);
        break;
    }
  }  //end loop over set of logic conditions

  return shouldEngage;
}


bool HPWH::HeatSource::shutsOff(double heatSourceAmbientT_C) const {
  bool shutsOff = false;
  int selection = 0;

  for (int i = 0; i < (int)shutOffLogicSet.size(); i++) {
    if (shutOffLogicSet[i].selector == "lowT") {
      selection = 1;
    }


    switch (selection) {
      case 1:
        //when the "external" temperature is too cold - typically used for compressor low temp. cutoffs
        if (heatSourceAmbientT_C < shutOffLogicSet[i].decisionPoint_C) {
          cout << "shut down" << endl;
          shutsOff = true;
        }
        break;
      
      default:
        cout << "You have input an incorrect shutOff logic choice specifier, exiting now" << endl;
        exit(1);
        break;
    }
  }

  return shutsOff;
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


void HPWH::HeatSource::addHeat(double externalT_C, double minutesPerStep) {
  double input_BTUperHr, cap_BTUperHr, cop, captmp_kJ;
  double *heatDistribution;

  // Reset the runtime of the Heat Source
  runtime_min = 0.0;

  // Could also have heatDistribution be part of the HeatSource class, rather than creating and destroying a vector every time we add heat
  heatDistribution = new double [hpwh->numNodes];

  // calculate capacity btu/hr? Just btu??
  getCapacity(externalT_C, &input_BTUperHr, &cap_BTUperHr, &cop);

  if((configuration == 1) | (configuration == 3)) {
    // Either the heat source is within the tank or wrapped around it
    calcHeatDist(heatDistribution);
    for(int i = 0; i < hpwh->numNodes; i++){
      captmp_kJ = BTU_TO_KJ(cap_BTUperHr * minutesPerStep / 60.0 * heatDistribution[i]);
      runtime_min += addHeatAboveNode(captmp_kJ, i, minutesPerStep);
    }
  } else {
    // Else the heat source is external. Sanden thingy
    runtime_min = addHeatExternal(cap_BTUperHr, minutesPerStep);
  }

  // Write the input & output energy
  energyInput_kWh = BTU_TO_KWH(input_BTUperHr * runtime_min / 60.0);
  energyOutput_kWh = BTU_TO_KWH(cap_BTUperHr * runtime_min / 60.0);

  delete[] heatDistribution;
}



//the private functions
double HPWH::HeatSource::expitFunc(double x, double offset) {
  double val;
  val = 1 / (1 + exp(x - offset));
  return val;
}


void HPWH::HeatSource::normalize(double *Z, int n) {
  double sum_tmp = 0.0;
  int i;

  for(i = 0; i < n; i++) {
    sum_tmp += Z[i];
  }

  for(i = 0; i < n; i++) {
    Z[i] /= sum_tmp;
  }
}


int HPWH::HeatSource::lowestNode() {
  int i, lowest;
  for(i = 0; i < hpwh->numNodes; i++) {
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
  }
  
  return condenserTemp_C;
}


void HPWH::HeatSource::getCapacity(double externalT_C, double *input_BTUperHr, double *cap_BTUperHr, double *cop) {
  double COP_T1, COP_T2;    			   //cop at ambient temperatures T1 and T2
  double inputPower_T1_Watts, inputPower_T2_Watts; //input power at ambient temperatures T1 and T2	
  double condenserTemp_C, externalT_F, condenserTemp_F;

  // Calculate the current water temp at the "condenser"
  condenserTemp_C = getCondenserTemp();
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

  inputPower_T1_Watts = inputPower_T1_constant;
  inputPower_T1_Watts += inputPower_T1_linear * condenserTemp_F;
  inputPower_T1_Watts += inputPower_T1_quadratic * condenserTemp_F * condenserTemp_F;

  inputPower_T2_Watts = inputPower_T2_constant;
  inputPower_T2_Watts += inputPower_T2_constant * condenserTemp_F;
  inputPower_T2_Watts += inputPower_T2_constant * condenserTemp_F * condenserTemp_F;


  // Interpolate to get COP and input power at the current ambient temperature
  *cop = COP_T1 + (externalT_F - T1_F) * ((COP_T2 - COP_T1) / (T2_F - T1_F));
  *input_BTUperHr = (inputPower_T1_Watts + (externalT_F - T1_F) * ((inputPower_T2_Watts - inputPower_T1_Watts) / (T2_F - T1_F))) * 3412;
  *cap_BTUperHr = (*cop) * (*input_BTUperHr);

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


void HPWH::HeatSource::calcHeatDist(double *heatDistribution) {
  double condentropy, s; // Should probably have shrinkage (by way of condensity) be a property of the HeatSource class
  double alpha = 1;
  double beta = 2; // Mapping from condentropy to shrinkage
  double offset = 5;
  int i, k;

  // Calculate condentropy and ==> shrinkage. Again this could/should be a property of the HeatSource.
  condentropy = 0;
  for(i = 0; i < hpwh->numNodes; i++) {
    if(condensity[i] > 0) {
      condentropy -= condensity[i] * log(condensity[i]);
    }
  }
  s = alpha + condentropy * beta;

  // Populate the vector of heat distribution
  for(i = 0; i < hpwh->numNodes; i++) {
    if(i < lowestNode()) {
      heatDistribution[i] = 0;
    } else {
      if(configuration == 1) { // Inside the tank, no swoopiness required
        k = floor(i / 12.0 * hpwh->numNodes);
        heatDistribution[i] = condensity[k];
      } else if(configuration == 2) { // Wrapped around the tank, send through the logistic function
        heatDistribution[i] = expitFunc( (hpwh->tankTemps_C[i] - hpwh->tankTemps_C[lowestNode()]) / s , offset);
        heatDistribution[i] *= (hpwh->setpoint_C - hpwh->tankTemps_C[i]);
      }
    }
  }
  normalize(heatDistribution, hpwh->numNodes);

}


double HPWH::HeatSource::addHeatAboveNode(double cap_kJ, int node, double minutesPerStep) {
  double Q_kJ, deltaT_C, targetTemp_C, runtime_min = 0.0;
  int i = 0, j = 0, setPointNodeNum;
  
  
  // find the first node (from the bottom) that does not have the same temperature as the one above it
  // if they all have the same temp., use the top node, hpwh->numNodes-1
  setPointNodeNum = node;
  for(i = node; i < hpwh->numNodes-1; i++){
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
    Q_kJ = CPWATER_kJperkgC * hpwh->tankVolume_L * DENSITYWATER_kgperL * (setPointNodeNum+1 - node) * deltaT_C;

    //Running the rest of the time won't recover
    if(Q_kJ > cap_kJ){
      for(j = node; j <= setPointNodeNum; j++) {
        hpwh->tankTemps_C[j] += cap_kJ / CPWATER_kJperkgC / hpwh->tankVolume_L / DENSITYWATER_kgperL / (setPointNodeNum + 1 - node);
      }
      runtime_min = minutesPerStep;
      cap_kJ = 0;
    }
    //temp will recover by/before end of timestep
    else{
      for(j = node; j <= setPointNodeNum; j++){
        hpwh->tankTemps_C[j] = targetTemp_C;
      }
      ++setPointNodeNum;
      //calculate how long the element was on, in minutes
      //(should be less than minutesPerStep, typically 1...),
      //based on the capacity and the required amount of heat
      runtime_min += (Q_kJ / cap_kJ) * minutesPerStep;
      cap_kJ -= Q_kJ;
    }
  }
  
  return runtime_min;
}


double HPWH::HeatSource::addHeatExternal(double cap_BTUperHr, double minutesPerStep) {
  double nodeHeat_kJ, heatingCapacity_kJ, remainingCapacity_kJ, deltaT_C, nodeFrac;
  int n;
  double heatingCutoff_C = 20; // When to turn off the compressor. Should be a property of the HeatSource?

  //how much heat is available this timestep
  heatingCapacity_kJ = BTU_TO_KJ(cap_BTUperHr * (minutesPerStep / 60.0));
  remainingCapacity_kJ = heatingCapacity_kJ;
	
  //if there's still remaining capacity and you haven't heated to "setpoint", keep heating
  while(remainingCapacity_kJ > 0 && getCondenserTemp() <= hpwh->setpoint_C - heatingCutoff_C) {
    // calculate what percentage of the bottom node can be heated to setpoint
    //with amount of heat available this timestep
    deltaT_C = hpwh->setpoint_C - getCondenserTemp();
    nodeHeat_kJ = hpwh->tankVolume_L * DENSITYWATER_kgperL * CPWATER_kJperkgC * deltaT_C;
    nodeFrac = remainingCapacity_kJ / nodeHeat_kJ;

  //if more than one, round down to 1 and subtract that amount of heat energy
  // from the capacity
    if(nodeFrac > 1){
      nodeFrac = 1;
      remainingCapacity_kJ -= nodeHeat_kJ;
    }
    //substract the amount of heat used - a full node's heat if nodeFrac == 1,
    //otherwise just the fraction available 
    //this should make heatingCapacity == 0  if nodeFrac < 1
    else{
      remainingCapacity_kJ = 0;
    }
	
    //move all nodes down, mixing if less than a full node
    for(n = 0; n < hpwh->numNodes - 1; n++) {
      hpwh->tankTemps_C[n] = hpwh->tankTemps_C[n] * (1 - nodeFrac) + hpwh->tankTemps_C[n + 1] * nodeFrac;
    }
    //add water to top node, heated to setpoint
    hpwh->tankTemps_C[hpwh->numNodes - 1] = hpwh->tankTemps_C[hpwh->numNodes - 1] * (1 - nodeFrac) + hpwh->setpoint_C * nodeFrac;
		
  }
		
  //This is runtime - is equal to timestep if compressor ran the whole time
  return (1 - (remainingCapacity_kJ / heatingCapacity_kJ)) * minutesPerStep;
}
