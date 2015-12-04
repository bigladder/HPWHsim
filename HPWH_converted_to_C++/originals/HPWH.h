//HPWH.h   -    Stand alone HPWH (Heat Pump Water Heater) simulation

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


#define N_NODES 6
#define OFFSET 5

//The HPWH struct
typedef struct {


	//user variable properties of hpwh model being simulated
	int type;				//to specify a particular hpwh that we have calibrated.  Tier.Type is use to ID a HPWH
	int tier;				//1,2,or 3 -- corresponds to NEEA Northern Climate Spec. 1.1=GE, 1.2=Voltex, 2.1= ATI, 2.2 = GE, 3.1 = GE,  4=Resistance Tank

	double HPWH_setpoint; 
	double heatingCutoff;	//added specifically for the Sanden hpwh, this is the temperature where the compressor cuts off, instead of heating all the way to setpoint

	double HPWH_size;
	double m;				//mass of water in lb at each segment of the water heater - not directly settable, depends on size


	int ductingType;		//what kind of ducting?   0 - none, 1 - duct out, 2 - duct in, duct out
	double fanFlow;		//for ducted systems, what's the air flow

	int location;		//garage, basement, etc.
	int	n_nodes;		//the number of nodes used to simulate the tank
	
	//properties of the hpwh model being simulated
	double UA;				//tank heat loss rate in Btu/hrF
	
	double cop_quadratic_T1, cop_linear_T1, cop_constant_T1; 			//coefficients for COP curves at T1
	double input_quadratic_T1, input_linear_T1, input_constant_T1; 		//coefficients for input power curves at T1
	double cop_quadratic_T2, cop_linear_T2, cop_constant_T2; 			//coefficients for COP curves at T2
	double input_quadratic_T2, input_linear_T2, input_constant_T2; 	//coefficients for input power curves at T2
	double curve_T1, curve_T2;  								//these are the temperatures at which the cop and input curves were "measured"

	double shrinkage;		//Changes the how added heat is distributed between the thermocouples
	double p;				//exponent that adjusts how the thermocouple lines join together esp. while finishing a recovery
	double condentropy;

	double ELEMENT_UPPER;			//upper resistance element input Watts
	double ELEMENT_LOWER;			//lower resistance element input Watts


	double comp_start;		//# of degrees that lower third of tank must drop to activate compressor
	double res_start;		//# of degrees that upper third of tank must drop to activate resistance element
	double standby_thres;  //# of degrees that the top node must drop to reheat (in standby mode)
	double cutoffTempHysteresis; //the difference between the standby low-T cutoff and the running low-T cutoff
	double lowTempCutoff; 	//the standby low-T cutoff

  
  // CONDENSITY
  double condensityTemp[12], condensityHeat[12];
  int top, bottom; // Top node of the condenser
  
  // Vertical Heat Flow Constant
  double gamma;
	

	//state of the hpwh instance
	double *T;		//an array, dynamically allocated, to track the temperatures - T[0] is the bottom, T[n_nodes - 1] is the top
	int lowerElementStatus;		//Reflects which components -- heat pump, upper resistance element, lower resistance element -- are operating
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
	
} HPWH;



//HPWH Functions

//public:
int HPWH_init(HPWH *hpwh, int HPWH_type, int HPWH_tier, double HPWH_size, double HPWH_setpoint, int location, int ductingType, int HPWH_fan, double resUA);
int simulateHPWH(HPWH *hpwh, double *inletTschedule, double *ambientTschedule, double *evaporatorTschedule, double *drawSchedule, double *DRschedule, int minutesToRun, int minute_of_year, double *T_sum, int *H_count, FILE *debugFILE);
int HPWH_free(HPWH *hpwh);

//private:
double condensertemp(HPWH *hpwh);
double temp_sum(double T[], int n_nodes);
double ave(double A[], int n);
double f(double x);
int normalize(double C[], int n_nodes);
int normalize(double C[], int n_nodes);
double bottom_third(HPWH *hpwh);
double middle_third(HPWH *hpwh);
double top_third(HPWH *hpwh);
double top_half(HPWH *hpwh);
double add_upper(HPWH *hpwh, double timestep);
double add_lower(HPWH *hpwh, double timestep);
int updateHPWHtemps(HPWH *hpwh, double add, double inletT, double Ta, double timestep);
double HPWHcap(HPWH *hpwh, double Ta, double *input, double *cop);
double addheatcomp(HPWH *hpwh, double timestep, double cap);
double addheatresistance(HPWH *hpwh, double timestep);
double addheat(HPWH *hpwh, double timestep, double cap);
double add_point(HPWH *hpwh, double timestep, int node_cur, double cap);


