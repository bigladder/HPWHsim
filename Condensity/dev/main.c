/*   ***************************************************************************
 * hpwhTestTool - Copyright (C) <2015>  Ecotope, Inc.
 * 
 * This program was created around 4/9/2015 by Nick Kvaltine at Ecotope, with the
 * purpose of running Ecotope's hpwh simulation for a "single day" in order to 
 * provide more insight into how the simulation was matching up against lab tested data
 * 
 * It reads input from several files:
 * ambientTschedule.csv
 * evaporatorTschedule.csv
 * drawschedule.csv
 * DRschedule.csv
 * hpwhProperties.txt
 * inletTschedule.csv
 * 
 * 
 * 
 * It is specifically designed to simulate the entire range of a file in one go.  Some 
 * modifications would be required for it to run a partial file, or to run portions
 * of a file separately.  Notably the schedule arrays would need to be sliced
 * and the minute_of_year would need to be updated for each portion.  
 * 
 * Facilities are not provided to specify the initial state of the tank.  The tank temperature
 * is initialized to have all node temperatures at the setpoint on start.
 *  
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
*************************************************************************** */



#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h> 

#include "global.h"
#include "HPWH.h"


char *dynamicstrcat(int n,...);
int readSchedule(double *scheduleArray, int scheduleLength, char *scheduleFileName, char *scheduleName, char *scheduleHeaders);

