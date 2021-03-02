

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
void testJustR134ACompressor();
void testJustR410ACompressor();
void testJustR744Compressor();
void testHybridModel();
void testStorageTankSetpoint();

const double REMaxShouldBe = 100.;

int main(int argc, char *argv[])
{
	testMaxSetpointResistanceTank();
	testJustR134ACompressor();
	testJustR410ACompressor();
	testJustR744Compressor();
	testHybridModel();
	testStorageTankSetpoint();

	//Made it through the gauntlet
	return 0;
}

void testMaxSetpointResistanceTank() {
	HPWH hpwh;
	double num;

	hpwh.HPWHinit_resTank();

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num)); // Can't go above boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(99., num)); // Can go to near boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(100., num)); // Can go to boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(10., num)); // Can go low, albiet dumb
	ASSERTTRUE(REMaxShouldBe == num);

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(99.) == 0)
}

void testJustR134ACompressor() {
	HPWH hpwh;

	string input = "TamScalable_SP"; // Just a compressor with R134A
	double num;

	// get preset model 
	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num)); // Can't go above boiling
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num)); // Can't go to near boiling
	ASSERTTRUE(HPWH::MAXOUTLET_R134A == num); //Assert we're getting the right number
	ASSERTTRUE(hpwh.isNewSetpointPossible(60., num)); // Can go to normal
	ASSERTTRUE(hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R134A, num)); // Can go to programed max

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(50.) == 0)
}

void testJustR410ACompressor() {
	HPWH hpwh;

	string input = "ColmacCxV_5_SP"; // Just a compressor with R410A
	double num;

	// get preset model 
	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num)); // Can't go above boiling
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num)); // Can't go to near boiling
	ASSERTTRUE(HPWH::MAXOUTLET_R410A == num); //Assert we're getting the right number
	ASSERTTRUE(hpwh.isNewSetpointPossible(50., num)); // Can go to normal
	ASSERTTRUE(hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R410A, num)); // Can go to programed max

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(50.) == 0)
}

void testJustR744Compressor() {
	HPWH hpwh;

	string input = "Sanden80"; // Just a compressor with R134A
	double num;

	// get preset model 
	getHPWHObject(hpwh, input);

	// isNewSetpointPossible should be fine, we aren't changing the setpoint of the Sanden.
	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num)); // Can't go above boiling
	ASSERTFALSE(hpwh.isNewSetpointPossible(99., num)); // Can't go to near boiling
	ASSERTTRUE(HPWH::MAXOUTLET_R744 == num); //Assert we're getting the right number
	ASSERTTRUE(hpwh.isNewSetpointPossible(60., num)); // Can go to normal
	ASSERTTRUE(hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R744, num)); // Can go to programed max

	// Check this carries over into setting the setpoint. Sanden won't change it's setpoint though
	ASSERTTRUE(hpwh.setSetpoint(101) == HPWH::HPWH_ABORT); // Can't go above boiling
}

void testHybridModel() {
	HPWH hpwh;

	string input = "AOSmithCAHP120"; //Hybrid unit with a compressor with R134A
	double num;

	// get preset model 
	getHPWHObject(hpwh, input);

	ASSERTFALSE(hpwh.isNewSetpointPossible(101., num)); // Can't go above boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(99., num)); // Can go to near boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(100., num)); // Can go to boiling
	ASSERTTRUE(hpwh.isNewSetpointPossible(10., num)); // Can go low, albiet dumb
	ASSERTTRUE(REMaxShouldBe == num); // Max is boiling

	// Check this carries over into setting the setpoint
	ASSERTTRUE(hpwh.setSetpoint(101.) == HPWH::HPWH_ABORT); // Can't go above boiling
	ASSERTTRUE(hpwh.setSetpoint(99.) == 0) // Can go lower than boiling though
}

void testStorageTankSetpoint() {
	HPWH hpwh;

	string input = "StorageTank"; //Hybrid unit with a compressor with R134A
	double num;
	// get preset model 
	getHPWHObject(hpwh, input);
}