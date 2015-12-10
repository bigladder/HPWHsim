/*This is not a substitute for a proper HPWH Test Tool, it is merely a short program
 * to aid in the testing of the new HPWH.cc as it is being written.
 * 
 * -NDK
 * 
 * 
 */
#include "HPWH.hh"


#define F_TO_C(T) ((T-32.0)*5.0/9.0)
#define GAL_TO_L(GAL) (GAL*3.78541)

int main(void)
{


HPWH hpwh;

hpwh.HPWHinit_presets(1);

//int HPWH::runOneStep(double inletT_C, double drawVolume_L, 
					//double ambientT_C, double externalT_C,
					//double DRstatus, double minutesPerStep)
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 30, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();
hpwh.runOneStep(10, 0, 50, 50, 1, 1);
hpwh.printTankTemps();





return 0;

}
