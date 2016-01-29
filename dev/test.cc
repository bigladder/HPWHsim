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

//hpwh.HPWHinit_presets(1);

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

//hpwh.HPWHinit_presets(HPWH::MODELS_restankNoUA);
//hpwh.HPWHinit_presets(HPWH::MODELS_restankHugeUA);
//hpwh.HPWHinit_presets(HPWH::MODELS_restankRealistic);
//hpwh.HPWHinit_presets(HPWH::MODELS_externalTest);
//hpwh.HPWHinit_presets(HPWH::MODELS_basicIntegrated);
//hpwh.HPWHinit_presets(HPWH::MODELS_Voltex60);
//hpwh.HPWHinit_presets(HPWH::MODELS_Voltex80);
hpwh.HPWHinit_presets(HPWH::MODELS_GEGeospring);
//int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
					//double ambientT_C, double externalT_C,
					//double DRstatus, double minutesPerStep)
int minutes = 1; 
HPWH::DRMODES drStatus = HPWH::DR_ALLOW;

#define SHORT 

#ifdef SHORT
int liters = 10;
        
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


////hpwh.HPWHinit_presets(3);
//for (int i = 0; i < 1440*365*2; i++) {
////for (int i = 0; i < 120000000; i++) {
    //hpwh.runOneStep(0, 0.2, 0, 50, drStatus, minutes);
//}


cout << sizeof(HPWH);

#endif

return 0;

}
