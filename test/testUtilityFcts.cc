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

#define F_TO_C(T) ((T - 32.0) * 5.0 / 9.0)
#define C_TO_F(T) (((9.0 / 5.0) * T) + 32.0)
#define dF_TO_dC(T) (T * 5.0 / 9.0)
#define GAL_TO_L(GAL) (GAL * 3.78541)
#define KW_TO_BTUperHR(KW) (KW * 3412.14)
#define KWH_TO_BTU(KW) (KW * 3412.14)

#define ASSERTTRUE(input, ...)                                                                     \
    if (!(input))                                                                                  \
    {                                                                                              \
        cout << "Assertation failed at " << __FILE__ << ", line: " << __LINE__ << ".\n";           \
        exit(1);                                                                                   \
    }
#define ASSERTFALSE(input, ...)                                                                    \
    if ((input))                                                                                   \
    {                                                                                              \
        cout << "Assertation failed at " << __FILE__ << ", line: " << __LINE__ << ".\n";           \
        exit(1);                                                                                   \
    }

// Compare doubles
bool cmpd(double A, double B, double epsilon = 0.0001) { return (fabs(A - B) < epsilon); }
// Relative Compare doubles
bool relcmpd(double A, double B, double epsilon = 0.00001)
{
    return fabs(A - B) < (epsilon * (fabs(A) < fabs(B) ? fabs(B) : fabs(A)));
}

bool compressorIsRunning(HPWH& hpwh)
{
    return (bool)hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex());
}

