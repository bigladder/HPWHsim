/*
 * unit test for energy balancing
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 

/* Test energy balance for AOSmithHPTS50*/
void testEnergyBalanceAOSmithHPTS50() {

	HPWH hpwh;
	getHPWHObject(hpwh, "AOSmithHPTS50");

	double maxDrawVol_L = 1.;
	const double ambientT_C = 20.;
	const double externalT_C = 20.;

	//
	hpwh.setTankToTemperature(20.);
	hpwh.setInletT(5.);

	const double Pi = 4. * atan(1.);
	double testDuration_min = 60.;
	for(int i_min = 0; i_min < testDuration_min; ++i_min)
	{
		double t_min = static_cast<double>(i_min);

		double flowFac = sin(Pi * t_min / testDuration_min) - 0.5;
		flowFac += fabs(flowFac); // semi-sinusoidal flow profile
		double drawVol_L = flowFac * maxDrawVol_L;

		double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
		hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW);
		ASSERTTRUE(hpwh.isEnergyBalanced(drawVol_L,prevHeatContent_kJ,1.e-6));
	}
}

/* Test energy balance for storage tank with extra heat (solar)*/
void testEnergyBalanceSolar() {

	HPWH hpwh;
	getHPWHObject(hpwh, "StorageTank");

	double maxDrawVol_L = 1.;
	const double ambientT_C = 20.;
	const double externalT_C = F_TO_C(-999.); // used in cse to keep heat source from turning on.

	std::vector<double> nodePowerExtra_W = {1000.};
	//
	hpwh.setUA(0.);
	hpwh.setTankToTemperature(20.);
	hpwh.setInletT(5.);

	const double Pi = 4. * atan(1.);
	double testDuration_min = 60.;
	for(int i_min = 0; i_min < testDuration_min; ++i_min)
	{
		double t_min = static_cast<double>(i_min);

		double flowFac = sin(Pi * t_min / testDuration_min) - 0.5;
		flowFac += fabs(flowFac); // semi-sinusoidal flow profile
		double drawVol_L = flowFac * maxDrawVol_L;

		double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
		hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW,0.,0.,&nodePowerExtra_W);
		ASSERTTRUE(hpwh.isEnergyBalanced(drawVol_L,prevHeatContent_kJ,1.e-6));
	}
}

int main(int, char*)
{
	testEnergyBalanceAOSmithHPTS50();
	testEnergyBalanceSolar();
	
	return 0;
}
