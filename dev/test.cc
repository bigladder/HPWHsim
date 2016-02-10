/*This is not a substitute for a proper HPWH Test Tool, it is merely a short program
 * to aid in the testing of the new HPWH.cc as it is being written.
 * 
 * -NDK
 * 
 * 
 */
#include "HPWH.hh"
#include <sstream>

#define F_TO_C(T) ((T-32.0)*5.0/9.0)
#define GAL_TO_L(GAL) (GAL*3.78541)

using std::cout;
using std::endl;
using std::string;

void printTankTemps(HPWH& hpwh);



int main(void)
{


HPWH hpwh;
int initReturn = 0;


//hpwh.setVerbosity(HPWH::VRB_emetic);

hpwh.HPWHinit_file("testInput.txt");


//return(0);


//initReturn = hpwh.HPWHinit_presets(1);

//int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
					//double ambientT_C, double externalT_C,
					//double DRstatus, double minutesPerStep)
//hpwh.runOneStep(0, 120, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 5, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 10, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, 0, 50, 50, 1, 1);
////hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.setVerbosity(HPWH::VRB_emetic);
//hpwh.setVerbosity(HPWH::VRB_typical);
//hpwh.setVerbosity(HPWH::VRB_reluctant);
//hpwh.setVerbosity(HPWH::VRB_silent);

//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_restankNoUA);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_restankHugeUA);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_restankRealistic);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_externalTest);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_basicIntegrated);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_Voltex60);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_Voltex80);
//initReturn = hpwh.HPWHinit_presets(HPWH::MODELS_GEGeospring);
//int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
					//double ambientT_C, double externalT_C,
					//double DRstatus, double minutesPerStep)


//cout << "Type of heatsource 1: " << hpwh.getNthHeatSourceType(0) << endl;
//cout << "Type of heatsource 2: " << hpwh.getNthHeatSourceType(1) << endl;
//cout << "Type of heatsource 3: " << hpwh.getNthHeatSourceType(2) << endl;



if (initReturn == HPWH::HPWH_ABORT) {
  return 1;
}

int minutes = 1; 
HPWH::DRMODES drStatus = HPWH::DR_ALLOW;

#define SHORT 

#ifdef SHORT
int liters = 20;
        
hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, liters, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.getTankNodeTemp(5);

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, liters, 0, 50, drStatus, minutes);
////hpwh.printTankTemps();
////hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, liters, 0, 50, drStatus, minutes);
////hpwh.printTankTemps();
////hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

//hpwh.runOneStep(0, liters, 0, 50, drStatus, minutes);
////hpwh.printTankTemps();
////hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 10, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(41.0), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(39.0), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(37), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(35), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(37), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(39.0), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(41.0), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(43.0), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();

hpwh.runOneStep(0, 0, 0, F_TO_C(45), drStatus, minutes);
//hpwh.printTankTemps();
//hpwh.printHeatSourceInfo();
#else


//initReturn = hpwh.HPWHinit_presets(3);
hpwh.setInletT(0);
hpwh.setMinutesPerStep(minutes);
for (int i = 0; i < 1440*365*20; i++) {
//for (int i = 0; i < 120000000; i++) {
    //hpwh.runOneStep(0, 0, 0, 50, drStatus, minutes);
    hpwh.runOneStep(0, 0, 50, drStatus);
}


//cout << sizeof(HPWH);

#endif

return 0;

}
