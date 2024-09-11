/*
Initialize all presets available in HPWHsim
*/

#include "HPWH.hh"
#include <btwxt/btwxt.h>

#include <algorithm>

void HPWH::initResistanceTank()
{
    // a default resistance tank, nominal 50 gallons, 0.95 EF, standard double 4.5 kW elements
    initResistanceTank({47.5, Units::gal}, 0.95, {4500, Units::W}, {4500, Units::W});
}

void HPWH::initResistanceTank(const Volume_t tankVol,
                              const RFactor_t rFactor,
                              const Power_t upperPower,
                              const Power_t lowerPower)
{
    setAllDefaults();

    heatSources.clear();

    // low power element will cause divide by zero/negative UA in EF -> UA conversion
    if (lowerPower(Units::W) < 550)
    {
        send_error("Resistance tank lower element wattage below 550 W.");
    }
    if (upperPower(Units::W) < 0.)
    {
        send_error("Upper resistance tank wattage below 0 W.");
    }
    if (rFactor <= 0.)
    {
        send_error("Energy Factor less than zero.");
    }

    setNumNodes(12);

    // use tank size setting function since it has bounds checking
    tankSizeFixed = false;
    setTankSize(tankVol);

    setpointT = {127.0, Units::F};

    // start tank off at setpoint
    resetTankToSetpoint();

    doTempDepression = false;
    tankMixesOnDraw = true;

    HeatSource* resistiveElementTop = NULL;
    HeatSource* resistiveElementBottom = NULL;

    heatSources.reserve(2);
    if (upperPower > 0.)
    {
        // Only add an upper element when the upperPower_W > 0 otherwise ignore this.
        // If the element is added this can mess with the intended logic.
        resistiveElementTop = addHeatSource("resistiveElementTop");
        resistiveElementTop->setupAsResistiveElement(8, upperPower);

        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->isVIP = true;
    }

    //
    resistiveElementBottom = addHeatSource("resistiveElementBottom");
    resistiveElementBottom->setupAsResistiveElement(0, lowerPower);

    // standard logic conditions
    resistiveElementBottom->addTurnOnLogic(bottomThird({40, Units::dF}));
    resistiveElementBottom->addTurnOnLogic(standby({10, Units::dF}));

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
    double numerator = (1.0 / rFactor(Units::ft2hF_per_Btu)) - (1.0 / recoveryEfficiency);
    double temp = 1.0 / (recoveryEfficiency * lowerPower * 3.41443);
    double denominator = 67.5 * ((24.0 / 41094.0) - temp);
    tankUA = UA_t(numerator / denominator, Units::Btu_per_hF);

    if (tankUA < 0.)
    {
        if (tankUA(Units::kJ_per_hC) < -0.1)
        {
            send_warning("Computed tankUA_kJperHrC is less than 0, and is reset to 0.");
        }
        tankUA = 0.0;
    }

    model = MODELS_CustomResTank;

    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isOn)
        {
            isHeating = true;
        }
        heatSources[i].sortPerformanceMap();
    }
}

void HPWH::initResistanceTankGeneric(Volume_t tankVol,
                                     RFactor_t rFactor,
                                     Power_t upperPower,
                                     Power_t lowerPower)
{

    setAllDefaults(); // reset all defaults if you're re-initilizing
    // sets simHasFailed = true; this gets cleared on successful completion of init
    // return 0 on success, HPWH_ABORT for failure
    heatSources.clear();
    // low power element will cause divide by zero/negative UA in EF -> UA conversion
    if (lowerPower < 0)
    {
        send_error("Lower resistance tank wattage below 0 W.");
    }
    if (upperPower < 0.)
    {
        send_error("Upper resistance tank wattage below 0 W.");
    }
    if (rFactor <= 0.)
    {
        send_error("R-Value is equal to or below 0.");
    }

    setNumNodes(12);

    // set tank size function has bounds checking
    tankSizeFixed = false;
    setTankSize(tankVol);
    canScale = true;

    setpointT = {127.0, Units::F};
    resetTankToSetpoint(); // start tank off at setpoint

    doTempDepression = false;
    tankMixesOnDraw = true;

    HeatSource* resistiveElementTop = {};
    HeatSource* resistiveElementBottom = {};

    heatSources.reserve(2);
    if (upperPower > 0.)
    {
        resistiveElementTop = addHeatSource("resistiveElementTop");
        resistiveElementTop->setupAsResistiveElement(8, upperPower);

        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->isVIP = true;
    }

    if (lowerPower > 0.)
    {
        resistiveElementBottom = addHeatSource("resistiveElementBottom");
        resistiveElementBottom->setupAsResistiveElement(0, lowerPower);

        resistiveElementBottom->addTurnOnLogic(bottomThird({40., Units::dF}));
        resistiveElementBottom->addTurnOnLogic(standby({10., Units::dF}));
    }

    if (resistiveElementTop && resistiveElementBottom)
    {
        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    }

    // Calc UA
    Area_t SA = getTankSurfaceArea(tankVol);
    double tankUA_WperC = SA(Units::m2) / rFactor(Units::m2C_per_W);
    tankUA = Units::scale(Units::W, Units::kJ_per_h) * tankUA_WperC;

    if (tankUA < 0.)
    {
        if (tankUA(Units::kJ_per_hC) < -0.1)
        {
            send_warning("Computed tankUA_kJperHrC is less than 0, and is reset to 0.");
        }
        tankUA = 0.0;
    }

    model = MODELS_CustomResTankGeneric;

    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (auto& source : heatSources)
    {
        if (source.isOn)
        {
            isHeating = true;
        }
        source.sortPerformanceMap();
    }
}

void HPWH::initGeneric(const Volume_t tankVol, RFactor_t rFactor, Temp_d_t resUse_dT)
{
    setAllDefaults(); // reset all defaults if you're re-initializing
    heatSources.clear();

    // except where noted, these values are taken from MODELS_GE2014STDMode on 5/17/16
    setNumNodes(12);
    setpointT = {127.0, Units::F};

    // start tank off at setpoint
    resetTankToSetpoint();

    tankSizeFixed = false;

    doTempDepression = false;
    tankMixesOnDraw = true;

    heatSources.reserve(3);
    auto resistiveElementTop = addHeatSource("resistiveElementTop");
    auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
    auto compressor = addHeatSource("compressor");

    // compressor values
    compressor->isOn = false;
    compressor->isVIP = false;
    compressor->typeOfHeatSource = TYPE_compressor;

    compressor->setCondensity({1., 0., 0.});

    compressor->perfMap.reserve(2);

    compressor->perfMap.push_back({
        {50, Units::F},                          // Temperature
        {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
        {5.4977772, -0.0243008, 0.0}             // COP_coeffs
    });

    compressor->perfMap.push_back({
        {70, Units::F},                        // Temperature
        {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
        {7.207307, -0.0335265, 0.0}            // COP_coeffs
    });

    compressor->minT = {45., Units::F};
    compressor->maxT = {120., Units::F};
    compressor->hysteresis_dT = {2, Units::dF};
    compressor->configuration = HeatSource::CONFIG_WRAPPED;
    compressor->maxSetpointT = MAXOUTLET_R134A();

    // top resistor values
    resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
    resistiveElementTop->isVIP = true;

    // bottom resistor values
    resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
    resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
    resistiveElementBottom->hysteresis_dT = {2, Units::dF};

    // logic conditions
    // this is set customly, from input
    // resistiveElementTop->addTurnOnLogic(topThird(Temp_d_t(19.6605, Units::F)));
    resistiveElementTop->addTurnOnLogic(topThird(GenTemp_t(resUse_dT)));

    resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({86.1111, Units::F}));

    compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
    compressor->addTurnOnLogic(standby({12.392, Units::dF}));

    // custom adjustment for poorer performance
    // compressor->addShutOffLogic(lowT(Temp_t(37, Units::F)));
    //
    // end section of parameters from GE model

    // set tank volume from input
    // use tank size setting function since it has bounds checking
    setTankSize(tankVol);

    // derive conservative (high) UA from tank volume
    //   curve fit by Jim Lutz, 5-25-2016
    double tankVol_gal = tankVol(Units::gal);
    double v1 = 7.5156316175 * pow(tankVol_gal, 0.33) + 5.9995357658;
    tankUA = 0.0076183819 * v1 * v1;

    // do a linear interpolation to scale COP curve constant, using measured values
    //  Chip's attempt 24-May-2014
    double uefSpan = 3.4 - 2.0;

    // force COP to be 70% of GE at UEF 2 and 95% at UEF 3.4
    // use a fudge factor to scale cop and input power in tandem to maintain constant capacity
    double fUEF = (rFactor - 2.0) / uefSpan;
    double genericFudge = (1. - fUEF) * .7 + fUEF * .95;

    compressor->perfMap[0].COP_coeffs[0] *= genericFudge;
    compressor->perfMap[0].COP_coeffs[1] *= genericFudge;
    compressor->perfMap[0].COP_coeffs[2] *= genericFudge;

    compressor->perfMap[1].COP_coeffs[0] *= genericFudge;
    compressor->perfMap[1].COP_coeffs[1] *= genericFudge;
    compressor->perfMap[1].COP_coeffs[2] *= genericFudge;

    compressor->perfMap[0].inputPower_coeffs[0] /= genericFudge;
    compressor->perfMap[0].inputPower_coeffs[1] /= genericFudge;
    compressor->perfMap[0].inputPower_coeffs[2] /= genericFudge;

    compressor->perfMap[1].inputPower_coeffs[0] /= genericFudge;
    compressor->perfMap[1].inputPower_coeffs[1] /= genericFudge;
    compressor->perfMap[1].inputPower_coeffs[2] /= genericFudge;

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
        if (heatSources[i].isOn)
        {
            isHeating = true;
        }
        heatSources[i].sortPerformanceMap();
    }
}

