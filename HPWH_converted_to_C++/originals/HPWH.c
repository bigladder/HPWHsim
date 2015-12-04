//HPWH.c   -    Stand alone HPWH (Heat Pump Water Heater) simulation

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


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


#include "global.h"
#include "HPWH.h"


/*Explanation of how all this works:
First off, notice that the hpwh is a struct.  The HPWH struct contains relevant information relating the 
the water heater function.  Note that it does not carry through accumulating values that are used in the 
simulation.  That capability is handled within the sim function.

The HPWH struct is divided into hpwh->n_nodes segments, which is a #define in HPWH.h.  The simulation tracks 
temperature in each of those nodes as draws remove water, standby losses decrease the tank temperature, 
and compressor and/or resistance elements heat the water.
*/



//Sum the HPWH temperature array
double temp_sum(double T[], int n_nodes){
	double sum;
	int i;
	sum = 0;
	for(i=0; i < n_nodes; i++){
		sum += T[i];
		}
	return sum;
}



double ave(double A[], int n){
	double sum;
	int i;
	sum=0;
	for(i=0;i<n;i++) sum+=A[i];
	return sum/n;
}



double logisticFunc(double x){
	double temp;
	temp=1/(1+exp(x-OFFSET));
	return temp;
}



//Normalize the coefficients to add to one
int normalize(double C[], int n_nodes){
	double temp;
	int n;
	temp = temp_sum(C, n_nodes);
	for(n=0; n < n_nodes; n++) C[n]=C[n]/temp;
	return 0;
}



double bottom_third(HPWH *hpwh){
	double sum;
	int i, count;
	
	count=hpwh->n_nodes/3;
	sum=0;
	for(i=0;i<count;i++) sum+=hpwh->T[i];
	return sum/count;
}


double middle_third(HPWH *hpwh){
	double sum;
	int i, count;
	
	count=hpwh->n_nodes/3;
	sum=0;
	for(i = count; i < 2*count; i++) sum += hpwh->T[i];
	return sum/count;
}

double bottom_twelfth(HPWH *hpwh){
	double sum;
	int i, count;
	
	count=hpwh->n_nodes/12;
	sum=0;
	for(i=0; i < count; i++) sum += hpwh->T[i];
	return sum/count;
}

double top_third(HPWH *hpwh){
	double sum;
	int i, count;
	
	count = hpwh->n_nodes/3;
	sum = 0;
	for(i=2*count;i<hpwh->n_nodes;i++) sum+=hpwh->T[i];
	return sum/count;
}

double top_half(HPWH *hpwh){
	double sum;
	int i, count;
	
	count = hpwh->n_nodes/2;
	sum = 0;
	for(i = count; i < hpwh->n_nodes; i++) sum += hpwh->T[i];
	return sum/count;
}



