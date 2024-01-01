/*
 * test UEF calculation
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <string>

/* Evaluate UEF based on simulations using standard profiles */
static bool testCalcUEF(const std::string& sModelName, double& UEF)
{
    HPWH hpwh;

    HPWH::MODELS model = mapStringToPreset(sModelName);
    if (hpwh.HPWHinit_presets(model) != 0)
    {
        return false;
    }

    return hpwh.calcUEF(hpwh.findUsageFromMaximumGPM_Rating(), UEF);
}

int main(int, char**)
{
    double UEF;
    ASSERTTRUE(testCalcUEF("AOSmithHPTS50", UEF));
    ASSERTTRUE(cmpd(UEF, 4.4091));

    ASSERTTRUE(testCalcUEF("AquaThermAire", UEF));
    ASSERTTRUE(cmpd(UEF, 3.5848));

    return 0;
}
