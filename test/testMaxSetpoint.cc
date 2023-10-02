

/*unit test for isTankSizeFixed() functionality
 *
 *
 *
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 


using std::cout;
using std::string;


void testMaxSetpointResistanceTank();
void testScalableCompressor();
void testJustR134ACompressor();
void testJustR410ACompressor();
void testJustQAHVCompressor();
void testHybridModel();
void testStorageTankSetpoint();
void testSetpointFixed();
void testSetTankTemps();

const double REMaxShouldBe = 100.;

int main(int, char*)
{
	testMaxSetpointResistanceTank();
	testScalableCompressor();
	testJustR134ACompressor();
	testJustR410ACompressor();
	testJustQAHVCompressor();
	testHybridModel();
	testStorageTankSetpoint();
	testSetpointFixed();
	testSetTankTemps();

	//Made it through the gauntlet
	return 0;
}

void testMaxSetpointResistanceTank() {
	HPWH hpwh;
	double num;
	string why;
	hpwh.HPWHinit_resTank();

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(99., num, why)); // Can go to near boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(100., num, why)); // Can go to boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(10., num, why)); // Can go low, albiet dumb
	ASSERTTRUE(REMaxShouldBe == num);

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(99.) == 0)
}

void testScalableCompressor() {
	HPWH hpwh;

	string input = "TamScalable_SP"; // Just a compressor with R134A
	double num;
	string why;

	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(99., num, why)); // Can go to near boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(60., num, why)); // Can go to normal
	ASSERTTRUE(hpwh.isNewSetpointPossible(100, num, why)); // Can go to programed max

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(50.) == 0)
}

void testJustR134ACompressor() {
	HPWH hpwh;

	string input = "NyleC90A_SP"; // Just a compressor with R134A
	double num;
	string why;

	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num, why)); // Can't go to near boiling
	ASSERTTRUE(HPWH::MAXOUTLET_R134A == num); //Assert we're getting the right number
	ASSERTTRUE(hpwh.isNewSetpointPossible(60., num, why)); // Can go to normal
	ASSERTTRUE(hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R134A, num, why)); // Can go to programed max

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(50.) == 0)
}

void testJustR410ACompressor() {
	HPWH hpwh;
	string input = "ColmacCxV_5_SP"; // Just a compressor with R410A
	double num;
	string why;

	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num, why)); // Can't go to near boiling
	ASSERTTRUE(HPWH::MAXOUTLET_R410A == num); //Assert we're getting the right number
	ASSERTTRUE(hpwh.isNewSetpointPossible(50., num, why)); // Can go to normal
	ASSERTTRUE(hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R410A, num, why)); // Can go to programed max

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(50.) == 0)
}

void testJustQAHVCompressor() {
	HPWH hpwh;
	string input = "QAHV_N136TAU_HPB_SP";
	double num;
	string why;

	getHPWHObject(hpwh, input);

	const double maxQAHVSetpoint = F_TO_C(176.1);
	const double qAHVHotSideTemepratureOffset = dF_TO_dC(15.);

	// isNewSetpointPossible should be fine, we aren't changing the setpoint of the Sanden.
	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num, why)); // Can't go to near boiling

	ASSERTTRUE(hpwh.isNewSetpointPossible(60., num, why)); // Can go to normal

	ASSERTFALSE(hpwh.isNewSetpointPossible(maxQAHVSetpoint, num, why));
	ASSERTTRUE(hpwh.isNewSetpointPossible(maxQAHVSetpoint-qAHVHotSideTemepratureOffset, num, why));

	// Check this carries over into setting the setpoint.
	ASSERTTRUE(hpwh.setSetpoint(101) == HPWH::HPWH_ABORT);
	ASSERTTRUE(hpwh.setSetpoint(maxQAHVSetpoint) == HPWH::HPWH_ABORT);
	ASSERTTRUE(hpwh.setSetpoint(maxQAHVSetpoint - qAHVHotSideTemepratureOffset) == 0);
}

void testHybridModel() {
	HPWH hpwh;
	string input = "AOSmithCAHP120"; //Hybrid unit with a compressor with R134A
	double num;
	string why;

	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(99., num, why)); // Can go to near boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(100., num, why)); // Can go to boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(10., num, why)); // Can go low, albiet dumb
	ASSERTTRUE(REMaxShouldBe == num); // Max is boiling

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(99.) == 0) // Can go lower than boiling though
}

void testStorageTankSetpoint() {
	HPWH hpwh;
	string input = "StorageTank"; //Hybrid unit with a compressor with R134A
	double num;
	string why;

	getHPWHObject(hpwh, input);

	// Storage tanks have free reign!
	ASSERTTRUE(hpwh.isNewSetpointPossible(101., num, why)); // Can go above boiling!
	ASSERTTRUE(hpwh.isNewSetpointPossible(99., num, why)); // Can go to near boiling!
	ASSERTTRUE(hpwh.isNewSetpointPossible(10., num, why)); // Can go low, albiet dumb

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == 0); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(99.) == 0) // Can go lower than boiling though

}

void testSetpointFixed() {
	HPWH hpwh;
	string input = "Sanden80"; //Fixed setpoint model
	double num, num1;
	string why;

	getHPWHObject(hpwh, input);

	// Storage tanks have free reign!
	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling!
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num, why)); // Can't go to near boiling!
	ASSERTFALSE(hpwh.isNewSetpointPossible(60., num, why)); // Can't go to normalish
	ASSERTFALSE(hpwh.isNewSetpointPossible(10., num, why)); // Can't go low, albiet dumb

	ASSERTTRUE(num == hpwh.getSetpoint()); // Make sure it thinks the max is the setpoint
	ASSERTTRUE(hpwh.isNewSetpointPossible(num, num1, why)) // Check that the setpoint can be set to the setpoint.

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(99.) == HPWH::HPWH_ABORT) // Can go lower than boiling though
	ASSERTTRUE(hpwh.setSetpoint(60.) == HPWH::HPWH_ABORT); // Can't go to normalish
	ASSERTTRUE(hpwh.setSetpoint(10.) == HPWH::HPWH_ABORT); // Can't go low, albiet dumb
}

void testSetTankTemps() {
	HPWH hpwh;
	getHPWHObject(hpwh, "Rheem2020Prem50"); // 12-node model

	std::vector<double> newTemps;

// test 1
	std::vector<double> setTemps{10., 60.};
	hpwh.setTankLayerTemperatures(setTemps);
	hpwh.getTankTemps(newTemps);

	// Check some expected values.
	ASSERTTRUE(newTemps[0] == 10.0); //
	ASSERTTRUE(newTemps[11] == 60.0); //

// test 2
	setTemps = {10., 20., 30., 40., 50., 60.};
	hpwh.setTankLayerTemperatures(setTemps);
	hpwh.getTankTemps(newTemps);

	// Check some expected values.
	ASSERTTRUE(newTemps[0] == 10.); //
	ASSERTTRUE(newTemps[5] == 30.); //
	ASSERTTRUE(newTemps[6] == 40.); //
	ASSERTTRUE(newTemps[11] == 60.); //

// test 3
	setTemps = {10., 15., 20., 25., 30., 35., 40., 45., 50., 55., 60., 65., 70., 75., 80., 85., 90.};
	hpwh.setTankLayerTemperatures(setTemps);
	hpwh.getTankTemps(newTemps);

	// Check some expected values.
	ASSERTTRUE(fabs(newTemps[2] - 25.3) < 0.1); //
	ASSERTTRUE(fabs(newTemps[8] - 67.6) < 0.1); //

// test 4
	const int nSet = 24;
	setTemps.resize(nSet);
	const double Ti = 20., Tf = 66.;
	for (std::size_t i = 0; i < nSet; ++i)
		setTemps[i] = Ti + (Tf - Ti) * i / (nSet - 1);
	hpwh.setTankLayerTemperatures(setTemps);
	hpwh.getTankTemps(newTemps);

	// Check some expected values.
	ASSERTTRUE(fabs(newTemps[4] - 37.) < 0.1); //
	ASSERTTRUE(fabs(newTemps[10] - 61.) < 0.1); //

}