//---------------------------------------------------------------------------------------------------//
//-------------------------------------Heat pump water heater functions------------------------------//
//---------------------------------------------------------------------------------------------------//
int HPWH_init(HPWH *hpwh, int HPWH_type, int HPWH_tier, double HPWH_size, double HPWH_setpoint, int location, int ductingType, int HPWH_fan, double resUA){
	int i = 0;
  int q;
  int Stiebel = 0;
	
	hpwh->HPWH_setpoint = HPWH_setpoint;
	hpwh->HPWH_size = HPWH_size;
	

	hpwh->tier = HPWH_tier;
	hpwh->type = HPWH_type;
	hpwh->fanFlow = HPWH_fan;
	
	hpwh->ductingType = ductingType;
	hpwh->location = location;
	
	
	hpwh->HPWHadded = 0;
	hpwh->inputenergy = 0;
	hpwh->compruntime = 0;
	hpwh->resruntime = 0;
	hpwh->resKWH = 0;
	hpwh->standbysum = 0;
	hpwh->energyremoved = 0;
	hpwh->standbyLosses = 0;


	hpwh->compressorStatus = 0;
	hpwh->upperElementStatus = 0;
	hpwh->lowerElementStatus = 0;
	
	
	hpwh->locationTemperature = 72;

	
	if(hpwh->type == 2) {
	  if(Stiebel) {
	    hpwh->condensityHeat[0] = 0;
	    hpwh->condensityHeat[1] = .25;
	    hpwh->condensityHeat[2] = .25;
	    hpwh->condensityHeat[3] = .25;
	    hpwh->condensityHeat[4] = .25;
	    hpwh->condensityHeat[5] = 0;
	    hpwh->gamma = 1.0;
	    hpwh->top = 4;	  
	    hpwh->bottom = 1;
	  } else {
	    hpwh->condensityHeat[0] = 1.0;
	    hpwh->condensityHeat[1] = 0;
	    hpwh->condensityHeat[2] = 0;
	    hpwh->condensityHeat[3] = 0;
	    hpwh->condensityHeat[4] = 0;
	    hpwh->condensityHeat[5] = 0;
	    hpwh->gamma = .3;
	    hpwh->top = 0;
	    hpwh->bottom = 0;
	  }

	} else {
	  if(hpwh->tier == 2) {
	    hpwh->condensityHeat[0] = .4;
	    hpwh->condensityHeat[1] = .4;
	    hpwh->condensityHeat[2] = 0.15;
	    hpwh->condensityHeat[3] = 0.05;
	    hpwh->condensityHeat[4] = 0;
	    hpwh->condensityHeat[5] = 0;
	    hpwh->gamma = 1;
	    hpwh->top = 3;
	    hpwh->bottom = 0;	    
	  } else {
	    hpwh->condensityHeat[0] = 1.0 / 3.0;
	    hpwh->condensityHeat[1] = 1.0 / 3.0;
	    hpwh->condensityHeat[2] = 1.0 / 3.0;
	    hpwh->condensityHeat[3] = 0;
	    hpwh->condensityHeat[4] = 0;
	    hpwh->condensityHeat[5] = 0;
	    hpwh->gamma = .8;
	    hpwh->top = 5;
	    hpwh->bottom = 0;
	  }
	}
	
  hpwh->condentropy = 0;

	for(q = 0; q < N_NODES; q++) {
	  
	  if(hpwh->condensityHeat[q] > 0) {
	    hpwh->condentropy -= hpwh->condensityHeat[q] * log(hpwh->condensityHeat[q]);
	  }
	  hpwh->condensityTemp[q] = 0;
	}



/*COMMENTS ON HPWH SELECTION AND PERFORMANCE
The different choices for water heating units are organized into a tier and type system.  The tiers are
based on the northwest power council water heater rating tiers.  Tier 3 added 4/2/15, the GEH50DFEJSRA (only in cold climate mode).
Tier 4 is used to specify a resistance water heater.  The different types are used to specify a specific 
model within the tier.  For example, in order to simulate a GE hpwh, an input of 1.1 would be used, which
will run tier 1, type 1.  Each tier/type combination has a set of parameters it uses to simulate its 
performance.


hpwh->cop_quadratic_T1, hpwh->cop_linear_T1, hpwh->cop_constant_T1
hpwh->cop_quadratic_T2, hpwh->cop_linear_T2, hpwh->cop_constant_T2

hpwh->input_quadratic_T1, hpwh->input_linear_T1, hpwh->input_constant_T1
hpwh->input_quadratic_T2, hpwh->input_linear_T2, hpwh->input_constant_T2


The parameters shown above provide coefficients for a quadratic model of COP and input power function of 
tank temperature.  Two sets of each are needed, T1 and T2, to account for the different performance
at different ambient air temperatures.  They should probably not be fiddled with because they are 
based on actual data, measured in the lab and calibrated from observational studies.  

To get a COP and Input Power, the ambient temperature is used to linearly interpolate
between the two existing lines.  T1 and T2 are generally 50 and 70 degrees F, but are specified separately.  

The two parameters shrinkage and p determine the qualitative geometric behavior of the 
thermocouple lines as the tank recovers.  At each time step, the aforementioned coefficients 
lead to the amount of heat that will be added.  The question, though, is how to distribute that heat 
along the tank?  What that shrinkage factor does is to "shrink" the perceived distance between the lower 
tank temperature and the upper tank temperature, and the algorithm favors equal distribution of heat
between thermocouples reading "close" in temperature.  In that case, then, the high shrinkage factor 
leads to the behavior observed in the Voltex, which is that, while the bottom of the tank heats rapidly,
upper portions of the tank are also heated, although at a slower rate.  For a very small shrinkage,
similar to the ATI, it essentially heats the lower thermocouples until they run into the 
next highest, at which point they join and the new thermocouple bundle is heated until they run into the 
next highest, and so forth.  "p" modifies the concavity with which thermocouples approach the setpoint.  
Hopefully these two parameters "p" and "shrinkage" will not need to be adjusted.

There are also settings where you can change tank UA and control logic temperatures.  "comp_start" is
the threshold below setpoint at which the compressor (or lower resistance element if it's a
resistance tank) activates.  This is compared with the mean temperature 
of the bottom third of the tank.  Similarly, "res_start" is the threshold below setpoint at which the 
upper resistance element activates, and is compared with the mean temperature of the top third of the 
tank.
*/


		switch(hpwh->tier){
			case 1:				//tier 1
				switch(hpwh->type) {			

					case 0:
						//	What is this Tier 1 code a model of? Which HPWH?
						//	Need to find out and document.  
						hpwh->n_nodes = N_NODES;
						
						
						hpwh->shrinkage=3.7;
						hpwh->p=1.0;
						
						//UA is in btu/hr/F
						hpwh->UA=5.0;
						hpwh->comp_start=35;
						hpwh->res_start=20;
						hpwh->standby_thres = 10.0;  
						hpwh->cutoffTempHysteresis = 4;						
		
						hpwh->curve_T1 = 50;					
						hpwh->curve_T2 = 70;
											
						//the multiplication in the following section is a holdover from an earlier system
						//where a single line was used, and to get 50 and 70 degree curves the line was multiplied
						//by constant factors
						
						//the new system makes room for quadratic terms, and allows for temperatures other than 50 and 70 degrees
						//this is more in line with the code used for calibrating the simulation for use with field data
		
						//cop lines
						hpwh->cop_quadratic_T1 = 0.0000*0.95;
						hpwh->cop_linear_T1 = -0.0255*0.95;
						hpwh->cop_constant_T1 = 5.259*0.95;
		
						hpwh->cop_quadratic_T2 = 0.0000*1.1;
						hpwh->cop_linear_T2 = -0.0255*1.1;
						hpwh->cop_constant_T2 = 5.259*1.1;
		
						//input power lines
						hpwh->input_quadratic_T1 = 0.0000*0.8;
						hpwh->input_linear_T1 = 0.00364*0.8;
						hpwh->input_constant_T1 = 0.379*0.8;
		
						hpwh->input_quadratic_T2 = 0.0000*0.9;
						hpwh->input_linear_T2 = 0.00364*0.9;
						hpwh->input_constant_T2 = 0.379*0.9;
		
						hpwh->ELEMENT_UPPER = 4500;
						hpwh->ELEMENT_LOWER = 4500;
		
						hpwh->lowTempCutoff = 32;
						
						break;

					case 1:			//GE calibration
						hpwh->n_nodes = N_NODES;
						
						hpwh->shrinkage=1;
						hpwh->p=1.0;
						
						//UA is in btu/hr/F
						hpwh->UA=3.6;
						
						hpwh->comp_start=24.4;	//calibrated
						//hpwh->comp_start=35.0;		//original
						//hpwh->comp_start=25.6;		//lab "calibrated"
						//hpwh->comp_start=48.1;		//lab "calibrated"
												
						//hpwh->res_start=22.0;
						hpwh->res_start=10.0;		//calibrated
						//hpwh->res_start=20.0;		//original
						//hpwh->res_start=34.0;		//lab "calibrated"
						//hpwh->res_start=18.0;		//lab "tinkered with"
						
						hpwh->standby_thres = 29.1;	//calibrated
						//hpwh->standby_thres = 10.0;		//original
						//hpwh->standby_thres = 5.2;		//lab "calibrated"
												
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 47;					
						hpwh->curve_T2 = 67;
	
						////cop lines - from calibration
						hpwh->cop_quadratic_T1 = 0.0;
						hpwh->cop_linear_T1 = -0.0210;
						hpwh->cop_constant_T1 = 4.92;
	
						hpwh->cop_quadratic_T2 = 0.0;
						hpwh->cop_linear_T2 = -0.0167;
						hpwh->cop_constant_T2 = 5.03;
	
						//cop lines - from labdata
						//hpwh->cop_quadratic_T1 = -0.0000133;
						//hpwh->cop_linear_T1 = -0.0187;
						//hpwh->cop_constant_T1 = 4.49;
	
						//hpwh->cop_quadratic_T2 = 0.00000254;
						//hpwh->cop_linear_T2 = -0.0252;
						//hpwh->cop_constant_T2 = 5.60;
	
						
						//input power lines
						hpwh->input_quadratic_T1 = 0.00000107;
						hpwh->input_linear_T1 = 0.00159;
						hpwh->input_constant_T1 = 0.247;
						//hpwh->input_constant_T1 = 0.290;		//"lab calibrated"
	
						hpwh->input_quadratic_T2 = 0.00000216;
						hpwh->input_linear_T2 = 0.00121;
						hpwh->input_constant_T2 = 0.328;
						//hpwh->input_constant_T2 = 0.375;		//"lab calibrated"
						
						hpwh->ELEMENT_UPPER = 4500;
						hpwh->ELEMENT_LOWER = 4500;
						
						hpwh->lowTempCutoff = 47;
						
						break;

					case 2:				//Voltex Calibration
						hpwh->n_nodes = N_NODES;

						hpwh->shrinkage=3.7;
						hpwh->p=1.0;
						
						//UA is in btu/hr/F
						//hpwh->UA=4.2;
						hpwh->UA=3.85;		//from test tool calibration
						
						if(Stiebel) {
						  hpwh->comp_start = 80;
						  hpwh->res_start = 1000;
						} else {
						  hpwh->comp_start=43.6;		//calibrated
						  hpwh->res_start=36.0;		//calibrated
						}
						
						//hpwh->comp_start=45.0;			//original
						//hpwh->comp_start=52.0;		//lab "calibrated"
						
						//hpwh->res_start=40.0;
						
						//hpwh->res_start=25.0;		//original
						//hpwh->res_start=29.2;		//lab "calibrated"
						
						hpwh->standby_thres = 23.8;		//calibrated
						//hpwh->standby_thres = 10.0;			//original  
						//hpwh->standby_thres = 20.0;			//lab "calibrated"  
												
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 47;					
						hpwh->curve_T2 = 67;
	
						////cop lines - from calibration
						hpwh->cop_quadratic_T1 = -0.00001;
						hpwh->cop_linear_T1 = -0.0222;
						hpwh->cop_constant_T1 = 4.86;
	
						hpwh->cop_quadratic_T2 = 0.0000407;
						hpwh->cop_linear_T2 = -0.0392;
						hpwh->cop_constant_T2 = 6.58;

						////cop lines - from labdata
						//hpwh->cop_quadratic_T1 = -0.00001;
						//hpwh->cop_linear_T1 = -0.0222;
						////hpwh->cop_constant_T1 = 4.68;		//from fit
						//hpwh->cop_constant_T1 = 5.05;		//from test tool calibration
	
						//hpwh->cop_quadratic_T2 = 0.0000407;
						//hpwh->cop_linear_T2 = -0.0392;
						////hpwh->cop_constant_T2 = 6.42;		//from fit
						//hpwh->cop_constant_T2 = 6.02;       //from test tool calibration
						
						
						//input power lines
						hpwh->input_quadratic_T1 = 0.0000072;
						hpwh->input_linear_T1 = 0.00281;		//from fit
						//hpwh->input_linear_T1 = 0.00271;		//from test tool calibration
						hpwh->input_constant_T1 = 0.467;		//from fit
						//hpwh->input_constant_T1 = 0.460;		//from test tool calibration
	
						hpwh->input_quadratic_T2 = 0.0000176;
						hpwh->input_linear_T2 = 0.00147;		//from fit
						//hpwh->input_linear_T2 = 0.00162;		//from test tool calibration
						hpwh->input_constant_T2 = 0.541;		
						                                        
						hpwh->ELEMENT_UPPER = 4250;
						hpwh->ELEMENT_LOWER = 2000;

						hpwh->lowTempCutoff = 40;
						
						break;
					
					
					default:
						printf("Error: Incorrect type and tier for HPWH.  \n");
						return 2;
						break;
						
					} //end tier 1 type switch
				
				break;  //end the tier switch case 1


			case 2:				//tier 2 - this is where the calibrated ATI lives
			
				switch(hpwh->type){
					case 0:				//in case no type was specified and just a generic tier 2 hpwh is desired
						hpwh->n_nodes = N_NODES;

						hpwh->shrinkage = 3.7;
						hpwh->p = 1.0;
						
						//UA is in btu/hr/F
						hpwh->UA=4.0;
						hpwh->comp_start=45;
						hpwh->res_start=30;
						hpwh->standby_thres = 10.0;  
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 50;					
						hpwh->curve_T2 = 70;
	
						//cop lines
						hpwh->cop_quadratic_T1 = 0.0000*1.04;
						hpwh->cop_linear_T1 = -0.0255*1.04;
						hpwh->cop_constant_T1 = 5.259*1.04;
	
						hpwh->cop_quadratic_T2 = 0.0000*1.21;
						hpwh->cop_linear_T2 = -0.0255*1.21;
						hpwh->cop_constant_T2 = 5.259*1.21;
	
						//input power lines
						hpwh->input_quadratic_T1 = 0.0000*1.09;
						hpwh->input_linear_T1 = 0.00364*1.09;
						hpwh->input_constant_T1 = 0.379*1.09;
	
						hpwh->input_quadratic_T2 = 0.0000*1.17;
						hpwh->input_linear_T2 = 0.00364*1.17;
						hpwh->input_constant_T2 = 0.379*1.17;

						hpwh->ELEMENT_UPPER = 4500;
						hpwh->ELEMENT_LOWER = 4500;

						hpwh->lowTempCutoff = 32;

						break;
					
					case 1:				//ATI calibration
						hpwh->n_nodes = N_NODES;

						hpwh->shrinkage = 0.4;
						hpwh->p = 1.4;
						
						//UA is in btu/hr/F
						hpwh->UA=3.3;
						
						hpwh->comp_start=45.9;		//calibrated
						//hpwh->comp_start=45.0;      //original
						//hpwh->comp_start=50.0;      //lab "calibrated"
						
						hpwh->res_start=24.7;		//calibrated
						//hpwh->res_start=25.0;       //original
						//hpwh->res_start=17.2;       //lab "calibrated"
						
						hpwh->standby_thres = 10.0;  		//calibrated
						//hpwh->standby_thres = 10.0;         //original
						//hpwh->standby_thres = 15.0;         //lab "calibrated"
						
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 50;					
						hpwh->curve_T2 = 67;
	
						//cop lines - from calibration
						hpwh->cop_quadratic_T1 = 0.000242;
						hpwh->cop_linear_T1 = -0.0861;
						hpwh->cop_constant_T1 = 8.326;
	
						hpwh->cop_quadratic_T2 = 0.000343;
						hpwh->cop_linear_T2 = -0.115;
						hpwh->cop_constant_T2 = 10.565;
						
						////cop lines - from labdata
						//hpwh->cop_quadratic_T1 = 0.000242;
						//hpwh->cop_linear_T1 = -0.0861;
						////hpwh->cop_constant_T1 = 8.56;
						//hpwh->cop_constant_T1 = 8.78;		//lab "calibrated"
	
						//hpwh->cop_quadratic_T2 = 0.000343;
						//hpwh->cop_linear_T2 = -0.115;
						////hpwh->cop_constant_T2 = 11.0;
						//hpwh->cop_constant_T2 = 11.3;		//lab "calibrated"


						//input power lines
						hpwh->input_quadratic_T1 = 0.000032;
						//hpwh->input_quadratic_T1 = 0.0000262;		//lab "calibrated"
						hpwh->input_linear_T1 = -0.000336;
						//hpwh->input_linear_T1 = -0.000356;		//lab "calibrated"
						hpwh->input_constant_T1 = 0.422;
						//hpwh->input_constant_T1 = 0.56;		//lab "calibrated"
	
						hpwh->input_quadratic_T2 = 0.0000275;
						hpwh->input_linear_T2 = 0.00108;		//lab "calibrated"
						//hpwh->input_linear_T2 = 0.00063;
						hpwh->input_constant_T2 = 0.363;
						//hpwh->input_constant_T2 = 0.463;		//lab "calibrated"
						
						hpwh->ELEMENT_UPPER = 5000;
						hpwh->ELEMENT_LOWER = 0;

						hpwh->lowTempCutoff = 32;
						
						break;
					
					case 2:				//GE GEH50DFEJSRA in hybrid mode
						hpwh->n_nodes = N_NODES;

						hpwh->shrinkage = 1.0;
						hpwh->p = 1.0;
						
						//UA is in btu/hr/F
						hpwh->UA= 3.3;
						
						hpwh->comp_start=37.0;		      //lab data
						
						hpwh->res_start=30.0;		      //lab data
						
						hpwh->standby_thres = 7.7;		      //lab data
						
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 50;					
						hpwh->curve_T2 = 67;
	
						//cop lines - from labdata
						hpwh->cop_quadratic_T1 = 0.000743;
						hpwh->cop_linear_T1 = -0.210;
						hpwh->cop_constant_T1 = 16.690;
	
						hpwh->cop_quadratic_T2 = 0.000626;
						hpwh->cop_linear_T2 = -0.184;
						hpwh->cop_constant_T2 = 15.819;
						
						//input power lines
						hpwh->input_quadratic_T1 = 0.0;  //did not fit quadratic to input power
						hpwh->input_linear_T1 = 0.00254;
						hpwh->input_constant_T1 = 0.107;
	
						hpwh->input_quadratic_T2 = 0.0;  //did not fit quadratic to input power
						hpwh->input_linear_T2 = 0.00310 ;
						hpwh->input_constant_T2 = 0.0827;
						
						hpwh->ELEMENT_UPPER = 4500;
						hpwh->ELEMENT_LOWER = 3800;

						hpwh->lowTempCutoff = 37;
						
						
						break;

					default:
						printf("Error: Incorrect type and tier for HPWH.  \n");
						return 2;
						break;
									
					}  //end tier 2 type switch
					
					break;   //end the tier switch case 2

			case 3:				//tier 3
				
				switch(hpwh->type){
					case 0:			//generic tier 3
						hpwh->n_nodes = N_NODES;

						hpwh->shrinkage = 3.7;
						hpwh->p = 1.0;
						
						//UA is in btu/hr/F
						hpwh->UA=3.0;
						hpwh->comp_start=45;
						hpwh->res_start=30;
						hpwh->standby_thres = 10;  
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 50;					
						hpwh->curve_T2 = 70;

						//cop lines
						hpwh->cop_quadratic_T1 = 0.0000*1.13;
						hpwh->cop_linear_T1 = -0.0255*1.13;
						hpwh->cop_constant_T1 = 5.259*1.13;

						hpwh->cop_quadratic_T2 = 0.0000*1.33;
						hpwh->cop_linear_T2 = -0.0255*1.33;
						hpwh->cop_constant_T2 = 5.259*1.33;

						//input power lines
						hpwh->input_quadratic_T1 = 0.0000*1.05;
						hpwh->input_linear_T1 = 0.00364*1.05;
						hpwh->input_constant_T1 = 0.379*1.05;

						hpwh->input_quadratic_T2 = 0.0000*1.2;
						hpwh->input_linear_T2 = 0.00364*1.2;
						hpwh->input_constant_T2 = 0.379*1.2;
						
						hpwh->ELEMENT_UPPER = 4500;
						hpwh->ELEMENT_LOWER = 4500;

						hpwh->lowTempCutoff = 32;
						
						break;	
					
					
					case 1:  //this is the GE GEH50DFEJSRA model, when run in cold climate mode
						hpwh->n_nodes = N_NODES;

						hpwh->shrinkage = 1.0;
						hpwh->p = 1.0;
						
						//UA is in btu/hr/F
						hpwh->UA= 3.3;
						
						hpwh->comp_start=38.0;		      //lab data
						
						hpwh->res_start=31;		      //lab data
						
						hpwh->standby_thres = 7.7;		      //lab data
						
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 50;					
						hpwh->curve_T2 = 67;
	
						//cop lines - from labdata
						hpwh->cop_quadratic_T1 = 0.000743;
						hpwh->cop_linear_T1 = -0.210;
						hpwh->cop_constant_T1 = 16.690;
	
						hpwh->cop_quadratic_T2 = 0.000626;
						hpwh->cop_linear_T2 = -0.184;
						hpwh->cop_constant_T2 = 15.819;
						
						//input power lines
						hpwh->input_quadratic_T1 = 0.0;  //did not fit quadratic to input power
						hpwh->input_linear_T1 = 0.00254;
						hpwh->input_constant_T1 = 0.107;
	
						hpwh->input_quadratic_T2 = 0.0;  //did not fit quadratic to input power
						hpwh->input_linear_T2 = 0.00310 ;
						hpwh->input_constant_T2 = 0.0827;
						
						hpwh->ELEMENT_UPPER = 4500;
						hpwh->ELEMENT_LOWER = 3800;

						hpwh->lowTempCutoff = 37;
						
						break;					
					} //end of tier 3 type switch
				break;  //end tier 3
			case 5:				//tier 5
				switch(hpwh->type){
					case 1:  //this is the Sanden GAU model - in reality the setpoint is fixed at 149
						
						//must be divisible by 12
						//hpwh->n_nodes = 168;
						hpwh->n_nodes = 96;
						if(hpwh->n_nodes % 12){
							printf("n_nodes for Sanden must be divisible by 12\n");
							exit(2);
							}
						
						//this is the average difference from setpoint of the bottom (or bottom twelfth, check the code) of the tank when the hpwh cuts off
						//hpwh->heatingCutoff = 5;
						hpwh->heatingCutoff = 20;
						
						//UA is in btu/hr/F
						//hpwh->UA= 4.0;
						//hpwh->UA= 6.3;
						
						
						//this is the one I've been using
						hpwh->UA= 5.0;
						
						
						hpwh->comp_start=85.0;		      //lab calibrated
						
						hpwh->res_start=5000;		     //no resistance element
						
						hpwh->standby_thres = 10;		      //lab calibrated
						
						hpwh->cutoffTempHysteresis = 4;
						
						hpwh->curve_T1 = 35;					
						hpwh->curve_T2 = 95;
	
						////cop lines - from labdata
						//hpwh->cop_quadratic_T1 = 0.000120;
						//hpwh->cop_linear_T1 = -0.0564;
						//hpwh->cop_constant_T1 = 5.68;
	
						//hpwh->cop_quadratic_T2 = 0.000339;
						//hpwh->cop_linear_T2 = -0.126;
						//hpwh->cop_constant_T2 = 10.95;
						
						////input power lines - from labdata
						//hpwh->input_quadratic_T1 = 0.0000202;  
						//hpwh->input_linear_T1 = 0.00150;
						//hpwh->input_constant_T1 = 1.169;
	
						//hpwh->input_quadratic_T2 = 0.0000448;  
						//hpwh->input_linear_T2 = -0.00507 ;
						//hpwh->input_constant_T2 = 1.129;
						
						
						//cop lines - lab calibrated
						hpwh->cop_quadratic_T1 = 0.000120;
						hpwh->cop_linear_T1 = -0.0589;
						//hpwh->cop_constant_T1 = 6.1;
						hpwh->cop_constant_T1 = 7.1;
	
						hpwh->cop_quadratic_T2 = 0.000339;
						hpwh->cop_linear_T2 = -0.136;
						hpwh->cop_constant_T2 = 13.0;
						
						//input power lines - lab calibrated
						//matched quadratic and linear terms to low temp measurements - high temp data was lousy for 80 gal sanden
						hpwh->input_quadratic_T1 = 0.0000207;  
						hpwh->input_linear_T1 = -0.00150;
						hpwh->input_constant_T1 = 1.17;
	
						hpwh->input_quadratic_T2 = 0.0000202;  
						hpwh->input_linear_T2 = -0.00150 ;
						hpwh->input_constant_T2 = 0.81;
						

						
						
						
						hpwh->ELEMENT_UPPER = 0;
						hpwh->ELEMENT_LOWER = 0;

						hpwh->lowTempCutoff = -1000;	//as far as we know, there is no low temperature cutoff
						
						break;					
					


					default:
						printf("Error: Incorrect type and tier for HPWH.  \n");
						return 2;
						break;
					
					} //end tier 5 type switch
					
				break;  //end tier switch case 5


			case 4:			//resistance tank
				hpwh->n_nodes = N_NODES;

				hpwh->shrinkage = 1.0;
				hpwh->p = 1.0;
			   
			    hpwh->UA = resUA;
				hpwh->comp_start=45;
				hpwh->res_start=35;
				hpwh->standby_thres = 10;

				hpwh->ELEMENT_UPPER = 4500;
				hpwh->ELEMENT_LOWER = 4500;
				
				break;  //end tier switch case 4

			default:
				printf("Error:  Incorrect tier chosen for HPWH\n");
				break;
			}

	//allocate space for the node array
	hpwh->T = malloc(hpwh->n_nodes * sizeof(*hpwh->T));
	//start the tank off at setpoint.  this is a reasonable place to start, plus the practice year will break it in nicely
	for(i=0; i < hpwh->n_nodes; i++){
		hpwh->T[i] = HPWH_setpoint;
		}
	
	//start with all elements off	
	hpwh->compressorStatus = 0;
	hpwh->upperElementStatus = 0;
	hpwh->lowerElementStatus = 0;

	//Take 1/N'th of the tank size, convert to cubic feet, multiply by weight of water...gives mass of water per segment
	hpwh->m = HPWH_size/hpwh->n_nodes*.13368*62.4;	

	//on successful init, return 0
	return 0;
}


