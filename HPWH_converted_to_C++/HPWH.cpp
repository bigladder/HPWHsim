// HPWH.cpp   -    Stand alone HPWH (Heat Pump Water Heater) simulation

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


#include <global.h>

#include <HPWH.h>


/*Explanation of how all this works:
First off, notice that the hpwh is a struct.  The HPWH struct contains relevant information relating the 
the water heater function.  Note that it does not carry through accumulating values that are used in the 
simulation.  That capability is handled within the sim function.

The HPWH struct is divided into N_NODES segments, which is a #define in HPWH.h.  The simulation tracks 
temperature in each of those nodes as draws remove water, standby losses decrease the tank temperature, 
and compressor and/or resistance elements heat the water.
*/



// Sum the HPWH temperature array
double temp_sum(const double T[]){
	int i;
	double sum=0.;
	for(i=0;i<N_NODES;i++){
		sum+=T[i];
		}
	return sum;
}



double ave( const double A[], int n){
	int i;
	double sum=0;
	for(i=0;i<n;i++) sum+=A[i];
	return sum/n;
}



double f( double x){
	double temp=1/(1+exp(x-OFFSET));
	return temp;
}



// Normalize the coefficients to add to one
int normalize(double C[])
{	int n;
	double temp=temp_sum(C);
	for(n=0;n<N_NODES;n++) C[n]=C[n]/temp;
	return 0;
}

///////////////////////////////////////////////////////////////////////////////
// class HPWH
///////////////////////////////////////////////////////////////////////////////
HPWH::HPWH()		// default c'tor
{
}	// HPWH::HPWH



double HPWH::bottom_third() const
{
	int count=N_NODES/3;
	double sum=0;
	for(int i=0;i<count;i++)
		sum+=T[i];
	return sum/count;
}	// HPWH::bottom_third



double HPWH::top_third() const
{
	int count = N_NODES/3;
	double sum = 0;
	for(int i=2*count;i<N_NODES;i++)
		sum+=T[i];
	return sum/count;
}	// HPWH::top_third