HPWH::MODELS mapStringToPreset(string modelName)
{

    HPWH::MODELS hpwhModel;

    if (modelName == "Voltex60" || modelName == "AOSmithPHPT60")
    {
        hpwhModel = HPWH::MODELS_AOSmithPHPT60;
    }
    else if (modelName == "Voltex80" || modelName == "AOSmith80")
    {
        hpwhModel = HPWH::MODELS_AOSmithPHPT80;
    }
    else if (modelName == "GEred" || modelName == "GE")
    {
        hpwhModel = HPWH::MODELS_GE2012;
    }
    else if (modelName == "SandenGAU" || modelName == "Sanden80" || modelName == "SandenGen3")
    {
        hpwhModel = HPWH::MODELS_Sanden80;
    }
    else if (modelName == "Sanden120")
    {
        hpwhModel = HPWH::MODELS_Sanden120;
    }
    else if (modelName == "SandenGES" || modelName == "Sanden40")
    {
        hpwhModel = HPWH::MODELS_Sanden40;
    }
    else if (modelName == "AOSmithHPTU50")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTU50;
    }
    else if (modelName == "AOSmithHPTU66")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTU66;
    }
    else if (modelName == "AOSmithHPTU80")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTU80;
    }
    else if (modelName == "AOSmithHPTS50")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTS50;
    }
    else if (modelName == "AOSmithHPTS66")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTS66;
    }
    else if (modelName == "AOSmithHPTS80")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTS80;
    }
    else if (modelName == "AOSmithHPTU80DR")
    {
        hpwhModel = HPWH::MODELS_AOSmithHPTU80_DR;
    }
    else if (modelName == "GE502014STDMode" || modelName == "GE2014STDMode")
    {
        hpwhModel = HPWH::MODELS_GE2014STDMode;
    }
    else if (modelName == "GE502014" || modelName == "GE2014")
    {
        hpwhModel = HPWH::MODELS_GE2014;
    }
    else if (modelName == "GE802014")
    {
        hpwhModel = HPWH::MODELS_GE2014_80DR;
    }
    else if (modelName == "RheemHB50")
    {
        hpwhModel = HPWH::MODELS_RheemHB50;
    }
    else if (modelName == "Stiebel220e" || modelName == "Stiebel220E")
    {
        hpwhModel = HPWH::MODELS_Stiebel220E;
    }
    else if (modelName == "Generic1")
    {
        hpwhModel = HPWH::MODELS_Generic1;
    }
    else if (modelName == "Generic2")
    {
        hpwhModel = HPWH::MODELS_Generic2;
    }
    else if (modelName == "Generic3")
    {
        hpwhModel = HPWH::MODELS_Generic3;
    }
    else if (modelName == "custom")
    {
        hpwhModel = HPWH::MODELS_CustomFile;
    }
    else if (modelName == "restankRealistic")
    {
        hpwhModel = HPWH::MODELS_restankRealistic;
    }
    else if (modelName == "StorageTank")
    {
        hpwhModel = HPWH::MODELS_StorageTank;
    }
    else if (modelName == "BWC2020_65")
    {
        hpwhModel = HPWH::MODELS_BWC2020_65;
    }
    // New Rheems
    else if (modelName == "Rheem2020Prem40")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Prem40;
    }
    else if (modelName == "Rheem2020Prem50")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Prem50;
    }
    else if (modelName == "Rheem2020Prem65")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Prem65;
    }
    else if (modelName == "Rheem2020Prem80")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Prem80;
    }
    else if (modelName == "Rheem2020Build40")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Build40;
    }
    else if (modelName == "Rheem2020Build50")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Build50;
    }
    else if (modelName == "Rheem2020Build65")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Build65;
    }
    else if (modelName == "Rheem2020Build80")
    {
        hpwhModel = HPWH::MODELS_Rheem2020Build80;
    }
    else if (modelName == "RheemPlugInDedicated40")
    {
        hpwhModel = HPWH::MODELS_RheemPlugInDedicated40;
    }
    else if (modelName == "RheemPlugInDedicated50")
    {
        hpwhModel = HPWH::MODELS_RheemPlugInDedicated50;
    }
    else if (modelName == "RheemPlugInShared40")
    {
        hpwhModel = HPWH::MODELS_RheemPlugInShared40;
    }
    else if (modelName == "RheemPlugInShared50")
    {
        hpwhModel = HPWH::MODELS_RheemPlugInShared50;
    }
    else if (modelName == "RheemPlugInShared65")
    {
        hpwhModel = HPWH::MODELS_RheemPlugInShared65;
    }
    else if (modelName == "RheemPlugInShared80")
    {
        hpwhModel = HPWH::MODELS_RheemPlugInShared80;
    }
    // Large HPWH's
    else if (modelName == "AOSmithCAHP120")
    {
        hpwhModel = HPWH::MODELS_AOSmithCAHP120;
    }
    else if (modelName == "ColmacCxV_5_SP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxV_5_SP;
    }
    else if (modelName == "ColmacCxA_10_SP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_10_SP;
    }
    else if (modelName == "ColmacCxA_15_SP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_15_SP;
    }
    else if (modelName == "ColmacCxA_20_SP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_20_SP;
    }
    else if (modelName == "ColmacCxA_25_SP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_25_SP;
    }
    else if (modelName == "ColmacCxA_30_SP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_30_SP;
    }

    else if (modelName == "ColmacCxV_5_MP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxV_5_MP;
    }
    else if (modelName == "ColmacCxA_10_MP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_10_MP;
    }
    else if (modelName == "ColmacCxA_15_MP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_15_MP;
    }
    else if (modelName == "ColmacCxA_20_MP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_20_MP;
    }
    else if (modelName == "ColmacCxA_25_MP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_25_MP;
    }
    else if (modelName == "ColmacCxA_30_MP")
    {
        hpwhModel = HPWH::MODELS_ColmacCxA_30_MP;
    }

    else if (modelName == "RheemHPHD60")
    {
        hpwhModel = HPWH::MODELS_RHEEM_HPHD60VNU_201_MP;
    }
    else if (modelName == "RheemHPHD135")
    {
        hpwhModel = HPWH::MODELS_RHEEM_HPHD135VNU_483_MP;
    }
    // Nyle Single pass models
    else if (modelName == "NyleC25A_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC25A_SP;
    }
    else if (modelName == "NyleC60A_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC60A_SP;
    }
    else if (modelName == "NyleC90A_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC90A_SP;
    }
    else if (modelName == "NyleC185A_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC185A_SP;
    }
    else if (modelName == "NyleC250A_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC250A_SP;
    }
    else if (modelName == "NyleC60A_C_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC60A_C_SP;
    }
    else if (modelName == "NyleC90A_C_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC90A_C_SP;
    }
    else if (modelName == "NyleC185A_C_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC185A_C_SP;
    }
    else if (modelName == "NyleC250A_C_SP")
    {
        hpwhModel = HPWH::MODELS_NyleC250A_C_SP;
    }
    // Nyle MP models
    else if (modelName == "NyleC60A_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC60A_MP;
    }
    else if (modelName == "NyleC90A_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC90A_MP;
    }
    else if (modelName == "NyleC125A_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC125A_MP;
    }
    else if (modelName == "NyleC185A_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC185A_MP;
    }
    else if (modelName == "NyleC250A_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC250A_MP;
    }
    else if (modelName == "NyleC60A_C_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC60A_C_MP;
    }
    else if (modelName == "NyleC90A_C_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC90A_C_MP;
    }
    else if (modelName == "NyleC125A_C_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC125A_C_MP;
    }
    else if (modelName == "NyleC185A_C_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC185A_C_MP;
    }
    else if (modelName == "NyleC250A_C_MP")
    {
        hpwhModel = HPWH::MODELS_NyleC250A_C_MP;
    }
    else if (modelName == "QAHV_N136TAU_HPB_SP")
    {
        hpwhModel = HPWH::MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP;
    }
    // Stack in a couple scalable models
    else if (modelName == "TamScalable_SP")
    {
        hpwhModel = HPWH::MODELS_TamScalable_SP;
    }
    else if (modelName == "TamScalable_SP_2X")
    {
        hpwhModel = HPWH::MODELS_TamScalable_SP;
    }
    else if (modelName == "TamScalable_SP_Half")
    {
        hpwhModel = HPWH::MODELS_TamScalable_SP;
    }
    else if (modelName == "Scalable_MP")
    {
        hpwhModel = HPWH::MODELS_Scalable_MP;
    }
    else if (modelName == "AWHSTier3Generic40")
    {
        hpwhModel = HPWH::MODELS_AWHSTier3Generic40;
    }
    else if (modelName == "AWHSTier3Generic50")
    {
        hpwhModel = HPWH::MODELS_AWHSTier3Generic50;
    }
    else if (modelName == "AWHSTier3Generic65")
    {
        hpwhModel = HPWH::MODELS_AWHSTier3Generic65;
    }
    else if (modelName == "AWHSTier3Generic80")
    {
        hpwhModel = HPWH::MODELS_AWHSTier3Generic80;
    }
    else
    {
        hpwhModel = HPWH::MODELS_basicIntegrated;
        cout << "Couldn't find model " << modelName << ".  Exiting...\n";
        exit(1);
    }
    return hpwhModel;
}

int getHPWHObject(HPWH& hpwh, string modelName)
{
    /**Sets up the preset HPWH object with modelName */
    int returnVal = 1;
    HPWH::MODELS model = mapStringToPreset(modelName);

    returnVal = hpwh.HPWHinit_presets(model);

    if (modelName == "TamScalable_SP_2X")
    {
        hpwh.setScaleHPWHCapacityCOP(2., 1.); // Scale the compressor
        hpwh.setResistanceCapacity(60.);      // Reset resistance elements in kW
    }
    else if (modelName == "TamScalable_SP_Half")
    {
        hpwh.setScaleHPWHCapacityCOP(1 / 2., 1.); // Scale the compressor
        hpwh.setResistanceCapacity(15.);          // Reset resistance elements in kW
    }
    return returnVal;
}