int HPWH_free(HPWH *hpwh){
	free(hpwh->T);
	return 0;
	}



double condensertemp(HPWH *hpwh){
  double TTank = 0;
  int a = 0;
  a = (int)floor(hpwh->n_nodes/3.0);
  TTank = ave(hpwh->T,a);
  return TTank;
}



double add_upper(HPWH *hpwh, double timestep){
	//adding in the heat from the upper resistor
	int i, j, floorNode, setPointNodeNum;
	double deltaT, Q, max, runtime, target;
	runtime=0;

		//BL 2015-05-11
		//note this should probably be (int)floor(hpwh->n_nodes*2.0/3.0) because I think we explicitly want the floor value
	floorNode = (int)(hpwh->n_nodes*2.0/3.0);
	
	//important to start setPointNodeNum at floorNode, not at 0, otherwise
	//when the first and second nodes are different, setPointNodeNum doesn't get set
	//and you heat the whole tank, and everything gets messed up
	setPointNodeNum = floorNode;
	for(i = floorNode; i < hpwh->n_nodes-1; i++){
		if(hpwh->T[i] != hpwh->T[i+1]){
			break;
			}
		else{
			setPointNodeNum = i+1;
			}
		}
	
	
	
	
	max=timestep/60*hpwh->ELEMENT_UPPER*3.412;
	while(max > 0  &&  setPointNodeNum < hpwh->n_nodes){
		if(setPointNodeNum < hpwh->n_nodes-1){
			target=hpwh->T[setPointNodeNum+1];
			}
		else if(setPointNodeNum == hpwh->n_nodes-1){
			target = hpwh->HPWH_setpoint;
			} 
		
		deltaT = target - hpwh->T[setPointNodeNum];
		
		//heat needed to bring all equal temp. nodes up to the temp of the next node
		Q=hpwh->m*(setPointNodeNum - floorNode + 1)*CPWATER*deltaT;
		
		//Running the rest of the time won't recover
		if(Q > max){
			for(j=floorNode; j <= setPointNodeNum; j++){
				hpwh->T[j] += max/CPWATER/hpwh->m/(setPointNodeNum - floorNode + 1);
				}
			runtime=timestep;
			max=0;
			}
		//temp will recover by/before end of timestep
		else{
			for(j=floorNode; j <= setPointNodeNum; j++){
				hpwh->T[j] = target;
				}
			
			//calculate how long the element was on, in minutes (should be less than 1...), based on the power of the resistor and the required amount of heat
			runtime += Q/(hpwh->ELEMENT_UPPER*3.412/60);

			max -= Q;
			++setPointNodeNum;
			}
		}
	return runtime;
}



