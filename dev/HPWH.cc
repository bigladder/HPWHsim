#include "HPWH.hh"


#define F_TO_C(T) ((T-32.0)*5.0/9.0)
#define C_TO_F(T) (((9.0/5.0)*T) + 32.0)
#define KWH_TO_BTU(kwh) (3412.14 * kwh)
#define GAL_TO_L(GAL) (GAL*3.78541)

using std::cout;
using std::endl;

HPWH::HPWH() :
	isHeating(false)
{}

HPWH::~HPWH()
{
delete[] tankTemps_C;

delete[] setOfSources;	

}


int HPWH::HPWHinit_presets(int presetNum)
{

	if(presetNum == 1){
		
		numNodes = 12;
		tankTemps_C = new double[numNodes];
		setpoint_C = 50;
		for(int i = 0; i < numNodes; i++){
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
				
		resistiveElementBottom.T1 = 50;
		resistiveElementBottom.T2 = 67;
		
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
		
		resistiveElementBottom.hysteresis = 0;	//no hysteresis

		//standard logic conditions
		resistiveElementBottom.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("bottomThird", 20));
		resistiveElementBottom.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("standby", 15));

				
		resistiveElementBottom.depressesTemperature = false;  //no temp depression



		//set up a resistive element at the top, 4500 kW
		HeatSource resistiveElementTop(this);
		
		resistiveElementTop.isOn = false;
		resistiveElementTop.isVIP = true;

		resistiveElementTop.setCondensity(0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0);
				
		resistiveElementTop.T1 = 50;
		resistiveElementTop.T2 = 67;
		
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
		
		resistiveElementTop.hysteresis = 0;	//no hysteresis

		resistiveElementTop.depressesTemperature = false;  //no temp depression

		//standard logic conditions
		resistiveElementTop.turnOnLogicSet.push_back(HeatSource::heatingLogicPair("topThird", 20));
		
		
		//assign heat sources into array in order of priority
		setOfSources[0] = resistiveElementTop;
		setOfSources[1] = resistiveElementBottom;
		

	}
	
	
	
	return 0;
	
}  //end HPWHinit_presets


void HPWH::printTankTemps() const
{
	cout << std::setw(7) << std::left << tankTemps_C[0] << ", " << std::setw(7) << tankTemps_C[1] << ", " << std::setw(7) << tankTemps_C[2] << ", " << std::setw(7) << tankTemps_C[3] << ", " << std::setw(7) << tankTemps_C[4] << ", " << std::setw(7) << tankTemps_C[5] << ", " << std::setw(7) << tankTemps_C[6] << ", " << std::setw(7) << tankTemps_C[7] << ", " << std::setw(7) << tankTemps_C[8] << ", " << std::setw(7) << tankTemps_C[9] << ", " << std::setw(7) << tankTemps_C[10] << ", " << std::setw(7) << tankTemps_C[11] << " ";
	cout << "heat source 0: " << setOfSources[0].isEngaged() <<  "\theat source 1: " << setOfSources[1].isEngaged() << endl;
}



int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
					double ambientT_C, double externalT_C,
					double DRstatus, double minutesPerStep)
{

//reset the output variables
outletTemp_C = 0;
energyRemovedFromEnvironment_kWh = 0;
standbyLosses_kWh = 0;

for(int i = 0; i < numHeatSources; i++){
	setOfSources[i].runtime_min = 0;
	setOfSources[i].energyInput_kWh = 0;
	setOfSources[i].energyOutput_kWh = 0;
}


//process draws and standby losses
updateTankTemps(drawVolume_L, inletT_C, ambientT_C, minutesPerStep);



//do HeatSource choice
for(int i = 0; i < numHeatSources; i++){
	//if there's a priority HeatSource (e.g. upper resistor) and it needs to 
	//come on, then turn everything off and start it up
	if(setOfSources[i].isVIP && setOfSources[i].shouldHeat()){
		turnAllHeatSourcesOff();
		setOfSources[i].engageHeatSource();
		//stop looking when you've found such a HeatSource
		break;
	}
	//is nothing is currently on, then check if something should come on
	else if(!isHeating){
		if(setOfSources[i].shouldHeat()){
			setOfSources[i].engageHeatSource();
			//engaging a heat source sets isHeating to true, so this will only trigger once
		}
	}
	//check if anything that is on needs to turn off (generally for lowT cutoffs)
	else{
		if(setOfSources[i].isEngaged() && setOfSources[i].shutsOff()){
			setOfSources[i].disengageHeatSource();
			//check if the backup heat source would have to shut off too
			if(setOfSources[i].backupHeatSource != NULL && setOfSources[i].backupHeatSource->shutsOff() != true){
				//and if not, go ahead and turn it on 
				setOfSources[i].backupHeatSource->engageHeatSource();
			}
		}
	}

}	//end loop over heat sources




//change the things according to DR schedule
if(DRstatus == 0){
	//force off
	turnAllHeatSourcesOff();
	isHeating = false;
}
else if(DRstatus == 1){
	//do nothing
}
else if(DRstatus == 2){
	//if nothing else is on, force the first heat source on
	if(areAllHeatSourcesOff() == true){
		setOfSources[0].engageHeatSource();
	}
}





//do heating logic
for(int i = 0; i < numHeatSources; i++){
	//going through in order, check if the heat source is on
	if(setOfSources[i].isEngaged()){
		//add heat
		setOfSources[i].addHeat_temp(externalT_C, minutesPerStep);
		//if it finished early
		if(setOfSources[i].runtime_min < minutesPerStep){
			//turn it off
			setOfSources[i].disengageHeatSource();
			//and if there's another heat source in the list, that's able to come on,
			if(numHeatSources > i+1 && setOfSources[i + 1].shutsOff() == false){
				//turn it on
				setOfSources[i + 1].engageHeatSource();
			}
		}
	}
}

if(areAllHeatSourcesOff() == true){
	isHeating = false;
}






//settle outputs

//outletTemp_C and standbyLosses_kWh are taken care of in updateTankTemps


//energyRemovedFromEnvironment_kWh;

	









return 0;
} //end runOneStep


