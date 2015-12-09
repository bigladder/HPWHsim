#ifndef HPWH_hh
#define HPWH_hh

#include <string>

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

	int HPWHinit_file(std::string configFile);
	/* This function will load in a set of parameters from a file
	 * The file name is the input - there should be at most one set of parameters per file
	 * This is useful for testing new variations, and for the sort of variability
	 * that we typically do when creating SEEM runs
	 * 
	 * The return value is 0 for successful init, something else otherwise
	 */

	int runOneStep(double inletT, double drawVolume, 
					double ambientT, double externalT,
					double DRstatus);
	/* This function will progress the simulation forward in time by one step
	 * all calculated outputs are stored in private variables and accessed through functions
	 * 
	 * The return value is 0 for successful simulation run, something else otherwise
	 */
	 

	int runNSteps(int N,  double inletT, double drawVolume, 
					double ambientT, double externalT,
					double DRstatus);
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
	
	double getOutletTemp() const;
	//a function to get the outlet temperature - returns 0 when no draw occurs
	double getEnergyRemovedFromEnvironment() const;
	//get the total energy removed from the environment by all elements (not net energy - does not include standby)
	double getStandbyLosses() const;
	//get the amount of heat lost through the tank
	 
	 
	 
private:	
	class Element;
	
	int numElements;
	//how many elements this HPWH has
	Element *setOfElements;
	//an array containing the Elements, in order of priority
	
	int numNodes;
	//the number of nodes in the tank
	double tankVolume;
	//the volume in gallons of the tank
	
	
	
	
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
	
	bool isEngaged;
	//is the element running or not	

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