double add_lower(HPWH *hpwh, double timestep){
	double Q, deltaT, target, max, runtime;
	int i, j, setPointNodeNum;
	i=0; j=0;
	runtime = 0;
	
	//find the first node (from the bottom) that does not have the same temperature as the one above it
	//if they all have the same temp., use the top node, hpwh->n_nodes-1
	setPointNodeNum = 0;
	for(i = 0; i < hpwh->n_nodes-1; i++){
		if(hpwh->T[i] != hpwh->T[i+1]){
			break;
			}
		else{
			setPointNodeNum = i+1;
			}
		}	
	

	//maximum heat deliverable in this timestep
	max=timestep/60*hpwh->ELEMENT_LOWER*3.412;
	
	while(max > 0 && setPointNodeNum < hpwh->n_nodes){
		//if the whole tank is at the same temp, the target temp is the setpoint
		if(setPointNodeNum == (hpwh->n_nodes-1)){
			target = hpwh->HPWH_setpoint;
			} 
		//otherwise the target temp is the first non-equal-temp node
		else{
			target = hpwh->T[setPointNodeNum+1];
			}
		
		deltaT = target-hpwh->T[setPointNodeNum];
		
		//heat needed to bring all equal temp. nodes up to the temp of the next node
		Q=hpwh->m*(setPointNodeNum+1)*CPWATER*deltaT;

		//Running the rest of the time won't recover
		if(Q > max){
			for(j=0;j<=setPointNodeNum;j++){
				hpwh->T[j] += max/CPWATER/hpwh->m/(setPointNodeNum+1);
				}
			runtime=timestep;
			max=0;
			}
		//temp will recover by/before end of timestep
		else{
			for(j=0; j <= setPointNodeNum; j++){
				hpwh->T[j]=target;
				}
			++setPointNodeNum;
			//calculate how long the element was on, in minutes (should be less than 1...), based on the power of the resistor and the required amount of heat
			runtime += Q/(hpwh->ELEMENT_LOWER*3.412/60);
			max -= Q;
			}
		}

	return runtime;
}


//Update the tank temperatures for this timestep
int updateHPWHtemps(HPWH *hpwh, double add, double inletT, double Ta, double timestep)
{
	double frac, losses, temp, a;
	int n, fl, bn, s;

	frac = add / (hpwh->HPWH_size / hpwh->n_nodes);		//fractional number of nodes drawn

	
	fl=(int)floor(frac);
	
	//if you don't subtract off the integer part of frac it 
	if(frac > 1){
		frac -= fl;
		}
	
	
	for(n=hpwh->n_nodes-1; n > fl; n--){
		hpwh->T[n] = frac * hpwh->T[n-fl-1] + (1-frac) * hpwh->T[n-fl];
		}
	hpwh->T[fl] = frac*inletT + (1 - frac)*hpwh->T[0];

	//fill in completely emptied nodes at the bottom of the tank
	for(n=0; n < fl; n++){
		hpwh->T[n] = inletT;
		}


	//Account for mixing at the bottom of the tank
	if(add > 0){
		bn = hpwh->n_nodes/3;
		a = ave(hpwh->T,bn);
		for(s=0; s < bn; s++){
			hpwh->T[s] += ((a-hpwh->T[s]) / 3);
			}
		}


	temp = (temp_sum(hpwh->T, hpwh->n_nodes)/hpwh->n_nodes-Ta)*(hpwh->UA)/(60/timestep);	//btu's lost as standby in the current time step
	hpwh->standbysum += temp;

	losses = temp/hpwh->n_nodes/hpwh->m/CPWATER;	//The effect of standby loss on temperature in each segment
	for(n=0;n<hpwh->n_nodes;n++) hpwh->T[n] -= losses;

	return 0;
}


//Update the tank temperatures for this timestep -Sanden version does no tank mixing
int updateHPWHtempsSanden(HPWH *hpwh, double add, double inletT, double Ta, double timestep)
{
	double frac, losses, temp;
	int n, fl;
	
	//double a;
	//int bn, s;


	frac = add / (hpwh->HPWH_size / hpwh->n_nodes);		//fractional number of nodes drawn

	
	fl=(int)floor(frac);
	
	//if you don't subtract off the integer part of frac it 
	if(frac > 1){
		frac -= fl;
		}
	
	
	for(n=hpwh->n_nodes-1; n > fl; n--){
		hpwh->T[n] = frac * hpwh->T[n-fl-1] + (1-frac) * hpwh->T[n-fl];
		}
	hpwh->T[fl] = frac*inletT + (1 - frac)*hpwh->T[0];

	//fill in completely emptied nodes at the bottom of the tank
	for(n=0; n < fl; n++){
		hpwh->T[n] = inletT;
		}


	//Account for mixing at the bottom of the tank
	//if(add > 0){
		//bn = hpwh->n_nodes/3;
		//a = ave(hpwh->T,bn);
		//for(s=0; s < bn; s++){
			//hpwh->T[s] += ((a-hpwh->T[s]) / 3);
			//}
		//}


	temp = (temp_sum(hpwh->T, hpwh->n_nodes)/hpwh->n_nodes-Ta)*(hpwh->UA)/(60/timestep);	//btu's lost as standby in the current time step
	hpwh->standbysum += temp;

	losses = temp/hpwh->n_nodes/hpwh->m/CPWATER;	//The effect of standby loss on temperature in each segment
	for(n=0;n<hpwh->n_nodes;n++) hpwh->T[n] -= losses;

	return 0;
}



