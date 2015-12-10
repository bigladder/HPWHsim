#ifndef HPWH_hh
#define HPWH_hh

#include <string>
using std::string;
#include <cmath>


#define DENSITYWATER_kgperL 0.998
#define CPWATER_kJperkgC 4.181


class HPWH
{
public:
	
	HPWH();  //default constructor
	
	int HPWHinit_presets(int presetNum);
	/* This function will load in a set of parameters that are hardcoded in this function - 
	 * which particular set of parameters is selected by presetNum.
	 * This is similar to the way the HPWHsim currently operates, as used in SEEM,
	 * but not quite as versatile.
	 * My impression is that this could be a useful input paradigm for CSE
	 * 
	 * The return value is 0 for successful init, something else otherwise
	 */

	int HPWHinit_file(string configFile);
	/* This function will load in a set of parameters from a file
	 * The file name is the input - there should be at most one set of parameters per file
	 * This is useful for testing new variations, and for the sort of variability
	 * that we typically do when creating SEEM runs
	 * 
	 * The return value is 0 for successful init, something else otherwise
	 */

	int runOneStep(double inletT_C, double drawVolume_L, 
					double ambientT_C, double externalT_C,
					double DRstatus, double minutesPerStep);
	/* This function will progress the simulation forward in time by one step
	 * all calculated outputs are stored in private variables and accessed through functions
	 * 
	 * The return value is 0 for successful simulation run, something else otherwise
	 */
	 

	int runNSteps(int N,  double inletT_C, double drawVolume_L, 
					double ambientT_C, double externalT_C,
					double DRstatus, double minutesPerStep);
	/* This function will progress the simulation forward in time by N steps
	 * The calculated values will be summed or averaged, as appropriate, and 
	 * then stored in the usual variables to be accessed through functions
	 * 
	 * The return value is 0 for successful simulation run, something else otherwise
	 */
	 

	double getNumNodes() const;
	//get the number of nodes
	double* getTankTemps() const;
	// get the array of tank temperatures
	
	int getNumElements() const;
	//get the number of elements
	double* getElementsEnergyInput() const;
	//get an array of the energy input to each element, in order of element priority
	double* getElementsEnergyOutput() const;
	//get an array of the energy output to each element, in order of element priority
	double* getElementsRunTime() const;
	//get an array of the run time for each element, in order of element priority - 
	//this may sum to more than 1 time step for concurrently running elements
	
	double getOutletTemp(string units) const;
	//a function to get the outlet temperature - returns 0 when no draw occurs
	double getEnergyRemovedFromEnvironment(string units) const;
	//get the total energy removed from the environment by all elements (not net energy - does not include standby)
	double getStandbyLosses(string units) const;
	//get the amount of heat lost through the tank
	 
	 
	 
private:	
	class Element;

	void updateTankTemps(double draw, double inletT, double ambientT, double minutesPerStep);


	
	int numElements;
	//how many elements this HPWH has
	Element *setOfElements;
	//an array containing the Elements, in order of priority
	
	int numNodes;
	//the number of nodes in the tank
	double tankVolume_L;
	//the volume in liters of the tank
	double tankUA_kJperHrC;
	
	
	double *tankTemps_C;
	//an array holding the temperature of each node - 0 is the bottom node, numNodes is the top
	
	

	//Some outputs
	double outletTemp_C;
	//the temperature of the outlet water - taken from top of tank, 0 if no flow
	double energyRemovedFromEnvironment_kWh;
	//the total energy removed from the environment, to heat the water
	double standbyLosses_kWh;
	//the amount of heat lost to standby
	
	
	//special variables for adding abilities
	bool tankMixing;
	
	bool doTempDepression;
	//whether the HPWH should track an alternate ambient temperature and 
	//cause it to be depressed when running
	double locationTemperature;
	//this is the special location temperature that stands in for the the 
	//ambient temperature if you are doing temp. depression
	
};  //end of HPWH class






class HPWH::Element
{
friend class HPWH;
public:
	Element() {};
	//default constructor, does not create a useful Element
	
	Element(HPWH *parentHPWH);
	//constructor assigns a pointer to the hpwh creating this element 
	
	void engageElement();
	//turn element on, i.e. set isEngaged to TRUE
	void disengageElement();
	//turn element off, i.e. set isEngaged to FALSE
	
	void addHeat();
	//adds head to the hpwh - this is the function that interprets the 
	//various configurations (internal/external, resistance/heat pump) to add heat
	
	void setCondensity(double cnd1, double cnd2, double cnd3, double cnd4, 
						double cnd5, double cnd6, double cnd7, double cnd8, 
						double cnd9, double cnd10, double cnd11, double cnd12);
	//a function to set the condensity values, it pretties up the init funcs.
	
private:
	HPWH *hpwh;
	//the creator of the element, necessary to access HPWH variables
	
	//these are the element state/output variables
	bool isEngaged;
	//is the element running or not	
	double runtime;
	//this is the percentage of the step that the element was running
	double energyInput;
	//the energy used by the element
	double energyOutput;
	//the energy put into the water by the element




	//these are the element property variables
	
	double condensity[12];
	//The condensity function is always composed of 12 nodes.  
	//It represents the location within the tank where heat will be distributed,
	//and it also is used to calculate the condenser temperature for inputPower/COP calcs.
	//It is conceptually linked to the way condenser coils are wrapped around 
	//(or within) the tank, however a resistance element can also be simulated
	//by specifying the entire condensity in one node.
	
	double T1, T2;
	//the two temperatures where the input and COP curves are defined
	double inputPower_T1_constant, inputPower_T2_constant;
	double inputPower_T1_linear, inputPower_T2_linear;
	double inputPower_T1_quadratic, inputPower_T2_quadratic;
	//these are the coefficients for the quadratic function 
	//defining the input power as a function of the condenser temperature
	double COP_T1_constant, COP_T2_constant;
	double COP_T1_linear, COP_T2_linear;
	double COP_T1_quadratic, COP_T2_quadratic;
	//these are the coefficients for the quadratic function 
	//defining the COP as a function of the condenser temperature


	double lowTlockout;
	//the lowest ambient temperature at which this element will work
	double hysteresis;
	//a hysteresis term that prevents short cycling due to heat pump self-interaction

	bool depressesTemperature;
	//heat pumps can depress the temperature of their space in certain instances - 
	//whether or not this occurs is a bool in HPWH, but an element must 
	//know if it is capable of contributing to this effect or not


};  //end of Element class






#endif
