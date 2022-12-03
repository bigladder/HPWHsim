#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 

void testGetStateOfCharge();
void testChargeBelowSetpoint();
void testGetStateOfChargeFromLogic();
void testGetStateOfChargeFailure();


int main()
{
	testGetStateOfCharge();
	testChargeBelowSetpoint();
	testGetStateOfChargeFromLogic();
	testGetStateOfChargeFailure();
}

void testGetStateOfCharge() {
	HPWH hpwh;
	string input = "Sanden80";
	getHPWHObject(hpwh, input);
	double tMains_C = F_TO_C(55.);
	double tMinUseful_C = F_TO_C(110.);
	double chargeFraction;

	// Check for errors	
	chargeFraction = hpwh.getSoCFraction(F_TO_C(125.), tMinUseful_C);
	ASSERTTRUE(chargeFraction == HPWH::HPWH_ABORT);
	chargeFraction = hpwh.getSoCFraction(tMains_C, F_TO_C(155.));
	ASSERTTRUE(chargeFraction == HPWH::HPWH_ABORT);
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(100.));
	ASSERTTRUE(chargeFraction == HPWH::HPWH_ABORT);

	// Check state of charge returns 1 at setpoint
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C);
	ASSERTTRUE(cmpd(chargeFraction, 1.));
	chargeFraction = hpwh.getSoCFraction(tMains_C + 5., tMinUseful_C);
	ASSERTTRUE(cmpd(chargeFraction, 1.));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C + 5.);
	ASSERTTRUE(cmpd(chargeFraction, 1.));

	// Varying max temp
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(110.));
	ASSERTTRUE(cmpd(chargeFraction, 1.70909));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(120.));
	ASSERTTRUE(cmpd(chargeFraction, 1.4461));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(135.));
	ASSERTTRUE(cmpd(chargeFraction, 1.175));
}

void testChargeBelowSetpoint() {
	HPWH hpwh;
	string input = "ColmacCxV_5_SP"; 
	getHPWHObject(hpwh, input); 	
	double tMains_C = F_TO_C(60.);
	double tMinUseful_C = F_TO_C(110.);
	double chargeFraction;

	// Check state of charge returns 1 at setpoint
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C);
	ASSERTTRUE(cmpd(chargeFraction, 1.));

	// Check state of charge returns 0 when tank below useful
	hpwh.setTankToTemperature(F_TO_C(109.));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(140.));
	ASSERTTRUE(cmpd(chargeFraction, 0.));

	// Check some lower values with tank set at constant temperatures
	hpwh.setTankToTemperature(F_TO_C(110.));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(140.));
	ASSERTTRUE(cmpd(chargeFraction, 0.625));

	hpwh.setTankToTemperature(F_TO_C(120.));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(140.));
	ASSERTTRUE(cmpd(chargeFraction, 0.75));

	hpwh.setTankToTemperature(F_TO_C(130.));
	chargeFraction = hpwh.getSoCFraction(tMains_C, tMinUseful_C, F_TO_C(140.));
	ASSERTTRUE(cmpd(chargeFraction, 0.875));
}

void testGetStateOfChargeFromLogic() {
	HPWH hpwh;
	string input = "Sanden80";
	getHPWHObject(hpwh, input);
	double tMains_C = F_TO_C(55.);
	double tMinUseful_C = F_TO_C(110.);
	double chargeFraction;

	// change to SOC control;
	hpwh.switchToSoCControls(.76, .05, tMinUseful_C, true, tMains_C);
	ASSERTTRUE(hpwh.isSoCControlled());

	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(cmpd(chargeFraction, 1.));

	hpwh.setTankToTemperature(F_TO_C(110.));
	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(cmpd(chargeFraction, 0.5851064));

	hpwh.setTankToTemperature(F_TO_C(121.));
	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(cmpd(chargeFraction, 0.702123));

	hpwh.setTankToTemperature(F_TO_C(134.));
	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(cmpd(chargeFraction, 0.8404255));

	hpwh.setTankToTemperature(F_TO_C(145.));
	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(cmpd(chargeFraction, 0.957447));
}


void testGetStateOfChargeFailure() {
	HPWH hpwh;
	string input = "Sanden80";
	getHPWHObject(hpwh, input);
	double tMains_C = F_TO_C(55.);
	double tMinUseful_C = F_TO_C(110.);
	double chargeFraction;

	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(chargeFraction == HPWH::HPWH_ABORT);

	hpwh.switchToSoCControls(.76, .05, tMinUseful_C, false, tMains_C);
	ASSERTTRUE(hpwh.isSoCControlled());

	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(chargeFraction == HPWH::HPWH_ABORT);

	hpwh.setInletT(tMains_C);
	chargeFraction = hpwh.getSoCFraction();
	ASSERTTRUE(cmpd(chargeFraction, 1.));
}