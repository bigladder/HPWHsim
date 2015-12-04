// HPWH.h   -    Stand alone HPWH (Heat Pump Water Heater) simulation

//Copyright (C) <2012, 2015>  Ecotope, Inc. written by Michael Logsdon, Nicholas Kvaltine, Ben Larson

//This program is free software: you can redistribute it and/or modify
//it under the terms of the GNU General Public License as published by
//the Free Software Foundation, either version 3 of the License, or
//(at your option) any later version.

//This program is distributed in the hope that it will be useful,
//but WITHOUT ANY WARRANTY; without even the implied warranty of
//MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//GNU General Public License for more details.

//You should have received a copy of the GNU General Public License
//along with this program.  If not, see <http://www.gnu.org/licenses/>.


#define N_NODES 12
#define OFFSET 5
#define SCFRAC 0.01		//0 if all air is from crawlspace. 1 if all air is from interior  --- SC is "smart closet"

// The HPWH class
class HPWH
{
public:

	//user variable properties of hpwh model being simulated
	int type;				//to specify a particular hpwh that we have calibrated.  Tier.Type is use to ID a HPWH
	int tier;				//1,2,or 3 -- corresponds to NEEA Northern Climate Spec. 1.1=GE, 1.2=Voltex, 2.1= ATI, 2.2 = GE, 3.1 = GE,  4=Resistance Tank

	double HPWH_setpoint; 

	double HPWH_size;
	double m;				//mass of water in lb at each segment of the water heater - not directly settable, depends on size


	double ductedm;		//typcialled used as a bool - is it ducted or not?
	double fanFlow;		//for ducted systems, what's the air flow

	int location;		//garage, basement, etc.

	
	//properties of the hpwh model being simulated
	double UA;				//tank heat loss rate in Btu/hrF
	
	double cop_quadratic_T1, cop_linear_T1, cop_constant_T1; 			//coefficients for COP curves at T1
	double input_quadratic_T1, input_linear_T1, input_constant_T1; 		//coefficients for input power curves at T1
	double cop_quadratic_T2, cop_linear_T2, cop_constant_T2; 			//coefficients for COP curves at T2
	double input_quadratic_T2, input_linear_T2, input_constant_T2; 	//coefficients for input power curves at T2
	double curve_T1, curve_T2;  								//these are the temperatures at which the cop and input curves were "measured"

	double shrinkage;		//Changes the how added heat is distributed between the thermocouples
	double p;				//exponent that adjusts how the thermocouple lines join together esp. while finishing a recovery

	double ELEMENT_UPPER;			//upper resistance element input Watts
	double ELEMENT_LOWER;			//lower resistance element input Watts


	double comp_start;		//# of degrees that lower third of tank must drop to activate compressor
	double res_start;		//# of degrees that upper third of tank must drop to activate resistance element
	double standby_thres;  //# of degrees that the top node must drop to reheat (in standby mode)
	double cutoffTempHysteresis; //the difference between the standby low-T cutoff and the running low-T cutoff
	double lowTempCutoff; 	//the standby low-T cutoff


	

	//state of the hpwh instance
	double T[ N_NODES + 10];		// Track the temperatures -- I'm not sure why the +10 is here, it shouldn't strictly be needed -NDK 11/12/14
	int lowerElementStatus;		// Reflects which components -- heat pump, upper resistance element, lower resistance element -- are operating
	int upperElementStatus;
	int compressorStatus;
	double locationTemperature;		//the temp of the hpwh's location (garage, basement, etc.) - used for self-cooling exponential decay


	//output variables
	double HPWHadded;
	double inputenergy;
	double compruntime;
	double resruntime;
	double resKWH;
	double standbysum;
	double energyremoved;
	double standbyLosses;

public:
	HPWH();
	int HPWH_init( int _type, int _tier, double _size, double _setpoint,
		int location, double ductedm, int _fan, double resUA);
	int simulateHPWH( double* inletTschedule, double* ambientTschedule,
		double* drawSchedule, int minutesToRun, int minute_of_year, double* T_sum,
		int* H_count, FILE* debugFILE);

private:
	double top_third() const;
	double bottom_third() const;
	double condensertemp() const;
	double add_upper( double timestep);
	double add_lower( double timestep);
	int updateHPWHtemps( double add, double inletT, double Ta, double timestep);
	double HPWHcap( double Ta, double *input, double *cop);
	double addheatcomp( double timestep, double cap);
	double addheatresistance( double timestep);

};	// class HPWH

// hpwh.h end

