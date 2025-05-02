/*
Initialize all presets available in HPWHsim
*/

#include <algorithm>

#include <btwxt/btwxt.h>

#include "HPWH.hh"
#include "HPWHHeatingLogic.hh"
#include "Condenser.hh"
#include "Resistance.hh"

void HPWH::initResistanceTank()
{
    // a default resistance tank, nominal 50 gallons, 0.95 EF, standard double 4.5 kW elements
    initResistanceTank(GAL_TO_L(47.5), 0.95, 4500, 4500);
}

void HPWH::initResistanceTank(double tankVol_L,
                              double energyFactor,
                              double upperPower_W,
                              double lowerPower_W)
{
    setAllDefaults();

    // low power element will cause divide by zero/negative UA in EF -> UA conversion
    if (lowerPower_W < 550)
    {
        send_error("Resistance tank lower element wattage below 550 W.");
    }
    if (upperPower_W < 0.)
    {
        send_error("Upper resistance tank wattage below 0 W.");
    }
    if (energyFactor <= 0.)
    {
        send_error("Energy Factor less than zero.");
    }

    heatSources.clear();

    setNumNodes(12);

    // use tank size setting function since it has bounds checking
    tank->volumeFixed = false;
    setTankSize(tankVol_L);

    setpoint_C = F_TO_C(127.0);

    // start tank off at setpoint
    resetTankToSetpoint();

    doTempDepression = false;
    tank->mixesOnDraw = true;

    Resistance* resistiveElementTop = NULL;
    Resistance* resistiveElementBottom = NULL;

    heatSources.reserve(2);
    if (upperPower_W > 0.)
    {
        // Only add an upper element when the upperPower_W > 0 otherwise ignore this.
        // If the element is added this can mess with the intended logic.
        resistiveElementTop = addResistance("resistiveElementTop");
        resistiveElementTop->setup(8, upperPower_W);

        resistiveElementTop->addTurnOnLogic(topThird(dF_TO_dC(20)));
        resistiveElementTop->isVIP = true;
    }

    //
    resistiveElementBottom = addResistance("resistiveElementBottom");
    resistiveElementBottom->setup(0, lowerPower_W);

    // standard logic conditions
    resistiveElementBottom->addTurnOnLogic(bottomThird(dF_TO_dC(40)));
    resistiveElementBottom->addTurnOnLogic(standby(dF_TO_dC(10)));

    if (resistiveElementTop && resistiveElementBottom)
    {
        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    }

    // (1/EnFac - 1/RecovEff) / (67.5 * ((24/41094) - 1/(RecovEff * Power_btuperHr)))
    // Previous comment and the equation source said (1/EnFac + 1/RecovEff) however this was
    // determined to be a typo from the source that was kept in the comment. On 8/12/2021
    // the comment was changed to reflect the correct mathematical formulation which is performed
    // below.
    double recoveryEfficiency = 0.98;
    double numerator = (1.0 / energyFactor) - (1.0 / recoveryEfficiency);
    double temp = 1.0 / (recoveryEfficiency * lowerPower_W * 3.41443);
    double denominator = 67.5 * ((24.0 / 41094.0) - temp);
    tank->UA_kJperHrC = UAf_TO_UAc(numerator / denominator);

    if (tank->UA_kJperHrC < 0.)
    {
        if (tank->UA_kJperHrC < -0.1)
        {
            send_warning("Computed tank->UA_kJperHrC is less than 0, and is reset to 0.");
        }
        tank->UA_kJperHrC = 0.0;
    }

    model = MODELS_CustomResTank;

    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isOn)
        {
            isHeating = true;
        }
    }
}

