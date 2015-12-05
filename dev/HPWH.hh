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
	 

	double getNumNodes() const;
	//get the number of nodes
	double* getTankTemps() const;
	// A function to access the array of tank temperatures
	
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
	
	
};




#endif
