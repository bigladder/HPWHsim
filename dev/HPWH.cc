#include "HPWH.hh"

using namespace std;

HPWH::HPWH()
{}

int HPWH::HPWHinit_presets(int presetNum)
{

	if(presetNum == 1){
		
		numNodes = 12;
		tankVolume = 50;
		
		doTempDepression = false;
		
		numElements = 1;
		for(int i = 0; i < numElements; i++){
			setOfElements[i] = Element(this);
		}
		
		setOfElements[0].isEngaged = false;
		
		//set up a resistive element at the bottom, 4500 kW
		setOfElements[0].setCondensity(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1);
				
		setOfElements[0].T1 = 50;
		setOfElements[0].T2 = 67;
		
		setOfElements[0].inputPower_T1_constant = 4500;
		setOfElements[0].inputPower_T1_linear = 0;
		setOfElements[0].inputPower_T1_quadratic = 0;
		
		setOfElements[0].inputPower_T2_constant = 4500;
		setOfElements[0].inputPower_T2_linear = 0;
		setOfElements[0].inputPower_T2_quadratic = 0;
		
		setOfElements[0].COP_T1_constant = 1;
		setOfElements[0].COP_T1_linear = 0;
		setOfElements[0].COP_T1_quadratic = 0;
		
		setOfElements[0].COP_T2_constant = 1;
		setOfElements[0].COP_T2_linear = 0;
		setOfElements[0].COP_T2_quadratic = 0;
		
		setOfElements[0].lowTlockout = -100;	//no lockout
		setOfElements[0].hysteresis = 0;	//no hysteresis

		setOfElements[0].depressesTemperature = false;  //no temp depression

		
	}
	
	
	
	return 0;
	
}  //end HPWHinit_presets




int HPWH::runOneStep(double inletT, double drawVolume, 
					double ambientT, double externalT,
					double DRstatus)
{






return 0;
} //end runOneStep




HPWH::Element::Element(HPWH *parentInput)
	:hpwh(parentInput), isEngaged(false)
{}

