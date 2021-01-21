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
#include <string> 

using std::cout;
using std::string;

HPWH::MODELS mapStringToPreset(string modelName);

HPWH::MODELS mapStringToPreset(string modelName) {

	HPWH::MODELS hpwhModel;

	if(modelName == "Voltex60" || modelName == "AOSmithPHPT60") {
		hpwhModel = HPWH::MODELS_AOSmithPHPT60;
	}
	else if (modelName == "Voltex80" || modelName == "AOSmith80") {
		hpwhModel = HPWH::MODELS_AOSmithPHPT80;
	}
	else if (modelName == "GEred" || modelName == "GE") {
		hpwhModel = HPWH::MODELS_GE2012;
	}
	else if (modelName == "SandenGAU" || modelName == "Sanden80" || modelName == "SandenGen3") {
		hpwhModel = HPWH::MODELS_Sanden80;
	}
	else if (modelName == "Sanden120") {
		hpwhModel = HPWH::MODELS_Sanden120;
	}
	else if (modelName == "SandenGES" || modelName == "Sanden40") {
		hpwhModel = HPWH::MODELS_Sanden40;
	}
	else if (modelName == "AOSmithHPTU50") {
		hpwhModel = HPWH::MODELS_AOSmithHPTU50;
	}
	else if (modelName == "AOSmithHPTU66") {
		hpwhModel = HPWH::MODELS_AOSmithHPTU66;
	}
	else if (modelName == "AOSmithHPTU80") {
		hpwhModel = HPWH::MODELS_AOSmithHPTU80;
	}
	else if (modelName == "AOSmithHPTU80DR") {
		hpwhModel = HPWH::MODELS_AOSmithHPTU80_DR;
	}
	else if (modelName == "GE502014STDMode" || modelName == "GE2014STDMode") {
		hpwhModel = HPWH::MODELS_GE2014STDMode;
	}
	else if (modelName == "GE502014" || modelName == "GE2014") {
		hpwhModel = HPWH::MODELS_GE2014;
	}
	else if (modelName == "GE802014") {
		hpwhModel = HPWH::MODELS_GE2014_80DR;
	}
	else if (modelName == "RheemHB50") {
		hpwhModel = HPWH::MODELS_RheemHB50;
	}
	else if (modelName == "Stiebel220e" || modelName == "Stiebel220E") {
		hpwhModel = HPWH::MODELS_Stiebel220E;
	}
	else if (modelName == "Generic1") {
		hpwhModel = HPWH::MODELS_Generic1;
	}
	else if (modelName == "Generic2") {
		hpwhModel = HPWH::MODELS_Generic2;
	}
	else if (modelName == "Generic3") {
		hpwhModel = HPWH::MODELS_Generic3;
	}
	else if (modelName == "custom") {
		hpwhModel = HPWH::MODELS_CustomFile;
	}
	else if (modelName == "restankRealistic") {
		hpwhModel = HPWH::MODELS_restankRealistic;
	}
	else if (modelName == "StorageTank") {
		hpwhModel = HPWH::MODELS_StorageTank;
	}
	else if (modelName == "BWC2020_65") {
		hpwhModel = HPWH::MODELS_BWC2020_65;
	}
	// New Rheems
	else if (modelName == "Rheem2020Prem40") {
		hpwhModel = HPWH::MODELS_Rheem2020Prem40;
	}
	else if (modelName == "Rheem2020Prem50") {
		hpwhModel = HPWH::MODELS_Rheem2020Prem50;
	}
	else if (modelName == "Rheem2020Prem65") {
		hpwhModel = HPWH::MODELS_Rheem2020Prem65;
	}
	else if (modelName == "Rheem2020Prem80") {
		hpwhModel = HPWH::MODELS_Rheem2020Prem80;
	}
	else if (modelName == "Rheem2020Build40") {
		hpwhModel = HPWH::MODELS_Rheem2020Build40;
	}
	else if (modelName == "Rheem2020Build50") {
		hpwhModel = HPWH::MODELS_Rheem2020Build50;
	}
	else if (modelName == "Rheem2020Build65") {
		hpwhModel = HPWH::MODELS_Rheem2020Build65;
	}
	else if (modelName == "Rheem2020Build80") {
		hpwhModel = HPWH::MODELS_Rheem2020Build80;
	}
	// Large HPWH's
	else if (modelName == "AOSmithCAHP120") {
		hpwhModel = HPWH::MODELS_AOSmithCAHP120;
	}
	else if (modelName == "ColmacCxV_5_SP") {
		hpwhModel = HPWH::MODELS_ColmacCxV_5_SP;
	}
	else if (modelName == "ColmacCxA_10_SP") {
		hpwhModel = HPWH::MODELS_ColmacCxA_10_SP;
	}
	else if (modelName == "ColmacCxA_15_SP") {
		hpwhModel = HPWH::MODELS_ColmacCxA_15_SP;
	}
	else if (modelName == "ColmacCxA_20_SP") {
		hpwhModel = HPWH::MODELS_ColmacCxA_20_SP;
	}
	else if (modelName == "ColmacCxA_25_SP") {
		hpwhModel = HPWH::MODELS_ColmacCxA_25_SP;
	}
	else if (modelName == "ColmacCxA_30_SP") {
		hpwhModel = HPWH::MODELS_ColmacCxA_30_SP;
	}
	else if (modelName == "NyleC25A_SP") {
		hpwhModel = HPWH::MODELS_NyleC25A_SP;
	}
	else if (modelName == "NyleC90A_SP") {
		hpwhModel = HPWH::MODELS_NyleC90A_SP;
	}
	else if (modelName == "NyleC185A_SP") {
		hpwhModel = HPWH::MODELS_NyleC185A_SP;
	}
	else if (modelName == "NyleC250A_SP") {
		hpwhModel = HPWH::MODELS_NyleC250A_SP;
	}
	else if (modelName == "NyleC90A_C_SP") {
		hpwhModel = HPWH::MODELS_NyleC90A_C_SP;
	}
	else if (modelName == "NyleC185A_C_SP") {
		hpwhModel = HPWH::MODELS_NyleC185A_C_SP;
	}
	else if (modelName == "NyleC250A_C_SP") {
		hpwhModel = HPWH::MODELS_NyleC250A_C_SP;

		//do nothin, use custom-compiled input specified later
	}
	else {
		hpwhModel = HPWH::MODELS_basicIntegrated;
		cout << "Couldn't find model " << modelName << ".  Exiting...\n";
		exit(1);
	}
	return hpwhModel;
}