//Capacity and input function.  Based on tank temperature and ambient dry bulb
double HPWHcap(HPWH *hpwh, double Ta, double *input, double *cop){
	double cap = 0, TTank = 0;
	double cop_T1 = 0, cop_T2 = 0;    			//cop at ambient temperatures T1 and T2
	double input_T1 = 0, input_T2 = 0;			//input power at ambient temperatures T1 and T2	
	double cop_interpolated = 0, input_interpolated = 0;  //temp variables to store interpolated values
	TTank = condensertemp(hpwh);


	cop_T1 = hpwh->cop_constant_T1 + hpwh->cop_linear_T1*TTank + hpwh->cop_quadratic_T1*TTank*TTank;
	cop_T2 = hpwh->cop_constant_T2 + hpwh->cop_linear_T2*TTank + hpwh->cop_quadratic_T2*TTank*TTank;
			
	input_T1 = hpwh->input_constant_T1 + hpwh->input_linear_T1*TTank + hpwh->input_quadratic_T1*TTank*TTank;
	input_T2 = hpwh->input_constant_T2 + hpwh->input_linear_T2*TTank + hpwh->input_quadratic_T2*TTank*TTank;

	cop_interpolated = cop_T1 + (Ta - hpwh->curve_T1)*((cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1));
	

	//printf("cop_T1 : %f\n", cop_T1);
	//printf("cop_T2 : %f\n", cop_T2);
	//printf("Ta : %f\n", Ta);
	//printf("TTank : %f\n", TTank);
	//printf("Ta - hpwh->curve_T1 : %f\n", Ta - hpwh->curve_T1);
	//printf("(cop_T2 - cop_T1) : %f\n", (cop_T2 - cop_T1));
	//printf("(hpwh->curve_T2 - hpwh->curve_T1) : %f\n", (hpwh->curve_T2 - hpwh->curve_T1));
	//printf("(cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1) : %f\n", (cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1));
	//printf("(Ta - hpwh->curve_T1)*((cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1)) : %f\n", (Ta - hpwh->curve_T1)*((cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1)));
	//printf("cop_interpolated : %f\n", cop_interpolated);
	//printf("\n");
	input_interpolated = input_T1 + (Ta - hpwh->curve_T1)*((input_T2 - input_T1)/(hpwh->curve_T2 - hpwh->curve_T1));


	//here is where the scaling for flow restriction goes
	//the input power doesn't change, we just scale the cop by a small percentage that is based on the ducted flow rate
	//the equation is a fit to three points, measured experimentally - 12 percent reduction at 150 cfm, 10 percent at 200, and 0 at 375
	//it's slightly adjust to be equal to 1 at 375
	if(hpwh->ductingType != 0){
		cop_interpolated *= 0.00056*hpwh->fanFlow + 0.79;
		}
		
	//printf("hpwh->fanFlow : %f\n", hpwh->fanFlow);	
	//printf("0.00056*hpwh->fanFlow + 0.79 : %f\n", 0.00056*hpwh->fanFlow + 0.79);
	//printf("cop_interpolated : %f\n", cop_interpolated);
	//printf("\n");
	//printf("curve_T1 : %f   \t   curve_T2 : %f   \t   Ta : %f\n",hpwh->curve_T1, hpwh->curve_T2, Ta);
	//printf("cop_T1 : %f   \t   cop_T2 : %f   \t   cop_interpolated : %f\n\n", cop_T1, cop_T2, cop_interpolated);
	cap = cop_interpolated * input_interpolated * 3412;	//Capacity in btu/hr
	*input = input_interpolated * 1000;	//Input in Watts
	*cop = cop_interpolated;

	return cap;
}




//Capacity and input function for sanden - difference is that it uses the bottom node for the condenser temp
double HPWHcapSanden(HPWH *hpwh, double Ta, double *input, double *cop){
	double cap, TTank;
	double cop_T1, cop_T2;    			//cop at ambient temperatures T1 and T2
	double input_T1, input_T2;			//input power at ambient temperatures T1 and T2	
	double cop_interpolated, input_interpolated;  //temp variables to store interpolated values
	
	//this is the different part
	//TTank = condensertemp(hpwh);
	TTank = hpwh->T[0];

	cop_T1 = hpwh->cop_constant_T1 + hpwh->cop_linear_T1*TTank + hpwh->cop_quadratic_T1*TTank*TTank;
	cop_T2 = hpwh->cop_constant_T2 + hpwh->cop_linear_T2*TTank + hpwh->cop_quadratic_T2*TTank*TTank;
			
	input_T1 = hpwh->input_constant_T1 + hpwh->input_linear_T1*TTank + hpwh->input_quadratic_T1*TTank*TTank;
	input_T2 = hpwh->input_constant_T2 + hpwh->input_linear_T2*TTank + hpwh->input_quadratic_T2*TTank*TTank;

	cop_interpolated = cop_T1 + (Ta - hpwh->curve_T1)*((cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1));
	
	//printf("cop_T1 : %f\n", cop_T1);
	//printf("cop_T2 : %f\n", cop_T2);
	//printf("Ta : %f\n", Ta);
	//printf("TTank : %f\n", TTank);
	//printf("Ta - hpwh->curve_T1 : %f\n", Ta - hpwh->curve_T1);
	//printf("(cop_T2 - cop_T1) : %f\n", (cop_T2 - cop_T1));
	//printf("(hpwh->curve_T2 - hpwh->curve_T1) : %f\n", (hpwh->curve_T2 - hpwh->curve_T1));
	//printf("(cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1) : %f\n", (cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1));
	//printf("(Ta - hpwh->curve_T1)*((cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1)) : %f\n", (Ta - hpwh->curve_T1)*((cop_T2 - cop_T1)/(hpwh->curve_T2 - hpwh->curve_T1)));
	//printf("cop_interpolated : %f\n", cop_interpolated);
	//printf("\n");
	input_interpolated = input_T1 + (Ta - hpwh->curve_T1)*((input_T2 - input_T1)/(hpwh->curve_T2 - hpwh->curve_T1));


	//here is where the scaling for flow restriction goes
	//the input power doesn't change, we just scale the cop by a small percentage that is based on the ducted flow rate
	//the equation is a fit to three points, measured experimentally - 12 percent reduction at 150 cfm, 10 percent at 200, and 0 at 375
	//it's slightly adjust to be equal to 1 at 375
	if(hpwh->ductingType != 0){
		cop_interpolated *= 0.00056*hpwh->fanFlow + 0.79;
		}
		
	//printf("hpwh->fanFlow : %f\n", hpwh->fanFlow);	
	//printf("0.00056*hpwh->fanFlow + 0.79 : %f\n", 0.00056*hpwh->fanFlow + 0.79);
	//printf("cop_interpolated : %f\n", cop_interpolated);
	//printf("\n");
	//printf("curve_T1 : %f   \t   curve_T2 : %f   \t   Ta : %f\n",hpwh->curve_T1, hpwh->curve_T2, Ta);
	//printf("cop_T1 : %f   \t   cop_T2 : %f   \t   cop_interpolated : %f\n\n", cop_T1, cop_T2, cop_interpolated);
	cap = cop_interpolated * input_interpolated * 3412;	//Capacity in btu/hr
	*input = input_interpolated * 1000;	//Input in Watts
	*cop = cop_interpolated;

	return cap;
}




/*Function to add heat pump heat to the water.*/
double addheatcomp(HPWH *hpwh, double timestep, double cap){
	double temp, frac, accum, tdiff;
	double *C;
	int n;
	
	C = calloc(hpwh->n_nodes, hpwh->n_nodes * sizeof(*C));
	
	frac=1;	
	accum=0;

	for(n=0;n<hpwh->n_nodes;n++)
	{
		C[n] = logisticFunc( (hpwh->T[n]-hpwh->T[0])/hpwh->shrinkage );
	
		tdiff = hpwh->HPWH_setpoint - hpwh->T[n];
		//if node temperature above the setpoint temperature DO NOT add any heat to that node
		//note this is a kludge. a potentially better idea is to use the top node temperature instead of the 
		//setpoint temperature to calculate the temperature difference. Implement some other time. 
		if (tdiff<=0) {
			//setting to zero is bad later. Setting to a small, non-negative, non-zero, real number
			C[n] = 0.0000000000000001;
		}
		else {
			C[n] = C[n] * pow(tdiff,hpwh->p);
		}
	}
	normalize(C, hpwh->n_nodes);
	for(n=hpwh->n_nodes-1;n>=0;n--)
	{
		temp=(C[n]*cap*timestep/60/hpwh->m/CPWATER);		//Degrees that can be added to this node
		if( (hpwh->T[n]+temp) > hpwh->HPWH_setpoint )		//If that would put it over the setpoint
		{
			frac-=((hpwh->HPWH_setpoint-hpwh->T[n])/temp*C[n]);	//Tabulate how much is being not used
			accum+=((hpwh->HPWH_setpoint-hpwh->T[n])/temp*C[n]);	//Units of accum are degrees F
			hpwh->T[n] = hpwh->HPWH_setpoint;			//and put it at setpoint
			}
		else{
			hpwh->T[n]+=temp;			//otherwise add all of it
			if( (hpwh->T[n]+accum) > hpwh->HPWH_setpoint )	//can't finish the accumulating
			{
				accum -= (hpwh->HPWH_setpoint - hpwh->T[n]);
				frac += (hpwh->HPWH_setpoint - hpwh->T[n]);
				hpwh->T[n] = hpwh->HPWH_setpoint;
			} else
			{
				hpwh->T[n]+=accum;
				accum=0;
				frac=1;
			}
		}
	}

	free(C);
	return frac*timestep;
	//This is runtime
}