void HPWH::updateTankTemps(double drawVolume_L, double inletT_C, double ambientT_C, double minutesPerStep)
{
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
if(wholeNodesToDraw > 0){				
	for(int i = 0; i < wholeNodesToDraw; i++){
		//add temperature of drawn nodes for outletT average
		outletTemp_C += tankTemps_C[numNodes-1 - i];  
	}

	for(int i = numNodes-1; i >= 0; i--){
		if(i > wholeNodesToDraw-1){
			//move nodes up
			tankTemps_C[i] = tankTemps_C[i - wholeNodesToDraw];
		}
		else{
			//fill in bottom nodes with inlet water
			tankTemps_C[i] = inletT_C;	
		}
	}
}
//move fractional node
if(drawFraction > 0){
	//add temperature for outletT average
	outletTemp_C += drawFraction*tankTemps_C[numNodes - 1];
	//move partial nodes up
	for(int i = numNodes-1; i > 0; i--){
		tankTemps_C[i] = tankTemps_C[i] *(1.0 - drawFraction) + tankTemps_C[i-1] * drawFraction;
	}
	//fill in bottom partial node with inletT
	tankTemps_C[0] = tankTemps_C[0] * (1.0 - drawFraction) + inletT_C*drawFraction;
}

//fill in average outlet T
outletTemp_C /= (wholeNodesToDraw + drawFraction);





//Account for mixing at the bottom of the tank
if(tankMixing == true && drawVolume_L > 0){
	int mixedBelowNode = numNodes / 3;
	double ave = 0;
	
	for(int i = 0; i < mixedBelowNode; i++){
		ave += tankTemps_C[i];
	}
	ave /= mixedBelowNode;
	
	for(int i = 0; i < mixedBelowNode; i++){
		tankTemps_C[i] += ((ave - tankTemps_C[i]) / 3.0);
	}
}

//calculate standby losses
//get average tank temperature
double avgTemp = 0;
for(int i = 0; i < numNodes; i++) avgTemp += tankTemps_C[i];
avgTemp /= numNodes;

//kJ's lost as standby in the current time step
double standbyLosses_kJ = (tankUA_kJperHrC * (avgTemp - ambientT_C) * (minutesPerStep / 60.0));	
standbyLosses_kWh = standbyLosses_kJ / 3600.0;

//The effect of standby loss on temperature in each segment
double lossPerNode_C = (standbyLosses_kJ / numNodes)    /    ((volPerNode_L * DENSITYWATER_kgperL) * CPWATER_kJperkgC);
for(int i = 0; i < numNodes; i++) tankTemps_C[i] -= lossPerNode_C;


}	//end updateTankTemps


void HPWH::turnAllHeatSourcesOff()
{
for(int i = 0; i < numHeatSources; i++){
	setOfSources[i].disengageHeatSource();
}
isHeating = false;
}


bool HPWH::areAllHeatSourcesOff() const
{
bool allOff = true;
for(int i = 0; i < numHeatSources; i++){
	if(setOfSources[i].isEngaged() == true){
		allOff = false;
	}
}
return allOff;
}


