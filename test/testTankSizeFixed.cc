

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

int testForceChangeTankSize(HPWH::MODELS model);
int testIsTankSizeFixed(HPWH::MODELS model);

int main(int argc, char *argv[])
{
	HPWH::MODELS presetModel;

	string input;

	if (argc != 2) {
		cout << "Invalid input. This program takes One arguments: preset model specification (ie. Sanden80). Recieved input: \n";
		for (int ii = 0; ii < argc; ii++) {
			cout << argv[ii] << " ";
		}
		exit(1);
	}
	else {
		input = argv[1];
	}

	// get model number
	presetModel = mapStringToPreset(input);


	if (testIsTankSizeFixed(presetModel)) {

	}

	//Made it through the gauntlet
	return 0;
}

int testIsTankSizeFixed(HPWH::MODELS model) {
	HPWH hpwh;

	// set preset
	if (hpwh.HPWHinit_presets(model) != 0) exit(1);

	double originalTankSize = hpwh.getTankSize();
	double newTankSize = originalTankSize + 100;

	// change the tank size
	int result = hpwh.setTankSize(newTankSize);

	if (result != 0 && result != HPWH::HPWH_ABORT) {
		cout << "Error, setTankSize() returned an invalid result: " << result << "\n";
	}

	if (hpwh.isTankSizeFixed()) { // better not have change!
		if (result == 0) {
			cout << "Error, setTankSize() returned 0 when should be HPWH_ABORT\n";
			exit(1);
		}
		if (originalTankSize != hpwh.getTankSize()) {
			cout << "Error, the tank size has changed when isTankSizeFixed is true\n";
			exit(1);
		}
	}
	else { // it better have changed
		if (result != 0) {
			cout << "Error, setTankSize() returned HPWH_ABORT when it should be 0\n";
			exit(1);
		}
		if (newTankSize != hpwh.getTankSize()) {
			cout << "Error, the tank size hasn't changed to the new tank size when it should have. New Size: " << newTankSize << ". Returned Value: " << hpwh.getTankSize() << "\n";
			exit(1);
		}
	}
	return 0;
}

int testForceChangeTankSize(HPWH::MODELS model) {
	HPWH hpwh;

	// set preset
	if (hpwh.HPWHinit_presets(model) != 0) exit(1);

	double newTankSize = 133.312; //No way a tank has this size originally

	// change the tank size
	int result = hpwh.setTankSize(newTankSize, HPWH::UNITS_GAL, true);

	if (result != 0 && result != HPWH::HPWH_ABORT) {
		cout << "Error, setTankSize() returned an invalid result: " << result << "\n";
	}

	// it better have changed
	if (result != 0) {
		cout << "Error, setTankSize() returned HPWH_ABORT when it should be 0\n";
		exit(1);
	}
	if (newTankSize != hpwh.getTankSize()) {
		cout << "Error, the tank size hasn't changed to the new tank size when it should have. New Size: " << newTankSize << ". Returned Value: " << hpwh.getTankSize() << "\n";
		exit(1);
	}
	return 0;
}