/*Function to add heat pump heat to the water.*/
//returns the length of time that the compressor ran
double addheatcompSanden(HPWH *hpwh, double timestep, double cap){
	double nodeHeat, nodeFrac, heatingCapacity, remainingCapacity, deltaT;
	int n;

	//how much heat is available this timestep
	heatingCapacity = cap*(timestep/60.0);
	remainingCapacity = heatingCapacity;
	
	//if there's still remaining capacity and you haven't heated to "setpoint", keep heating
	while(remainingCapacity > 0 && bottom_twelfth(hpwh) <= hpwh->HPWH_setpoint - hpwh->heatingCutoff){
	//while(remainingCapacity > 0 && hpwh->T[0] <= hpwh->HPWH_setpoint - hpwh->heatingCutoff){
		//calculate what percentage of the bottom node can be heated to setpoint with amount of heat available this timestep
		deltaT = hpwh->HPWH_setpoint - hpwh->T[0];
		nodeHeat =  hpwh->m * CPWATER * deltaT;
		nodeFrac = remainingCapacity/nodeHeat;

		//printf("nodeHeat: %lf \t nodeFrac: %f\n", nodeHeat, nodeFrac);
		
		//if more than one, round down to 1 and subtract that amount of heat energy from the capacity
		if(nodeFrac > 1){
			nodeFrac = 1;
			remainingCapacity -= nodeHeat;
			}
		//substract the amount of heat used - a full node's heat if nodeFrac == 1, otherwise just the fraction available 
		//this should make heatingCapacity == 0  if nodeFrac < 1
		else{
			remainingCapacity = 0;
			}
	
		//move all nodes down, mixing if less than a full node
		for(n = 0; n < hpwh->n_nodes-1; n++){
			hpwh->T[n] = hpwh->T[n]*(1 - nodeFrac) + hpwh->T[n+1]*nodeFrac;
			}	
		//add water to top node, heated to setpoint
		hpwh->T[hpwh->n_nodes-1] = hpwh->T[hpwh->n_nodes-1] * (1 - nodeFrac) + hpwh->HPWH_setpoint * nodeFrac;
		
		
		//printf("nodeTemp: %f \t remaining capacity : %lf\n", hpwh->T[0], remainingCapacity);
		}
		//printf("BREAK\n");
		
	//This is runtime - is equal to timestep if compressor ran the whole time
	return (1 - remainingCapacity/heatingCapacity)*timestep;
}