int main(int argc, char* argv[]){
	//.......................................
	//variable declarations
	//.......................................
	double inputVariable, HPWH_setpoint, HPWH_size;
	double resUA;			//currently not used, the test only works for hpwh's at the moment, not resistance tanks
	
	int minute_of_year, HPWH_tier, HPWH_type, HPWH_fan, HPWH_location, HPWH_ductingType, minutesToRun, outputCode;
	int allValues = 0;
	
	double *T_sum = malloc(sizeof(*T_sum));
	int *H_count = malloc(sizeof(*H_count));
	
	HPWH *hpwh = malloc(sizeof(HPWH));
	
	double *drawSchedule = NULL;
	double *DRSchedule = NULL;
	double *inletTschedule = NULL;
	double *ambientTschedule = NULL;
	double *evaporatorTschedule = NULL;
	
	FILE *hpwhTestToolOutputFILE = NULL;
	FILE *inputFILE = NULL;

	char *testDirectory = NULL;	//no need to malloc, it will receive a strdup
	char *fileToOpen = NULL;		//also do not malloc, this gets the malloc from dynamicstrcat
	char *scheduleName = NULL, *scheduleHeaders = NULL;		//these two receive string literals, and so don't need to be malloced
	
	char *inputVariableName = malloc(MAX_DIR_LENGTH*sizeof(*inputVariableName));


	//.......................................
	//process command line arguments
	//.......................................	

	//Obvious wrong number of command line arguments
	if ((argc >=3)) {
		printf("Invalid input.  This program takes a single argument.  Help is on the way:\n\n");
		}
	//Help message
	if ((argc != 2) || ( *argv[1] == '?') || (strcmp(argv[1],"help") == 0)) {
		printf("Standard usage: \"hpwhTestTool.x test_directory\"\n");
		printf("All input files should be located in the test directory, with these names:\n");
		printf("drawschedule.csv DRschedule.csv ambientTschedule.csv evaporatorTschedule.csv inletTschedule.csv hpwhProperties.csv\n");
		printf("An output file, hpwhTestOutput.csv, will be written in the test directory\n");
		return 2;
		}



	//Only input file specified -- don't suffix with .csv
	testDirectory = strdup(argv[1]);
	

	

	//.......................................	
	//Read in input files
	//.......................................
	
	
	//first, the properties of the hpwh to be read in
	fileToOpen = dynamicstrcat(3, testDirectory, "/", "hpwhProperties.txt");
	inputFILE = fopen(fileToOpen,"r");
	if (inputFILE == NULL){
		perror("Error: ");
		return 1;
		}
	free(fileToOpen);  fileToOpen = NULL;
	
	while( fscanf(inputFILE, "%s %lf", inputVariableName, &inputVariable) != EOF){
		if(DEBUG == 1) printf("inputVariableName: %s, inputVariable: %lf\n", inputVariableName, inputVariable);

		if(strcmp(inputVariableName, "HPWH_tier") == 0){
			HPWH_tier = inputVariable;
			allValues++;
			}
		else if(strcmp(inputVariableName, "HPWH_type") == 0){
			HPWH_type = inputVariable;
			allValues++;
			}
		else if(strcmp(inputVariableName, "HPWH_size") == 0){
			HPWH_size = inputVariable;
			allValues++;
			}
		else if(strcmp(inputVariableName, "HPWH_setpoint") == 0){
			HPWH_setpoint = inputVariable;
			allValues++;
			}
		else if(strcmp(inputVariableName, "length_of_test") == 0){
			minutesToRun = inputVariable;
			allValues++;
			}
		else if(strcmp(inputVariableName, "HPWH_location") == 0){
			HPWH_location = inputVariable;
			allValues++;
			}
		else if(strcmp(inputVariableName, "HPWH_ductingType") == 0){
			HPWH_ductingType = inputVariable;
			allValues++;
			}
		else{
			printf("You broke it, way to go.  Seems there's something in the hpwhProperties file that doesn't belong.\n");
			return(1);
			}

		}

	if(allValues != 7){
		printf("allvaules = %d\n", allValues);
		printf("\n\nYou did not specify all of the variables for %s.  DOES NOT COMPUTE\n", testDirectory);
		return(1);
		}
	
	
	
	
	//print them out to confirm
	printf("HPWH_tier = %d\n", HPWH_tier);
	printf("HPWH_type = %d\n", HPWH_type);
	printf("HPWH_size = %0.lf gallons\n", HPWH_size);
	printf("HPWH_setpoint = %0.lf degrees F\n", HPWH_setpoint);
	printf("HPWH_location = %d\n", HPWH_location);
	printf("HPWH_ductingType = %d\n", HPWH_ductingType);
	printf("Length of test = %d minutes\n\n", minutesToRun);
	
	fclose(inputFILE);
	
	//now, let's get the schedules
	//---------------------------------------
	
	//malloc some space for all the schedules
	drawSchedule = malloc(minutesToRun*sizeof(*drawSchedule));
	DRSchedule = malloc(minutesToRun*sizeof(*DRSchedule));
	inletTschedule = malloc(minutesToRun*sizeof(*inletTschedule));
	ambientTschedule = malloc(minutesToRun*sizeof(*ambientTschedule));
	evaporatorTschedule = malloc(minutesToRun*sizeof(*evaporatorTschedule));
	if(drawSchedule == NULL || DRSchedule == NULL || inletTschedule == NULL || ambientTschedule == NULL || evaporatorTschedule == NULL){
		printf("One of the schedule arrays has failed to malloc.  Yikes, I'm out.\n");
		return 1;
		}
	
	
	
	//first, let's get the draw schedule
	//---------------------------------------
	scheduleName = "draw";
	scheduleHeaders = "minutes,flow";
	fileToOpen = dynamicstrcat(4, testDirectory, "/", scheduleName, "schedule.csv");

	outputCode = readSchedule(drawSchedule, minutesToRun, fileToOpen, scheduleName, scheduleHeaders);
	if(outputCode != 0){
		printf("readSchedule returns an error on %s schedule!\n", scheduleName);
		exit(outputCode);
		}
	free(fileToOpen);  fileToOpen = NULL;
	
	
	//now, let's get the DR schedule
	//---------------------------------------
	scheduleName = "DR";
	scheduleHeaders = "minutes,OnOff";
	fileToOpen = dynamicstrcat(4, testDirectory, "/", scheduleName, "schedule.csv");

	outputCode = readSchedule(DRSchedule, minutesToRun, fileToOpen, scheduleName, scheduleHeaders);
	if(outputCode != 0){
		printf("readSchedule returns an error on %s schedule!\n", scheduleName);
		exit(outputCode);
		}
	free(fileToOpen);  fileToOpen = NULL;
	
	
	//next, let's get the inletT schedule
	//---------------------------------------
	scheduleName = "inletT";
	scheduleHeaders = "minutes,temperature";
	fileToOpen = dynamicstrcat(4, testDirectory, "/", scheduleName, "schedule.csv");

	outputCode = readSchedule(inletTschedule, minutesToRun, fileToOpen, scheduleName, scheduleHeaders);
	if(outputCode != 0){
		printf("readSchedule returns an error on %s schedule!\n", scheduleName);
		exit(outputCode);
		}
	free(fileToOpen);  fileToOpen = NULL;


	//penultimately, let's get the ambientT schedule
	//---------------------------------------
	scheduleName = "ambientT";
	scheduleHeaders = "minutes,temperature";
	fileToOpen = dynamicstrcat(4, testDirectory, "/", scheduleName, "schedule.csv");

	outputCode = readSchedule(ambientTschedule, minutesToRun, fileToOpen, scheduleName, scheduleHeaders);
	if(outputCode != 0){
		printf("readSchedule returns an error on %s schedule!\n", scheduleName);
		exit(outputCode);
		}
	free(fileToOpen);  fileToOpen = NULL;


	//ultimately, let's get the evaporatorT schedule
	//---------------------------------------
	scheduleName = "evaporatorT";
	scheduleHeaders = "minutes,temperature";
	fileToOpen = dynamicstrcat(4, testDirectory, "/", scheduleName, "schedule.csv");

	outputCode = readSchedule(evaporatorTschedule, minutesToRun, fileToOpen, scheduleName, scheduleHeaders);
	if(outputCode != 0){
		printf("readSchedule returns an error on %s schedule!\n", scheduleName);
		exit(outputCode);
		}
	free(fileToOpen);  fileToOpen = NULL;

	




	//.......................................	
	//Initializations not drawn from input files
	//.......................................
	
	//thou shalt not test resistance water heaters
	resUA = 0;

	//lab tests shall not adjust fan speed
	HPWH_fan = 0;
	//HPWH_fan = 375;	
	
	//thou shalt start the test at minute zero
	minute_of_year = 0;
	
	//these are for getting the average outlet temperature
	*T_sum = 0;
	*H_count = 0;
		
	
	
	//open the output file
	fileToOpen = dynamicstrcat(3, testDirectory, "/", "hpwhTestToolOutput.csv");
	hpwhTestToolOutputFILE = fopen(fileToOpen,"w");
	//printf the output header to file
	fprintf(hpwhTestToolOutputFILE, "%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n", \
						"minute_of_year", "compressorStatus ", "upperElementStatus ", "lowerElementStatus", "Ta", \
						"inputCompKWH", "inputResKWH", "draw", "inletT", "T[0]", "T[1]", "T[2]", "T[3]", "T[4]", "T[5]", "T[6]", \
						"T[7]", "T[8]", "T[9]", "T[10]", "T[11]", \
						"simTCouples0", "simTCouples1", "simTCouples2", "simTCouples3", "simTCouples4", "simTCouples5", "COP");



	//now that we have all the variables, initialize the hpwh
	outputCode = HPWH_init(hpwh, HPWH_type, HPWH_tier, HPWH_size, HPWH_setpoint, HPWH_location, HPWH_ductingType, HPWH_fan, resUA);
	if(outputCode != 0){
		printf("HPWH_init returns an error!\n");
		exit(outputCode);
		}



	//.......................................
	//Commence simulating!
	//.......................................

	outputCode = simulateHPWH(hpwh, inletTschedule, ambientTschedule, evaporatorTschedule, drawSchedule, DRSchedule, minutesToRun, minute_of_year, T_sum, H_count, hpwhTestToolOutputFILE);
	if(outputCode != 0){
		printf("simulateHPWH returns an error!\n");
		exit(outputCode);
		}

	
	//.......................................
	//Simulation Complete!
	//.......................................

	printf("\n\n\nSimulation Complete...\n\n");
	printf("Average Outlet T: %.1f\n", *T_sum / *H_count);
	
	
	
	
	fclose(hpwhTestToolOutputFILE);
	
	free(T_sum); 
	free(H_count);
	
	HPWH_free(hpwh);
	free(hpwh);
	
	free(drawSchedule);
	free(DRSchedule);
	free(inletTschedule);
	free(ambientTschedule);
	free(evaporatorTschedule);
		
	free(testDirectory);
	free(fileToOpen);
	free(inputVariableName);


	return 0;
	
	}
	
	
	
	
	
	
	
	