void HPWH::initResistanceTankGeneric(double tankVol_L,
                                     double rValue_m2KperW,
                                     double upperPower_W,
                                     double lowerPower_W)
{
    setAllDefaults(); // reset all defaults if you're re-initializing
    heatSources.clear();

    // low power element will cause divide by zero/negative UA in EF -> UA conversion
    if (lowerPower_W < 0)
    {
        send_error("Lower resistance tank wattage below 0 W.");
    }
    if (upperPower_W < 0.)
    {
        send_error("Upper resistance tank wattage below 0 W.");
    }
    if (rValue_m2KperW <= 0.)
    {
        send_error("R-Value is equal to or below 0.");
    }

    setNumNodes(12);

    // set tank size function has bounds checking
    tank->volumeFixed = false;
    setTankSize(tankVol_L);
    canScale = true;

    setpoint_C = F_TO_C(127.0);
    resetTankToSetpoint(); // start tank off at setpoint

    doTempDepression = false;
    tank->mixesOnDraw = true;

    Resistance* resistiveElementTop = nullptr;
    Resistance* resistiveElementBottom = nullptr;

    heatSources.reserve(2);
    if (upperPower_W > 0.)
    {
        resistiveElementTop = addResistance("resistiveElementTop");
        resistiveElementTop->setup(8, upperPower_W);

        resistiveElementTop->addTurnOnLogic(topThird(dF_TO_dC(20)));
        resistiveElementTop->isVIP = true;
    }

    if (lowerPower_W > 0.)
    {
        resistiveElementBottom = addResistance("resistiveElementBottom");
        resistiveElementBottom->setup(0, lowerPower_W);

        resistiveElementBottom->addTurnOnLogic(bottomThird(dF_TO_dC(40.)));
        resistiveElementBottom->addTurnOnLogic(standby(dF_TO_dC(10.)));
    }

    if (resistiveElementTop && resistiveElementBottom)
    {
        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    }

    // Calc UA
    double SA_M2 = getTankSurfaceArea(tankVol_L, UNITS_L, UNITS_M2);
    double tankUA_WperK = SA_M2 / rValue_m2KperW;
    tank->UA_kJperHrC = tankUA_WperK * sec_per_hr / 1000.; // 1000 J/kJ

    if (tank->UA_kJperHrC < 0.)
    {
        if (tank->UA_kJperHrC < -0.1)
        {
            send_warning("Computed tank->UA_kJperHrC is less than 0, and is reset to 0.");
        }
        tank->UA_kJperHrC = 0.0;
    }

    model = MODELS_CustomResTankGeneric;

    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (auto& source : heatSources)
    {
        if (source->isOn)
        {
            isHeating = true;
        }
    }
}