double HPWH::getOutletTemp(string units) const
{
double returnVal = 0;

if(units == "C"){
	returnVal = outletTemp_C;
}
else if(units == "F"){
	returnVal = C_TO_F(outletTemp_C);
	}
else{
	cout << "Incorrect unit specification for getOutletTemp" << endl;
	exit(1);
}
return returnVal;
}

double HPWH::getEnergyRemovedFromEnvironment(string units) const
{
double returnVal = 0;

if(units == "kWh"){
	returnVal = energyRemovedFromEnvironment_kWh;
}
else if(units == "btu"){
	returnVal = KWH_TO_BTU(energyRemovedFromEnvironment_kWh);
	}
else{
	cout << "Incorrect unit specification for getEnergyRemovedFromEnvironment" << endl;
	exit(1);
}
return returnVal;
}

double HPWH::getStandbyLosses(string units) const
{
double returnVal = 0;

if(units == "kWh"){
	returnVal = standbyLosses_kWh;
}
else if(units == "btu"){
	returnVal = KWH_TO_BTU(standbyLosses_kWh);
	}
else{
	cout << "Incorrect unit specification for getStandbyLosses" << endl;
	exit(1);
}
return returnVal;
}


double HPWH::topThirdAvg_C() const
{
	double sum = 0;
	int num = 0;
	
	for(int i = 2*(numNodes/3); i < numNodes; i++){
		sum += tankTemps_C[i];
		num++;
	}
	
	return sum/num;
}

double HPWH::bottomThirdAvg_C() const
{
	double sum = 0;
	int num = 0;
	
	for(int i = 0; i < numNodes/3; i++){
		sum += tankTemps_C[i];
		num++;
	}
	
	return sum/num;
}



//these are the HeatSource functions

HPWH::HeatSource::HeatSource(HPWH *parentInput)
	:hpwh(parentInput), isOn(false), backupHeatSource(NULL)
{}


void HPWH::HeatSource::setCondensity(double cnd1, double cnd2, double cnd3, double cnd4, 
									double cnd5, double cnd6, double cnd7, double cnd8, 
									double cnd9, double cnd10, double cnd11, double cnd12)
{
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
									

bool HPWH::HeatSource::isEngaged() const
{
return isOn;
}


void HPWH::HeatSource::engageHeatSource()
{
isOn = true;
hpwh->isHeating = true;
}							

									
void HPWH::HeatSource::disengageHeatSource()
{
isOn = false;
}							

									
bool HPWH::HeatSource::shouldHeat() const
{
bool shouldEngage = false;
int selection = 0;


if(turnOnLogicSet[0].selector == "topThird"){
	selection = 1;
}
else if(turnOnLogicSet[0].selector == "bottomThird"){
	selection = 2;
}
else if(turnOnLogicSet[0].selector == "standby"){
	selection = 3;
}



for(int i = 0; i < (int)turnOnLogicSet.size(); i++){
	switch (selection){
		case 1:
			//when the top third is too cold - typically used for upper resistance/VIP heat sources
			if(hpwh->topThirdAvg_C() < hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint_C){
				shouldEngage = true;
			}
			break;
		
		case 2:
			//when the bottom third is too cold - typically used for compressors
			if(hpwh->bottomThirdAvg_C() < hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint_C){
				shouldEngage = true;
			}		
			break;
			
		case 3:
			//when the top node is too cold - typically used for standby heating
			if(hpwh->tankTemps_C[hpwh->numNodes] < hpwh->setpoint_C - turnOnLogicSet[i].decisionPoint_C){
				shouldEngage = true;
			}
			break;
			
		default:
			cout << "You have input an incorrect logic choice specifier, exiting now" << endl;
			exit(1);
			break;
	}
}

return shouldEngage;
}


bool HPWH::HeatSource::shutsOff() const
{
return false;
}


void HPWH::HeatSource::addHeat_temp(double externalT_C, double minutesPerStep)
{
//cout << "heat source 0: " << hpwh->setOfSources[0].isEngaged() <<  "\theat source 1: " << hpwh->setOfSources[1].isEngaged() << endl;
//a temporary function, for testing
int lowerBound = 0;
for(int i = 0; i < hpwh->numNodes; i++){
	if(condensity[i] == 1){
		lowerBound = i;
		}
}


for(int i = lowerBound; i < hpwh->numNodes; i++){
	if(hpwh->tankTemps_C[i] < hpwh->setpoint_C){
		if(hpwh->tankTemps_C[i] + 2.0 > hpwh->setpoint_C){
			hpwh->tankTemps_C[i] = hpwh->setpoint_C;
			if(i == 0){
				runtime_min = 0.5*minutesPerStep;
			}
			
		
		}
		else{
			hpwh->tankTemps_C[i] += 2.0;
			runtime_min = minutesPerStep;
		}
	}
}


}