int simulateHPWH(HPWH *hpwh, double *inletTschedule, double *ambientTschedule, double *evaporatorTschedule, double *drawSchedule, double *DRSchedule, int minutesToRun, int minute_of_year, double *T_sum, int *H_count, FILE *debugFILE){
	/*EXPLAINING THE FUNCTION CONCEPTUALLY
		This function progresses the state of the HPWH forward in time.  It uses 1 minute intervals, the number of which are specified by minutesToRun.
		Originally the variable "timestep" was intended to be settable, to change from 1 minute intervals to other, but now it should not be altered.
		Changing timestep might still work, with appropriate draw schedules, but it would require careful testing.
		Since the input draw schedules are in 1 minute increments, any changes to the timestep require new drawschedules as well.  
		The function loops through those 1 minute intervals and 
		at each timestep 1) updates the hpwh temp for draw (if there) and standby losses.  2) Check to see which components 
		are/should be running.  3) Add heat accordingly.  
	*/

	double cap, input, comp_runtime, cop;
	const double timestep = 1.0;	//Stepping through 1 minute at a time.  Do not change this, draw schedules are in 1 minute increments.
	
	int minute = 0;
	int i = 0, j = 0, s = 0;
	double inputCompKWH, inputResKWH, outputCompBTU;
	double res_runtime_upper, res_runtime_lower;
	double temperatureGoal;
	double Ta, inletT;
	double timeRemaining;
	
	double draw;
	short DR;
	
	double simTCouples[6];
	
	
	//these are summed outside of HPWH
	hpwh->resruntime = 0;
	hpwh->resKWH = 0;
	hpwh->compruntime = 0;
	hpwh->energyremoved = 0;
	hpwh->standbyLosses = 0;

	

	//the loop over all steps
	for(minute=0 ; minute < minutesToRun; minute++){
		timeRemaining = timestep;
		cap = 0; input = 0; comp_runtime = 0;
		res_runtime_upper = 0;
		res_runtime_lower = 0;

		
		//set the inletT, draw, and DR from their schedules
		//--------------------------------
		inletT = inletTschedule[minute];
		draw = drawSchedule[minute];
		DR = DRSchedule[minute];

		//set the ambient T from the schedule
		//-----------------------------------
		Ta = ambientTschedule[minute];
		//the location temperature is Ta, unless you're doing unducted interior (and non-resistance)
		//also, the evaporator temperature is the same as the location temperature, since it must be unducted - normally this is taken care of
		//outside the hpwhSim, but since the location temperature changes here, it must be done inside
		if(hpwh->location == 4 && hpwh->ductingType == 0 && hpwh->tier != 4){
			//leaving hpwh->locationTemperature as it is is the right move
			//in the interior, non-ducted, non-resistance case, the locationTemp has to float from one step to the next, so it doesn't get set
			//the goal changes based on ambient though, and also on if it is running, but that is checked later
			
			//since the locationTemperature is constantly changing, you have to reset the evaporator T every minute
			
			evaporatorTschedule[minute] = hpwh->locationTemperature;
			}
		else{
			hpwh->locationTemperature = Ta;
			}






		//if(DEBUG == 1) printf("\tmoy : %d\n", minute_of_year);
		minute_of_year++;


		//to find average outlet temperature, average top node when there's a draw
		//this isn't quite correct - if it draws more than one node during a step, for instance
		if(draw > 0){
			////DEBUG KLUDGE
			////massive kludge here to get percent of time outlet T is above 105
			//if(hpwh->T[hpwh->n_nodes-1] > 105){
				//(*T_sum)++;
				//}
			//(*H_count)++;
			
			
			//original, correct, code
			*T_sum += hpwh->T[hpwh->n_nodes-1];
			(*H_count)++;
			}
			
		
		
		if(hpwh->tier == 5 && hpwh->type == 1 ){
			updateHPWHtempsSanden(hpwh, draw, inletT, hpwh->locationTemperature, timeRemaining);
			}
		else{
			updateHPWHtemps(hpwh, draw, inletT, hpwh->locationTemperature, timeRemaining);
			}
		
		//if the demand response allows it, do some heating
		if(DR == 1){

			//the logic loop, which decides how to turn on the hpwh, and then does it
			switch(hpwh->tier){
				case 1:
					switch(hpwh->type){
						case 1:		//GE
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
								}
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the field measured data, 
							//so that the compressor stays off when it drives its own ambient space temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
							
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  //do nothing - GE will not return to compressor until the tank is hot if an element has started to run
								  //hpwh->compressorStatus =1;
								  //hpwh->lowerElementStatus =0;
								  }
							}
	
					
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							//if we are supposed to be adding heat to the tank, status will be greater than zero
							if(s > 0){
								//In upper resistance heating mode
								if(hpwh->upperElementStatus == 1){		
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating, GE uses lower resistor to finish what the upper started in the case of high draws 
										//(which we interpret as low temperature in bottom of tank)
										//bottom_third low temperature check. Increase to use more resistance heat. 
										
	
										// 68.1F was the number BL & NK found to best match lab tests (2015-05-08)
										if(bottom_third(hpwh) < 90.0 || evaporatorTschedule[minute] < hpwh->lowTempCutoff){
											hpwh->lowerElementStatus = 1;						
											}
										else{
											hpwh->compressorStatus = 1;
											}
										  }
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									timeRemaining -= res_runtime_lower;
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; // end tier 1, type 1, GE
	
	
	
						case 2:		//Voltex
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
								}
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  hpwh->compressorStatus =1;
								  hpwh->lowerElementStatus =0;
								  }
								
								
							  }
							
					
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating
										if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
											//Turn on the compressor
											hpwh->compressorStatus = 1;
											}
										else{
											//lower resistor activates
											hpwh->lowerElementStatus = 1;						
											}
										}
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									timeRemaining -= res_runtime_lower;
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; // end tier 1, type 2, Voltex			
	
	
						
						default:
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
							  }
								
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  hpwh->compressorStatus =1;
								  hpwh->lowerElementStatus =0;
								  }
								}
				
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating
										if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
											//Turn on the compressor
											hpwh->compressorStatus = 1;
											}
										else{
											//lower resistor activates
											hpwh->lowerElementStatus = 1;						
											}
										}
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									timeRemaining -= res_runtime_lower;
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; //default, untyped hpwh
						}	//end tier 1 type switch
					break; // end tier 1
	
				case 2: 
					switch(hpwh->type){
						case 1: 	//ATI - has no lower element, just shuts off when it's too cold
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//nothing, would normally be lower element
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//nothing, would normally be lower element				
										}
									}
								}
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //just shut off compressor, no lower element to turn on
								  hpwh->compressorStatus =0;
								  }
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  //do nothing here, the lower resistor doesn't exist and so will never be on anyways
								  }
								}
					
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating
										if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
											//Turn on the compressor
											hpwh->compressorStatus = 1;
											}
										else{
											//no lower resistor
											}
										}
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									//none
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; // end tier 2, type 1, ATI				
	
						case 2:		//GE502014 - hybrid mode - uses same logic as tier 1, just different numbers
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
								}
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
							
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  //do nothing - GE will not return to compressor until the tank is hot if an element has started to run
								  //hpwh->compressorStatus =1;
								  //hpwh->lowerElementStatus =0;
								  }
								
								
								}
					
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating, GE only uses lower resistor to finish what the upper started
										//lower resistor activates
										hpwh->lowerElementStatus = 1;						
										}
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									timeRemaining -= res_runtime_lower;
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; // end tier 2, type 2, GE502014 in hybrid mode
	
						default:
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
								}
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  hpwh->compressorStatus =1;
								  hpwh->lowerElementStatus =0;
								  }
								
								
								}
					
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating
										if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
											//Turn on the compressor
											hpwh->compressorStatus = 1;
											}
										else{
											//lower resistor activates
											hpwh->lowerElementStatus = 1;						
											}
										}
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									timeRemaining -= res_runtime_lower;
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; // end tier 2, default
						} //end tier 2 type switch
					break; //end tier 2
	
				case 3: //tier 3
					switch(hpwh->type){
						case 1:  //GE502014 - cold climate mode - turns off upper resistance after heating to 120, unless lower tank is too cold.  also, runs lower element and compressor together
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
								}
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
							
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  //do nothing - GE will not return to compressor until the tank is hot if an element has started to run
								  //hpwh->compressorStatus =1;
								  //hpwh->lowerElementStatus =0;
								  }
								
								//the new strategy - if the upper resistance heats the top to 120 (irregardless of the setpoint), switch back to compressor
								else if(hpwh->upperElementStatus == 1 && evaporatorTschedule[minute] > hpwh->lowTempCutoff && top_third(hpwh) >= 120){
									if(bottom_third(hpwh) < 65.0){
										//lower element - this helps to simulate the "high draw" resorting to resistance heating seen in lab tests
										//also uses compressor to boost efficiency
										hpwh->compressorStatus = 1;
										hpwh->upperElementStatus = 0;
										hpwh->lowerElementStatus = 1;
										}
									else{
										//compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}	
									}
								}
					
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
	
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating, GE only uses lower resistor to finish what the upper started
										//lower resistor activates
										hpwh->lowerElementStatus = 1;	
										//the cold climate mode also runs the compressor at the same time
										if(evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis)){
											hpwh->compressorStatus = 1;
											}
										}
									timeRemaining -= res_runtime_upper;	
									}
								
								//to allow compressor and resistor to run concurrently, the timeRemaining must be subtracted after they both get a chance to run
								//this will lead to negative timeRemaining, but that is probably ok...
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									}
								timeRemaining -= comp_runtime;
								timeRemaining -= res_runtime_lower;
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								}
							break; // end tier 3 type 1, GE502014 cold climate mode
						
						default:
							if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
								//Upper resistance element activates.  others shut off
								hpwh->compressorStatus =0;
								hpwh->upperElementStatus =1;
								hpwh->lowerElementStatus =0;
								}
							//only check for these if there is no heating element already running
							else if(hpwh->compressorStatus == 0 && hpwh->upperElementStatus == 0 && hpwh->lowerElementStatus == 0){
								
								 //these two are if the bottom of the tank is cold
								if( bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Compressor Activates
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;
										}
									}
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Standby recovery when top of tank falls more than standby_thres degrees
									if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
										//Turn on the compressor
										hpwh->compressorStatus =1;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =0;						
										}
									else{
										//lower resistor activates
										hpwh->compressorStatus =0;
										hpwh->upperElementStatus =0;
										hpwh->lowerElementStatus =1;						
										}
									}
								}
							
							
							//if something was already running, decide whether to keep compressor or resistor on
							//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
							//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
							//so that the compressor stays off when it drives its own temperature down
							else{
								//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
								if(hpwh->compressorStatus == 1 && evaporatorTschedule[minute]  < (hpwh->lowTempCutoff - hpwh->cutoffTempHysteresis)  && hpwh->upperElementStatus == 0){
								  //shut off compressor, turn on lower element
								  hpwh->compressorStatus =0;
								  hpwh->lowerElementStatus =1;
								  }
								//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
								else if(hpwh->lowerElementStatus == 1 && evaporatorTschedule[minute] > (hpwh->lowTempCutoff + hpwh->cutoffTempHysteresis) && hpwh->upperElementStatus == 0){
								  hpwh->compressorStatus =1;
								  hpwh->lowerElementStatus =0;
								  }
								
								
								}
				
							s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
							cap = HPWHcap(hpwh, evaporatorTschedule[minute], &input, &cop);
							
							if(s>0){
								if(hpwh->upperElementStatus == 1){		//In upper resistance heating mode
									res_runtime_upper = add_upper(hpwh, timeRemaining);
									if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
										//turn off resistor
										hpwh->upperElementStatus = 0;
										//finish heating
										if(evaporatorTschedule[minute] > hpwh->lowTempCutoff){
											//Turn on the compressor
											hpwh->compressorStatus = 1;
											}
										else{
											//lower resistor activates
											hpwh->lowerElementStatus = 1;						
											}
										}
									timeRemaining -= res_runtime_upper;	
									}
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									// comp_runtime = addheatcomp(hpwh,timeRemaining,cap);
									comp_runtime = addheat(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									timeRemaining -= comp_runtime;
									}
								//lower element?
								if(hpwh->lowerElementStatus == 1){
									res_runtime_lower = add_lower(hpwh, timeRemaining);
									//if finished, turn off element
									if(res_runtime_lower < timeRemaining){
										hpwh->lowerElementStatus = 0;
										}
									timeRemaining -= res_runtime_lower;
									}
								
								
								if(timeRemaining > 0){
									//finish running in this step?  everything off
									hpwh->compressorStatus =0;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								
								} //end if(s > 0)
							break;  //end default					
	
						
						}	//end tier 3 type switch
					break;  //end tier 3
				
				case 4:			//This is the resistance heater
				
					if(top_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->res_start)){
						//Turn on the upper element
						hpwh->compressorStatus =0;
						hpwh->upperElementStatus =1;
						hpwh->lowerElementStatus =0;
						} 
					else if(bottom_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start)){
						//Turn on the lower element.  Note that I'm using hpwh->comp_start here.  Deal with it
						hpwh->compressorStatus =0;
						hpwh->upperElementStatus =0;
						hpwh->lowerElementStatus =1;
						}
					else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres) && hpwh->compressorStatus == 0 && hpwh->upperElementStatus ==0 && hpwh->lowerElementStatus ==0){
						//Standby recovery when top of tank falls more than standby_thres degrees
						hpwh->compressorStatus =0;
						hpwh->upperElementStatus =0;
						hpwh->lowerElementStatus =1;
						}
					
					s = hpwh->compressorStatus + hpwh->upperElementStatus + hpwh->lowerElementStatus;
					if(s > 0){
						//Upper element recovery
						if(hpwh->upperElementStatus ==1){	
							res_runtime_upper = add_upper(hpwh, timeRemaining);
							//if upper element is finished, turn it off
							if(res_runtime_upper < timeRemaining) {
								hpwh->upperElementStatus = 0;
								//if the upper was on, you'll probably want to heat some with the lower while you're at it
								hpwh->lowerElementStatus = 1;
								}
							timeRemaining -= res_runtime_upper;
							}
						
						//lower element?
						if(hpwh->lowerElementStatus == 1){
							res_runtime_lower = add_lower(hpwh, timeRemaining);
							//if finished, turn off element
							if(res_runtime_lower < timeRemaining){
								hpwh->lowerElementStatus = 0;
								}
							timeRemaining -= res_runtime_lower;
							}
	
						if(timeRemaining > 0){
							//finish running in this step?  everything off
							hpwh->compressorStatus =0;
							hpwh->upperElementStatus =0;
							hpwh->lowerElementStatus =0;
							}
						}
					break;
	
				case 5:
					switch(hpwh->type){
						default:
							printf("There is no default for Sanden, you must enter an acceptable type number.\n");
							exit(2);
						case 1:  //SandenGAU, the 80 gallon unit.  Can be used for general Sanden HPWH
							//the operation of this unit is fundamentally different than the others.  
							//water to be heated comes from the bottom and returns to the top, at setpoint
							//there is no resistance element, and no low temperature cutoff
						
							//only check for these if there is no heating element already running
							if(hpwh->compressorStatus == 0){
								 //this is if the middle of the tank is cold
								if( middle_third(hpwh) < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
								//if( hpwh->T[5*hpwh->n_nodes / 12] < (hpwh->HPWH_setpoint - hpwh->comp_start) ){
									//Compressor Activates
									hpwh->compressorStatus =1;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;
									}
								//Standby recovery when top of tank falls more than standby_thres degrees
								else if( hpwh->T[hpwh->n_nodes-1] < (hpwh->HPWH_setpoint - hpwh->standby_thres)){
									//Turn on the compressor
									hpwh->compressorStatus =1;
									hpwh->upperElementStatus =0;
									hpwh->lowerElementStatus =0;						
									}
								}
					
							s = hpwh->compressorStatus;
							cap = HPWHcapSanden(hpwh, evaporatorTschedule[minute], &input, &cop);
		
							if(s>0){
								//In compressor heating mode	
								if(hpwh->compressorStatus == 1){
									comp_runtime = addheatcompSanden(hpwh,timeRemaining,cap);
									if(comp_runtime < timeRemaining){	//The compressor finished
										hpwh->compressorStatus = 0;
										}
									}
								timeRemaining -= comp_runtime;
								}
							break; // end tier 5 type 1, Sanden GAU
						}	//end type switch for tier 5
					break;  //end tier 5
					
			} //end tier switch

		}//end DR switch

		//summing variables at the end
		inputCompKWH = (comp_runtime/60.0)*(input/1000.0);
		outputCompBTU = (comp_runtime/60.0)*cap;

		inputResKWH = (res_runtime_upper/60.0)*((hpwh->ELEMENT_UPPER)/1000.0) + (res_runtime_lower/60.0)*((hpwh->ELEMENT_LOWER)/1000.0);
		
		
		//if not ducted, energy comes from the air
		if(hpwh->ductingType == 0){
			//heat from compressor minus energy input into compressor as electricity, converted to btu
			hpwh->energyremoved += outputCompBTU - inputCompKWH*3412.0;
			}
		//if it is ducted, the air is being replaced from outside, and the infiltration model handles it
		else{
			hpwh->energyremoved = 0.0;
			}
		

		//heat moving from tank to space
		hpwh->standbyLosses += (ave(hpwh->T, hpwh->n_nodes) - hpwh->locationTemperature) * ((hpwh->UA)*(timestep/60.0));

		hpwh->HPWHadded += (outputCompBTU/3412.0 + inputResKWH);

		hpwh->inputenergy += inputCompKWH + inputResKWH;

		//these will be total summed outside of simulateHPWH, but here each minute is summer over the whole step period
		hpwh->compruntime += comp_runtime;
		hpwh->resruntime += res_runtime_upper + res_runtime_lower;
		hpwh->resKWH += inputResKWH;
	
	
		//this is the section for exponential temperature changes in unducted interior spaces - NDK 7/2014
		//adjusting the location temperature as the last step in the simulation; temperature doesn't change until it's run for a minute...makes sense to me
		if(hpwh->ductingType == 0 && hpwh->location == 4 && hpwh->tier != 4){
			//if the compressor's on, we're cooling
			if(hpwh->compressorStatus == 1){
				temperatureGoal = Ta - 4.5;		//hardcoded 4.5 degree total drop - from experimental data
				}
			//otherwise, we're going back to ambient
			else{
				temperatureGoal = Ta;
				}
			
			//shrink the gap by the same percentage every minute - that gives us exponential behavior
			//the percentage was determined by a fit to experimental data - 9.4 minute half life and 4.5 degree total drop
			//minus-equals is important, and fits with the order of locationTemperature and temperatureGoal, so as to not use fabs() and conditional tests
			hpwh->locationTemperature -= (hpwh->locationTemperature - temperatureGoal)*(1 - 0.9289);
			}
		
		if(TEST == 1){
				
				//do averaging of node pairs (or triplets, etc.) to simulate the thermocouples used in lab tests
				if(hpwh->n_nodes%6 != 0){
					printf("Must have a multiple of 6 nodes in order to average them to match lab tests.\n");
					return 1;
					}
				for(i = 0; i < 6; i++){				//loop over simulated lab-test thermocouples
					simTCouples[i] = 0;
					for(j = 0; j < hpwh->n_nodes/6; j++){			//loop over the nodes that make up each "thermocouple"
						simTCouples[i] += hpwh->T[(i * hpwh->n_nodes/6) + j];
						}
					simTCouples[i] /= hpwh->n_nodes/6;
					
					//just grab the node temp in the middle of the section
					//simTCouples[i] += hpwh->T[  i * (hpwh->n_nodes / 6) + (hpwh->n_nodes/6)/2];
					
					}
				
				
				//some debugging stuff  -- delete when sanden works
				//for (i = 0; i < hpwh->n_nodes; i++){
				//for (i = 0; i < 28; i++){
					//printf("hpwh->T[%i] = %.1f\t", i, hpwh->T[i]);
					//}
				//printf("\n\n\n\n\n");
				
				fprintf(debugFILE, "%d,%d,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", minute_of_year,  
							hpwh->compressorStatus , hpwh->upperElementStatus , hpwh->lowerElementStatus, Ta, inputCompKWH, inputResKWH, draw, inletT, 	
							hpwh->T[0*hpwh->n_nodes/12], hpwh->T[1*hpwh->n_nodes/12], hpwh->T[2*hpwh->n_nodes/12], hpwh->T[3*hpwh->n_nodes/12], hpwh->T[4*hpwh->n_nodes/12], hpwh->T[5*hpwh->n_nodes/12], hpwh->T[6*hpwh->n_nodes/12], 
							hpwh->T[7*hpwh->n_nodes/12], hpwh->T[8*hpwh->n_nodes/12], hpwh->T[9*hpwh->n_nodes/12], hpwh->T[10*hpwh->n_nodes/12], hpwh->T[11*hpwh->n_nodes/12],  
							simTCouples[0], simTCouples[1], simTCouples[2], simTCouples[3], simTCouples[4], simTCouples[5], cop);
				}
		if(DEBUG == 1)	fprintf(debugFILE, "%d %d %d %d %f %f %f %f %f %f %f %f %f %f %f\n", minute_of_year, hpwh->compressorStatus , hpwh->upperElementStatus , hpwh->lowerElementStatus, hpwh->locationTemperature, Ta, inputCompKWH + inputResKWH, draw, inletT, hpwh->T[0], hpwh->T[2], hpwh->T[4], hpwh->T[6], hpwh->T[8], hpwh->T[10]);
	
	}  //end loop over all steps


	return 0;
}






