#include "HPWH.hh"


#define F_TO_C(T) ((T-32.0)*5.0/9.0)
#define GAL_TO_L(GAL) (GAL*3.78541)

using std::cout;
using std::endl;

HPWH::HPWH()
{}

HPWH::~HPWH()
{
delete[] tankTemps_C;

for(int i = 0; i < numElements; i++){
	delete[] setOfElements;
}
	
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
		tankUA_kJperHrC = 0; //idk, check on this
		
		doTempDepression = false;
		tankMixing = false;



		numElements = 1;
		setOfElements = new Element[numElements];

		//set up a resistive element at the bottom, 4500 kW
		Element resistiveElement(this);
		
		resistiveElement.isEngaged = false;
		
		resistiveElement.setCondensity(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
				
		resistiveElement.T1 = 50;
		resistiveElement.T2 = 67;
		
		resistiveElement.inputPower_T1_constant = 4500;
		resistiveElement.inputPower_T1_linear = 0;
		resistiveElement.inputPower_T1_quadratic = 0;
		
		resistiveElement.inputPower_T2_constant = 4500;
		resistiveElement.inputPower_T2_linear = 0;
		resistiveElement.inputPower_T2_quadratic = 0;
		
		resistiveElement.COP_T1_constant = 1;
		resistiveElement.COP_T1_linear = 0;
		resistiveElement.COP_T1_quadratic = 0;
		
		resistiveElement.COP_T2_constant = 1;
		resistiveElement.COP_T2_linear = 0;
		resistiveElement.COP_T2_quadratic = 0;
		
		resistiveElement.lowTlockout = -100;	//no lockout
		resistiveElement.hysteresis = 0;	//no hysteresis

		resistiveElement.depressesTemperature = false;  //no temp depression

		
		//assign elements into array in order of priority
		setOfElements[0] = resistiveElement;
		
	}
	
	
	
	return 0;
	
}  //end HPWHinit_presets


void HPWH::printTankTemps() const
{
	cout << std::setw(7) << std::left << tankTemps_C[0] << ", " << std::setw(7) << tankTemps_C[1] << ", " << std::setw(7) << tankTemps_C[2] << ", " << std::setw(7) << tankTemps_C[3] << ", " << std::setw(7) << tankTemps_C[4] << ", " << std::setw(7) << tankTemps_C[5] << ", " << std::setw(7) << tankTemps_C[6] << ", " << std::setw(7) << tankTemps_C[7] << ", " << std::setw(7) << tankTemps_C[8] << ", " << std::setw(7) << tankTemps_C[9] << ", " << std::setw(7) << tankTemps_C[10] << ", " << std::setw(7) << tankTemps_C[11] << endl;
}



int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
					double ambientT_C, double externalT_C,
					double DRstatus, double minutesPerStep)
{




//reset the output variables
outletTemp_C = 0;
energyRemovedFromEnvironment_kWh = 0;
standbyLosses_kWh = 0;

for(int i = 0; i < numElements; i++){
	setOfElements[i].runtime = 0;
	setOfElements[i].energyInput = 0;
	setOfElements[i].energyOutput = 0;
}



//process draws and standby losses
updateTankTemps(drawVolume_L, inletT_C, ambientT_C, minutesPerStep);



//do element choice
for(int i = 0; i < numElements; i++){
	//if there's a priority element (e.g. upper resistor) and it needs to 
	//come on, then turn everything off and start it up
	if(setOfElements[i].isVIP && setOfElements[i].shouldHeat()){
		allElementsOff();
		setOfElements[i].engageElement();
		//stop looking when you've found such an element
		break;
	}
	//is nothing is currently on, then check if something should come on
	else if(!isHeating){
		if(setOfElements[i].shouldHeat()){
			setOfElements[i].engageElement();
			//engaging and element sets isHeating to true, so this will only trigger once
		}
	}
	//check if anything that is on needs to turn off (generally for lowT cutoffs)
	else{
		if(setOfElements[i].cannotContinue){
			setOfElements[i].disengageElement();
			setOfElements[i].engageBackup();
		}
	}

}	//end loop over elements



//change the things according to DR schedule



//do heating logic




//settle outputs











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

void HPWH::allElementsOff()
{
for(int i = 0; i < numElements; i++){
	setOfElements[i].disengageElement();
	isHeating = false;
}
	
	
}







HPWH::Element::Element(HPWH *parentInput)
	:hpwh(parentInput), isEngaged(false)
{}

void HPWH::Element::setCondensity(double cnd1, double cnd2, double cnd3, double cnd4, 
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
									
									
void HPWH::Element::engageElement()
{
isEngaged = true;
hpwh->isHeating = true;
}							
									
void HPWH::Element::disengageElement()
{
isEngaged = false;
}							
									
bool HPWH::Element::shouldHeat()
{
bool shouldEngage;
//a temporary setting, for testing
if(hpwh->tankTemps_C[8] < hpwh->setpoint_C - 20){
	shouldEngage = true;
}

return shouldEngage;
}