void HPWH::initGeneric(double tankVol_L, double energyFactor, double resUse_C)
{
    setAllDefaults(); // reset all defaults if you're re-initializing
    heatSources.clear();

    // except where noted, these values are taken from MODELS_GE2014STDMode on 5/17/16
    setNumNodes(12);
    setpoint_C = F_TO_C(127.0);

    // start tank off at setpoint
    resetTankToSetpoint();

    tank->volumeFixed = false;

    doTempDepression = false;
    tank->mixesOnDraw = true;

    heatSources.reserve(3);
    auto resistiveElementTop = addResistance("resistiveElementTop");
    auto resistiveElementBottom = addResistance("resistiveElementBottom");
    auto compressor = addCondenser("compressor");

    // compressor values
    compressor->isOn = false;
    compressor->isVIP = false;

    compressor->setCondensity({1., 0., 0.});

    compressor->performanceMap.reserve(2);

    compressor->performanceMap.push_back({
        50,                          // Temperature (T_F)
        {187.064124, 1.939747, 0.0}, // Input Power Coefficients (inputPower_coeffs)
        {5.4977772, -0.0243008, 0.0} // COP Coefficients (COP_coeffs)
    });

    compressor->performanceMap.push_back({
        70,                         // Temperature (T_F)
        {148.0418, 2.553291, 0.0},  // Input Power Coefficients (inputPower_coeffs)
        {7.207307, -0.0335265, 0.0} // COP Coefficients (COP_coeffs)
    });

    compressor->minT = F_TO_C(45.);
    compressor->maxT = F_TO_C(120.);
    compressor->hysteresis_dC = dF_TO_dC(2);
    compressor->configuration = Condenser::CONFIG_WRAPPED;
    compressor->maxSetpoint_C = MAXOUTLET_R134A;

    // top resistor values
    resistiveElementTop->setup(6, 4500);
    resistiveElementTop->isVIP = true;

    // bottom resistor values
    resistiveElementBottom->setup(0, 4000);
    resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});

    // logic conditions
    // this is set customly, from input
    // resistiveElementTop->addTurnOnLogic(topThird(dF_TO_dC(19.6605)));
    resistiveElementTop->addTurnOnLogic(topThird(resUse_C));

    resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp(F_TO_C(86.1111)));

    compressor->addTurnOnLogic(bottomThird(dF_TO_dC(33.6883)));
    compressor->addTurnOnLogic(standby(dF_TO_dC(12.392)));

    // custom adjustment for poorer performance
    // compressor->addShutOffLogic(lowT(F_TO_C(37)));
    //
    // end section of parameters from GE model

    // set tank volume from input
    // use tank size setting function since it has bounds checking
    setTankSize(tankVol_L);

    // derive conservative (high) UA from tank volume
    //   curve fit by Jim Lutz, 5-25-2016
    double tankVol_gal = tankVol_L / GAL_TO_L(1.);
    double v1 = 7.5156316175 * pow(tankVol_gal, 0.33) + 5.9995357658;
    tank->UA_kJperHrC = 0.0076183819 * v1 * v1;

    // do a linear interpolation to scale COP curve constant, using measured values
    //  Chip's attempt 24-May-2014
    double uefSpan = 3.4 - 2.0;

    // force COP to be 70% of GE at UEF 2 and 95% at UEF 3.4
    // use a fudge factor to scale cop and input power in tandem to maintain constant capacity
    double fUEF = (energyFactor - 2.0) / uefSpan;
    double genericFudge = (1. - fUEF) * .7 + fUEF * .95;

    compressor->performanceMap[0].COP_coeffs[0] *= genericFudge;
    compressor->performanceMap[0].COP_coeffs[1] *= genericFudge;
    compressor->performanceMap[0].COP_coeffs[2] *= genericFudge;

    compressor->performanceMap[1].COP_coeffs[0] *= genericFudge;
    compressor->performanceMap[1].COP_coeffs[1] *= genericFudge;
    compressor->performanceMap[1].COP_coeffs[2] *= genericFudge;

    compressor->performanceMap[0].inputPower_coeffs[0] /= genericFudge;
    compressor->performanceMap[0].inputPower_coeffs[1] /= genericFudge;
    compressor->performanceMap[0].inputPower_coeffs[2] /= genericFudge;

    compressor->performanceMap[1].inputPower_coeffs[0] /= genericFudge;
    compressor->performanceMap[1].inputPower_coeffs[1] /= genericFudge;
    compressor->performanceMap[1].inputPower_coeffs[2] /= genericFudge;

    //
    compressor->backupHeatSource = resistiveElementBottom;
    resistiveElementBottom->backupHeatSource = compressor;

    resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    resistiveElementBottom->followedByHeatSource = compressor;

    model = MODELS_genericCustomUEF;

    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isOn)
        {
            isHeating = true;
        }
    }
    compressor->sortPerformanceMap();
}

void HPWH::initPreset(MODELS presetNum)
{
    if (presetNum == MODELS_StorageTank)
    {
        setAllDefaults();

        heatSources.clear();

        setNumNodes(12);
        setpoint_C = 800.;

        tank->volumeFixed = false;
        tank->volume_L = GAL_TO_L(80);
        tank->UA_kJperHrC = 10; // 0 to turn off

        doTempDepression = false;
        tank->mixesOnDraw = false;

        setTankToTemperature(F_TO_C(127.));
    }
    else
    {
        init(presetNum);
        // If the model is the TamOMatic, HotTam, Generic... This model is scalable.
        if ((MODELS_TamScalable_SP <= presetNum) && (presetNum <= MODELS_TamScalable_SP_Half))
        {
            tank->volumeFixed = false; // a fully scalable model
            canScale = true;
        }
        else if (presetNum == MODELS_Scalable_MP)
        {
            tank->volumeFixed = false;
            canScale = true;
        }
        else if (presetNum == MODELS_AOSmithHPTS50)
        {
        }
        resetTankToSetpoint();
    }

    model = presetNum;
    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i]->isOn)
        {
            isHeating = true;
        }
        if (heatSources[i]->isACompressor())
        {
            reinterpret_cast<Condenser*>(heatSources[i].get())->sortPerformanceMap();
        }
    }

} // end HPWHinit_presets