//---------------------------------------------------------------------------------------------------//
//-------------------------------------Heat pump water heater functions------------------------------//
//---------------------------------------------------------------------------------------------------//
int HPWH::HPWH_init(
	int _type, 
	int _tier, 
	double _size,
	double _setpoint,
	int _location,
	double _ductedm,
	int _fan,
	double resUA)
{
	int i = 0;
	
	HPWH_setpoint = _setpoint;
	HPWH_size = _size;
	

	tier = _tier;
	type = _type;
	fanFlow = _fan;
	
	ductedm = _ductedm;
	location = _location;
	
	
	HPWHadded = 0;
	inputenergy = 0;
	compruntime = 0;
	resruntime = 0;
	resKWH = 0;
	standbysum = 0;
	energyremoved = 0;
	standbyLosses = 0;


	compressorStatus = 0;
	upperElementStatus = 0;
	lowerElementStatus = 0;
	
	
	locationTemperature = 72;

	
	


/*COMMENTS ON HPWH SELECTION AND PERFORMANCE
The different choices for water heating units are organized into a tier and type system.  The tiers are
based on the northwest power council water heater rating tiers.  Tier 3 added 4/2/15, the GEH50DFEJSRA (only in cold climate mode).
Tier 4 is used to specify a resistance water heater.  The different types are used to specify a specific 
model within the tier.  For example, in order to simulate a GE hpwh, an input of 1.1 would be used, which
will run tier 1, type 1.  Each tier/type combination has a set of parameters it uses to simulate its 
performance.


cop_quadratic_T1, cop_linear_T1, cop_constant_T1
cop_quadratic_T2, cop_linear_T2, cop_constant_T2

input_quadratic_T1, input_linear_T1, input_constant_T1
input_quadratic_T2, input_linear_T2, input_constant_T2


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


		switch(tier){
			case 1:				//tier 1
				switch(type) {			

					case 0:
						//	What is this Tier 1 code a model of? Which HPWH?
						//	Need to find out and document.  
						shrinkage=3.7;
						p=1.0;
						
						//UA is in btu/hr/F
						UA=5.0;
						comp_start=35;
						res_start=20;
						standby_thres = 10.0;  
						cutoffTempHysteresis = 4;						
		
						curve_T1 = 50;					
						curve_T2 = 70;
											
						//the multiplication in the following section is a holdover from an earlier system
						//where a single line was used, and to get 50 and 70 degree curves the line was multiplied
						//by constant factors
						
						//the new system makes room for quadratic terms, and allows for temperatures other than 50 and 70 degrees
						//this is more in line with the code used for calibrating the simulation for use with field data
		
						//cop lines
						cop_quadratic_T1 = 0.0000*0.95;
						cop_linear_T1 = -0.0255*0.95;
						cop_constant_T1 = 5.259*0.95;
		
						cop_quadratic_T2 = 0.0000*1.1;
						cop_linear_T2 = -0.0255*1.1;
						cop_constant_T2 = 5.259*1.1;
		
						//input power lines
						input_quadratic_T1 = 0.0000*0.8;
						input_linear_T1 = 0.00364*0.8;
						input_constant_T1 = 0.379*0.8;
		
						input_quadratic_T2 = 0.0000*0.9;
						input_linear_T2 = 0.00364*0.9;
						input_constant_T2 = 0.379*0.9;
		
						ELEMENT_UPPER = 4500;
						ELEMENT_LOWER = 4500;
		
						lowTempCutoff = 32;
						
						break;

					case 1:			//GE calibration
						shrinkage=1;
						p=1.0;
						
						//UA is in btu/hr/F
						UA=3.6;
						
						//comp_start=24.4;	//calibrated
						//comp_start=35.0;		//original
						//comp_start=25.6;		//lab "calibrated"
						comp_start=48.1;		//lab "calibrated"
												
						//res_start=22.0;
						//res_start=10.0;		//calibrated
						//res_start=20.0;		//original
						//res_start=34.0;		//lab "calibrated"
						res_start=18.0;		//lab "tinkered with"
						
						//standby_thres = 29.1;	//calibrated
						//standby_thres = 10.0;		//original
						standby_thres = 5.2;		//lab "calibrated"
												
						cutoffTempHysteresis = 4;
						
						curve_T1 = 47;					
						curve_T2 = 67;
	
						////cop lines - from calibration
						//cop_quadratic_T1 = 0.0;
						//cop_linear_T1 = -0.0210;
						//cop_constant_T1 = 4.92;
	
						//cop_quadratic_T2 = 0.0;
						//cop_linear_T2 = -0.0167;
						//cop_constant_T2 = 5.03;
	
						//cop lines - from labdata
						cop_quadratic_T1 = -0.0000133;
						cop_linear_T1 = -0.0187;
						cop_constant_T1 = 4.49;
	
						cop_quadratic_T2 = 0.00000254;
						cop_linear_T2 = -0.0252;
						cop_constant_T2 = 5.60;
	
						
						//input power lines
						input_quadratic_T1 = 0.00000107;
						input_linear_T1 = 0.00159;
						//input_constant_T1 = 0.247;
						input_constant_T1 = 0.290;
	
						input_quadratic_T2 = 0.00000216;
						input_linear_T2 = 0.00121;
						//input_constant_T2 = 0.328;
						input_constant_T2 = 0.375;
						
						ELEMENT_UPPER = 4500;
						ELEMENT_LOWER = 4500;
						
						lowTempCutoff = 47;
						
						break;

					case 2:				//Voltex Calibration
						shrinkage=3.7;
						p=1.0;
						
						//UA is in btu/hr/F
						//UA=4.2;
						UA=3.85;		//from test tool calibration
						
						//comp_start=43.6;		//calibrated
						//comp_start=45.0;			//original
						comp_start=52.0;		//lab "calibrated"
						
						//res_start=40.0;
						//res_start=36.0;		//calibrated
						//res_start=25.0;		//original
						res_start=29.2;		//lab "calibrated"
						
						//standby_thres = 23.8;		//calibrated
						//standby_thres = 10.0;			//original  
						standby_thres = 20.0;			//lab "calibrated"  
												
						cutoffTempHysteresis = 4;
						
						curve_T1 = 47;					
						curve_T2 = 67;
	
						////cop lines - from calibration
						//cop_quadratic_T1 = -0.00001;
						//cop_linear_T1 = -0.0222;
						//cop_constant_T1 = 4.86;
	
						//cop_quadratic_T2 = 0.0000407;
						//cop_linear_T2 = -0.0392;
						//cop_constant_T2 = 6.58;

						////cop lines - from labdata
						cop_quadratic_T1 = -0.00001;
						cop_linear_T1 = -0.0222;
						//cop_constant_T1 = 4.68;		//from fit
						cop_constant_T1 = 5.05;		//from test tool calibration
	
						cop_quadratic_T2 = 0.0000407;
						cop_linear_T2 = -0.0392;
						//cop_constant_T2 = 6.42;		//from fit
						cop_constant_T2 = 6.02;       //from test tool calibration
						
						
						//input power lines
						input_quadratic_T1 = 0.0000072;
						//input_linear_T1 = 0.00281;		//from fit
						input_linear_T1 = 0.00271;		//from test tool calibration
						//input_constant_T1 = 0.467;		//from fit
						input_constant_T1 = 0.460;		//from test tool calibration
	
						input_quadratic_T2 = 0.0000176;
						//input_linear_T2 = 0.00147;		//from fit
						input_linear_T2 = 0.00162;		//from test tool calibration
						input_constant_T2 = 0.541;		
						                                        
						ELEMENT_UPPER = 4250;
						ELEMENT_LOWER = 2000;

						//lowTempCutoff = 43;
						lowTempCutoff = 40;
						
						break;
					
					
					default:
						printf("Error: Incorrect type and tier for HPWH.  \n");
						return 2;
						break;
						
					} //end tier 1 type switch
				
				break;  //end the tier switch case 1


			case 2:				//tier 2 - this is where the calibrated ATI lives
			
				switch(type){
					case 0:				//in case no type was specified and just a generic tier 2 hpwh is desired
						shrinkage = 3.7;
						p = 1.0;
						
						//UA is in btu/hr/F
						UA=4.0;
						comp_start=45;
						res_start=30;
						standby_thres = 10.0;  
						cutoffTempHysteresis = 4;
						
						curve_T1 = 50;					
						curve_T2 = 70;
	
						//cop lines
						cop_quadratic_T1 = 0.0000*1.04;
						cop_linear_T1 = -0.0255*1.04;
						cop_constant_T1 = 5.259*1.04;
	
						cop_quadratic_T2 = 0.0000*1.21;
						cop_linear_T2 = -0.0255*1.21;
						cop_constant_T2 = 5.259*1.21;
	
						//input power lines
						input_quadratic_T1 = 0.0000*1.09;
						input_linear_T1 = 0.00364*1.09;
						input_constant_T1 = 0.379*1.09;
	
						input_quadratic_T2 = 0.0000*1.17;
						input_linear_T2 = 0.00364*1.17;
						input_constant_T2 = 0.379*1.17;

						ELEMENT_UPPER = 4500;
						ELEMENT_LOWER = 4500;

						lowTempCutoff = 32;

						break;
					
					case 1:				//ATI calibration
						shrinkage = 0.4;
						p = 1.4;
						
						//UA is in btu/hr/F
						UA=3.3;
						
						//comp_start=45.9;		//calibrated
						//comp_start=45.0;      //original
						comp_start=50.0;      //lab "calibrated"
						
						//res_start=24.7;		//calibrated
						//res_start=25.0;       //original
						res_start=17.2;       //lab "calibrated"
						
						//standby_thres = 10.0;  		//calibrated
						//standby_thres = 10.0;         //original
						standby_thres = 15.0;         //lab "calibrated"
						
						cutoffTempHysteresis = 4;
						
						curve_T1 = 50;					
						curve_T2 = 67;
	
						//cop lines - from calibration
						//cop_quadratic_T1 = 0.000242;
						//cop_linear_T1 = -0.0861;
						//cop_constant_T1 = 8.326;
	
						//cop_quadratic_T2 = 0.000343;
						//cop_linear_T2 = -0.115;
						//cop_constant_T2 = 10.565;
						
						////cop lines - from labdata
						cop_quadratic_T1 = 0.000242;
						cop_linear_T1 = -0.0861;
						//cop_constant_T1 = 8.56;
						cop_constant_T1 = 8.78;		//lab "calibrated"
	
						cop_quadratic_T2 = 0.000343;
						cop_linear_T2 = -0.115;
						//cop_constant_T2 = 11.0;
						cop_constant_T2 = 11.3;		//lab "calibrated"


						//input power lines
						//input_quadratic_T1 = 0.000032;
						input_quadratic_T1 = 0.0000262;		//lab "calibrated"
						//input_linear_T1 = -0.000336;
						input_linear_T1 = -0.000356;		//lab "calibrated"
						//input_constant_T1 = 0.422;
						input_constant_T1 = 0.56;		//lab "calibrated"
	
						input_quadratic_T2 = 0.0000275;
						//input_linear_T2 = 0.00108;		//lab "calibrated"
						input_linear_T2 = 0.00063;
						//input_constant_T2 = 0.363;
						input_constant_T2 = 0.463;		//lab "calibrated"
						
						ELEMENT_UPPER = 5000;
						ELEMENT_LOWER = 0;

						lowTempCutoff = 32;
						
						break;
					
					case 2:				//GE GEH50DFEJSRA in hybrid mode
						shrinkage = 1.0;
						p = 1.0;
						
						//UA is in btu/hr/F
						UA= 3.3;
						
						comp_start=37.0;		      //lab data
						
						res_start=30.0;		      //lab data
						
						standby_thres = 7.7;		      //lab data
						
						cutoffTempHysteresis = 4;
						
						curve_T1 = 50;					
						curve_T2 = 67;
	
						//cop lines - from labdata
						cop_quadratic_T1 = 0.000743;
						cop_linear_T1 = -0.210;
						cop_constant_T1 = 16.690;
	
						cop_quadratic_T2 = 0.000626;
						cop_linear_T2 = -0.184;
						cop_constant_T2 = 15.819;
						
						//input power lines
						input_quadratic_T1 = 0.0;  //did not fit quadratic to input power
						input_linear_T1 = 0.00254;
						input_constant_T1 = 0.107;
	
						input_quadratic_T2 = 0.0;  //did not fit quadratic to input power
						input_linear_T2 = 0.00310 ;
						input_constant_T2 = 0.0827;
						
						ELEMENT_UPPER = 4500;
						ELEMENT_LOWER = 3800;

						lowTempCutoff = 37;
						
						
						break;

					default:
						printf("Error: Incorrect type and tier for HPWH.  \n");
						return 2;
						break;
									
					}  //end tier 2 type switch
					
					break;   //end the tier switch case 2

			case 3:				//tier 3
				
				switch(type){
					case 0:
						shrinkage = 3.7;
						p = 1.0;
						
						//UA is in btu/hr/F
						UA=3.0;
						comp_start=45;
						res_start=30;
						standby_thres = 10;  
						cutoffTempHysteresis = 4;
						
						curve_T1 = 50;					
						curve_T2 = 70;

						//cop lines
						cop_quadratic_T1 = 0.0000*1.13;
						cop_linear_T1 = -0.0255*1.13;
						cop_constant_T1 = 5.259*1.13;

						cop_quadratic_T2 = 0.0000*1.33;
						cop_linear_T2 = -0.0255*1.33;
						cop_constant_T2 = 5.259*1.33;

						//input power lines
						input_quadratic_T1 = 0.0000*1.05;
						input_linear_T1 = 0.00364*1.05;
						input_constant_T1 = 0.379*1.05;

						input_quadratic_T2 = 0.0000*1.2;
						input_linear_T2 = 0.00364*1.2;
						input_constant_T2 = 0.379*1.2;
						
						ELEMENT_UPPER = 4500;
						ELEMENT_LOWER = 4500;

						lowTempCutoff = 32;
						
						break;	
					
					
					case 1:  //this is the GE GEH50DFEJSRA model, when run in cold climate mode
						shrinkage = 1.0;
						p = 1.0;
						
						//UA is in btu/hr/F
						UA= 3.3;
						
						comp_start=38.0;		      //lab data
						
						res_start=31;		      //lab data
						
						standby_thres = 7.7;		      //lab data
						
						cutoffTempHysteresis = 4;
						
						curve_T1 = 50;					
						curve_T2 = 67;
	
						//cop lines - from labdata
						cop_quadratic_T1 = 0.000743;
						cop_linear_T1 = -0.210;
						cop_constant_T1 = 16.690;
	
						cop_quadratic_T2 = 0.000626;
						cop_linear_T2 = -0.184;
						cop_constant_T2 = 15.819;
						
						//input power lines
						input_quadratic_T1 = 0.0;  //did not fit quadratic to input power
						input_linear_T1 = 0.00254;
						input_constant_T1 = 0.107;
	
						input_quadratic_T2 = 0.0;  //did not fit quadratic to input power
						input_linear_T2 = 0.00310 ;
						input_constant_T2 = 0.0827;
						
						ELEMENT_UPPER = 4500;
						ELEMENT_LOWER = 3800;

						lowTempCutoff = 37;
						
						break;					
					


					default:
						printf("Error: Incorrect type and tier for HPWH.  \n");
						return 2;
						break;
					
					} //end tier 3 type switch
					
					break;  //end tier switch case 3
					
			case 4:			//resistance tank
				shrinkage = 1.0;
				p = 1.0;
			   
			    UA = resUA;
				comp_start=45;
				res_start=35;
				standby_thres = 10;

				ELEMENT_UPPER = 4500;
				ELEMENT_LOWER = 4500;
				
				break;  //end tier switch case 4

			default:
				printf("Error:  Incorrect tier chosen for HPWH\n");
				break;
			}



	//start the tank off at setpoint.  this is a reasonable place to start, plus the practice year will break it in nicely
	for(i=0; i < N_NODES; i++){
		T[i] = HPWH_setpoint;
		}
	
	//start with all elements off	
	compressorStatus = 0;
	upperElementStatus = 0;
	lowerElementStatus = 0;

	//Take 1/N'th of the tank size, convert to cubic feet, multiply by weight of water...gives mass of water per segment
	m = HPWH_size/N_NODES*.13368*62.4;	

	//on successful init, return 0
	return 0;
}	// HPWH_init



double HPWH::condensertemp() const
{
  int a = (int)floor(N_NODES/3.0);
  double TTank = ave(T,a);
  return TTank;
}	// HPWH::condensertemp



double HPWH::add_upper( double timestep)
{
	//adding in the heat from the upper resistor
	int i, j, floorNode, setPointNodeNum;
	double deltaT, Q, max, runtime, target;
	runtime=0;

		//BL 2015-05-11
		//note this should probably be (int)floor(N_NODES*2.0/3.0) because I think we explicitly want the floor value
	floorNode = (int)(N_NODES*2.0/3.0);
	
	//important to start setPointNodeNum at floorNode, not at 0, otherwise
	//when the first and second nodes are different, setPointNodeNum doesn't get set
	//and you heat the whole tank, and everything gets messed up
	setPointNodeNum = floorNode;
	for(i = floorNode; i < N_NODES-1; i++){
		if(T[i] != T[i+1]){
			break;
			}
		else{
			setPointNodeNum = i+1;
			}
		}
	
	
	
	
	max=timestep/60*ELEMENT_UPPER*3.412;
	while(max > 0  &&  setPointNodeNum < N_NODES){
		if(setPointNodeNum < N_NODES-1){
			target=T[setPointNodeNum+1];
			}
		else if(setPointNodeNum == N_NODES-1){
			target = HPWH_setpoint;
			} 
		
		deltaT = target - T[setPointNodeNum];
		
		//heat needed to bring all equal temp. nodes up to the temp of the next node
		Q=m*(setPointNodeNum - floorNode + 1)*CPWATER*deltaT;
		
		//Running the rest of the time won't recover
		if(Q > max){
			for(j=floorNode; j <= setPointNodeNum; j++){
				T[j] += max/CPWATER/m/(setPointNodeNum - floorNode + 1);
				}
			runtime=timestep;
			max=0;
			}
		//temp will recover by/before end of timestep
		else{
			for(j=floorNode; j <= setPointNodeNum; j++){
				T[j] = target;
				}
			
			//calculate how long the element was on, in minutes (should be less than 1...), based on the power of the resistor and the required amount of heat
			runtime += Q/(ELEMENT_UPPER*3.412/60);

			max -= Q;
			++setPointNodeNum;
			}
		}
	return runtime;
}	// HPWH::add_upper



double HPWH::add_lower( double timestep)
{
	double Q, deltaT, target, max, runtime;
	int i, j, setPointNodeNum;
	i=0; j=0;
	runtime = 0;
	
	//find the first node (from the bottom) that does not have the same temperature as the one above it
	//if they all have the same temp., use the top node, N_NODES-1
	setPointNodeNum = 0;
	for(i = 0; i < N_NODES-1; i++){
		if(T[i] != T[i+1]){
			break;
			}
		else{
			setPointNodeNum = i+1;
			}
		}	
	

	//maximum heat deliverable in this timestep
	max=timestep/60*ELEMENT_LOWER*3.412;
	
	while(max > 0 && setPointNodeNum < N_NODES){
		//if the whole tank is at the same temp, the target temp is the setpoint
		if(setPointNodeNum == (N_NODES-1)){
			target = HPWH_setpoint;
			} 
		//otherwise the target temp is the first non-equal-temp node
		else{
			target = T[setPointNodeNum+1];
			}
		
		deltaT = target-T[setPointNodeNum];
		
		//heat needed to bring all equal temp. nodes up to the temp of the next node
		Q=m*(setPointNodeNum+1)*CPWATER*deltaT;

		//Running the rest of the time won't recover
		if(Q > max){
			for(j=0;j<=setPointNodeNum;j++){
				T[j] += max/CPWATER/m/(setPointNodeNum+1);
				}
			runtime=timestep;
			max=0;
			}
		//temp will recover by/before end of timestep
		else{
			for(j=0; j <= setPointNodeNum; j++){
				T[j]=target;
				}
			++setPointNodeNum;
			//calculate how long the element was on, in minutes (should be less than 1...), based on the power of the resistor and the required amount of heat
			runtime += Q/(ELEMENT_LOWER*3.412/60);
			max -= Q;
			}
		}

	return runtime;
}	// HPWH::add_lower



//Update the three temperatures in the tank at the start of a STEP...start of a 20 minute interval.  This accounts for the draw and the standby losses
int HPWH::updateHPWHtemps( double add, double inletT, double Ta, double timestep)
{
	static int count = 0;
	double frac, losses, temp, a;
	int n = 0, fl = 0, bn = 0, s = 0;

	count++;

	frac = add / (HPWH_size / N_NODES);		//fractional number of nodes drawn

	
	fl=(int)floor(frac);
	
	//if you don't subtract off the integer part of frac it 
	if(frac > 1){
		frac -= fl;
		}
	
	
	for(n=N_NODES-1; n > fl; n--){
		T[n] = frac * T[n-fl-1] + (1-frac) * T[n-fl];
		}
	T[fl] = frac*inletT + (1 - frac)*T[0];

	//fill in completely emptied nodes at the bottom of the tank
	for(n=0; n < fl; n++){
		T[n] = inletT;
		}


	//Account for mixing at the bottom of the tank
	if(add > 0){
		bn = N_NODES/3;
		a = ave(T,bn);
		for(s=0; s < bn; s++){
			T[s] += ((a-T[s]) / 3);
			}
		}


	temp = (temp_sum(T)/N_NODES-Ta)*(UA)/(60/timestep);	//btu's lost as standby in the current time step
	standbysum += temp;

	losses = temp/N_NODES/m/CPWATER;	//The effect of standby loss on temperature in each segment
	for(n=0;n<N_NODES;n++) T[n] -= losses;

	return 0;
}	// HPWH::updateHPWHtemps

//Capacity and input function.  Based on tank temperature and ambient dry bulb
double HPWH::HPWHcap( double Ta, double *input, double *cop){
	double cap = 0, TTank = 0;
	double cop_T1 = 0, cop_T2 = 0;    			//cop at ambient temperatures T1 and T2
	double input_T1 = 0, input_T2 = 0;			//input power at ambient temperatures T1 and T2	
	double cop_interpolated = 0, input_interpolated = 0;  //temp variables to store interpolated values
	TTank = condensertemp();


	cop_T1 = cop_constant_T1 + cop_linear_T1*TTank + cop_quadratic_T1*TTank*TTank;
	cop_T2 = cop_constant_T2 + cop_linear_T2*TTank + cop_quadratic_T2*TTank*TTank;
			
	input_T1 = input_constant_T1 + input_linear_T1*TTank + input_quadratic_T1*TTank*TTank;
	input_T2 = input_constant_T2 + input_linear_T2*TTank + input_quadratic_T2*TTank*TTank;

	cop_interpolated = cop_T1 + (Ta - curve_T1)*((cop_T2 - cop_T1)/(curve_T2 - curve_T1));
	
	//printf("cop_T1 : %f\n", cop_T1);
	//printf("cop_T2 : %f\n", cop_T2);
	//printf("Ta : %f\n", Ta);
	//printf("TTank : %f\n", TTank);
	//printf("Ta - curve_T1 : %f\n", Ta - curve_T1);
	//printf("(cop_T2 - cop_T1) : %f\n", (cop_T2 - cop_T1));
	//printf("(curve_T2 - curve_T1) : %f\n", (curve_T2 - curve_T1));
	//printf("(cop_T2 - cop_T1)/(curve_T2 - curve_T1) : %f\n", (cop_T2 - cop_T1)/(curve_T2 - curve_T1));
	//printf("(Ta - curve_T1)*((cop_T2 - cop_T1)/(curve_T2 - curve_T1)) : %f\n", (Ta - curve_T1)*((cop_T2 - cop_T1)/(curve_T2 - curve_T1)));
	//printf("cop_interpolated : %f\n", cop_interpolated);
	//printf("\n");
	input_interpolated = input_T1 + (Ta - curve_T1)*((input_T2 - input_T1)/(curve_T2 - curve_T1));


	//here is where the scaling for flow restriction goes
	//the input power doesn't change, we just scale the cop by a small percentage that is based on the ducted flow rate
	//the equation is a fit to three points, measured experimentally - 12 percent reduction at 150 cfm, 10 percent at 200, and 0 at 375
	//it's slightly adjust to be equal to 1 at 375
	if(ductedm != 0){
		cop_interpolated *= 0.00056*fanFlow + 0.79;
		}
		
	//printf("fanFlow : %f\n", fanFlow);	
	//printf("0.00056*fanFlow + 0.79 : %f\n", 0.00056*fanFlow + 0.79);
	//printf("cop_interpolated : %f\n", cop_interpolated);
	//printf("\n");
	//printf("curve_T1 : %f   \t   curve_T2 : %f   \t   Ta : %f\n",curve_T1, curve_T2, Ta);
	//printf("cop_T1 : %f   \t   cop_T2 : %f   \t   cop_interpolated : %f\n\n", cop_T1, cop_T2, cop_interpolated);
	cap = cop_interpolated * input_interpolated * 3412;	//Capacity in btu/hr
	*input = input_interpolated * 1000;	//Input in Watts
	*cop = cop_interpolated;

	return cap;
}	// HPWH::HPWHcap



/*Function to add heat pump heat to the water.*/
double HPWH::addheatcomp( double timestep, double cap)
{
	double temp, frac, accum, C[N_NODES], tdiff;
	int n;
	frac=1;	
	accum=0;

	for(n=0;n<N_NODES;n++)
	{
		C[n]=f((T[n]-T[0])/shrinkage);
	
		tdiff = HPWH_setpoint - T[n];
		//if node temperature above the setpoint temperature DO NOT add any heat to that node
		//note this is a kludge. a potentially better idea is to use the top node temperature instead of the 
		//setpoint temperature to calculate the temperature difference. Implement some other time. 
		if (tdiff<=0) {
			//setting to zero is bad later. Setting to a small, non-negative, non-zero, real number
			C[n] = 0.0000000000000001;
		}
		else {
			C[n] = C[n] * pow(tdiff,p);
		}
	}
	normalize(C);
	for(n=N_NODES-1;n>=0;n--)
	{
		temp=(C[n]*cap*timestep/60/m/CPWATER);		//Degrees that can be added to this node
		if( (T[n]+temp) > HPWH_setpoint )		//If that would put it over the setpoint
		{
			frac-=((HPWH_setpoint-T[n])/temp*C[n]);	//Tabulate how much is being not used
			accum+=((HPWH_setpoint-T[n])/temp*C[n]);	//Units of accum are degrees F
			T[n] = HPWH_setpoint;			//and put it at setpoint
			}
		else{
			T[n]+=temp;			//otherwise add all of it
			if( (T[n]+accum) > HPWH_setpoint )	//can't finish the accumulating
			{
				accum -= (HPWH_setpoint - T[n]);
				frac += (HPWH_setpoint - T[n]);
				T[n] = HPWH_setpoint;
			} else
			{
				T[n]+=accum;
				accum=0;
				frac=1;
			}
		}
	}


	return frac*timestep;
	//This is runtime
}	// HPWH::addheatcomp


int HPWH::simulateHPWH(
	double *inletTschedule,
	double *ambientTschedule,
	double *drawSchedule,
	int minutesToRun,
	int minute_of_year, 
	double* T_sum, 
	int *H_count,
	FILE *debugFILE)

	/*EXPLAINING THE FUNCTION CONCEPTUALLY
		This function progresses the state of the HPWH forward in time.  It uses 1 minute intervals, the number of which are specified by minutesToRun.
		Originally the variable "timestep" was intended to be settable, to change from 1 minute intervals to other, but now it should not be altered.
		Changing timestep might still work, with appropriate draw schedules, but it would require careful testing.
		Since the input draw schedules are in 1 minute increments, any changes to the timestep require new drawschedules as well.  
		The function loops through those 1 minute intervals and 
		at each timestep 1) updates the hpwh temp for draw (if there) and standby losses.  2) Check to see which components 
		are/should be running.  3) Add heat accordingly.  
	*/

{
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

	
	double simTCouples[6];
	
	
	//these are summed outside of HPWH
	resruntime = 0;
	resKWH = 0;
	compruntime = 0;
	energyremoved = 0;
	standbyLosses = 0;

	

	//the loop over all steps
	for(minute=0 ; minute < minutesToRun; minute++){
		timeRemaining = timestep;
		comp_runtime = 0;
		res_runtime_upper = 0;
		res_runtime_lower = 0;

		
		//set the inletT from the schedule
		//--------------------------------
		inletT = inletTschedule[minute];
		

		//set the draw from the schedule
		//------------------------------
		draw = drawSchedule[minute];

		
		//set the ambient T from the schedule
		//-----------------------------------
		Ta = ambientTschedule[minute];
		//the location temperature is Ta, unless you're doing unducted interior (and non-resistance)
		if(location == 4 && ductedm == 0 && tier != 4){
			//leaving locationTemperature as it is is the right move
			//but set the goal as the current ambient 
			temperatureGoal = Ta;
			}
		else{
			locationTemperature = Ta;
			}







		//if(DEBUG == 1) printf("\tmoy : %d\n", minute_of_year);
		minute_of_year++;


		//to find average outlet temperature, average top node when there's a draw
		if(draw > 0){
			*T_sum += T[N_NODES-1];
			(*H_count)++;
			}
			
		updateHPWHtemps( draw, inletT, locationTemperature, timeRemaining);

		//the logic loop, which decides how to turn on the hpwh, and then does it
		switch(tier){
			case 1:
				switch(type){
					case 1:		//GE
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
							}
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the field measured data, 
						//so that the compressor stays off when it drives its own ambient space temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
						
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  //do nothing - GE will not return to compressor until the tank is hot if an element has started to run
							  //compressorStatus =1;
							  //lowerElementStatus =0;
							  }
						}

				
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						//if we are supposed to be adding heat to the tank, status will be greater than zero
						if(s > 0){
							//In upper resistance heating mode
							if(upperElementStatus == 1){		
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating, GE uses lower resistor to finish what the upper started in the case of high draws 
									//(which we interpret as low temperature in bottom of tank)
									//bottom_third low temperature check. Increase to use more resistance heat. 
									

									// 68.1F was the number BL & NK found to best match lab tests (2015-05-08)
									if(bottom_third() < 90.0 || locationTemperature < lowTempCutoff){
										lowerElementStatus = 1;						
										}
									else{
										compressorStatus = 1;
										}
									  }
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								timeRemaining -= res_runtime_lower;
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; // end tier 1, type 1, GE



					case 2:		//Voltex
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
							}
						
						
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
						//so that the compressor stays off when it drives its own temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  compressorStatus =1;
							  lowerElementStatus =0;
							  }
							
							
						  }
						
				
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating
									if(locationTemperature > lowTempCutoff){
										//Turn on the compressor
										compressorStatus = 1;
										}
									else{
										//lower resistor activates
										lowerElementStatus = 1;						
										}
									}
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								timeRemaining -= res_runtime_lower;
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; // end tier 1, type 2, Voltex			


					
					default:
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
						  }
							
						
						
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
						//so that the compressor stays off when it drives its own temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  compressorStatus =1;
							  lowerElementStatus =0;
							  }
							}
			
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating
									if(locationTemperature > lowTempCutoff){
										//Turn on the compressor
										compressorStatus = 1;
										}
									else{
										//lower resistor activates
										lowerElementStatus = 1;						
										}
									}
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								timeRemaining -= res_runtime_lower;
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; //default, untyped hpwh
					}	//end tier 1 type switch
				break; // end tier 1

			case 2: 
				switch(type){
					case 1: 	//ATI - has no lower element, just shuts off when it's too cold
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//nothing, would normally be lower element
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
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
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //just shut off compressor, no lower element to turn on
							  compressorStatus =0;
							  }
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  //do nothing here, the lower resistor doesn't exist and so will never be on anyways
							  }
							}
				
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating
									if(locationTemperature > lowTempCutoff){
										//Turn on the compressor
										compressorStatus = 1;
										}
									else{
										//no lower resistor
										}
									}
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								//none
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; // end tier 2, type 1, ATI				

					case 2:		//GE502014 - hybrid mode - uses same logic as tier 1, just different numbers
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
							}
						
						
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
						//so that the compressor stays off when it drives its own temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
						
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  //do nothing - GE will not return to compressor until the tank is hot if an element has started to run
							  //compressorStatus =1;
							  //lowerElementStatus =0;
							  }
							
							
							}
				
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating, GE only uses lower resistor to finish what the upper started
									//lower resistor activates
									lowerElementStatus = 1;						
									}
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								timeRemaining -= res_runtime_lower;
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; // end tier 2, type 2, GE502014 in hybrid mode

					default:
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
							}
						
						
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
						//so that the compressor stays off when it drives its own temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  compressorStatus =1;
							  lowerElementStatus =0;
							  }
							
							
							}
				
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating
									if(locationTemperature > lowTempCutoff){
										//Turn on the compressor
										compressorStatus = 1;
										}
									else{
										//lower resistor activates
										lowerElementStatus = 1;						
										}
									}
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining, cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								timeRemaining -= res_runtime_lower;
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; // end tier 2, default
					} //end tier 2 type switch
				break; //end tier 2

			case 3: //tier 3
				switch(type){
					case 1:  //GE502014 - cold climate mode - turns off upper resistance after heating to 120, unless lower tank is too cold.  also, runs lower element and compressor together
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
							}
						
						
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
						//so that the compressor stays off when it drives its own temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
						
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  //do nothing - GE will not return to compressor until the tank is hot if an element has started to run
							  //compressorStatus =1;
							  //lowerElementStatus =0;
							  }
							
							//the new strategy - if the upper resistance heats the top to 120 (irregardless of the setpoint), switch back to compressor
							else if(upperElementStatus == 1 && locationTemperature > lowTempCutoff && top_third() >= 120){
								if(bottom_third() < 65.0){
									//lower element - this helps to simulate the "high draw" resorting to resistance heating seen in lab tests
									//also uses compressor to boost efficiency
									compressorStatus = 1;
									upperElementStatus = 0;
									lowerElementStatus = 1;
									}
								else{
									//compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}	
								}
							}
				
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);

						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating, GE only uses lower resistor to finish what the upper started
									//lower resistor activates
									lowerElementStatus = 1;	
									//the cold climate mode also runs the compressor at the same time
									if(locationTemperature > (lowTempCutoff + cutoffTempHysteresis)){
										compressorStatus = 1;
										}
									}
								timeRemaining -= res_runtime_upper;	
								}
							
							//to allow compressor and resistor to run concurrently, the timeRemaining must be subtracted after they both get a chance to run
							//this will lead to negative timeRemaining, but that is probably ok...
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								}
							timeRemaining -= comp_runtime;
							timeRemaining -= res_runtime_lower;
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							}
						break; // end tier 3 type 1, GE502014 cold climate mode
					
					default:
						if(top_third() < (HPWH_setpoint - res_start)){
							//Upper resistance element activates.  others shut off
							compressorStatus =0;
							upperElementStatus =1;
							lowerElementStatus =0;
							}
						//only check for these if there is no heating element already running
						else if(compressorStatus == 0 && upperElementStatus == 0 && lowerElementStatus == 0){
							
							 //these two are if the bottom of the tank is cold
							if( bottom_third() < (HPWH_setpoint - comp_start) ){
								if(locationTemperature > lowTempCutoff){
									//Compressor Activates
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;
									}
								}
							else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres)){
								//Standby recovery when top of tank falls more than standby_thres degrees
								if(locationTemperature > lowTempCutoff){
									//Turn on the compressor
									compressorStatus =1;
									upperElementStatus =0;
									lowerElementStatus =0;						
									}
								else{
									//lower resistor activates
									compressorStatus =0;
									upperElementStatus =0;
									lowerElementStatus =1;						
									}
								}
							}
						
						
						//if something was already running, decide whether to keep compressor or resistor on
						//probably uneccessary to check if upper resistor is off, but I'm doing it anyways
						//the cutoffTempHysteresis variable provides a hysteresis effect, as seen in the data, 
						//so that the compressor stays off when it drives its own temperature down
						else{
							//Low temperature cutoff - if the compressor is on and temp. is too low, switch to lower resistor
							if(compressorStatus == 1 && locationTemperature  < (lowTempCutoff - cutoffTempHysteresis)  && upperElementStatus == 0){
							  //shut off compressor, turn on lower element
							  compressorStatus =0;
							  lowerElementStatus =1;
							  }
							//however, if the resistor is still on but the temperature has come up, turn off resistor and turn on compressor
							else if(lowerElementStatus == 1 && locationTemperature > (lowTempCutoff + cutoffTempHysteresis) && upperElementStatus == 0){
							  compressorStatus =1;
							  lowerElementStatus =0;
							  }
							
							
							}
			
						s = compressorStatus + upperElementStatus + lowerElementStatus;
						cap = HPWHcap( locationTemperature, &input, &cop);
						
						if(s>0){
							if(upperElementStatus == 1){		//In upper resistance heating mode
								res_runtime_upper = add_upper( timeRemaining);
								if(res_runtime_upper < timeRemaining){	//If finished heating in resistance mode
									//turn off resistor
									upperElementStatus = 0;
									//finish heating
									if(locationTemperature > lowTempCutoff){
										//Turn on the compressor
										compressorStatus = 1;
										}
									else{
										//lower resistor activates
										lowerElementStatus = 1;						
										}
									}
								timeRemaining -= res_runtime_upper;	
								}
							//In compressor heating mode	
							if(compressorStatus == 1){
								comp_runtime = addheatcomp( timeRemaining,cap);
								if(comp_runtime < timeRemaining){	//The compressor finished
									compressorStatus = 0;
									}
								timeRemaining -= comp_runtime;
								}
							//lower element?
							if(lowerElementStatus == 1){
								res_runtime_lower = add_lower( timeRemaining);
								//if finished, turn off element
								if(res_runtime_lower < timeRemaining){
									lowerElementStatus = 0;
									}
								timeRemaining -= res_runtime_lower;
								}
							
							
							if(timeRemaining > 0){
								//finish running in this step?  everything off
								compressorStatus =0;
								upperElementStatus =0;
								lowerElementStatus =0;
								}
							
							} //end if(s > 0)
						break;  //end default					

					
					}	//end tier 3 type switch
				break;  //end tier 3
			
			case 4:			//This is the resistance heater
			
				if(top_third() < (HPWH_setpoint - res_start)){
					//Turn on the upper element
					compressorStatus =0;
					upperElementStatus =1;
					lowerElementStatus =0;
					} 
				else if(bottom_third() < (HPWH_setpoint - comp_start)){
					//Turn on the lower element.  Note that I'm using comp_start here.  Deal with it
					compressorStatus =0;
					upperElementStatus =0;
					lowerElementStatus =1;
					}
				else if( T[N_NODES-1] < (HPWH_setpoint - standby_thres) && compressorStatus == 0 && upperElementStatus ==0 && lowerElementStatus ==0){
					//Standby recovery when top of tank falls more than standby_thres degrees
					compressorStatus =0;
					upperElementStatus =0;
					lowerElementStatus =1;
					}
				
				s = compressorStatus + upperElementStatus + lowerElementStatus;
				if(s > 0){
					//Upper element recovery
					if(upperElementStatus ==1){	
						res_runtime_upper = add_upper( timeRemaining);
						//if upper element is finished, turn it off
						if(res_runtime_upper < timeRemaining) {
							upperElementStatus = 0;
							//if the upper was on, you'll probably want to heat some with the lower while you're at it
							lowerElementStatus = 1;
							}
						timeRemaining -= res_runtime_upper;
						}
					
					//lower element?
					if(lowerElementStatus == 1){
						res_runtime_lower = add_lower( timeRemaining);
						//if finished, turn off element
						if(res_runtime_lower < timeRemaining){
							lowerElementStatus = 0;
							}
						timeRemaining -= res_runtime_lower;
						}

					if(timeRemaining > 0){
						//finish running in this step?  everything off
						compressorStatus =0;
						upperElementStatus =0;
						lowerElementStatus =0;
						}
					}
				break;

		}

		//summing variables at the end
		inputCompKWH = (comp_runtime/60.0)*(input/1000.0);
		outputCompBTU = (comp_runtime/60.0)*cap;

		inputResKWH = (res_runtime_upper/60.0)*((ELEMENT_UPPER)/1000.0) + (res_runtime_lower/60.0)*((ELEMENT_LOWER)/1000.0);
		
		
		//if not ducted, energy comes from the air
		if(ductedm == 0){
			//heat from compressor minus energy input into compressor as electricity, converted to btu
			energyremoved += outputCompBTU - inputCompKWH*3412.0;
			}
		//if it is ducted, the air is being replaced from outside, and the infiltration model handles it
		else{
			energyremoved = 0.0;
			}
		

		//heat moving from tank to space
		standbyLosses += (ave(T, N_NODES) - locationTemperature) * ((UA)*(timestep/60.0));

		
		HPWHadded += (outputCompBTU/3412.0 + inputResKWH);

		inputenergy += inputCompKWH + inputResKWH;

		//these will be total summed outside of simulateHPWH, but here each minute is summer over the whole step period
		compruntime += comp_runtime;
		resruntime += res_runtime_upper + res_runtime_lower;
		resKWH += inputResKWH;
	
	
		//this is the section for exponential temperature changes in unducted interior spaces - NDK 7/2014
		//adjusting the location temperature as the last step in the simulation; temperature doesn't change until it's run for a minute...makes sense to me
		if(ductedm == 0 && location == 4 && tier != 4){
			//if the compressor's on, we're cooling
			if(compressorStatus == 1){
				temperatureGoal = Ta - 4.5;		//hardcoded 4.5 degree total drop - from experimental data
				}
			//otherwise, we're going back to ambient
			else{
				temperatureGoal = Ta;
				}
			
			//shrink the gap by the same percentage every minute - that gives us exponential behavior
			//the percentage was determined by a fit to experimental data - 9.4 minute half life and 4.5 degree total drop
			//minus-equals is important, and fits with the order of locationTemperature and temperatureGoal, so as to not use fabs() and conditional tests
			locationTemperature -= (locationTemperature - temperatureGoal)*(1 - 0.9289);
			}
		
		if(TEST == 1){
				
				//do averaging of node pairs (or triplets, etc.) to simulate the thermocouples used in lab tests
				if(N_NODES%6 != 0){
					printf("Must have a multiple of 6 nodes in order to average them to match lab tests.\n");
					return 1;
					}
				for(i = 0; i < 6; i++){				//loop over simulated lab-test thermocouples
					simTCouples[i] = 0;
					for(j = 0; j < N_NODES/6; j++){			//loop over the nodes that make up each "thermocouple"
						simTCouples[i] += T[(i * N_NODES/6) + j];
						}
					simTCouples[i] /= N_NODES/6;
					}
				
				fprintf(debugFILE, "%d,%d,%d,%d,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f,%f\n", minute_of_year,  
							compressorStatus , upperElementStatus , lowerElementStatus, Ta, inputCompKWH, inputResKWH, draw, inletT, 	
							T[0], T[1], T[2], T[3], T[4], T[5], T[6], 
							T[7], T[8], T[9], T[10], T[11],  
							simTCouples[0], simTCouples[1], simTCouples[2], simTCouples[3], simTCouples[4], simTCouples[5], cop);
				}
		if(DEBUG == 1)	fprintf(debugFILE, "%d %d %d %d %f %f %f %f %f %f %f %f %f %f %f\n", minute_of_year, compressorStatus , upperElementStatus , lowerElementStatus, locationTemperature, Ta, inputCompKWH + inputResKWH, draw, inletT, T[0], T[2], T[4], T[6], T[8], T[10]);
	}


	return 0;
}		// HPWH::