//concatenate strings - there's apparently not a good library function to do this in C
char *dynamicstrcat(int n,...) {
  char *newstr;
  int sum, i;
  va_list ap;

  sum=0;
  va_start(ap,n);

  //Go through once to determine the final length
  for(i=0;i<n;i++)
    sum+=strlen(va_arg(ap,char *));
  sum++;
  va_end(ap);

  //Allocate memory for new, concatenated string
  newstr=(char *) malloc(sum);
  if(newstr == NULL){
	  printf("Error: malloc returns NULL in dynamicstrcat\n");
	  return NULL;
	  }
  //Go through again to concatenate
  va_start(ap,n);
  strcpy(newstr,va_arg(ap,char *));
  for(i=1;i<n;i++)
    strcat(newstr,va_arg(ap,char *));
  va_end(ap);

  //Return the new string
  return newstr;
}


//this function reads the named schedule into the provided array
int readSchedule(double *scheduleArray, int scheduleLength, char *scheduleFileName, char *scheduleName, char *scheduleHeaders){
	int i;
	double inputVariable, inputVariable2, defaultVal;
	
	char *inputVariableName = malloc(MAX_DIR_LENGTH*sizeof(*inputVariableName));
	
	FILE *inputFILE = NULL;


	//open the schedule file provided
	printf("Opening %s\n", scheduleFileName);
	inputFILE = fopen(scheduleFileName,"r");
	if (inputFILE == NULL){
		perror("Error: ");
		return 1;
		}
			
	//check for contents - it should not be empty, and begin with the default value
	if(fscanf(inputFILE, "%s %lf", inputVariableName, &inputVariable) == EOF){
		printf("The %s file is empty.  Please use your words.\n", scheduleFileName);
		return 1;
		}
	else if(strcmp(inputVariableName, "default") != 0){
		printf("The %s file should begin with \"default\".  \nSelf Destruct in \n3...\n", scheduleFileName);
		printf("2...\n");
		printf("1...\n");
		return 1;
		}
	else{
		defaultVal = inputVariable;
		printf("Default %s value is: %0.1lf\n", scheduleName, defaultVal);
		}	
	
	//fill schedule array with its default value
	for(i = 0; i < scheduleLength; i++){
		scheduleArray[i] = defaultVal;
		}
	
	
	//read in header line and check it for validity
	if(fscanf(inputFILE, "%s", inputVariableName) == EOF || strcmp(inputVariableName, scheduleHeaders) != 0 ){
		printf("\n\nThe %s file is missing headers.  Please be more specific.\n", scheduleFileName);
		return 1;
		}


	//at last, put the schedule in to the array
	while(fscanf(inputFILE, "%lf,%lf", &inputVariable, &inputVariable2) != EOF){
		//if(DEBUG == 1) printf("inputVariable: %0.2lf   inputVariable2: %0.2lf\n", inputVariable, inputVariable2);
		if((int)inputVariable - 1 > scheduleLength){
			printf("Warning:  This schedule has more values than there are minutes of test\n");
			break;
			}
		
		scheduleArray[(int)inputVariable - 1] = inputVariable2;
		}

	//print out the whole schedule
	if(DEBUG == 1){
		for(i = 0; i < scheduleLength; i++){
			printf("scheduleArray[%d] = %0.1lf\n", i, scheduleArray[i]);
			}
		}
	
	
	printf("\n");
	fclose(inputFILE);
	free(inputVariableName);

	return 0;
	
	}
	