/*Function to add heat pump heat to the water.*/
double addheat(HPWH *hpwh, double timestep, double cap){
  double runtime, capTmp, xtmp;
  double *Z, s, tdiff;
  double beta = 2;
  int i, n;
  double dist = 2;

  // Find how many nodes to heat "n"
  n = hpwh->top;
  for(i = hpwh->top + 1; i < hpwh->n_nodes; i++) {
    if(hpwh->T[i] - hpwh->T[hpwh->top] < dist) {
      n = i;
    }
  }

  Z = calloc(hpwh->n_nodes, hpwh->n_nodes * sizeof(*Z));
  
  // New shrinkage
  s = 1 + hpwh->condentropy * beta;

  for(n=0;n<hpwh->n_nodes;n++)
  {
    if(n < hpwh->bottom) {
      Z[n] = 0;
    } else {
      Z[n] = logisticFunc( (hpwh->T[n]-hpwh->T[hpwh->bottom]) / s );
      tdiff = pow(hpwh->HPWH_setpoint - hpwh->T[n], 1.4);
      Z[n] = Z[n] * tdiff; 
    }
  }
  normalize(Z, hpwh->n_nodes);
  
  for(i = hpwh->n_nodes - 1; i >= 0; i--) {
  // for(i = 0; i < hpwh->n_nodes; i++) {
    // xtmp = (1 - p) * hpwh->condensityHeat[i] + p * 1 / n;
    xtmp = hpwh->condensityHeat[i] * (1 - hpwh->gamma) + Z[i] * hpwh->gamma;
    capTmp = cap * timestep / 60 * xtmp;
    
    // if(hpwh->condensityHeat[i] > 0) {
    if(capTmp > 0) {
      // printf("Capacity = %f, condensity[%d] = %f\n", cap, i, hpwh->condensityHeat[i]);
      runtime = add_point(hpwh, timestep, i, capTmp);
    }
  }
  free(Z);
  
  return runtime;
}




double add_point(HPWH *hpwh, double timestep, int node_cur, double cap){
  double Q, deltaT, target, max, runtime;
  int i, j, setPointNodeNum;
  i=0; j=0;
  runtime = 0;

  
  //find the first node (from the bottom) that does not have the same temperature as the one above it
  //if they all have the same temp., use the top node, hpwh->n_nodes-1
  setPointNodeNum = node_cur;
  for(i = node_cur; i < hpwh->n_nodes-1; i++){
    if(hpwh->T[i] != hpwh->T[i+1]){
      break;
    }
    else{
      setPointNodeNum = i+1;
    }
  }	

  //maximum heat deliverable in this timestep
  // max=timestep/60*hpwh->ELEMENT_LOWER*3.412;
  max = cap;
  
  while(max > 0 && setPointNodeNum < hpwh->n_nodes){
    //if the whole tank is at the same temp, the target temp is the setpoint
    if(setPointNodeNum == (hpwh->n_nodes-1)){
      target = hpwh->HPWH_setpoint;
    } 
    //otherwise the target temp is the first non-equal-temp node
    else{
      target = hpwh->T[setPointNodeNum+1];
    }

    deltaT = target-hpwh->T[setPointNodeNum];
    
    //heat needed to bring all equal temp. nodes up to the temp of the next node
    Q=hpwh->m * (setPointNodeNum+1 - node_cur) * CPWATER * deltaT;

    //Running the rest of the time won't recover
    if(Q > max){
      for(j = node_cur; j <= setPointNodeNum; j++){
        hpwh->T[j] += max / CPWATER / hpwh->m / (setPointNodeNum + 1 - node_cur);
      }
      runtime=timestep;
      max=0;
    }
    //temp will recover by/before end of timestep
    else{
      for(j=node_cur; j <= setPointNodeNum; j++){
        hpwh->T[j] = target;
      }
      ++setPointNodeNum;
      //calculate how long the element was on, in minutes (should be less than 1...), based on the power of the resistor and the required amount of heat
      runtime += Q / max;
      max -= Q;
    }
  }
  
  return runtime;
}