void HPWH::initPreset(MODELS presetNum)
{
    setAllDefaults();

    heatSources.clear();

    bool hasInitialTankTemp = false;
    Temp_t initialTankT(120, Units::F);

    // resistive with no UA losses for testing
    if (presetNum == MODELS_restankNoUA)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankSizeFixed = false;
        tankVolume = {50, Units::gal};
        tankUA = 0; // 0 to turn off

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(2);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});

        // standard logic conditions
        resistiveElementBottom->addTurnOnLogic(bottomThird({40, Units::dF}));
        resistiveElementBottom->addTurnOnLogic(standby({10, Units::dF}));

        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->isVIP = true;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    }

    // resistive tank with massive UA loss for testing
    else if (presetNum == MODELS_restankHugeUA)
    {
        setNumNodes(12);
        setpointT = 50;

        tankSizeFixed = false;
        tankVolume = {120, Units::L};
        tankUA = {500, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;

        // set up a resistive element at the bottom, 4500 kW
        heatSources.reserve(2);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementTop->setupAsResistiveElement(9, {4500, Units::W});

        // standard logic conditions
        resistiveElementBottom->addTurnOnLogic(bottomThird({20, Units::dC}));
        resistiveElementBottom->addTurnOnLogic(standby({15, Units::dC}));

        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dC}));
        resistiveElementTop->isVIP = true;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    }

    // realistic resistive tank
    else if (presetNum == MODELS_restankRealistic)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankSizeFixed = false;
        tankVolume = {50, Units::gal};
        tankUA = {10, Units::kJ_per_hC};

        doTempDepression = false;
        // should eventually put tankmixes to true when testing progresses
        tankMixesOnDraw = false;

        heatSources.reserve(2);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementTop->setupAsResistiveElement(9, {4500, Units::W});

        // standard logic conditions
        resistiveElementBottom->addTurnOnLogic(bottomThird({20, Units::dC}));
        resistiveElementBottom->addTurnOnLogic(standby({15, Units::dC}));

        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dC}));
        resistiveElementTop->isVIP = true;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
    }

    else if (presetNum == MODELS_StorageTank)
    {
        setNumNodes(12);
        setpointT = 800.;

        initialTankT = {127., Units::F};
        hasInitialTankTemp = true;

        tankSizeFixed = false;
        tankVolume = {80, Units::gal};
        tankUA = {10, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;
    }

    // basic compressor tank for testing
    else if (presetNum == MODELS_basicIntegrated)
    {
        setNumNodes(12);
        setpointT = 50;

        tankSizeFixed = false;
        tankVolume = {120, Units::gal};
        tankUA = {10, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto compressor = addHeatSource("compressor");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementTop->setupAsResistiveElement(9, {4500, Units::W});

        resistiveElementBottom->hysteresis_dT = {4, Units::dF};

        // standard logic conditions
        resistiveElementBottom->addTurnOnLogic(bottomThird({20, Units::dF}));
        resistiveElementBottom->addTurnOnLogic(standby({15, Units::dF}));

        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->isVIP = true;

        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        // double oneSixth = 1.0 / 6.0;
        compressor->setCondensity({1., 0.});

        // GE tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {47, Units::F},
            {{0.290, 0.00159, 0.00000107}, Units::kW}, // inputPower_coeffs
            {4.49, -0.0187, -0.0000133}                // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},
            {{0.375, 0.00121, 0.00000216}, Units::W}, // inputPower_coeffs
            {5.60, -0.0252, 0.00000254}               // COP_coeffs
        });

        compressor->minT = {0., Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {4, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED; // wrapped around tank
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->addTurnOnLogic(bottomThird({20, Units::dF}));
        compressor->addTurnOnLogic(standby({20, Units::dC}));

        //
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        compressor->followedByHeatSource = resistiveElementBottom;
    }

    // simple external style for testing
    else if (presetNum == MODELS_externalTest)
    {
        setNumNodes(96);
        setpointT = {50, Units::C};

        tankSizeFixed = false;
        tankVolume = {120, Units::L};
        tankUA = 0;

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

        // GE tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {47, Units::F},
            {{0.290, 0.00159, 0.00000107}, Units::kW}, // inputPower_coeffs
            {4.49, -0.0187, -0.0000133}                // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},
            {{0.375, 0.00121, 0.00000216}, Units::kW}, // inputPower_coeffs
            {5.60, -0.0252, 0.00000254}                // COP_coeffs
        });

        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = 0; // no hysteresis
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->addTurnOnLogic(bottomThird({20, Units::dC}));
        compressor->addTurnOnLogic(standby({15, Units::C}));

        // lowT cutoff
        compressor->addShutOffLogic(bottomNodeMaxTemp({20, Units::C}, true));
    }
    // voltex 60 gallon
    else if (presetNum == MODELS_AOSmithPHPT60)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {215.8, Units::L};
        tankUA = {7.31, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto compressor = addHeatSource("compressor");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        double split = 1.0 / 5.0;
        compressor->setCondensity({split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);
        compressor->perfMap.push_back(
            {{47, Units::F},
             {{0.467, 0.00281, 0.0000072}, Units::kW}, // inputPower_coeffs
             {4.86, -0.0222, -0.00001}});              // COP_coeffs

        compressor->perfMap.push_back(
            {{67, Units::F},
             {{0.541, 0.00147, 0.0000176}, Units::kW}, // inputPower_coeffs
             {6.58, -0.0392, 0.0000407}}               // COP_coeffs
        );

        compressor->minT = {45.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {4, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4250, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {2000, Units::W});
        resistiveElementBottom->hysteresis_dT = {4, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT = {43.6, Units::dF};
        Temp_d_t standby_dT = {23.8, Units::dF};
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addTurnOnLogic(bottomThird(compStart_dT));

        resistiveElementTop->addTurnOnLogic(topThird({25, Units::dF}));

        //
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        compressor->followedByHeatSource = resistiveElementBottom;
    }
    else if (presetNum == MODELS_AOSmithPHPT80)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {283.9, Units::L};
        tankUA = {8.8, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto compressor = addHeatSource("compressor");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        double split = 1.0 / 5.0;
        compressor->setCondensity({split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {47, Units::F},
            {{0.467, 0.00281, 0.0000072}, Units::kW}, // inputPower_coeffs
            {4.86, -0.0222, -0.00001}                 // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},
            {{0.541, 0.00147, 0.0000176}, Units::kW}, // inputPower_coeffs
            {6.58, -0.0392, 0.0000407}                // COP_coeffs
        });

        compressor->minT = {45.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {4, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4250, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {2000, Units::W});
        resistiveElementBottom->hysteresis_dT = {4, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT = {43.6, Units::dF};
        Temp_d_t standby_dT = {23.8, Units::dF};
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addTurnOnLogic(bottomThird(compStart_dT));

        resistiveElementTop->addTurnOnLogic(topThird({25.0, Units::dF}));

        // and you have to do this after putting them into heatSources, otherwise
        // you don't get the right pointers
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        compressor->followedByHeatSource = resistiveElementBottom;
    }
    else if (presetNum == MODELS_GE2012)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {172, Units::L};
        tankUA = {6.8, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto compressor = addHeatSource("compressor");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        double split = 1.0 / 5.0;
        compressor->setCondensity({split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {47, Units::F},
            {{0.3, 0.00159, 0.00000107}, Units::kW}, // inputPower_coeffs
            {4.7, -0.0210, 0.0}                      // COP_coeffs

        });

        compressor->perfMap.push_back({
            {67, Units::F},
            {{0.378, 0.00121, 0.00000216}, Units::kW}, // inputPower_coeffs
            {4.8, -0.0167, 0.0}                        // COP_coeffs
        });

        compressor->minT = {45.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {4, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4200, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4200, Units::W});
        resistiveElementBottom->hysteresis_dT = {4, Units::dF};

        // logic conditions
        //     double compStart_dT = Temp_d_t(24.4, Units::F);
        Temp_d_t compStart_dT = {40.0, Units::dF};
        Temp_d_t standby_dT = {5.2, Units::dF};
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));
        // compressor->addShutOffLogic(largeDraw(Temp_t(66)));
        compressor->addShutOffLogic(largeDraw({65, Units::dF}));

        resistiveElementBottom->addTurnOnLogic(bottomThird(compStart_dT));
        // resistiveElementBottom->addShutOffLogic(lowTreheat(lowTcutoff));
        // GE element never turns off?

        // resistiveElementTop->addTurnOnLogic(topThird(Temp_d_t(31.0, Units::F)));
        resistiveElementTop->addTurnOnLogic(topThird({28.0, Units::F}));

        // and you have to do this after putting them into heatSources, otherwise
        // you don't get the right pointers
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        compressor->followedByHeatSource = resistiveElementBottom;
    }
    // If a Colmac single pass preset cold weather or not
    else if (MODELS_ColmacCxV_5_SP <= presetNum && presetNum <= MODELS_ColmacCxA_30_SP)
    {
        setNumNodes(96);
        setpointT = {135.0, Units::F};
        tankSizeFixed = false;

        doTempDepression = false;
        tankMixesOnDraw = false;

        tankVolume = {315, Units::L};
        tankUA = {7, Units::kJ_per_hC};

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->perfMap.reserve(1);
        compressor->hysteresis_dT = 0;

        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = getNumNodes() - 1;

        // logic conditions
        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(15, Units::dF), this));

        // lowT cutoff
        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(1);
        compressor->addShutOffLogic(
            std::make_shared<TempBasedHeatingLogic>("bottom node",
                                                    nodeWeights1,
                                                    Temp_d_t {15., Units::dF},
                                                    this,
                                                    std::greater<double>(),
                                                    true));
        compressor->depressesTemperature = false; // no temp depression

        // Defrost Derate
        compressor->setupDefrostMap();

        if (presetNum == MODELS_ColmacCxV_5_SP)
        {
            setTankSizeWithSameU({200., Units::gal});
            // logic conditions
            compressor->minT = {-4.0, Units::F};
            compressor->maxSetpointT = MAXOUTLET_R410A();

            compressor->perfMap.push_back({
                {100, Units::F},

                {{4.9621645063,
                  -0.0096084144,
                  -0.0095647009,
                  -0.0115911960,
                  -0.0000788517,
                  0.0000886176,
                  0.0001114142,
                  0.0001832377,
                  -0.0000451308,
                  0.0000411975,
                  0.0000003535},
                 Units::kW}, // inputPower_coeffs

                {3.8189922420,
                 0.0569412237,
                 -0.0320101962,
                 -0.0012859036,
                 0.0000576439,
                 0.0001101241,
                 -0.0000352368,
                 -0.0002630301,
                 -0.0000509365,
                 0.0000369655,
                 -0.0000000606} // COP_coeffs
            });
        }
        else
        {
            // logic conditions
            compressor->minT = {40., Units::F};
            compressor->maxSetpointT = MAXOUTLET_R134A();

            if (presetNum == MODELS_ColmacCxA_10_SP)
            {
                setTankSizeWithSameU({500., Units::gal});

                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{5.9786974243,
                      0.0194445115,
                      -0.0077802278,
                      0.0053809029,
                      -0.0000334832,
                      0.0001864310,
                      0.0001190540,
                      0.0000040405,
                      -0.0002538279,
                      -0.0000477652,
                      0.0000014101},
                     Units::kW}, // inputPower_coeffs

                    {3.6128563086,
                     0.0527064498,
                     -0.0278198945,
                     -0.0070529748,
                     0.0000934705,
                     0.0000781711,
                     -0.0000359215,
                     -0.0002223206,
                     0.0000359239,
                     0.0000727189,
                     -0.0000005037} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_15_SP)
            {
                setTankSizeWithSameU({600., Units::gal});

                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{15.5869846555,
                      -0.0044503761,
                      -0.0577941202,
                      -0.0286911185,
                      -0.0000803325,
                      0.0003399817,
                      0.0002009576,
                      0.0002494761,
                      -0.0000595773,
                      0.0001401800,
                      0.0000004312},
                     Units::kW}, // inputPower_coeffs

                    {1.6643120405,
                     0.0515623393,
                     -0.0110239930,
                     0.0041514430,
                     0.0000481544,
                     0.0000493424,
                     -0.0000262721,
                     -0.0002356218,
                     -0.0000989625,
                     -0.0000070572,
                     0.0000004108} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_20_SP)
            {
                setTankSizeWithSameU({800., Units::gal});

                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{23.0746692231,
                      0.0248584608,
                      -0.1417927282,
                      -0.0253733303,
                      -0.0004882754,
                      0.0006508079,
                      0.0002139934,
                      0.0005552752,
                      -0.0002026772,
                      0.0000607338,
                      0.0000021571},
                     Units::kW}, // inputPower_coeffs

                    {1.7692660120,
                     0.0525134783,
                     -0.0081102040,
                     -0.0008715405,
                     0.0001274956,
                     0.0000369489,
                     -0.0000293775,
                     -0.0002778086,
                     -0.0000095067,
                     0.0000381186,
                     -0.0000003135} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_25_SP)
            {
                setTankSizeWithSameU({1000., Units::gal});

                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{20.4185336541,
                      -0.0236920615,
                      -0.0736219119,
                      -0.0260385082,
                      -0.0005048074,
                      0.0004940510,
                      0.0002632660,
                      0.0009820050,
                      -0.0000223587,
                      0.0000885101,
                      0.0000005649},
                     Units::kW}, // inputPower_coeffs

                    {0.8942843854,
                     0.0677641611,
                     -0.0001582927,
                     0.0048083998,
                     0.0001196407,
                     0.0000334921,
                     -0.0000378740,
                     -0.0004146401,
                     -0.0001213363,
                     -0.0000031856,
                     0.0000006306} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_30_SP)
            {
                setTankSizeWithSameU({1200., Units::gal});

                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{11.3687485772,
                      -0.0207292362,
                      0.0496254077,
                      -0.0038394967,
                      -0.0005991041,
                      0.0001304318,
                      0.0003099774,
                      0.0012092717,
                      -0.0001455509,
                      -0.0000893889,
                      0.0000018221},
                     Units::kW}, // inputPower_coeffs

                    {4.4170108542,
                     0.0596384263,
                     -0.0416104579,
                     -0.0017199887,
                     0.0000774664,
                     0.0001521934,
                     -0.0000251665,
                     -0.0003289731,
                     -0.0000801823,
                     0.0000325972,
                     0.0000002705} // COP_coeffs
                });
            }
        } // End if MODELS_ColmacCxV_5_SP
    }

    // if colmac multipass
    else if (MODELS_ColmacCxV_5_MP <= presetNum && presetNum <= MODELS_ColmacCxA_30_MP)
    {
        setNumNodes(24);
        setpointT = {135.0, Units::F};
        tankSizeFixed = false;

        doTempDepression = false;
        tankMixesOnDraw = false;

        tankVolume = {315, Units::L};
        tankUA = {7, Units::kJ_per_hC};

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->perfMap.reserve(1);
        compressor->hysteresis_dT = 0;
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = static_cast<int>(getNumNodes() / 3) - 1;

        // logic conditions
        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(5., Units::dF), this));

        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(4);
        compressor->addShutOffLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights1, Temp_d_t(0., Units::dF), this, std::greater<double>()));

        compressor->depressesTemperature = false; // no temp depression

        // Defrost Derate
        compressor->setupDefrostMap();

        if (presetNum == MODELS_ColmacCxV_5_MP)
        {
            setTankSizeWithSameU({200., Units::gal});
            compressor->mpFlowRate = {9., Units::gal_per_min};
            // https://colmacwaterheat.com/wp-content/uploads/2020/10/Technical-Datasheet-Air-Source.pdf

            // logic conditions
            compressor->minT = {-4.0, Units::F};
            compressor->maxT = {105., Units::F};
            compressor->maxSetpointT = MAXOUTLET_R410A();
            compressor->perfMap.push_back({
                {100, Units::F},

                {{5.8438525529,
                  0.0003288231,
                  -0.0494255840,
                  -0.0000386642,
                  0.0004385362,
                  0.0000647268},
                 Units::kW}, // inputPower_coeffs

                {0.6679056901,
                 0.0499777846,
                 0.0251828292,
                 0.0000699764,
                 -0.0001552229,
                 -0.0002911167} // COP_coeffs
            });
        }
        else
        {
            // logic conditions
            compressor->minT = {40., Units::F};
            compressor->maxT = {105., Units::F};
            compressor->maxSetpointT = MAXOUTLET_R134A();

            if (presetNum == MODELS_ColmacCxA_10_MP)
            {
                setTankSizeWithSameU({500., Units::gal});
                compressor->mpFlowRate = {18., Units::gal_per_min};
                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{8.6918824405,
                      0.0136666667,
                      -0.0548348214,
                      -0.0000208333,
                      0.0005301339,
                      -0.0000250000},
                     Units::kW}, // inputPower_coeffs

                    {0.6944181117,
                     0.0445926666,
                     0.0213188804,
                     0.0001172913,
                     -0.0001387694,
                     -0.0002365885} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_15_MP)
            {
                setTankSizeWithSameU({600., Units::gal});
                compressor->mpFlowRate = {26., Units::gal_per_min};
                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{12.4908723958,
                      0.0073988095,
                      -0.0411417411,
                      0.0000000000,
                      0.0005789621,
                      0.0000696429},
                     Units::kW}, // inputPower_coeffs

                    {1.2846349520,
                     0.0334658309,
                     0.0019121906,
                     0.0002840970,
                     0.0000497136,
                     -0.0004401737} // COP_coeffs

                });
            }
            else if (presetNum == MODELS_ColmacCxA_20_MP)
            {
                setTankSizeWithSameU({800., Units::gal});
                compressor->mpFlowRate = {36., Units::gal_per_min};
                // https://colmacwaterheat.com/wp-content/uploads/2020/10/Technical-Datasheet-Air-Source.pdf

                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{14.4893345424,
                      0.0355357143,
                      -0.0476593192,
                      -0.0002916667,
                      0.0006120954,
                      0.0003607143},
                     Units::kW}, // inputPower_coeffs

                    {1.2421582831,
                     0.0450256569,
                     0.0051234755,
                     0.0001271296,
                     -0.0000299981,
                     -0.0002910606} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_25_MP)
            {
                setTankSizeWithSameU({1000., Units::gal});
                compressor->mpFlowRate = {32., Units::gal_per_min};
                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{14.5805808222,
                      0.0081934524,
                      -0.0216169085,
                      -0.0001979167,
                      0.0007376535,
                      0.0004955357},
                     Units::kW}, // inputPower_coeffs

                    {2.0013175767,
                     0.0576617432,
                     -0.0130480870,
                     0.0000856818,
                     0.0000610760,
                     -0.0003684106} // COP_coeffs
                });
            }
            else if (presetNum == MODELS_ColmacCxA_30_MP)
            {
                setTankSizeWithSameU({1200., Units::gal});
                compressor->mpFlowRate = {41., Units::gal_per_min};
                compressor->perfMap.push_back({
                    {100, Units::F},

                    {{14.5824911644,
                      0.0072083333,
                      -0.0278055246,
                      -0.0002916667,
                      0.0008841378,
                      0.0008125000},
                     Units::kW}, // inputPower_coeffs

                    {2.6996807527,
                     0.0617507969,
                     -0.0220966420,
                     0.0000336149,
                     0.0000890989,
                     -0.0003682431} // COP_coeffs
                });
            }
        }
    }
    // If Nyle single pass preset
    else if (MODELS_NyleC25A_SP <= presetNum && presetNum <= MODELS_NyleC250A_C_SP)
    {
        setNumNodes(96);
        setpointT = {135.0, Units::F};
        tankSizeFixed = false;

        tankVolume = {315, Units::L};
        tankUA = {7, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->extrapolationMethod = EXTRAP_NEAREST;
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->perfMap.reserve(1);
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = getNumNodes() - 1;

        // logic conditions
        if (MODELS_NyleC25A_SP <= presetNum && presetNum <= MODELS_NyleC250A_SP)
        { // If not cold weather package
            compressor->minT = {40., Units::F};
        }
        else
        {
            compressor->minT = {35., Units::F};
        }
        compressor->maxT = {120.0, Units::F}; // Max air temperature
        compressor->hysteresis_dT = 0;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // Defines the maximum outlet temperature at the a low air temperature
        compressor->maxOut_at_LowT.outT = {140., Units::F};
        compressor->maxOut_at_LowT.airT = {40., Units::F};

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(15., Units::dF), this));

        // lowT cutoff
        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(1);
        compressor->addShutOffLogic(
            std::make_shared<TempBasedHeatingLogic>("bottom node",
                                                    nodeWeights1,
                                                    Temp_d_t(15., Units::dF),
                                                    this,
                                                    std::greater<double>(),
                                                    true));
        compressor->depressesTemperature = false; // no temp depression

        // Defrost Derate
        compressor->setupDefrostMap();

        // Perfmaps for each compressor size
        if (presetNum == MODELS_NyleC25A_SP)
        {
            setTankSizeWithSameU({200., Units::gal});
            compressor->perfMap.push_back({
                {90, Units::F},

                {{4.060120364,
                  -0.020584279,
                  -0.024201054,
                  -0.007023945,
                  0.000017461,
                  0.000110366,
                  0.000060338,
                  0.000120015,
                  0.000111068,
                  0.000138907,
                  -0.000001569},
                 Units::kW}, // inputPower_coeffs

                {0.462979529,
                 0.065656840,
                 0.001077377,
                 0.003428059,
                 0.000243692,
                 0.000021522,
                 0.000005143,
                 -0.000384778,
                 -0.000404744,
                 -0.000036277,
                 0.000001900} // COP_coeffs
            });
        }
        else if (presetNum == MODELS_NyleC60A_SP || presetNum == MODELS_NyleC60A_C_SP)
        {
            setTankSizeWithSameU({300., Units::gal});
            compressor->perfMap.push_back({
                {90, Units::F},

                {{-0.1180905709,
                  0.0045354306,
                  0.0314990479,
                  -0.0406839757,
                  0.0002355294,
                  0.0000818684,
                  0.0001943834,
                  -0.0002160871,
                  0.0003053633,
                  0.0003612413,
                  -0.0000035912},
                 Units::kW}, // inputPower_coeffs

                {6.8205043418,
                 0.0860385185,
                 -0.0748330699,
                 -0.0172447955,
                 0.0000510842,
                 0.0002187441,
                 -0.0000321036,
                 -0.0003311463,
                 -0.0002154270,
                 0.0001307922,
                 0.0000005568} // COP_coeffs
            });
        }
        else if (presetNum == MODELS_NyleC90A_SP || presetNum == MODELS_NyleC90A_C_SP)
        {
            setTankSizeWithSameU({400., Units::gal});
            compressor->perfMap.push_back({
                {90, Units::F},

                {{13.27612215047,
                  -0.01014009337,
                  -0.13401028549,
                  -0.02325705976,
                  -0.00032515646,
                  0.00040270625,
                  0.00001988733,
                  0.00069451670,
                  0.00069067890,
                  0.00071091372,
                  -0.00000854352},
                 Units::kW}, // inputPower_coeffs

                {1.49112327987,
                 0.06616282153,
                 0.00715307252,
                 -0.01269458185,
                 0.00031448571,
                 0.00001765313,
                 0.00006002498,
                 -0.00045661397,
                 -0.00034003896,
                 -0.00004327766,
                 0.00000176015} // COP_coeffs
            });
        }
        else if (presetNum == MODELS_NyleC125A_SP || presetNum == MODELS_NyleC125A_C_SP)
        {
            setTankSizeWithSameU({500., Units::gal});
            compressor->perfMap.push_back({
                {90, Units::F},

                {{-3.558277209,
                  -0.038590968,
                  0.136307181,
                  -0.016945699,
                  0.000983753,
                  -5.18201E-05,
                  0.000476904,
                  -0.000514211,
                  -0.000359172,
                  0.000266509,
                  -1.58646E-07},
                 Units::kW}, // inputPower_coeffs

                {4.889555031,
                 0.117102769,
                 -0.060005795,
                 -0.011871234,
                 -1.79926E-05,
                 0.000207293,
                 -1.4452E-05,
                 -0.000492486,
                 -0.000376814,
                 7.85911E-05,
                 1.47884E-06} // COP_coeffs
            });
        }
        else if (presetNum == MODELS_NyleC185A_SP || presetNum == MODELS_NyleC185A_C_SP)
        {
            setTankSizeWithSameU({800., Units::gal});
            compressor->perfMap.push_back({
                {90, Units::F},

                {{18.58007733,
                  -0.215324777,
                  -0.089782421,
                  0.01503161,
                  0.000332503,
                  0.000274216,
                  2.70498E-05,
                  0.001387914,
                  0.000449199,
                  0.000829578,
                  -5.28641E-06},
                 Units::kW}, // inputPower_coeffs

                {-0.629432348,
                 0.181466663,
                 0.00044047,
                 0.012104957,
                 -6.61515E-05,
                 9.29975E-05,
                 9.78042E-05,
                 -0.000872708,
                 -0.001013945,
                 -0.00021852,
                 5.55444E-06} // COP_coeffs
            });
        }
        else if (presetNum == MODELS_NyleC250A_SP || presetNum == MODELS_NyleC250A_C_SP)
        {
            setTankSizeWithSameU({800., Units::gal});
            compressor->perfMap.push_back({
                {90, Units::F},

                {{-13.89057656,
                  0.025902417,
                  0.304250541,
                  0.061695153,
                  -0.001474249,
                  -0.001126845,
                  -0.000220192,
                  0.001241026,
                  0.000571009,
                  -0.000479282,
                  9.04063E-06},
                 Units::kW}, // inputPower_coeffs

                {7.443904067,
                 0.185978755,
                 -0.098481635,
                 -0.002500073,
                 0.000127658,
                 0.000444321,
                 0.000139547,
                 -0.001000195,
                 -0.001140199,
                 -8.77557E-05,
                 4.87405E-06} // COP_coeffs
            });
        }
    }

    // If Nyle multipass presets
    else if (MODELS_NyleC60A_MP <= presetNum && presetNum <= MODELS_NyleC250A_C_MP)
    {
        setNumNodes(24);
        setpointT = {135.0, Units::F};
        tankSizeFixed = false;

        doTempDepression = false;
        tankMixesOnDraw = false;

        tankVolume = {315, Units::L};
        tankUA = {7, Units::kJ_per_hC};

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->extrapolationMethod = EXTRAP_NEAREST;
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->hysteresis_dT = 0;
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = static_cast<int>(getNumNodes() / 3.) - 1;

        // logic conditions//logic conditions
        if (MODELS_NyleC60A_MP <= presetNum && presetNum <= MODELS_NyleC250A_MP)
        {                                       // If not cold weather package
            compressor->minT = {40., Units::F}; // Min air temperature sans Cold Weather Package
        }
        else
        {
            compressor->minT = {35., Units::F}; // Min air temperature WITH Cold Weather Package
        }
        compressor->maxT = {130.0, Units::F}; // Max air temperature
        compressor->maxSetpointT = {160., Units::F};

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(5., Units::dF), this));

        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(4);
        compressor->addShutOffLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights1, Temp_d_t(0., Units::dF), this, std::greater<double>()));
        compressor->depressesTemperature = false; // no temp depression

        // Defrost Derate
        compressor->setupDefrostMap();

        // Performance grid
        compressor->perfGrid.reserve(2);
        compressor->perfGridValues.reserve(2);

        // Nyle MP models are all on the same grid axes
        const TempVect_t t0 = {{40., 60., 80., 90.}, Units::F};
        const TempVect_t t1 = {{40., 60., 80., 100., 130., 150.}, Units::F};

        compressor->perfGrid.push_back(t0); // Grid Axis 1 Tair (F)
        compressor->perfGrid.push_back(t1); // Grid Axis 2 Tin (F)

        if (presetNum == MODELS_NyleC60A_MP || presetNum == MODELS_NyleC60A_C_MP)
        {
            setTankSizeWithSameU({360., Units::gal});
            compressor->mpFlowRate = {13., Units::gal_per_min};
            if (presetNum == MODELS_NyleC60A_C_MP)
            {
                compressor->resDefrost = {
                    {4.5, Units::kW}, // inputPwr_kW;
                    {5.0, Units::dF}, // constTempLift_dF;
                    {40.0, Units::F}  // onBelowT_F
                };
            }
            const PowerVect_t powerV({3.64, 4.11, 4.86, 5.97,  8.68, 9.95, 3.72, 4.27,
                                      4.99, 6.03, 8.55, 10.02, 3.98, 4.53, 5.24, 6.24,
                                      8.54, 9.55, 4.45, 4.68,  5.37, 6.34, 8.59, 9.55},
                                     Units::kW);

            // Grid values in long format, table 1, input power
            compressor->perfGridValues.push_back(powerV);
            // Grid values in long format, table 2, COP
            compressor->perfGridValues.push_back(
                {3.362637363, 2.917274939, 2.407407407, 1.907872697, 1.296082949, 1.095477387,
                 4.438172043, 3.772833724, 3.132264529, 2.505804312, 1.678362573, 1.386227545,
                 5.467336683, 4.708609272, 3.921755725, 3.169871795, 2.165105386, 1.860732984,
                 5.512359551, 5.153846154, 4.290502793, 3.417981073, 2.272409779, 1.927748691});
        }
        else if (presetNum == MODELS_NyleC90A_MP || presetNum == MODELS_NyleC90A_C_MP)
        {
            setTankSizeWithSameU({480., Units::gal});
            compressor->mpFlowRate = {20., Units::gal_per_min};
            if (presetNum == MODELS_NyleC90A_C_MP)
            {
                compressor->resDefrost = {
                    {5.4, Units::kW}, // inputPwr_kW;
                    {5.0, Units::dF}, // constTempLift_dF;
                    {40.0, Units::F}  // onBelowT_F
                };
            }

            const PowerVect_t powerV({4.41,  6.04,  7.24,  9.14,  12.23, 14.73, 4.78,  6.61,
                                      7.74,  9.40,  12.47, 14.75, 5.51,  6.66,  8.44,  9.95,
                                      13.06, 15.35, 6.78,  7.79,  8.81,  10.01, 11.91, 13.35},
                                     Units::kW);

            // Grid values in long format, table 1, input power
            compressor->perfGridValues.push_back(powerV);

            // Grid values in long format, table 2, COP
            compressor->perfGridValues.push_back(
                {4.79138322,  3.473509934, 2.801104972, 2.177242888, 1.569910057, 1.272233537,
                 6.071129707, 4.264750378, 3.536175711, 2.827659574, 2.036086608, 1.666440678,
                 7.150635209, 5.659159159, 4.305687204, 3.493467337, 2.487748851, 2.018241042,
                 6.750737463, 5.604621309, 4.734392736, 3.94005994,  3.04534005,  2.558801498});
        }
        else if (presetNum == MODELS_NyleC125A_MP || presetNum == MODELS_NyleC125A_C_MP)
        {
            setTankSizeWithSameU({600., Units::gal});
            compressor->mpFlowRate = {28., Units::gal_per_min};
            if (presetNum == MODELS_NyleC125A_C_MP)
            {
                compressor->resDefrost = {
                    {9.0, Units::kW}, // inputPwr_kW;
                    {5.0, Units::dF}, // constTempLift_dF;
                    {40.0, Units::F}  // onBelowT_F
                };
            }
            const PowerVect_t powerV({6.4,   7.72,  9.65,  12.54, 20.54, 24.69, 6.89,  8.28,
                                      10.13, 12.85, 19.75, 24.39, 7.69,  9.07,  10.87, 13.44,
                                      19.68, 22.35, 8.58,  9.5,   11.27, 13.69, 19.72, 22.4},
                                     Units::kW);

            // Grid values in long format, table 1, input power
            compressor->perfGridValues.push_back(powerV);

            // Grid values in long format, table 2, COP
            compressor->perfGridValues.push_back(
                {4.2390625,   3.465025907, 2.718134715, 2.060606061, 1.247809153, 1.016605913,
                 5.374455733, 4.352657005, 3.453109576, 2.645136187, 1.66278481,  1.307093071,
                 6.503250975, 5.276736494, 4.229070837, 3.27827381,  2.113821138, 1.770469799,
                 6.657342657, 5.749473684, 4.612244898, 3.542731921, 2.221095335, 1.816964286});
        }
        else if (presetNum == MODELS_NyleC185A_MP || presetNum == MODELS_NyleC185A_C_MP)
        {
            setTankSizeWithSameU({960., Units::gal});
            compressor->mpFlowRate = {40., Units::gal_per_min};
            if (presetNum == MODELS_NyleC185A_C_MP)
            {
                compressor->resDefrost = {
                    {7.2, Units::kW}, // inputPwr_kW;
                    {5.0, Units::dF}, // constTempLift_dF;
                    {40.0, Units::F}  // onBelowTemp_F
                };
            }
            // Grid values in long format, table 1, input power
            const PowerVect_t powerV({7.57,  11.66, 14.05, 18.3,  25.04, 30.48, 6.99,  10.46,
                                      14.28, 18.19, 26.24, 32.32, 7.87,  12.04, 15.02, 18.81,
                                      25.99, 31.26, 8.15,  12.46, 15.17, 18.95, 26.23, 31.62},
                                     Units::kW);
            compressor->perfGridValues.push_back(powerV);

            // Grid values in long format, table 2, COP
            compressor->perfGridValues.push_back(
                {5.531043593, 3.556603774, 2.918149466, 2.214754098, 1.590255591, 1.291010499,
                 8.010014306, 5.258126195, 3.778711485, 2.916437603, 1.964176829, 1.56404703,
                 9.65819568,  6.200166113, 4.792276964, 3.705475811, 2.561369758, 2.05950096,
                 10.26993865, 6.350722311, 5.04218853,  3.841688654, 2.574151735, 2.025616698});
        }
        else if (presetNum == MODELS_NyleC250A_MP || presetNum == MODELS_NyleC250A_C_MP)
        {
            setTankSizeWithSameU({960., Units::gal});
            compressor->mpFlowRate = {50., Units::gal_per_min};
            if (presetNum == MODELS_NyleC250A_C_MP)
            {
                compressor->resDefrost = {
                    {18.0, Units::kW}, // inputPwr_kW;
                    {5.0, Units::dF},  // constTempLift_dF;
                    {40.0, Units::F}   // onBelowT_F
                };
            }
            // Grid values in long format, table 1, input power
            const PowerVect_t powerV({10.89, 12.23, 13.55, 14.58, 15.74, 16.72, 11.46, 13.76,
                                      15.97, 17.79, 20.56, 22.50, 10.36, 14.66, 18.07, 21.23,
                                      25.81, 29.01, 8.67,  15.05, 18.76, 21.87, 26.63, 30.02},
                                     Units::kW);
            compressor->perfGridValues.push_back(powerV);

            // Grid values in long format, table 2, COP
            compressor->perfGridValues.push_back(
                {5.81818181, 4.50040883, 3.69667896, 3.12414266, 2.38500635, 1.93540669,
                 7.24520069, 5.50145348, 4.39323732, 3.67734682, 2.73249027, 2.23911111,
                 10.6196911, 7.05320600, 5.41228555, 4.28638718, 3.04804339, 2.46053085,
                 14.7831603, 7.77903268, 5.71801705, 4.40237768, 2.92489673, 2.21419054});
        }

        // Set up regular grid interpolator.
        compressor->perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(
            Btwxt::RegularGridInterpolator(compressor->perfGrid,
                                           compressor->perfGridValues,
                                           "RegularGridInterpolator",
                                           get_courier()));
        compressor->useBtwxtGrid = true;
    }
    // if rheem multipass
    else if (MODELS_RHEEM_HPHD60HNU_201_MP <= presetNum &&
             presetNum <= MODELS_RHEEM_HPHD135VNU_483_MP)
    {
        setNumNodes(24);
        setpointT = Temp_t(135.0, Units::F);
        tankSizeFixed = false;

        doTempDepression = false;
        tankMixesOnDraw = false;

        tankVolume = {315, Units::L};
        tankUA = {7, Units::kJ_per_hC};

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->perfMap.reserve(1);
        compressor->hysteresis_dT = 0;
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = static_cast<int>(getNumNodes() / 3.) - 1;

        // logic conditions
        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(5., Units::dF), this));

        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(4);
        compressor->addShutOffLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights1, Temp_d_t(0), this, std::greater<double>()));
        compressor->depressesTemperature = false; // no temp depression

        // Defrost Derate
        compressor->setupDefrostMap();

        // logic conditions
        compressor->minT = {45., Units::F};
        compressor->maxT = Temp_t(110., Units::F);
        compressor->maxSetpointT = MAXOUTLET_R134A(); // data says 150...

        if (presetNum == MODELS_RHEEM_HPHD60HNU_201_MP ||
            presetNum == MODELS_RHEEM_HPHD60VNU_201_MP)
        {
            setTankSizeWithSameU({250., Units::gal});
            compressor->mpFlowRate = {17.4, Units::gal_per_min};
            compressor->perfMap.push_back({
                {110, Units::F},

                {{1.8558438453,
                  0.0120796155,
                  -0.0135443327,
                  0.0000059621,
                  0.0003010506,
                  -0.0000463525},
                 Units::kW}, // inputPower_coeffs

                {3.6840046360,
                 0.0995685071,
                 -0.0398107723,
                 -0.0001903160,
                 0.0000980361,
                 -0.0003469814} // COP_coeffs
            });
        }
        else if (presetNum == MODELS_RHEEM_HPHD135HNU_483_MP ||
                 presetNum == MODELS_RHEEM_HPHD135VNU_483_MP)
        {
            setTankSizeWithSameU({500., Units::gal});
            compressor->mpFlowRate = {34.87, Units::gal_per_min};
            compressor->perfMap.push_back({
                {110, Units::F},

                {{5.1838201136,
                  0.0247312962,
                  -0.0120766440,
                  0.0000493862,
                  0.0005422089,
                  -0.0001385078},
                 Units::kW}, // inputPower_coeffs

                {5.0207181209,
                 0.0442525790,
                 -0.0418284882,
                 0.0000793531,
                 0.0001132421,
                 -0.0002491563} // COP_coeffs
            });
        }
    }

    else if (presetNum == MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP)
    {
        setNumNodes(96);
        setpointT = {65, Units::C};

        tankVolume = {500, Units::gal};
        tankUA = {12, Units::kJ_per_hC};
        tankSizeFixed = false;

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->minT = {-13., Units::F};
        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = getNumNodes() - 1;

        // What to do about these?!
        compressor->hysteresis_dT = {4, Units::dC};
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->maxSetpointT = {176.1, Units::F};

        // Turn on
        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "eighth node absolute", nodeWeights, Temp_t(110., Units::F), this));

        // lowT cutoff
        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(1);
        compressor->addShutOffLogic(
            std::make_shared<TempBasedHeatingLogic>("bottom node",
                                                    nodeWeights1,
                                                    Temp_d_t(15., Units::dF),
                                                    this,
                                                    std::greater<double>(),
                                                    true));
        compressor->depressesTemperature = false;

        // Performance grid: externalT_F, Tout_F, condenserTemp_F
        compressor->perfGrid.reserve(2);
        compressor->perfGridValues.reserve(2);

        const TempVect_t tempV1({-13,  -11.2, -7.6, -4,   -0.4, 3.2,   6.8,  10.4, 14,
                                 17.6, 21.2,  24.8, 28.4, 32,   35.6,  39.2, 42.8, 46.4,
                                 50,   53.6,  57.2, 60.8, 64.4, 68,    71.6, 75.2, 78.8,
                                 82.4, 86,    89.6, 93.2, 96.8, 100.4, 104},
                                Units::F);

        compressor->perfGrid.push_back(tempV1); // Grid Axis 1 Tair

        const TempVect_t tempV2({140., 158., 176.}, Units::F);

        compressor->perfGrid.push_back(tempV2); // Grid Axis 2 Tout

        const TempVect_t tempV3({41, 48.2, 62.6, 75.2, 84.2}, Units::F);
        compressor->perfGrid.push_back(tempV3); // Grid Axis 3 Tin

        // Grid values in long format, table 1, input power (Btu/hr)

        const PowerVect_t powerV(
            {56518.565328,     57130.739544,     57094.73612,      57166.756616,
             57238.777112,     56518.565328,     57130.739544,     57094.73612,
             57166.756616,     57238.777112,     58061.348896,     58360.24692,
             58591.39286,      58870.368216,     59093.547136,     56626.5960719999,
             57202.763452,     57310.794196,     57310.794196,     57490.848848,
             56626.5960719999, 57202.763452,     57310.794196,     57310.794196,
             57490.848848,     58280.539188,     58519.65556,      58786.67868,
             59093.547136,     59372.5190799999, 56950.695128,     57850.95474,
             57814.947904,     57814.947904,     57922.978648,     56950.695128,
             57850.95474,      57814.947904,     57814.947904,     57922.978648,
             58679.067612,     58977.969048,     59372.5190799999, 59651.494436,
             59930.46638,      57418.831764,     58175.053796,     58283.08454,
             58247.07088,      58427.12212,      57418.831764,     58175.053796,
             58283.08454,      58247.07088,      58427.12212,      59137.384512,
             59376.504296,     59902.566456,     60265.23476,      60516.313604,
             57778.930832,     58643.190432,     58823.23826,      58643.190432,
             58967.282664,     57778.930832,     58643.190432,     58823.23826,
             58643.190432,     58967.282664,     59515.99368,      59954.377676,
             60321.031196,     60934.77152,      61213.743464,     58103.029888,
             59075.313408,     59219.357812,     59291.374896,     59579.45688,
             58103.029888,     59075.313408,     59219.357812,     59291.374896,
             59579.45688,      60014.159328,     60273.205192,     60934.77152,
             61436.922384,     61799.587276,     58535.152864,     59399.412464,
             59795.525192,     59903.555936,     60299.672076,     58535.152864,
             59399.412464,     59795.525192,     59903.555936,     60299.672076,
             60512.321564,     60751.448172,     61409.02246,      62022.769608,
             62329.641476,     58931.275828,     59651.4842,       60011.58668,
             60263.66524,      60551.747224,     58931.275828,     59651.4842,
             60011.58668,      60263.66524,      60551.747224,     60910.860224,
             61369.1703,       62022.769608,     62580.713496,     62803.895828,
             59219.357812,     60155.634496,     60443.71648,      60659.777968,
             61163.92144,      59219.357812,     60155.634496,     60443.71648,
             60659.777968,     61163.92144,      61329.321552,     61687.997816,
             62441.224112,     63166.56072,      63585.015224,     59651.4842,
             60479.726728,     61019.88386,      61163.92144,      61920.146884,
             59651.4842,       60479.726728,     61019.88386,      61163.92144,
             61920.146884,     61707.923896,     62186.166876,     63027.071336,
             63724.504608,     64170.862448,     59723.504696,     60839.83262,
             61379.993164,     61524.030744,     62352.273272,     59723.504696,
             60839.83262,      61379.993164,     61524.030744,     62352.273272,
             61966.96976,      62445.21274,      63361.839716,     64142.969348,
             64505.63424,      59759.511532,     60695.788216,     61524.030744,
             61884.136636,     62712.382576,     59759.511532,     60695.788216,
             61524.030744,     61884.136636,     62712.382576,     62166.240796,
             62744.114176,     63668.714996,     64282.451908,     64589.323776,
             59759.511532,     61019.88386,      61848.1298,       62064.191288,
             62964.4509,       59759.511532,     61019.88386,      61848.1298,
             62064.191288,     62964.4509,       62425.28666,      63003.16004,
             63975.58004,      64533.523928,     64617.2237,       59471.429548,
             61055.894108,     61956.160544,     62460.304016,     63216.526048,
             59471.429548,     61055.894108,     61956.160544,     62460.304016,
             63216.526048,     62624.550872,     63202.424252,     64310.351832,
             64756.70626,      64728.806336,     61632.061488,     63036.474808,
             63828.7105,       64296.847136,     65053.069168,     61632.061488,
             63036.474808,     63828.7105,       64296.847136,     65053.069168,
             63561.10734,      64099.125148,     65258.8605359999, 65593.625504,
             65621.525428,     51189.004268,     52917.506408,     54826.070024,
             57094.73612,      59327.388556,     51189.004268,     52917.506408,
             54826.070024,     57094.73612,      59327.388556,     55391.172288,
             58240.683616,     62106.459144,     65733.114888,     65705.214964,
             41286.100768,     42726.524336,     45931.460292,     49784.590948,
             53457.6669519999, 41286.100768,     42726.524336,     45931.460292,
             49784.590948,     53457.6669519999, 49433.093532,     51067.083272,
             57921.8561,       61604.3082799999, 64477.734316,     35632.444064,
             37108.87788,      41394.134924,     45139.228012,     49568.526048,
             35632.444064,     37108.87788,      41394.134924,     45139.228012,
             49568.526048,     47540.06134,      47221.233824,     56080.631716,
             59539.904976,     61883.276812,     34192.023908,     35596.430404,
             39809.6669519999, 43410.72246,      47659.969256,     34192.023908,
             35596.430404,     39809.6669519999, 43410.72246,      47659.969256,
             47559.98742,      47061.821772,     54490.482764,     57893.959588,
             60153.648712,     32895.641332,     34192.023908,     38225.202392,
             41646.20666,      45751.409052,     32895.641332,     34192.023908,
             38225.202392,     41646.20666,      45751.409052,     47659.621232,
             46842.624656,     52956.13366,      56220.1211,       58312.420916,
             31563.25192,      32715.58668,      36532.707088,     39881.687448,
             43914.869344,     31563.25192,      32715.58668,      36532.707088,
             39881.687448,     43914.869344,     47719.402884,     46742.994256,
             51393.877808,     54518.382688,     56554.886068,     30194.85226,
             31347.18702,      35092.286932,     38117.171648,     42006.312552,
             30194.85226,      31347.18702,      35092.286932,     38117.171648,
             42006.312552,     47799.1072039999, 46324.532928,     49692.139396,
             52732.951328,     54769.45812,      30446.923996,     31383.197268,
             34300.054652,     37144.884716,     40781.953884,     30446.923996,
             31383.197268,     34300.054652,     37144.884716,     40781.953884,
             45547.391924,     44212.307032,     47320.867636,     50389.57608,
             52202.90054,      30698.995732,     31311.176772,     33723.88386,
             36316.6456,       39701.636208,     30698.995732,     31311.176772,
             33723.88386,      36316.6456,       39701.636208,     43295.680056,
             41980.517832,     45089.078436,     47795.121988,     49720.03932,
             30987.081128,     31311.176772,     33183.726728,     35344.358668,
             38477.27754,      30987.081128,     31311.176772,     33183.726728,
             35344.358668,     38477.27754,      41562.059916,     40486.017476,
             43415.236536,     46037.590552,     47795.121988,     31203.146028,
             31311.176772,     32571.545688,     34552.126388,     37360.949616,
             31203.146028,     31311.176772,     32571.545688,     34552.126388,
             37360.949616,     40486.017476,     39310.3446359999, 42159.859376,
             44782.20998,      46511.844904,     31167.132368,     31311.176772,
             32355.4842,       34156.010248,     36712.758328,     31167.132368,
             31311.176772,     32355.4842,       34156.010248,     36712.758328,
             39449.830608,     38353.862088,     41127.657724,     43666.31538,
             45284.360844,     31059.101624,     31131.12212,      32319.473952,
             34156.010248,     36748.771988,     31059.101624,     31131.12212,
             32319.473952,     34156.010248,     36748.771988,     38592.985284,
             37457.161192,     40151.249096,     42801.496212,     44419.541676,
             30879.050384,     31023.091376,     32319.473952,     34192.023908,
             36820.789072,     30879.050384,     31023.091376,     32319.473952,
             34192.023908,     36820.789072,     37756.062628,     36680.0236,
             39258.536828,     41797.194484,     43387.343436,     30807.029888,
             30951.07088,      32427.504696,     34156.010248,     36748.771988,
             30807.029888,     30951.07088,      32427.504696,     34156.010248,
             36748.771988,     37795.9182,       36779.657412,     39258.536828,
             41769.29456,      43331.547,        30626.976942,     30735.007686,
             32319.473952,     34156.010248,     36748.771988,     30626.976942,
             30735.007686,     32319.473952,     34156.010248,     36748.771988,
             37815.84428,      36779.657412,     39286.431634,     41825.090996,
             43359.446924,     30446.923996,     30518.944492,     32355.4842,
             34264.040992,     36820.789072,     30446.923996,     30518.944492,
             32355.4842,       34264.040992,     36820.789072,     37775.988708,
             36759.731332,     39286.431634,     41825.090996,     43359.446924,
             30230.859096,     30410.913748,     32319.473952,     34192.023908,
             36748.771988,     30230.859096,     30410.913748,     32319.473952,
             34192.023908,     36748.771988,     37775.988708,     36759.731332,
             39342.226364,     41825.090996,     43303.650488,     30194.85226,
             30194.85226,      32391.491036,     34156.010248,     36748.771988,
             30194.85226,      30194.85226,      32391.491036,     34156.010248,
             36748.771988,     37756.062628,     36779.657412,     39342.226364,
             41825.090996,     43359.446924},
            Units::Btu_per_h);
        compressor->perfGridValues.push_back(powerV);

        // Grid values in long format, table 2, COP
        compressor->perfGridValues.push_back(
            {1.177126, 1.1393,   1.091664, 1.033858, 0.981755, 1.177126, 1.1393,   1.091664,
             1.033858, 0.981755, 1.134534, 1.106615, 1.04928,  0.989101, 0.946182, 1.228935,
             1.190326, 1.136507, 1.075244, 1.023802, 1.228935, 1.190326, 1.136507, 1.075244,
             1.023802, 1.174944, 1.147101, 1.087318, 1.026909, 0.980265, 1.324165, 1.27451,
             1.218468, 1.156182, 1.103823, 1.324165, 1.27451,  1.218468, 1.156182, 1.103823,
             1.267595, 1.236929, 1.165864, 1.102888, 1.052601, 1.415804, 1.365212, 1.301359,
             1.238022, 1.180586, 1.415804, 1.365212, 1.301359, 1.238022, 1.180586, 1.358408,
             1.324943, 1.249272, 1.179609, 1.128155, 1.510855, 1.453792, 1.381237, 1.31747,
             1.253435, 1.510855, 1.453792, 1.381237, 1.31747,  1.253435, 1.446639, 1.404653,
             1.331368, 1.249056, 1.195511, 1.60469,  1.540079, 1.462451, 1.39417,  1.326987,
             1.60469,  1.540079, 1.462451, 1.39417,  1.326987, 1.529924, 1.492107, 1.404944,
             1.325122, 1.265433, 1.690249, 1.630494, 1.539446, 1.464082, 1.395939, 1.690249,
             1.630494, 1.539446, 1.464082, 1.395939, 1.610302, 1.56761,  1.479273, 1.393118,
             1.33076,  1.776046, 1.715967, 1.624662, 1.540783, 1.469819, 1.776046, 1.715967,
             1.624662, 1.540783, 1.469819, 1.693465, 1.646725, 1.550545, 1.459601, 1.400666,
             1.865309, 1.797965, 1.703902, 1.612645, 1.539888, 1.865309, 1.797965, 1.703902,
             1.612645, 1.539888, 1.776866, 1.728095, 1.626829, 1.531743, 1.461555, 1.956837,
             1.881215, 1.777073, 1.690021, 1.603082, 1.956837, 1.881215, 1.777073, 1.690021,
             1.603082, 1.857512, 1.807899, 1.699347, 1.599321, 1.526899, 2.038891, 1.956496,
             1.848049, 1.757975, 1.668207, 2.038891, 1.956496, 1.848049, 1.757975, 1.668207,
             1.934721, 1.878022, 1.764777, 1.657606, 1.583414, 2.110576, 2.028182, 1.922739,
             1.818155, 1.725237, 2.110576, 2.028182, 1.922739, 1.818155, 1.725237, 1.997516,
             1.937435, 1.824625, 1.712596, 1.630169, 2.185899, 2.091178, 1.989083, 1.883087,
             1.786388, 2.185899, 2.091178, 1.989083, 1.883087, 1.786388, 2.06464,  2.003084,
             1.88041,  1.763428, 1.68041,  2.29337,  2.155411, 2.063354, 1.942635, 1.844204,
             2.29337,  2.155411, 2.063354, 1.942635, 1.844204, 2.125448, 2.061323, 1.933955,
             1.810339, 1.720612, 2.213555, 2.111682, 2.027503, 1.91515,  1.819817, 2.213555,
             2.111682, 2.027503, 1.91515,  1.819817, 2.126499, 2.064584, 1.935343, 1.818713,
             1.728239, 2.665142, 2.578088, 2.488506, 2.388206, 2.29651,  2.665142, 2.578088,
             2.488506, 2.388206, 2.29651,  2.46407,  2.344111, 2.198428, 2.057188, 1.96338,
             3.304405, 3.191319, 2.971384, 2.741049, 2.550691, 3.304405, 3.191319, 2.971384,
             2.741049, 2.550691, 2.763177, 2.673398, 2.356773, 2.216348, 2.116712, 3.827691,
             3.676371, 3.297085, 3.022338, 2.752997, 3.827691, 3.676371, 3.297085, 3.022338,
             2.752997, 2.871739, 2.890389, 2.434648, 2.292726, 2.205906, 3.989995, 3.834598,
             3.428313, 3.14185,  2.862486, 3.989995, 3.834598, 3.428313, 3.14185,  2.862486,
             2.871269, 2.900921, 2.506208, 2.358391, 2.270261, 4.147236, 3.992101, 3.570419,
             3.274967, 2.982684, 4.147236, 3.992101, 3.570419, 3.274967, 2.982684, 2.865266,
             2.913751, 2.578296, 2.428607, 2.341945, 4.322305, 4.172262, 3.73583,  3.419865,
             3.107421, 4.322305, 4.172262, 3.73583,  3.419865, 3.107421, 2.861677, 2.919962,
             2.65667,  2.504413, 2.414725, 4.518187, 4.354394, 3.889174, 3.578177, 3.248607,
             4.518187, 4.354394, 3.889174, 3.578177, 3.248607, 2.856905, 2.946338, 2.747649,
             2.589208, 2.493442, 4.481963, 4.349398, 3.979002, 3.671837, 3.346137, 4.481963,
             4.349398, 3.979002, 3.671837, 3.346137, 2.998141, 3.087098, 2.885335, 2.709619,
             2.616032, 4.445162, 4.359402, 4.046983, 3.755577, 3.437188, 4.445162, 4.359402,
             4.046983, 3.755577, 3.437188, 3.154067, 3.251216, 3.028152, 2.856705, 2.746669,
             4.402673, 4.359402, 4.112859, 3.858889, 3.546561, 4.402673, 4.359402, 4.112859,
             3.858889, 3.546561, 3.285629, 3.371232, 3.1449,   2.965763, 2.857289, 4.373341,
             4.359402, 4.19016,  3.947368, 3.65253,  4.373341, 4.359402, 4.19016,  3.947368,
             3.65253,  3.372955, 3.472057, 3.238544, 3.048902, 2.936122, 4.378394, 4.359402,
             4.218141, 3.993147, 3.717018, 4.378394, 4.359402, 4.218141, 3.993147, 3.717018,
             3.461548, 3.558644, 3.319824, 3.126817, 3.015709, 4.391304, 4.384616, 4.222841,
             3.993147, 3.713376, 4.391304, 4.384616, 4.222841, 3.993147, 3.713376, 3.538402,
             3.643836, 3.400556, 3.189995, 3.074423, 4.419242, 4.399884, 4.222841, 3.988941,
             3.706113, 4.419242, 4.399884, 4.222841, 3.988941, 3.706113, 3.616836, 3.721038,
             3.477882, 3.266644, 3.147565, 4.429573, 4.410122, 4.208773, 3.993147, 3.713376,
             4.429573, 4.410122, 4.208773, 3.993147, 3.713376, 3.613022, 3.710957, 3.477882,
             3.268826, 3.151618, 4.45679,  4.441125, 4.222841, 3.993147, 3.713376, 4.45679,
             4.441125, 4.222841, 3.993147, 3.713376, 3.611119, 3.710957, 3.475413, 3.264466,
             3.14959,  4.483146, 4.472566, 4.218141, 3.980557, 3.706113, 4.483146, 4.472566,
             4.218141, 3.980557, 3.706113, 3.614928, 3.712969, 3.475413, 3.264466, 3.14959,
             4.515188, 4.488454, 4.222841, 3.988941, 3.713376, 4.515188, 4.488454, 4.222841,
             3.988941, 3.713376, 3.614928, 3.712969, 3.470484, 3.264466, 3.153648, 4.522957,
             4.520572, 4.213452, 3.993147, 3.713376, 4.522957, 4.520572, 4.213452, 3.993147,
             3.713376, 3.616836, 3.710957, 3.470484, 3.264466, 3.14959});

        // Set up regular grid interpolator.
        Btwxt::GridAxis g0(compressor->perfGrid[0],
                           Btwxt::InterpolationMethod::linear,
                           Btwxt::ExtrapolationMethod::constant,
                           {-DBL_MAX, DBL_MAX},
                           "TAir",
                           get_courier());
        Btwxt::GridAxis g1(compressor->perfGrid[1],
                           Btwxt::InterpolationMethod::linear,
                           Btwxt::ExtrapolationMethod::constant,
                           {-DBL_MAX, DBL_MAX},
                           "TOut",
                           get_courier());
        Btwxt::GridAxis g2(compressor->perfGrid[2],
                           Btwxt::InterpolationMethod::linear,
                           Btwxt::ExtrapolationMethod::linear,
                           {-DBL_MAX, DBL_MAX},
                           "Tin",
                           get_courier());

        std::vector<Btwxt::GridAxis> gx {g0, g1, g2};

        compressor->perfRGI =
            std::make_shared<Btwxt::RegularGridInterpolator>(Btwxt::RegularGridInterpolator(
                gx, compressor->perfGridValues, "RegularGridInterpolator", get_courier()));

        compressor->useBtwxtGrid = true;

        compressor->secondaryHeatExchanger = {{10., Units::dF}, {15., Units::dF}, {27., Units::W}};
    }

    else if (presetNum == MODELS_SANCO2_83 || presetNum == MODELS_SANCO2_GS3_45HPA_US_SP ||
             presetNum == MODELS_SANCO2_119)
    {
        setNumNodes(96);
        setpointT = {65, Units::C};
        setpointFixed = true;

        if (presetNum == MODELS_SANCO2_119)
        {
            tankVolume = {119, Units::gal};
            tankUA = {9, Units::kJ_per_hC};
        }
        else
        {
            tankVolume = {315, Units::L};
            tankUA = {7, Units::kJ_per_hC};
            if (presetNum == MODELS_SANCO2_GS3_45HPA_US_SP)
            {
                tankSizeFixed = false;
            }
        }

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->minT = {-25., Units::F};
        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = getNumNodes() - 1;

        compressor->perfMap.reserve(5);

        compressor->perfMap.push_back({
            {17, Units::F},               // Temperature
            {{1650, 5.5, 0.0}, Units::W}, // inputPower_coeffs
            {3.2, -0.015, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {35, Units::F},               // Temperature
            {{1100, 4.0, 0.0}, Units::W}, // inputPower_coeffs
            {3.7, -0.015, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {50, Units::F},              // Temperature
            {{880, 3.1, 0.0}, Units::W}, // inputPower_coeffs
            {5.25, -0.025, 0.0}          // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},              // Temperature
            {{740, 4.0, 0.0}, Units::W}, // inputPower_coeffs
            {6.2, -0.03, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},            // Temperature
            {{790, 2, 0.0}, Units::W}, // inputPower_coeffs
            {7.15, -0.04, 0.0}         // COP_coeffs
        });

        compressor->hysteresis_dT = {4, Units::dC};
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->maxSetpointT = MAXOUTLET_R744();

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(8);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>("eighth node absolute",
                                                                           nodeWeights,
                                                                           Temp_t(113, Units::F),
                                                                           this,
                                                                           std::less<double>(),
                                                                           true));
        if (presetNum == MODELS_SANCO2_83 || presetNum == MODELS_SANCO2_119)
        {
            compressor->addTurnOnLogic(standby({8.2639, Units::dF}));
            // Adds a bonus standby logic so the external heater does not cycle, recommended for any
            // external heater with standby
            std::vector<NodeWeight> nodeWeightStandby;
            nodeWeightStandby.emplace_back(0);
            compressor->standbyLogic =
                std::make_shared<TempBasedHeatingLogic>("bottom node absolute",
                                                        nodeWeightStandby,
                                                        Temp_t(113, Units::F),
                                                        this,
                                                        std::greater<double>());
        }
        // lowT cutoff
        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(1);
        compressor->addShutOffLogic(std::make_shared<TempBasedHeatingLogic>("bottom node absolute",
                                                                            nodeWeights1,
                                                                            Temp_t(135, Units::F),
                                                                            this,
                                                                            std::greater<double>(),
                                                                            true));
        compressor->depressesTemperature = false; // no temp depression
    }
    else if (presetNum == MODELS_SANCO2_43)
    {
        setNumNodes(96);
        setpointT = 65;
        setpointFixed = true;

        tankVolume = {160, Units::L};
        tankUA = {5, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->minT = {-25., Units::F};
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = getIndexTopNode();

        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});

        compressor->perfMap.reserve(5);

        compressor->perfMap.push_back({
            {17, Units::F},               // Temperature
            {{1650, 5.5, 0.0}, Units::W}, // inputPower_coeffs
            {3.2, -0.015, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {35, Units::F},               // Temperature
            {{1100, 4.0, 0.0}, Units::W}, // inputPower_coeffs
            {3.7, -0.015, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {50, Units::F},              // Temperature
            {{880, 3.1, 0.0}, Units::W}, // inputPower_coeffs
            {5.25, -0.025, 0.0}          // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},              // Temperature
            {{740, 4.0, 0.0}, Units::W}, // inputPower_coeffs
            {6.2, -0.03, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},            // Temperature
            {{790, 2, 0.0}, Units::W}, // inputPower_coeffs
            {7.15, -0.04, 0.0}         // COP_coeffs
        });

        compressor->hysteresis_dT = {4, Units::dC};
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->maxSetpointT = MAXOUTLET_R744();

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        std::vector<NodeWeight> nodeWeightStandby;
        nodeWeightStandby.emplace_back(0);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node absolute", nodeWeights, Temp_t(113, Units::F), this));
        compressor->addTurnOnLogic(standby({8.2639, Units::F}));
        compressor->standbyLogic = std::make_shared<TempBasedHeatingLogic>("bottom node absolute",
                                                                           nodeWeightStandby,
                                                                           Temp_t(113, Units::F),
                                                                           this,
                                                                           std::greater<double>());

        // lowT cutoff
        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(1);
        compressor->addShutOffLogic(
            std::make_shared<TempBasedHeatingLogic>("bottom twelfth absolute",
                                                    nodeWeights1,
                                                    Temp_t(135, Units::F),
                                                    this,
                                                    std::greater<double>(),
                                                    true));
        compressor->depressesTemperature = false; // no temp depression
    }
    else if (presetNum == MODELS_AOSmithHPTU50 || presetNum == MODELS_RheemHBDR2250 ||
             presetNum == MODELS_RheemHBDR4550)
    {
        setNumNodes(24);
        setpointT = Temp_t(127.0, Units::F);

        tankVolume = {171, Units::L};
        tankUA = {6, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        double split = 1.0 / 5.0;
        compressor->setCondensity({split, split, split, split, split, 0, 0, 0, 0, 0, 0, 0});

        // performance map
        compressor->perfMap.reserve(3);

        compressor->perfMap.push_back({
            {50, Units::F},               // Temperature
            {{170, 2.02, 0.0}, Units::W}, // inputPower_coeffs
            {5.93, -0.027, 0.0}           // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                 // Temperature
            {{144.5, 2.42, 0.0}, Units::W}, // inputPower_coeffs
            {7.67, -0.037, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},                // Temperature
            {{94.1, 3.15, 0.0}, Units::W}, // inputPower_coeffs
            {11.1, -0.056, 0.0}            // COP_coeffs
        });

        compressor->minT = {42.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        if (presetNum == MODELS_RheemHBDR2250)
        {
            resistiveElementTop->setupAsResistiveElement(8, {2250, Units::W});
        }
        else
        {
            resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        }
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        if (presetNum == MODELS_RheemHBDR2250)
        {
            resistiveElementBottom->setupAsResistiveElement(0, 2250);
        }
        else
        {
            resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        }
        resistiveElementBottom->hysteresis_dT = Temp_d_t(2, Units::dF);

        // logic conditions
        Temp_d_t compStart_dT(35, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp(Temp_t(100, Units::F)));

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(11);
        nodeWeights.emplace_back(12);
        resistiveElementTop->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "top sixth absolute", nodeWeights, Temp_t(105, Units::F), this));
        //		resistiveElementTop->addTurnOnLogic(topThird(Temp_d_t(28, Units::F)));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else if (presetNum == MODELS_AOSmithHPTU66 || presetNum == MODELS_RheemHBDR2265 ||
             presetNum == MODELS_RheemHBDR4565)
    {
        setNumNodes(24);
        setpointT = {127.0, Units::F};

        if (presetNum == MODELS_AOSmithHPTU66)
        {
            tankVolume = {244.6, Units::L};
        }
        else
        {
            tankVolume = {221.4, Units::L};
        }
        tankUA = {8, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        // double split = 1.0 / 4.0;
        compressor->setCondensity({1., 0., 0.});

        // performance map
        compressor->perfMap.reserve(3);

        compressor->perfMap.push_back({
            {50, Units::F},               // Temperature
            {{170, 2.02, 0.0}, Units::W}, // inputPower_coeffs
            {5.93, -0.027, 0.0}           // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                 // Temperature
            {{144.5, 2.42, 0.0}, Units::W}, // inputPower_coeffs
            {7.67, -0.037, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},                // Temperature
            {{94.1, 3.15, 0.0}, Units::W}, // inputPower_coeffs
            {11.1, -0.056, 0.0}            // COP_coeffs
        });

        compressor->minT = {42.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        if (presetNum == MODELS_RheemHBDR2265)
        {
            resistiveElementTop->setupAsResistiveElement(8, {2250, Units::W});
        }
        else
        {
            resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        }
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        if (presetNum == MODELS_RheemHBDR2265)
        {
            resistiveElementBottom->setupAsResistiveElement(0, 2250);
        }
        else
        {
            resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        }
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT(35, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({100, Units::F}));

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(11);
        nodeWeights.emplace_back(12);
        resistiveElementTop->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "top sixth absolute", nodeWeights, Temp_t(105, Units::F), this));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else if (presetNum == MODELS_AOSmithHPTU80 || presetNum == MODELS_RheemHBDR2280 ||
             presetNum == MODELS_RheemHBDR4580)
    {
        setNumNodes(24);
        setpointT = {127.0, Units::F};

        tankVolume = {299.5, Units::L};
        tankUA = {9, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->setCondensity({1., 0., 0., 0.});

        compressor->perfMap.reserve(3);

        compressor->perfMap.push_back({
            {50, Units::F},               // Temperature
            {{170, 2.02, 0.0}, Units::W}, // inputPower_coeffs
            {5.93, -0.027, 0.0}           // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                 // Temperature
            {{144.5, 2.42, 0.0}, Units::W}, // inputPower_coeffs
            {7.67, -0.037, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},                // Temperature
            {{94.1, 3.15, 0.0}, Units::W}, // inputPower_coeffs
            {11.1, -0.056, 0.0}            // COP_coeffs
        });

        compressor->minT = Temp_t(42.0, Units::F);
        compressor->maxT = Temp_t(120.0, Units::F);
        compressor->hysteresis_dT = {1, Units::dF};

        // top resistor values
        if (presetNum == MODELS_RheemHBDR2280)
        {
            resistiveElementTop->setupAsResistiveElement(8, {2250, Units::W});
        }
        else
        {
            resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        }
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        if (presetNum == MODELS_RheemHBDR2280)
        {
            resistiveElementBottom->setupAsResistiveElement(0, {2250, Units::W});
        }
        else
        {
            resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        }
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT(35, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({100, Units::F}));

        std::vector<NodeWeight> nodeWeights;
        //		nodeWeights.emplace_back(9); nodeWeights.emplace_back(10);
        nodeWeights.emplace_back(11);
        nodeWeights.emplace_back(12);
        resistiveElementTop->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "top sixth absolute", nodeWeights, Temp_t(105, Units::F), this));
        //		resistiveElementTop->addTurnOnLogic(topThird(Temp_d_t(35, Units::F)));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else if (presetNum == MODELS_AOSmithHPTU80_DR)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {283.9, Units::L};
        tankUA = {9, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {47, Units::F},                  // Temperature
            {{142.6, 2.152, 0.0}, Units::W}, // inputPower_coeffs
            {6.989258, -0.038320, 0.0}       // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                   // Temperature
            {{120.14, 2.513, 0.0}, Units::W}, // inputPower_coeffs
            {8.188, -0.0432, 0.0}             // COP_coeffs
        });

        compressor->minT = {42.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT(34.1636, Units::dF);
        Temp_d_t standby_dT(7.1528, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({80.108, Units::F}));

        // resistiveElementTop->addTurnOnLogic(topThird(Temp_d_t(39.9691, Units::F)));
        resistiveElementTop->addTurnOnLogic(topThird({87, Units::F}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_AOSmithCAHP120)
    {
        setNumNodes(24);
        setpointT = {150.0, Units::F};

        tankVolume = {111.76, Units::gal}; // AOSmith docs say 111.76
        tankUA = {3.94, Units::Btu_per_hF};

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0.3, 0.3, 0.2, 0.1, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0});

        // From CAHP 120 COP Tests
        compressor->perfMap.reserve(3);

        // Tuned on the multiple K167 tests
        compressor->perfMap.push_back({
            {50., Units::F},                              // Temperature
            {{2010.49966, -4.20966, 0.085395}, Units::W}, // inputPower_coeffs
            {5.91, -0.026299, 0.0}                        // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67.5, Units::F},                             // Temperature
            {{2171.012, -6.936571, 0.1094962}, Units::W}, // inputPower_coeffs
            {7.26272, -0.034135, 0.0}                     // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95., Units::F},                              // Temperature
            {{2276.0625, -7.106608, 0.119911}, Units::W}, // inputPower_coeffs
            {8.821262, -0.042059, 0.0}                    // COP_coeffs
        });

        compressor->minT = {47.0, Units::F};
        // Product documentation says 45F doesn't look like it in CMP-T test//
        compressor->maxT = {110.0, Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        Power_t wattRE(6000, Units::W); // 5650.;
        resistiveElementTop->setupAsResistiveElement(7, wattRE);
        resistiveElementTop->isVIP = true; // VIP is the only source that turns on independently
                                           // when something else is already heating.

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, wattRE);
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};
        resistiveElementBottom->setCondensity(
            {0.2, 0.8, 0., 0., 0., 0., 0., 0., 0., 0., 0., 0.}); // Based of CMP test

        // logic conditions for
        Temp_d_t compStart_dT(5.25, Units::dF);
        Temp_d_t standby_dT(5., Units::dF); // Given CMP_T test
        compressor->addTurnOnLogic(secondSixth(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        Temp_d_t resistanceStartT(12., Units::dC);
        resistiveElementTop->addTurnOnLogic(topThird(resistanceStartT));
        resistiveElementBottom->addTurnOnLogic(topThird(resistanceStartT));

        resistiveElementTop->addShutOffLogic(fifthSixthMaxTemp(Temp_t(117., Units::F)));
        resistiveElementBottom->addShutOffLogic(secondSixthMaxTemp(Temp_t(109., Units::F)));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = resistiveElementBottom;
        resistiveElementBottom->companionHeatSource = compressor;
    }
    else if (MODELS_AOSmithHPTS50 <= presetNum && presetNum <= MODELS_AOSmithHPTS80)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        if (presetNum == MODELS_AOSmithHPTS50)
        {
            tankVolume = {45.6, Units::gal};
            tankUA = {6.403, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AOSmithHPTS66)
        {
            tankVolume = {67.63, Units::gal};
            tankUA = {1.5 / 1.16 * 6.403, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AOSmithHPTS80)
        {
            tankVolume = {81.94, Units::gal};
            tankUA = {1.73 / 1.16 * 6.403, Units::kJ_per_hC};
        }
        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0, 0.2, 0.2, 0.2, 0.2, 0.2, 0, 0, 0, 0, 0, 0});

        // performance map
        compressor->perfMap.reserve(3);

        compressor->perfMap.push_back({
            {50, Units::F},                 // Temperature
            {{66.82, 2.49, 0.0}, Units::W}, // inputPower_coeffs
            {8.64, -0.0436, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67.5, Units::F},              // Temperature
            {{85.1, 2.38, 0.0}, Units::W}, // inputPower_coeffs
            {10.82, -0.0551, 0.0}          // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},              // Temperature
            {{89, 2.62, 0.0}, Units::W}, // inputPower_coeffs
            {12.52, -0.0534, 0.0}        // COP_coeffs
        });

        compressor->minT = {37., Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {1., Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        resistiveElementTop->setupAsResistiveElement(8, Power_t(4500, Units::W));
        resistiveElementTop->isVIP = true;

        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT(30.2, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementTop->addTurnOnLogic(topThird({11.87, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = compressor;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }

    else if (presetNum == MODELS_GE2014STDMode)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {45, Units::gal};
        tankUA = {6.5, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.4977772, -0.0243008, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {7.207307, -0.0335265, 0.0}            // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({19.6605, Units::dF}));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({86.1111, Units::F}));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({12.392, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_GE2014STDMode_80)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {75.4, Units::gal};
        tankUA = {10, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.4977772, -0.0243008, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {7.207307, -0.0335265, 0.0}            // COP_coeffs
        });

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({19.6605, Units::dF}));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp(Temp_t(86.1111, Units::F)));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({12.392, Units::dF}));
        compressor->minT = {37, Units::F};
        compressor->maxSetpointT = MAXOUTLET_R134A();

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_GE2014)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {45, Units::gal};
        tankUA = {6.5, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.4977772, -0.0243008, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {7.207307, -0.0335265, 0.0}            // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->addShutOffLogic(topNodeMaxTemp({116.6358, Units::F}));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({11.0648, Units::dF}));

        resistiveElementBottom->addTurnOnLogic(thirdSixth({60, Units::dF}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({80, Units::F}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_GE2014_80)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {75.4, Units::gal};
        tankUA = UA_t(10, Units::kJ_per_hC);

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.4977772, -0.0243008, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {7.207307, -0.0335265, 0.0}            // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, 4000);
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->addShutOffLogic(topNodeMaxTemp({116.6358, Units::F}));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({11.0648, Units::dF}));
        // compressor->addShutOffLogic(largerDraw(Temp_t(62.4074, Units::F)));

        resistiveElementBottom->addTurnOnLogic(thirdSixth({60, Units::dF}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({80, Units::F}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_GE2014_80DR)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {75.4, Units::L};
        tankUA = {10., Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.4977772, -0.0243008, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {7.207307, -0.0335265, 0.0}            // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({87, Units::F}));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({11.0648, Units::dF}));

        resistiveElementBottom->addTurnOnLogic(thirdSixth({60, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    // PRESET USING GE2014 DATA
    else if (presetNum == MODELS_BWC2020_65)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {64, Units::gal};
        tankUA = {7.6, Units::Btu_per_hF};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.4977772, -0.0243008, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {7.207307, -0.0335265, 0.0}            // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->addShutOffLogic(topNodeMaxTemp({116.6358, Units::F}));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({11.0648, Units::dF}));

        resistiveElementBottom->addTurnOnLogic(thirdSixth({60, Units::dF}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({80, Units::F}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;
    }
    // If Rheem Premium
    else if (MODELS_Rheem2020Prem40 <= presetNum && presetNum <= MODELS_Rheem2020Prem80)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        if (presetNum == MODELS_Rheem2020Prem40)
        {
            tankVolume = {36.1, Units::gal};
            tankUA = {9.5, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_Rheem2020Prem50)
        {
            tankVolume = {45.1, Units::gal};
            tankUA = {8.55, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_Rheem2020Prem65)
        {
            tankVolume = {58.5, Units::gal};
            tankUA = {10.64, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_Rheem2020Prem80)
        {
            tankVolume = {72.0, Units::gal};
            tankUA = {10.83, Units::kJ_per_hC};
        }

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({0.2, 0.2, 0.2, 0.2, 0.2, 0, 0, 0, 0, 0, 0, 0});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                     // Temperature
            {{250, -1.0883, 0.0176}, Units::W}, // inputPower_coeffs
            {6.7, -0.0087, -0.0002}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                        // Temperature
            {{275.0, -0.6631, 0.01571}, Units::W}, // inputPower_coeffs
            {7.0, -0.0168, -0.0001}                // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120.0, Units::F};
        compressor->hysteresis_dT = {1, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {4, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT(32, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({100, Units::F}));
        resistiveElementTop->addTurnOnLogic(topSixth({20.4167, Units::dF}));

        //
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }

    // If Rheem Build
    else if (MODELS_Rheem2020Build40 <= presetNum && presetNum <= MODELS_Rheem2020Build80)
    {
        setNumNodes(12);
        setpointT = Temp_t(127.0, Units::F);

        if (presetNum == MODELS_Rheem2020Build40)
        {
            tankVolume = {36.1, Units::gal};
            tankUA = {9.5, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_Rheem2020Build50)
        {
            tankVolume = {45.1, Units::gal};
            tankUA = {8.55, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_Rheem2020Build65)
        {
            tankVolume = {58.5, Units::gal};
            tankUA = {10.64, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_Rheem2020Build80)
        {
            tankVolume = {72.0, Units::gal};
            tankUA = {10.83, Units::kJ_per_hC};
        }

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({0.2, 0.2, 0.2, 0.2, 0.2, 0, 0, 0, 0, 0, 0, 0});

        compressor->perfMap.reserve(2);
        compressor->perfMap.push_back({
            {50, Units::F},                       // Temperature
            {{220.0, 0.8743, 0.00454}, Units::W}, // inputPower_coeffs
            {7.96064, -0.0448, 0.0}               // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                        // Temperature
            {{275.0, -0.6631, 0.01571}, Units::W}, // inputPower_coeffs
            {8.45936, -0.04539, 0.0}               // COP_coeffs
        });

        compressor->hysteresis_dT = {1, Units::dF};
        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120.0, Units::F};
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->configuration = HeatSource::CONFIG_WRAPPED;

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {4, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT(30, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({100, Units::F}));
        resistiveElementTop->addTurnOnLogic(topSixth({20.4167, Units::dF}));

        //
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else if (MODELS_RheemPlugInShared40 <= presetNum && presetNum <= MODELS_RheemPlugInShared80)
    {
        setNumNodes(12);

        if (presetNum == MODELS_RheemPlugInShared40)
        {
            tankVolume = {36.0, Units::gal};
            tankUA = {9.5, Units::kJ_per_hC};
            setpointT = {140.0, Units::F};
        }
        else if (presetNum == MODELS_RheemPlugInShared50)
        {
            tankVolume = {45.0, Units::gal};
            tankUA = {8.55, Units::kJ_per_hC};
            setpointT = {140.0, Units::F};
        }
        else if (presetNum == MODELS_RheemPlugInShared65)
        {
            tankVolume = {58.5, Units::gal};
            tankUA = {10.64, Units::kJ_per_hC};
            setpointT = {127.0, Units::F};
        }
        else if (presetNum == MODELS_RheemPlugInShared80)
        {
            tankVolume = {72.0, Units::gal};
            tankUA = {10.83, Units::kJ_per_hC};
            setpointT = {127.0, Units::F};
        }

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0.2, 0.2, 0.2, 0.2, 0.2, 0, 0, 0, 0, 0, 0, 0});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                     // Temperature
            {{250, -1.0883, 0.0176}, Units::W}, // inputPower_coeffs
            {6.7, -0.0087, -0.0002}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                        // Temperature
            {{275.0, -0.6631, 0.01571}, Units::W}, // inputPower_coeffs
            {7.0, -0.0168, -0.0001}                // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120.0, Units::F};
        compressor->hysteresis_dT = {1, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // logic conditions
        Temp_d_t compStart_dT = {32, Units::dF};
        Temp_d_t standby_dT = {9, Units::dF};
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));
    }
    else if (presetNum == MODELS_RheemPlugInDedicated40 ||
             presetNum == MODELS_RheemPlugInDedicated50)
    {
        setNumNodes(12);
        setpointT = Temp_t(127.0, Units::F);
        if (presetNum == MODELS_RheemPlugInDedicated40)
        {
            tankVolume = {36, Units::gal};
            tankUA = {5.5, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_RheemPlugInDedicated50)
        {
            tankVolume = {45, Units::gal};
            tankUA = {6.33, Units::kJ_per_hC};
        }
        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(1);
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0.5, 0.5, 0.});

        compressor->perfMap.reserve(2);
        compressor->perfMap.push_back({
            {50, Units::F},                    // Temperature
            {{528.91, 4.8988, 0.0}, Units::W}, // inputPower_coeffs
            {4.3943, -0.012443, 0.0}           // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},                    // Temperature
            {{494.03, 7.7266, 0.0}, Units::W}, // inputPower_coeffs
            {5.48189, -0.01604, 0.0}           // COP_coeffs
        });

        compressor->hysteresis_dT = {1, Units::dF};
        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120.0, Units::F};
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->configuration = HeatSource::CONFIG_WRAPPED;

        // logic conditions
        Temp_d_t compStart_dT(20, Units::dF);
        Temp_d_t standby_dT(9, Units::dF);
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));
    }
    else if (presetNum == MODELS_RheemHB50)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {45, Units::gal};
        tankUA = {7, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {47, Units::F},                  // Temperature
            {{280, 4.97342, 0.0}, Units::W}, // inputPower_coeffs
            {5.634009, -0.029485, 0.0}       // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                  // Temperature
            {{280, 5.35992, 0.0}, Units::W}, // inputPower_coeffs
            {6.3, -0.03, 0.0}                // COP_coeffs
        });

        compressor->hysteresis_dT = {1, Units::dF};
        compressor->minT = {40.0, Units::F};
        compressor->maxT = {120.0, Units::F};
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->configuration = HeatSource::CONFIG_WRAPPED;

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4200, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {2250, Units::W});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        Temp_d_t compStart_dT = {38, Units::dF};
        Temp_d_t standby_dT = {13.2639, Units::dF};
        compressor->addTurnOnLogic(bottomThird(compStart_dT));
        compressor->addTurnOnLogic(standby(standby_dT));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({76.7747, Units::F}));

        resistiveElementTop->addTurnOnLogic(topSixth({20.4167, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_Stiebel220E)
    {
        setNumNodes(12);
        setpointT = {127, Units::F};

        tankVolume = {56, Units::gal};
        tankUA = {9, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;

        heatSources.reserve(2);
        auto compressor = addHeatSource("compressor");
        auto resistiveElement = addHeatSource("resistiveElement");

        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        resistiveElement->setupAsResistiveElement(0, {1500, Units::W});
        resistiveElement->hysteresis_dT = 0.;

        compressor->setCondensity({0, 0.12, 0.22, 0.22, 0.22, 0.22, 0, 0, 0, 0, 0, 0});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                        // Temperature
            {{295.55337, 2.28518, 0.0}, Units::W}, // inputPower_coeffs
            {5.744118, -0.025946, 0.0}             // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                       // Temperature
            {{282.2126, 2.82001, 0.0}, Units::W}, // inputPower_coeffs
            {8.012112, -0.039394, 0.0}            // COP_coeffs
        });

        compressor->minT = {32.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = 0;
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->addTurnOnLogic(thirdSixth({6.5509, Units::dF}));
        compressor->addShutOffLogic(bottomTwelfthMaxTemp({100, Units::F}));

        compressor->depressesTemperature = false;

        //
        compressor->backupHeatSource = resistiveElement;
    }
    else if (presetNum == MODELS_Generic1)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {50, Units::gal};
        tankUA = {9, Units::kJ_per_hC};
        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                        // Temperature
            {{472.58616, 2.09340, 0.0}, Units::W}, // inputPower_coeffs
            {2.942642, -0.0125954, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                       // Temperature
            {{439.5615, 2.62997, 0.0}, Units::W}, // inputPower_coeffs
            {3.95076, -0.01638033, 0.0}           // COP_coeffs
        });

        compressor->minT = {45.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        compressor->addTurnOnLogic(bottomThird({40.0, Units::dF}));
        compressor->addTurnOnLogic(standby({10, Units::dF}));
        compressor->addShutOffLogic(largeDraw({65, Units::F}));

        resistiveElementBottom->addTurnOnLogic(bottomThird({80, Units::F}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({110, Units::F}));

        resistiveElementTop->addTurnOnLogic(topThird({35, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_Generic2)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {50, Units::gal};
        tankUA = {7.5, Units::kJ_per_hC};
        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                        // Temperature
            {{272.58616, 2.09340, 0.0}, Units::W}, // inputPower_coeffs
            {4.042642, -0.0205954, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                       // Temperature
            {{239.5615, 2.62997, 0.0}, Units::W}, // inputPower_coeffs
            {5.25076, -0.02638033, 0.0}           // COP_coeffs
        });

        compressor->minT = {40.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        compressor->addTurnOnLogic(bottomThird({40, Units::dF}));
        compressor->addTurnOnLogic(standby({10, Units::dF}));
        compressor->addShutOffLogic(largeDraw({60, Units::F}));

        resistiveElementBottom->addTurnOnLogic(bottomThird({80, Units::F}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({100, Units::F}));

        resistiveElementTop->addTurnOnLogic(topThird({40, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_Generic3)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {50, Units::gal};
        tankUA = {5, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                        // Temperature
            {{172.58616, 2.09340, 0.0}, Units::W}, // inputPower_coeffs
            {5.242642, -0.0285954, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                       // Temperature
            {{139.5615, 2.62997, 0.0}, Units::W}, // inputPower_coeffs
            {6.75076, -0.03638033, 0.0}           // COP_coeffs
        });

        compressor->minT = {35.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        compressor->addTurnOnLogic(bottomThird({40, Units::dF}));
        compressor->addTurnOnLogic(standby({10, Units::dF}));
        compressor->addShutOffLogic(largeDraw(Temp_t(55, Units::F)));

        resistiveElementBottom->addTurnOnLogic(bottomThird({60, Units::dF}));

        resistiveElementTop->addTurnOnLogic(topThird({40, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (presetNum == MODELS_UEF2generic)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        tankVolume = {45, Units::gal};
        tankUA = {6.5, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {4.29, -0.0243008, 0.0}                  // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {5.61, -0.0335265, 0.0}                // COP_coeffs
        });

        compressor->minT = {37.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({18.6605, Units::dF}));

        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({86.1111, Units::dF}));

        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({12.392, Units::dF}));

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    else if (MODELS_AWHSTier3Generic40 <= presetNum && presetNum <= MODELS_AWHSTier3Generic80)
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        if (presetNum == MODELS_AWHSTier3Generic40)
        {
            tankVolume = {36.1, Units::gal};
            tankUA = {5, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AWHSTier3Generic50)
        {
            tankVolume = {45, Units::gal};
            tankUA = {6.5, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AWHSTier3Generic65)
        {
            tankVolume = {64, Units::gal};
            tankUA = {7.6, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AWHSTier3Generic80)
        {
            tankVolume = {75.4, Units::gal};
            tankUA = {10., Units::kJ_per_hC};
        }
        else
        {
            send_error("Incorrect model specification.");
        }

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        // voltex60 tier 1 values
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                          // Temperature
            {{187.064124, 1.939747, 0.0}, Units::W}, // inputPower_coeffs
            {5.22288834, -0.0243008, 0.0}            // COP_coeffs
        });

        compressor->perfMap.push_back({
            {70, Units::F},                        // Temperature
            {{148.0418, 2.553291, 0.0}, Units::W}, // inputPower_coeffs
            {6.84694165, -0.0335265, 0.0}          // COP_coeffs
        });

        compressor->minT = {42.0, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4000, Units::W});
        resistiveElementBottom->setCondensity({0, 0.2, 0.8, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        resistiveElementBottom->hysteresis_dT = {2, Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({20, Units::dF}));
        resistiveElementTop->addShutOffLogic(topNodeMaxTemp({116.6358, Units::F}));
        compressor->addTurnOnLogic(bottomThird({33.6883, Units::dF}));
        compressor->addTurnOnLogic(standby({11.0648, Units::dF}));
        resistiveElementBottom->addTurnOnLogic(thirdSixth({60, Units::dF}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({80, Units::F}));

        // and you have to do this after putting them into heatSources, otherwise
        // you don't get the right pointers
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;
    }
    // If the model is the TamOMatic, HotTam, Generic... This model is scalable.
    else if ((MODELS_TamScalable_SP <= presetNum) && (presetNum <= MODELS_TamScalable_SP_Half))
    {
        setNumNodes(24);
        setpointT = {135.0, Units::F};
        tankSizeFixed = false;
        canScale = true; // a fully scallable model

        doTempDepression = false;
        tankMixesOnDraw = false;

        tankVolume = {315, Units::L};
        tankUA = {7, Units::kJ_per_hC};
        setTankSizeWithSameU({600., Units::gal});

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->isMultipass = false;
        compressor->perfMap.reserve(1);
        compressor->hysteresis_dT = 0;

        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = getIndexTopNode();

        // Defrost Derate
        compressor->setupDefrostMap();

        // Perfmap for input power and COP made from data for poor preforming modeled to be
        // scalled for this model
        PowerVect_t inputPower_coeffs = {{13.6,
                                          0.00995,
                                          -0.0342,
                                          -0.014,
                                          -0.000110,
                                          0.00026,
                                          0.000232,
                                          0.000195,
                                          -0.00034,
                                          5.30E-06,
                                          2.3600E-06},
                                         Units::kW};
        std::vector<double> COP_coeffs = {1.945,
                                          0.0412,
                                          -0.0112,
                                          -0.00161,
                                          0.0000492,
                                          0.0000348,
                                          -0.0000323,
                                          -0.000166,
                                          0.0000112,
                                          0.0000392,
                                          -3.52E-07};

        // Set model scale factor
        double scaleFactor = 1.;
        if (presetNum == MODELS_TamScalable_SP_2X)
        {
            scaleFactor = 2.;
        }
        else if (presetNum == MODELS_TamScalable_SP_Half)
        {
            scaleFactor = 0.5;
        }

        // Scale the compressor capacity
        inputPower_coeffs.rescale(scaleFactor);

        compressor->perfMap.push_back({
            {105, Units::F},   // Temperature
            inputPower_coeffs, // Input Power Coefficients (inputPower_coeffs
            COP_coeffs         // COP_coeffs
        });

        // logic conditions
        compressor->minT = {40., Units::F};
        compressor->maxSetpointT = MAXOUTLET_R134A();

        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(15, Units::dF), this));

        // lowT cutoff
        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(1);
        compressor->addShutOffLogic(
            std::make_shared<TempBasedHeatingLogic>("bottom node",
                                                    nodeWeights1,
                                                    Temp_d_t(15., Units::dF),
                                                    this,
                                                    std::greater<double>(),
                                                    true));
        compressor->depressesTemperature = false; // no temp depression

        // Scale the resistance-element power
        Power_t elementPower = {scaleFactor * 30000., Units::W};
        resistiveElementBottom->setupAsResistiveElement(0, elementPower);
        resistiveElementTop->setupAsResistiveElement(9, elementPower);

        // top resistor values
        // standard logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({15, Units::dF}));
        resistiveElementTop->isVIP = true;

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else if (presetNum == MODELS_Scalable_MP)
    {
        setNumNodes(24);
        setpointT = {135.0, Units::F};
        tankSizeFixed = false;
        canScale = true; // a fully scallable model

        doTempDepression = false;
        tankMixesOnDraw = false;

        tankVolume = {315,
                      Units::L}; // Gets adjust per model but ratio between vol and UA is important
        tankUA = {7, Units::kJ_per_hC};

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        compressor->isOn = false;
        compressor->isVIP = true;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0});
        compressor->configuration = HeatSource::CONFIG_EXTERNAL;
        compressor->perfMap.reserve(1);
        compressor->hysteresis_dT = 0;
        compressor->externalOutletHeight = 0;
        compressor->externalInletHeight = static_cast<int>(getNumNodes() / 3.) - 1;

        // logic conditions
        std::vector<NodeWeight> nodeWeights;
        nodeWeights.emplace_back(4);
        compressor->addTurnOnLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights, Temp_d_t(5., Units::dF), this));

        std::vector<NodeWeight> nodeWeights1;
        nodeWeights1.emplace_back(4);
        compressor->addShutOffLogic(std::make_shared<TempBasedHeatingLogic>(
            "fourth node", nodeWeights1, Temp_d_t(0., Units::dF), this, std::greater<double>()));
        compressor->depressesTemperature = false; // no temp depression

        // Defrost Derate
        compressor->setupDefrostMap();

        // logic conditions
        compressor->minT = {40., Units::F};
        compressor->maxT = {105., Units::F};
        compressor->maxSetpointT = MAXOUTLET_R134A();

        setTankSizeWithSameU({600., Units::gal});
        compressor->mpFlowRate = {25., Units::gal_per_min};
        compressor->perfMap.push_back({
            {100, Units::F},

            {{12.4, 0.00739, -0.0410, 0.0, 0.000578, 0.0000696},
             Units::kW}, // Input Power Coefficients
                         // (inputPower_coeffs)

            {1.20, 0.0333, 0.00191, 0.000283, 0.0000496, -0.000440} // COP Coefficients
                                                                    // (COP_coeffs)

        });

        resistiveElementBottom->setupAsResistiveElement(0, {30000, Units::W});
        resistiveElementTop->setupAsResistiveElement(9, {30000, Units::W});

        // top resistor values
        // standard logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({30, Units::dF}));
        resistiveElementTop->isVIP = true;

        //
        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else if (presetNum == MODELS_AquaThermAire)
    {
        setNumNodes(12);
        setpointT = {50., Units::C};

        initialTankT = {49.32, Units::C};
        hasInitialTankTemp = true;

        tankVolume = {54.4, Units::gal};
        tankUA = {10.35, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = false;

        // heat exchangers only
        hasHeatExchanger = true;
        heatExchangerEffectiveness = 0.93;

        heatSources.reserve(3);
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1.});

        // AOSmithPHPT60 values
        compressor->perfMap.reserve(4);

        compressor->perfMap.push_back({
            {5, Units::F},                  // Temperature
            {{-1356, 39.80, 0.}, Units::W}, // inputPower_coeffs
            {2.003, -0.003637, 0.}          // COP_coeffs
        });

        compressor->perfMap.push_back({
            {34, Units::F},                 // Temperature
            {{-1485, 43.60, 0.}, Units::W}, // inputPower_coeffs
            {2.805, -0.005092, 0.}          // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67, Units::F},                 // Temperature
            {{-1632, 47.93, 0.}, Units::W}, // inputPower_coeffs
            {4.076, -0.007400, 0.}          // COP_coeffs
        });

        compressor->perfMap.push_back({
            {95, Units::F},                 // Temperature
            {{-1757, 51.60, 0.}, Units::W}, // inputPower_coeffs
            {6.843, -0.012424, 0.}          // COP_coeffs
        });

        compressor->minT = {-25, Units::F};
        compressor->maxT = {125., Units::F};
        compressor->hysteresis_dT = {1, Units::dF};
        compressor->configuration = HeatSource::CONFIG_SUBMERGED;

        // logic conditions
        compressor->addTurnOnLogic(wholeTank({111, Units::F}));
        compressor->addTurnOnLogic(standby({14, Units::dF}));
    }
    else if (presetNum == MODELS_GenericUEF217)
    { // GenericUEF217: 67 degF COP coefficients refined to give UEF=2.17 with high draw profile
        setNumNodes(12);
        setpointT = {127., Units::F};

        tankVolume = {58.5, Units::gal};
        tankUA = {8.5, Units::kJ_per_hC};

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;

        compressor->setCondensity({1., 0., 0.});

        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                         // Temperature (F)
            {{187.064124, 1.939747, 0.}, Units::W}, // Input Power Coefficients (kW)
            {5.4977772, -0.0243008, 0.}             // COP Coefficients
        });

        compressor->perfMap.push_back({
            {67, Units::F},                         // Temperature (F)
            {{148.0418, 2.553291, 0.}, Units::W},   // Input Power Coefficients (kW)
            {6.556322712161, -0.03974367485016, 0.} // COP Coefficients
        });

        compressor->minT = {45, Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {2, Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;

        compressor->addTurnOnLogic(bottomThird({30., Units::dF}));
        compressor->addTurnOnLogic(standby({11., Units::dF}));

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(6, {4500, Units::W});
        resistiveElementTop->addTurnOnLogic(topThird({19., Units::dF}));
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->addTurnOnLogic(thirdSixth({60., Units::F}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({85., Units::F}));
        resistiveElementBottom->isVIP = false;

        resistiveElementTop->followedByHeatSource = resistiveElementBottom;
        resistiveElementBottom->followedByHeatSource = compressor;

        compressor->backupHeatSource = resistiveElementBottom;
        resistiveElementBottom->backupHeatSource = compressor;
    }
    else if ((MODELS_AWHSTier4Generic40 <= presetNum) && (presetNum <= MODELS_AWHSTier4Generic80))
    {
        setNumNodes(12);
        setpointT = {127.0, Units::F};

        if (presetNum == MODELS_AWHSTier4Generic40)
        {
            tankVolume = {36.0, Units::gal};
            tankUA = {5.0, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AWHSTier4Generic50)
        {
            tankVolume = {45.0, Units::gal};
            tankUA = {6.5, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AWHSTier4Generic65)
        {
            tankVolume = {64.0, Units::gal};
            tankUA = {7.6, Units::kJ_per_hC};
        }
        else if (presetNum == MODELS_AWHSTier4Generic80)
        {
            tankVolume = {75.4, Units::gal};
            tankUA = {10.0, Units::kJ_per_hC};
        }
        else
        {
            send_error("Incorrect model specification.");
        }

        doTempDepression = false;
        tankMixesOnDraw = true;

        heatSources.reserve(3);
        auto resistiveElementTop = addHeatSource("resistiveElementTop");
        auto resistiveElementBottom = addHeatSource("resistiveElementBottom");
        auto compressor = addHeatSource("compressor");

        // compressor values
        compressor->isOn = false;
        compressor->isVIP = false;
        compressor->typeOfHeatSource = TYPE_compressor;
        compressor->setCondensity({0.2, 0.2, 0.2, 0.2, 0.2, 0., 0., 0., 0., 0., 0., 0.});

        //
        compressor->perfMap.reserve(2);

        compressor->perfMap.push_back({
            {50, Units::F},                  // Temperature
            {{126.9, 2.215, 0.0}, Units::W}, // inputPower_coeffs
            {6.931, -0.03395, 0.0}           // COP_coeffs
        });

        compressor->perfMap.push_back({
            {67.5, Units::F},                // Temperature
            {{116.6, 2.467, 0.0}, Units::W}, // inputPower_coeffs
            {8.833, -0.04431, 0.0}           // COP_coeffs
        });

        compressor->minT = {37., Units::F};
        compressor->maxT = {120., Units::F};
        compressor->hysteresis_dT = {1., Units::dF};
        compressor->configuration = HeatSource::CONFIG_WRAPPED;
        compressor->maxSetpointT = MAXOUTLET_R134A();

        // top resistor values
        resistiveElementTop->setupAsResistiveElement(8, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {0., Units::dF};
        resistiveElementTop->isVIP = true;

        // bottom resistor values
        resistiveElementBottom->setupAsResistiveElement(0, {4500, Units::W});
        resistiveElementBottom->hysteresis_dT = {2., Units::dF};

        // logic conditions
        resistiveElementTop->addTurnOnLogic(topThird({12.0, Units::dF}));
        compressor->addTurnOnLogic(bottomThird({30.0, Units::dF}));
        compressor->addTurnOnLogic(standby({9.0, Units::dF}));
        resistiveElementBottom->addTurnOnLogic(thirdSixth({60, Units::dF}));
        resistiveElementBottom->addShutOffLogic(bottomTwelfthMaxTemp({80, Units::F}));

        //
        resistiveElementBottom->backupHeatSource = compressor;
        compressor->backupHeatSource = resistiveElementBottom;

        resistiveElementTop->followedByHeatSource = compressor;
        resistiveElementBottom->followedByHeatSource = compressor;

        resistiveElementTop->companionHeatSource = compressor;
    }
    else
    {
        send_error("You have tried to select a preset model which does not exist.");
    }

    if (hasInitialTankTemp)
        setTankToT(initialTankT);
    else // start tank off at setpoint
        resetTankToSetpoint();

    model = presetNum;

    calcDerivedValues();

    checkInputs();

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isOn)
        {
            isHeating = true;
        }
        heatSources[i].sortPerformanceMap();
    }
} // end HPWHinit_presets
