/*This is not a substitute for a proper HPWH Test Tool, it is merely a short program
 * to aid in the testing of the new HPWH.cc as it is being written.
 * 
 * -NDK
 *
 * Bring on the HPWH Test Tool!!! -MJL
 *
 * 
 * 
 */
#include "HPWH.hh"
#include <iostream>
#include <string.h>
#include <stdlib.h>

#define MAX_DIR_LENGTH 255
#define DEBUG 0

using std::cout;
using std::endl;
using std::string;


#define F_TO_C(T) ((T-32.0)*5.0/9.0)
#define GAL_TO_L(GAL) (GAL*3.78541)


char *dynamicstrcat(int n,...);
int readSchedule(double *scheduleArray, int scheduleLength, char *scheduleFileName, char *scheduleName, char *scheduleHeaders);

int main(int argc, char *argv[])
{
  HPWH hpwh;


  double *drawSchedule = NULL;
  double *DRSchedule = NULL;
  double *inletTschedule = NULL;
  double *ambientTschedule = NULL;
  double *evaporatorTschedule = NULL;

  double *heatSourcesEnergyInput, *heatSourcesEnergyOutput;
  double simTcouples[6];
  
  FILE *hpwhTestToolOutputFILE = NULL;
  FILE *inputFILE = NULL;

  char *testDirectory = NULL;  //no need to malloc, it will receive a strdup
  char *fileToOpen = NULL;    //also do not malloc, this gets the malloc from dynamicstrcat
  char *scheduleName = NULL, *scheduleHeaders = NULL;    //these two receive string literals, and so don't need to be malloced
  
  string inputVariableName;

  int i, j, minutesToRun;

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
  testDirectory = argv[1];
  
  /*drawSchedule = malloc(minutesToRun*sizeof(*drawSchedule));
  DRSchedule = malloc(minutesToRun*sizeof(*DRSchedule));
  inletTschedule = malloc(minutesToRun*sizeof(*inletTschedule));
  ambientTschedule = malloc(minutesToRun*sizeof(*ambientTschedule));
  evaporatorTschedule = malloc(minutesToRun*sizeof(*evaporatorTschedule));*/

  drawSchedule = new double[minutesToRun];
  DRSchedule = new double[minutesToRun];
  inletTschedule = new double[minutesToRun];
  ambientTschedule = new double[minutesToRun];
  evaporatorTschedule = new double[minutesToRun];
  
  if(drawSchedule == NULL || DRSchedule == NULL || inletTschedule == NULL || ambientTschedule == NULL || evaporatorTschedule == NULL){
    printf("One of the schedule arrays has failed to malloc.  Yikes, I'm out.\n");
    return 1;
    }
  
  
  // ------------------------------------- Read Schedules--------------------------------------- //
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

  

  // Open the output file and print the header
  fileToOpen = dynamicstrcat(3, testDirectory, "/", "hpwhTestToolOutput.csv");
  hpwhTestToolOutputFILE = fopen(fileToOpen,"w");
  fprintf(hpwhTestToolOutputFILE, "%s,%s,%s,%s,", "minute_of_year", "Ta", "inletT", "draw");
  for(i = 0; i < hpwh.getNumHeatSources(); i++) {
    fprintf(hpwhTestToolOutputFILE, "%s%d,%s%d,", "input_kJ", i + 1, "output_kJ", i + 1);
  }
  fprintf(hpwhTestToolOutputFILE, "%s,%s,%s,%s,%s,%s\n", "simTCouples1", "simTCouples2", "simTCouples3", "simTCouples4", "simTCouples5", "simTcouples6");




  // ------------------------------------- Simulate --------------------------------------- //

  // Set the hpwh properties
  hpwh.HPWHinit_presets(3);

  // Loop over the minutes in the test
  minutesToRun = 100; // Need to figure out how to read this...
  for(i = 0; i < minutesToRun; i++) {
    // Run the step
    hpwh.runOneStep(inletTschedule[i], drawSchedule[i], ambientTschedule[i], evaporatorTschedule[i], DRSchedule[i], 1.0);

    // ???
    // hpwh.getHeatSourcesEnergyInput(heatSourcesEnergyInput);
    // hpwh.getHeatSourcesEnergyOutput(heatSourcesEnergyOutput);
    // hpwh.getSimTcouples(simTcouples);

    // Copy current status into the output file
    fprintf(hpwhTestToolOutputFILE, "%d,%f,%f,%f,", i, ambientTschedule[i], inletTschedule[i], drawSchedule[i]);
    for(j = 0; j < hpwh.getNumHeatSources(); j++) {
      fprintf(hpwhTestToolOutputFILE, "%f,%f,", heatSourcesEnergyInput[j], heatSourcesEnergyOutput[j]);
    }
    fprintf(hpwhTestToolOutputFILE, "%f,%f,%f,%f,%f,%f\n", simTCouples[0], simTCouples[1], simTCouples[2], simTCouples[3], simTCouples[4], simTCouples[5]);


  }



//hpwh.HPWHinit_presets(1);

//int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
          //double ambientT_C, double externalT_C,
          //double DRstatus, double minutesPerStep)
//hpwh.runOneStep(0, 120, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();
//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
//hpwh.printTankTemps();

/*
hpwh.runOneStep(0, 10, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, -10, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, -10, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, -10, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(0, 0, 0, 50, 1, 1);
hpwh.printTankTemps();
*/


//hpwh.HPWHinit_presets(3);
//for (int i = 0; i < 1440*365; i++) {
    //hpwh.runOneStep(0, 0.2, 0, 50, 1, 1);
//}



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
  
