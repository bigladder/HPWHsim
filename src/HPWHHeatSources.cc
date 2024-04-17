/*
 * Implementation of class HPWH::HeatSource
 */

#include <algorithm>
#include <regex>

// vendor
#include <btwxt/btwxt.h>

#include "HPWH.hh"

// public HPWH::HeatSource functions
HPWH::HeatSource::HeatSource(HPWH* parentInput)
    : hpwh(parentInput)
    , isOn(false)
    , lockedOut(false)
    , doDefrost(false)
    , backupHeatSource(NULL)
    , companionHeatSource(NULL)
    , followedByHeatSource(NULL)
    , useBtwxtGrid(false)
    , standbyLogic(NULL)
    , maxOut_at_LowT {100, -273.15}
    , secondaryHeatExchanger {0., 0., 0.}
    , minT(-273.15)
    , maxT(100)
    , maxSetpoint_C(100.)
    , hysteresis_dC(0)
    , airflowFreedom(1.0)
    , externalInletHeight(-1)
    , externalOutletHeight(-1)
    , mpFlowRate_LPS(0.)
    , typeOfHeatSource(TYPE_none)
    , isMultipass(true)
    , extrapolationMethod(EXTRAP_LINEAR)
{
}

HPWH::HeatSource::HeatSource(const HeatSource& hSource) { *this = hSource; }

HPWH::HeatSource& HPWH::HeatSource::operator=(const HeatSource& hSource)
{
    if (this == &hSource)
    {
        return *this;
    }

    hpwh = hSource.hpwh;
    isOn = hSource.isOn;
    lockedOut = hSource.lockedOut;
    doDefrost = hSource.doDefrost;

    runtime_min = hSource.runtime_min;
    energyInput_kWh = hSource.energyInput_kWh;
    energyOutput_kWh = hSource.energyOutput_kWh;

    isVIP = hSource.isVIP;

    if (hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL ||
        hSource.followedByHeatSource != NULL)
    {
        hpwh->simHasFailed = true;
        LOG_ERROR(hpwh,
                    "HeatSources cannot be copied if they contain pointers to other HeatSources")
    }
    else
    {
        companionHeatSource = NULL;
        backupHeatSource = NULL;
        followedByHeatSource = NULL;
    }

    condensity = hSource.condensity;

    Tshrinkage_C = hSource.Tshrinkage_C;

    perfMap = hSource.perfMap;

    perfGrid = hSource.perfGrid;
    perfGridValues = hSource.perfGridValues;
    perfRGI = hSource.perfRGI;
    useBtwxtGrid = hSource.useBtwxtGrid;

    defrostMap = hSource.defrostMap;
    resDefrost = hSource.resDefrost;

    // i think vector assignment works correctly here
    turnOnLogicSet = hSource.turnOnLogicSet;
    shutOffLogicSet = hSource.shutOffLogicSet;
    standbyLogic = hSource.standbyLogic;

    minT = hSource.minT;
    maxT = hSource.maxT;
    maxOut_at_LowT = hSource.maxOut_at_LowT;
    hysteresis_dC = hSource.hysteresis_dC;
    maxSetpoint_C = hSource.maxSetpoint_C;

    depressesTemperature = hSource.depressesTemperature;
    airflowFreedom = hSource.airflowFreedom;

    configuration = hSource.configuration;
    typeOfHeatSource = hSource.typeOfHeatSource;
    isMultipass = hSource.isMultipass;
    mpFlowRate_LPS = hSource.mpFlowRate_LPS;

    externalInletHeight = hSource.externalInletHeight;
    externalOutletHeight = hSource.externalOutletHeight;

    lowestNode = hSource.lowestNode;
    extrapolationMethod = hSource.extrapolationMethod;
    secondaryHeatExchanger = hSource.secondaryHeatExchanger;

    return *this;
}

void HPWH::HeatSource::setCondensity(const std::vector<double>& condensity_in)
{
    condensity = condensity_in;
}

int HPWH::HeatSource::getCondensitySize() const { return static_cast<int>(condensity.size()); }

int HPWH::HeatSource::findParent() const
{
    for (int i = 0; i < hpwh->getNumHeatSources(); ++i)
    {
        if (this == hpwh->heatSources[i].backupHeatSource)
        {
            return i;
        }
    }
    return -1;
}

bool HPWH::HeatSource::isEngaged() const { return isOn; }

bool HPWH::HeatSource::isLockedOut() const { return lockedOut; }

void HPWH::HeatSource::lockOutHeatSource() { lockedOut = true; }

void HPWH::HeatSource::unlockHeatSource() { lockedOut = false; }

bool HPWH::HeatSource::shouldLockOut(double heatSourceAmbientT_C) const
{

    // if it's already locked out, keep it locked out
    if (isLockedOut() == true)
    {
        return true;
    }
    else
    {
        // when the "external" temperature is too cold - typically used for compressor low temp.
        // cutoffs when running, use hysteresis
        bool lock = false;
        if (isEngaged() == true && heatSourceAmbientT_C < minT - hysteresis_dC)
        {
            lock = true;
            LOG_WARNING(hpwh,
                        fmt::format("lock-out: running below minT\tambient: {},\tminT: {}",
                                    heatSourceAmbientT_C,
                                    minT));
        }
        // when not running, don't use hysteresis
        else if (isEngaged() == false && heatSourceAmbientT_C < minT)
        {
            lock = true;
            LOG_WARNING(hpwh,
                        fmt::format("lock-out: already below minT\tambient: {}\tminT: {}",
                                    heatSourceAmbientT_C,
                                    minT));
        }

        // when the "external" temperature is too warm - typically used for resistance lockout
        // when running, use hysteresis
        if (isEngaged() == true && heatSourceAmbientT_C > maxT + hysteresis_dC)
        {
            lock = true;
            LOG_WARNING(hpwh,
                        fmt::format("lock-out: running above maxT\tambient: {}\tmaxT: {}",
                                    heatSourceAmbientT_C,
                                    maxT));
        }
        // when not running, don't use hysteresis
        else if (isEngaged() == false && heatSourceAmbientT_C > maxT)
        {
            lock = true;
            LOG_WARNING(hpwh,
                        fmt::format("lock-out: already above maxT\tambient: {}\tmaxT: {}",
                                    heatSourceAmbientT_C,
                                    maxT));
        }

        if (maxedOut())
        {
            lock = true;
            LOG_WARNING(
                hpwh,
                fmt::format("lock-out: condenser water temperature above max: {}", maxSetpoint_C));
        }

        return lock;
    }
}

bool HPWH::HeatSource::shouldUnlock(double heatSourceAmbientT_C) const
{

    // if it's already unlocked, keep it unlocked
    if (isLockedOut() == false)
    {
        return true;
    }
    // if the heat source is capped and can't produce hotter water
    else if (maxedOut())
    {
        return false;
    }
    else
    {
        // when the "external" temperature is no longer too cold or too warm
        // when running, use hysteresis
        bool unlock = false;
        if (isEngaged() && (heatSourceAmbientT_C > minT + hysteresis_dC) &&
            (heatSourceAmbientT_C < maxT - hysteresis_dC))
        {
            unlock = true;
            if (heatSourceAmbientT_C > minT + hysteresis_dC)
            {
                LOG_WARNING(hpwh,
                            fmt::format("unlock: running above minT\tambient: {}\tminT: {}",
                                        heatSourceAmbientT_C,
                                        minT));
            }
            if (heatSourceAmbientT_C < maxT - hysteresis_dC)
            {
                LOG_WARNING(hpwh,
                            fmt::format("unlock: running below maxT\tambient: {}\tmaxT: {}",
                                        heatSourceAmbientT_C,
                                        maxT));
            }
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT_C > minT) && (heatSourceAmbientT_C < maxT))
        {
            unlock = true;
            if (heatSourceAmbientT_C > minT)
            {
                LOG_WARNING(hpwh,
                            fmt::format("unlock: already above minT\tambient: {}\tminT: {}",
                                        heatSourceAmbientT_C,
                                        minT));
            }
            if (heatSourceAmbientT_C < maxT)
            {
                LOG_WARNING(hpwh,
                            fmt::format("unlock: already below maxT\tambient: {}\tmaxT: {}",
                                        heatSourceAmbientT_C,
                                        maxT));
            }
        }
        return unlock;
    }
}

bool HPWH::HeatSource::toLockOrUnlock(double heatSourceAmbientT_C)
{

    if (shouldLockOut(heatSourceAmbientT_C))
    {
        lockOutHeatSource();
    }
    if (shouldUnlock(heatSourceAmbientT_C))
    {
        unlockHeatSource();
    }

    return isLockedOut();
}

void HPWH::HeatSource::engageHeatSource(DRMODES DR_signal)
{
    isOn = true;
    hpwh->isHeating = true;
    if ((companionHeatSource != NULL) && !(companionHeatSource->shutsOff()) &&
        !(companionHeatSource->isEngaged()) &&
        !(hpwh->shouldDRLockOut(companionHeatSource->typeOfHeatSource, DR_signal)))
    {
        companionHeatSource->engageHeatSource(DR_signal);
    }
}

void HPWH::HeatSource::disengageHeatSource() { isOn = false; }

bool HPWH::HeatSource::shouldHeat() const
{
    // return true if the heat source logic tells it to come on, false if it doesn't,
    // or if an unsepcified selector was used
    bool shouldEngage = false;

    for (int i = 0; i < (int)turnOnLogicSet.size(); i++)
    {
        LOG_INFO(hpwh,
                 fmt::format("shouldHeat logic: {} ", turnOnLogicSet[i]->description.c_str()));

        double average = turnOnLogicSet[i]->getTankValue();
        double comparison = turnOnLogicSet[i]->getComparisonValue();

        if (turnOnLogicSet[i]->compare(average, comparison))
        {
            if (turnOnLogicSet[i]->description == "standby" && standbyLogic != NULL)
            {
                double comparisonStandby = standbyLogic->getComparisonValue();
                double avgStandby = standbyLogic->getTankValue();

                if (turnOnLogicSet[i]->compare(avgStandby, comparisonStandby))
                {
                    shouldEngage = true;
                }
            }
            else
            {
                shouldEngage = true;
            }
        }

        // quit searching the logics if one of them turns it on
        if (shouldEngage)
        {
            // debugging message handling
            LOG_INFO(hpwh, "engages!");
            break;
        }

        LOG_INFO(hpwh, fmt::format("returns: {}", shouldEngage));
    } // end loop over set of logic conditions

    // if everything else wants it to come on, but if it would shut off anyways don't turn it on
    if (shouldEngage && shutsOff())
    {
        shouldEngage = false;
        LOG_INFO(hpwh, "but is denied by shutsOff");
    }

    return shouldEngage;
}

bool HPWH::HeatSource::shutsOff() const
{
    bool shutOff = false;

    if (hpwh->tankTemps_C[0] >= hpwh->setpoint_C)
    {
        shutOff = true;
        LOG_INFO(hpwh,
                 fmt::format("shutsOff  bottom node hot: {} C returns true", hpwh->tankTemps_C[0]));
        return shutOff;
    }

    for (int i = 0; i < (int)shutOffLogicSet.size(); i++)
    {
        LOG_INFO(hpwh, fmt::format("shutsOff logic: {}", shutOffLogicSet[i]->description.c_str()));

        double average = shutOffLogicSet[i]->getTankValue();
        double comparison = shutOffLogicSet[i]->getComparisonValue();

        if (shutOffLogicSet[i]->compare(average, comparison))
        {
            shutOff = true;

            // debugging message handling
            LOG_INFO(hpwh, fmt::format("shuts down {}", shutOffLogicSet[i]->description.c_str()));
        }
    }

    LOG_INFO(hpwh, "returns: {}", shutOff);
    return shutOff;
}

bool HPWH::HeatSource::maxedOut() const
{
    bool maxed = false;

    // If the heat source can't produce water at the setpoint and the control logics are saying to
    // shut off
    if (hpwh->setpoint_C > maxSetpoint_C)
    {
        if (hpwh->tankTemps_C[0] >= maxSetpoint_C || shutsOff())
        {
            maxed = true;
        }
    }
    return maxed;
}

double HPWH::HeatSource::fractToMeetComparisonExternal() const
{
    double fracTemp;
    double frac = 1.;

    for (int i = 0; i < (int)shutOffLogicSet.size(); i++)
    {
        LOG_INFO(hpwh, "shutsOff logic: %s ", shutOffLogicSet[i]->description.c_str());

        fracTemp = shutOffLogicSet[i]->getFractToMeetComparisonExternal();
        frac = fracTemp < frac ? fracTemp : frac;
    }

    return frac;
}

void HPWH::HeatSource::addHeat(double externalT_C, double minutesToRun)
{
    double input_BTUperHr = 0., cap_BTUperHr = 0., cop = 0.;

    switch (configuration)
    {
    case CONFIG_SUBMERGED:
    case CONFIG_WRAPPED:
    {
        std::vector<double> heatDistribution;

        // calcHeatDist takes care of the swooping for wrapped configurations
        calcHeatDist(heatDistribution);

        // calculate capacity btu/hr, input btu/hr, and cop
        if (isACompressor())
        {
            hpwh->condenserInlet_C = getTankTemp();
            getCapacity(externalT_C, getTankTemp(), input_BTUperHr, cap_BTUperHr, cop);
        }
        else
        {
            getCapacity(externalT_C, getTankTemp(), input_BTUperHr, cap_BTUperHr, cop);
        }
        // some outputs for debugging
        LOG_INFO(hpwh,
                 "capacity_kWh: {}, \t\t cap_BTUperHr: {}",
                 BTU_TO_KWH(cap_BTUperHr) * (minutesToRun) / min_per_hr,
                 cap_BTUperHr)

        // the loop over nodes here is intentional - essentially each node that has
        // some amount of heatDistribution acts as a separate resistive element
        // maybe start from the top and go down?  test this with graphs

        // set the leftover capacity to 0
        double leftoverCap_kJ = 0.;
        for (int i = hpwh->getNumNodes() - 1; i >= 0; i--)
        {
            // for(int i = 0; i < hpwh->numNodes; i++){
            double nodeCap_kJ =
                BTU_TO_KJ(cap_BTUperHr * minutesToRun / min_per_hr * heatDistribution[i]);
            if (nodeCap_kJ != 0.)
            {
                double heatToAdd_kJ = nodeCap_kJ + leftoverCap_kJ;
                // add leftoverCap to the next run, and keep passing it on
                leftoverCap_kJ = hpwh->addHeatAboveNode(heatToAdd_kJ, i, maxSetpoint_C);
            }
        }

        if (isACompressor())
        { // outlet temperature is the condenser temperature after heat has been added
            hpwh->condenserOutlet_C = getTankTemp();
        }

        // after you've done everything, any leftover capacity is time that didn't run
        double cap_kJ = BTU_TO_KJ(cap_BTUperHr * minutesToRun / min_per_hr);
        runtime_min = (1. - (leftoverCap_kJ / cap_kJ)) * minutesToRun;

        if (runtime_min < -TOL_MINVALUE)
        {
            LOG_ERROR(hpwh, "Internal error: Negative runtime = {} min", runtime_min)
        }
    }
    break;

    case CONFIG_EXTERNAL:
        // Else the heat source is external. SANCO2 system is only current example
        // capacity is calculated internal to this functio
        //  n, and cap/input_BTUperHr, cop are outputs
        if (isMultipass)
        {
            runtime_min =
                addHeatExternalMP(externalT_C, minutesToRun, cap_BTUperHr, input_BTUperHr, cop);
        }
        else
        {
            runtime_min =
                addHeatExternal(externalT_C, minutesToRun, cap_BTUperHr, input_BTUperHr, cop);
        }
        break;
    }

    // Write the input & output energy
    energyInput_kWh += BTU_TO_KWH(input_BTUperHr * runtime_min / min_per_hr);
    energyOutput_kWh += BTU_TO_KWH(cap_BTUperHr * runtime_min / min_per_hr);
}

// private HPWH::HeatSource functions
void HPWH::HeatSource::sortPerformanceMap()
{
    std::sort(perfMap.begin(),
              perfMap.end(),
              [](const HPWH::HeatSource::perfPoint& a, const HPWH::HeatSource::perfPoint& b) -> bool
              { return a.T_F < b.T_F; });
}

double HPWH::HeatSource::getTankTemp() const
{

    std::vector<double> resampledTankTemps(getCondensitySize());
    resample(resampledTankTemps, hpwh->tankTemps_C);

    double tankTemp_C = 0.;

    std::size_t j = 0;
    for (auto& resampledNodeTemp : resampledTankTemps)
    {
        tankTemp_C += condensity[j] * resampledNodeTemp;
        // Note that condensity is normalized.
        ++j;
    }
    LOG_INFO(hpwh, fmt::format("tank temp {}", tankTemp_C));
    return tankTemp_C;
}

void HPWH::HeatSource::getCapacity(double externalT_C,
                                   double condenserTemp_C,
                                   double setpointTemp_C,
                                   double& input_BTUperHr,
                                   double& cap_BTUperHr,
                                   double& cop)
{
    double externalT_F, condenserTemp_F;

    // Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate
    // a secondary heat exchange in line with the compressor
    condenserTemp_F = C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
    externalT_F = C_TO_F(externalT_C);

    // Get bounding performance map points for interpolation/extrapolation
    size_t i_prev = 0;
    size_t i_next = 1;
    double Tout_F = C_TO_F(setpointTemp_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC);

    if (useBtwxtGrid)
    {
        std::vector<double> target {externalT_F, Tout_F, condenserTemp_F};
        btwxtInterp(input_BTUperHr, cop, target);
    }
    else
    {
        if (perfMap.empty())
        { // Avoid using empty perfMap
            input_BTUperHr = 0.;
            cop = 0.;
        }
        else if (perfMap.size() > 1)
        {
            double COP_T1, COP_T2; // cop at ambient temperatures T1 and T2
            double inputPower_T1_Watts,
                inputPower_T2_Watts; // input power at ambient temperatures T1 and T2

            for (size_t i = 0; i < perfMap.size(); ++i)
            {
                if (externalT_F < perfMap[i].T_F)
                {
                    if (i == 0)
                    {
                        i_prev = 0;
                        i_next = 1;
                    }
                    else
                    {
                        i_prev = i - 1;
                        i_next = i;
                    }
                    break;
                }
                else
                {
                    if (i == perfMap.size() - 1)
                    {
                        i_prev = i - 1;
                        i_next = i;
                        break;
                    }
                }
            }

            // Calculate COP and Input Power at each of the two reference temepratures
            COP_T1 = perfMap[i_prev].COP_coeffs[0];
            COP_T1 += perfMap[i_prev].COP_coeffs[1] * condenserTemp_F;
            COP_T1 += perfMap[i_prev].COP_coeffs[2] * condenserTemp_F * condenserTemp_F;

            COP_T2 = perfMap[i_next].COP_coeffs[0];
            COP_T2 += perfMap[i_next].COP_coeffs[1] * condenserTemp_F;
            COP_T2 += perfMap[i_next].COP_coeffs[2] * condenserTemp_F * condenserTemp_F;

            inputPower_T1_Watts = perfMap[i_prev].inputPower_coeffs[0];
            inputPower_T1_Watts += perfMap[i_prev].inputPower_coeffs[1] * condenserTemp_F;
            inputPower_T1_Watts +=
                perfMap[i_prev].inputPower_coeffs[2] * condenserTemp_F * condenserTemp_F;

            inputPower_T2_Watts = perfMap[i_next].inputPower_coeffs[0];
            inputPower_T2_Watts += perfMap[i_next].inputPower_coeffs[1] * condenserTemp_F;
            inputPower_T2_Watts +=
                perfMap[i_next].inputPower_coeffs[2] * condenserTemp_F * condenserTemp_F;

            // Interpolate to get COP and input power at the current ambient temperature
            linearInterp(
                cop, externalT_F, perfMap[i_prev].T_F, perfMap[i_next].T_F, COP_T1, COP_T2);
            linearInterp(input_BTUperHr,
                         externalT_F,
                         perfMap[i_prev].T_F,
                         perfMap[i_next].T_F,
                         inputPower_T1_Watts,
                         inputPower_T2_Watts);
            input_BTUperHr = KWH_TO_BTU(input_BTUperHr / 1000.0); // 1000 converts w to kw);
        }
        else
        { // perfMap.size() == 1 or we've got an issue.
            if (externalT_F > perfMap[0].T_F)
            {
                if (extrapolationMethod == EXTRAP_NEAREST)
                {
                    externalT_F = perfMap[0].T_F;
                }
            }

            regressedMethod(
                input_BTUperHr, perfMap[0].inputPower_coeffs, externalT_F, Tout_F, condenserTemp_F);
            input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

            regressedMethod(cop, perfMap[0].COP_coeffs, externalT_F, Tout_F, condenserTemp_F);
        }
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT_F);
    }

    cap_BTUperHr = cop * input_BTUperHr;

    // here is where the scaling for flow restriction happens
    // the input power doesn't change, we just scale the cop by a small percentage
    // that is based on the flow rate.  The equation is a fit to three points,
    // measured experimentally - 12 percent reduction at 150 cfm, 10 percent at
    // 200, and 0 at 375. Flow is expressed as fraction of full flow.
    if (airflowFreedom != 1)
    {
        double airflow = 375 * airflowFreedom;
        cop *= 0.00056 * airflow + 0.79;
    }
    if (cop < 0.)
    {
        LOG_WARNING(hpwh, "Warning: COP is Negative!")
    }
    if (cop < 1.)
    {
        LOG_WARNING(hpwh, "Warning: COP is Less than 1!")
    }
}

void HPWH::HeatSource::getCapacityMP(double externalT_C,
                                     double condenserTemp_C,
                                     double& input_BTUperHr,
                                     double& cap_BTUperHr,
                                     double& cop)
{
    double externalT_F, condenserTemp_F;
    bool resDefrostHeatingOn = false;
    // Convert Celsius to Fahrenheit for the curve fits
    condenserTemp_F = C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
    externalT_F = C_TO_F(externalT_C);

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr_kW > 0)
    {
        if (externalT_F < resDefrost.onBelowT_F)
        {
            externalT_F += resDefrost.constTempLift_dF;
            resDefrostHeatingOn = true;
        }
    }

    if (useBtwxtGrid)
    {
        std::vector<double> target {externalT_F, condenserTemp_F};
        btwxtInterp(input_BTUperHr, cop, target);
    }
    else
    {
        // Get bounding performance map points for interpolation/extrapolation
        if (externalT_F > perfMap[0].T_F)
        {
            if (extrapolationMethod == EXTRAP_NEAREST)
            {
                externalT_F = perfMap[0].T_F;
            }
        }

        // Const Tair Tin Tair2 Tin2 TairTin
        regressedMethodMP(
            input_BTUperHr, perfMap[0].inputPower_coeffs, externalT_F, condenserTemp_F);
        regressedMethodMP(cop, perfMap[0].COP_coeffs, externalT_F, condenserTemp_F);
    }
    input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT_F);
    }

    cap_BTUperHr = cop * input_BTUperHr;

    // For accounting add the resistance defrost to the input energy
    if (resDefrostHeatingOn)
    {
        input_BTUperHr += KW_TO_BTUperH(resDefrost.inputPwr_kW);
    }
}

void HPWH::HeatSource::setupDefrostMap(double derate35 /*=0.8865*/)
{
    doDefrost = true;
    defrostMap.reserve(3);
    defrostMap.push_back({17., 1.});
    defrostMap.push_back({35., derate35});
    defrostMap.push_back({47., 1.});
}

void HPWH::HeatSource::defrostDerate(double& to_derate, double airT_F)
{
    if (airT_F <= defrostMap[0].T_F || airT_F >= defrostMap[defrostMap.size() - 1].T_F)
    {
        return; // Air temperature outside bounds of the defrost map. There is no extrapolation
                // here.
    }
    double derate_factor = 1.;
    size_t i_prev = 0;
    for (size_t i = 1; i < defrostMap.size(); ++i)
    {
        if (airT_F <= defrostMap[i].T_F)
        {
            i_prev = i - 1;
            break;
        }
    }
    linearInterp(derate_factor,
                 airT_F,
                 defrostMap[i_prev].T_F,
                 defrostMap[i_prev + 1].T_F,
                 defrostMap[i_prev].derate_fraction,
                 defrostMap[i_prev + 1].derate_fraction);
    to_derate *= derate_factor;
}

void HPWH::HeatSource::linearInterp(
    double& ynew, double xnew, double x0, double x1, double y0, double y1)
{
    ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

void HPWH::HeatSource::regressedMethod(
    double& ynew, std::vector<double>& coefficents, double x1, double x2, double x3)
{
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x3 +
           coefficents[4] * x1 * x1 + coefficents[5] * x2 * x2 + coefficents[6] * x3 * x3 +
           coefficents[7] * x1 * x2 + coefficents[8] * x1 * x3 + coefficents[9] * x2 * x3 +
           coefficents[10] * x1 * x2 * x3;
}

void HPWH::HeatSource::regressedMethodMP(double& ynew,
                                         std::vector<double>& coefficents,
                                         double x1,
                                         double x2)
{
    // Const Tair Tin Tair2 Tin2 TairTin
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x1 * x1 +
           coefficents[4] * x2 * x2 + coefficents[5] * x1 * x2;
}

void HPWH::HeatSource::btwxtInterp(double& input_BTUperHr, double& cop, std::vector<double>& target)
{
    std::vector<double> result;
    try
    {
        result = perfRGI->get_values_at_target(target);
    }
    catch (std::string message)
    {
        LOG_ERROR(hpwh, message);
    }
    input_BTUperHr = result[0];
    cop = result[1];
}

void HPWH::HeatSource::calcHeatDist(std::vector<double>& heatDistribution)
{

    // Populate the vector of heat distribution
    if (configuration == CONFIG_SUBMERGED)
    {
        heatDistribution.resize(hpwh->getNumNodes());
        resampleExtensive(heatDistribution, condensity);
    }
    else if (configuration == CONFIG_WRAPPED)
    { // Wrapped around the tank, send through the logistic function
        calcThermalDist(
            heatDistribution, Tshrinkage_C, lowestNode, hpwh->tankTemps_C, hpwh->setpoint_C);
    }
}

bool HPWH::HeatSource::isACompressor() const { return this->typeOfHeatSource == TYPE_compressor; }

bool HPWH::HeatSource::isAResistance() const { return this->typeOfHeatSource == TYPE_resistance; }
bool HPWH::HeatSource::isExternalMultipass() const
{
    return isMultipass && configuration == HeatSource::CONFIG_EXTERNAL;
}

//-----------------------------------------------------------------------------
///	@brief	Add external heat for a single-pass configuration.
/// @param[in]	externalT_C	        external temperature
///	@param[in]	stepTime_min		heating time available
///	@param[out]	cap_BTUperHr		heating capacity delivered
///	@param[out]	input_BTUperHr		input power drawn
/// @param[out]	cop		            average cop
/// @return	elapsed time (min)
//-----------------------------------------------------------------------------
double HPWH::HeatSource::addHeatExternal(double externalT_C,
                                         double stepTime_min,
                                         double& cap_BTUperHr,
                                         double& input_BTUperHr,
                                         double& cop)
{
    input_BTUperHr = 0.;
    cap_BTUperHr = 0.;
    cop = 0.;

    double setpointT_C = std::min(maxSetpoint_C, hpwh->setpoint_C);
    double remainingTime_min = stepTime_min;
    do
    {
        double tempInput_BTUperHr = 0., tempCap_BTUperHr = 0., temp_cop = 0.;
        double& externalOutletT_C = hpwh->tankTemps_C[externalOutletHeight];

        // how much heat is available in remaining time
        getCapacity(externalT_C, externalOutletT_C, tempInput_BTUperHr, tempCap_BTUperHr, temp_cop);

        double heatingPower_kW = BTUperH_TO_KW(tempCap_BTUperHr);

        double targetT_C = setpointT_C;

        // temperature increase
        double deltaT_C = targetT_C - externalOutletT_C;

        if (deltaT_C <= 0.)
        {
            break;
        }

        // maximum heat that can be added in remaining time
        double heatingCapacity_kJ = heatingPower_kW * (remainingTime_min * sec_per_min);

        // heat for outlet node to reach target temperature
        double nodeHeat_kJ = hpwh->nodeCp_kJperC * deltaT_C;

        // assume one node will be heated this pass
        double neededHeat_kJ = nodeHeat_kJ;

        // reduce node fraction to heat if limited by capacity
        double nodeFrac = 1.;
        if (heatingCapacity_kJ < nodeHeat_kJ)
        {
            nodeFrac = heatingCapacity_kJ / nodeHeat_kJ;
            neededHeat_kJ = heatingCapacity_kJ;
        }

        // limit node fraction to heat by comparison criterion
        double fractToShutOff = fractToMeetComparisonExternal();
        if (fractToShutOff < nodeFrac)
        {
            nodeFrac = fractToShutOff;
            neededHeat_kJ = nodeFrac * nodeHeat_kJ;
        }

        // heat for less than remaining time if full capacity will not be used
        double heatingTime_min = remainingTime_min;
        if (heatingCapacity_kJ > neededHeat_kJ)
        {
            heatingTime_min *= neededHeat_kJ / heatingCapacity_kJ;
            remainingTime_min -= heatingTime_min;
        }
        else
        {
            remainingTime_min = 0.;
        }

        // track the condenser temperatures before mixing nodes
        if (isACompressor())
        {
            hpwh->condenserInlet_C += externalOutletT_C * heatingTime_min;
            hpwh->condenserOutlet_C += targetT_C * heatingTime_min;
        }

        // mix with node above from outlet to inlet
        // mix inlet water at target temperature with inlet node
        for (std::size_t nodeIndex = externalOutletHeight;
             static_cast<int>(nodeIndex) <= externalInletHeight;
             ++nodeIndex)
        {
            double& mixT_C = (static_cast<int>(nodeIndex) == externalInletHeight)
                                 ? targetT_C
                                 : hpwh->tankTemps_C[nodeIndex + 1];
            hpwh->tankTemps_C[nodeIndex] =
                (1. - nodeFrac) * hpwh->tankTemps_C[nodeIndex] + nodeFrac * mixT_C;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        input_BTUperHr +=
            (tempInput_BTUperHr + W_TO_BTUperH(secondaryHeatExchanger.extraPumpPower_W)) *
            heatingTime_min;
        cap_BTUperHr += tempCap_BTUperHr * heatingTime_min;
        cop += temp_cop * heatingTime_min;

        hpwh->externalVolumeHeated_L += nodeFrac * hpwh->nodeVolume_L;

        // if there's still time remaining and you haven't heated to the cutoff
        // specified in shutsOff logic, keep heating
    } while ((remainingTime_min > 0.) && (!shutsOff()));

    // divide outputs by sum of weight - the total time ran
    // not remainingTime_min == minutesToRun is possible
    //   must prevent divide by 0 (added 4-11-2023)
    double runTime_min = stepTime_min - remainingTime_min;
    if (runTime_min > 0.)
    {
        input_BTUperHr /= runTime_min;
        cap_BTUperHr /= runTime_min;
        cop /= runTime_min;
        hpwh->condenserInlet_C /= runTime_min;
        hpwh->condenserOutlet_C /= runTime_min;
    }

    LOG_INFO(hpwh, fmt::format("final remaining time: {}", remainingTime_min));

    // return the time left
    return runTime_min;
}

//-----------------------------------------------------------------------------
///	@brief	Add external heat for a multipass configuration.
/// @param[in]	externalT_C	        external temperature
///	@param[in]	stepTime_min		heating time available
///	@param[out]	cap_BTUperHr		heating capacity delivered
///	@param[out]	input_BTUperHr		input power drawn
/// @param[out]	cop		            average cop
/// @return	elapsed time (min)
//-----------------------------------------------------------------------------
double HPWH::HeatSource::addHeatExternalMP(double externalT_C,
                                           double stepTime_min,
                                           double& cap_BTUperHr,
                                           double& input_BTUperHr,
                                           double& cop)
{
    input_BTUperHr = 0.;
    cap_BTUperHr = 0.;
    cop = 0.;

    double remainingTime_min = stepTime_min;
    do
    {
        // find node fraction to heat in remaining time
        double nodeFrac = mpFlowRate_LPS * (remainingTime_min * sec_per_min) / hpwh->nodeVolume_L;
        if (nodeFrac > 1.)
        { // heat no more than one node each pass
            nodeFrac = 1.;
        }

        // mix the tank
        // apply nodeFrac as the degree of mixing (formerly 1.0)
        hpwh->mixTankNodes(0, hpwh->getNumNodes(), nodeFrac);

        double tempInput_BTUperHr = 0., tempCap_BTUperHr = 0., temp_cop = 0.;
        double& externalOutletT_C = hpwh->tankTemps_C[externalOutletHeight];

        // find heating capacity
        getCapacityMP(
            externalT_C, externalOutletT_C, tempInput_BTUperHr, tempCap_BTUperHr, temp_cop);

        double heatingPower_kW = BTUperH_TO_KW(tempCap_BTUperHr);

        // temperature increase at this power and flow rate
        double deltaT_C =
            heatingPower_kW / (mpFlowRate_LPS * CPWATER_kJperkgC * DENSITYWATER_kgperL);

        // find target temperature
        double targetT_C = externalOutletT_C + deltaT_C;

        // maximum heat that can be added in remaining time
        double heatingCapacity_kJ = heatingPower_kW * (remainingTime_min * sec_per_min);

        // heat needed to raise temperature of one node by deltaT_C
        double nodeHeat_kJ = hpwh->nodeCp_kJperC * deltaT_C;

        // heat no more than one node this step
        double heatingTime_min = remainingTime_min;
        if (heatingCapacity_kJ > nodeHeat_kJ)
        {
            heatingTime_min *= nodeHeat_kJ / heatingCapacity_kJ;
            remainingTime_min -= heatingTime_min;
        }
        else
        {
            remainingTime_min = 0.;
        }

        // track the condenser temperatures before mixing nodes
        if (isACompressor())
        {
            hpwh->condenserInlet_C += externalOutletT_C * heatingTime_min;
            hpwh->condenserOutlet_C += targetT_C * heatingTime_min;
        }

        // mix with node above from outlet to inlet
        // mix inlet water at target temperature with inlet node
        for (std::size_t nodeIndex = externalOutletHeight;
             static_cast<int>(nodeIndex) <= externalInletHeight;
             ++nodeIndex)
        {
            double& mixT_C = (static_cast<int>(nodeIndex) == externalInletHeight)
                                 ? targetT_C
                                 : hpwh->tankTemps_C[nodeIndex + 1];
            hpwh->tankTemps_C[nodeIndex] =
                (1. - nodeFrac) * hpwh->tankTemps_C[nodeIndex] + nodeFrac * mixT_C;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        input_BTUperHr +=
            (tempInput_BTUperHr + W_TO_BTUperH(secondaryHeatExchanger.extraPumpPower_W)) *
            heatingTime_min;
        cap_BTUperHr += tempCap_BTUperHr * heatingTime_min;
        cop += temp_cop * heatingTime_min;

        hpwh->externalVolumeHeated_L += nodeFrac * hpwh->nodeVolume_L;

        // continue until time expired or cutoff condition met
    } while ((remainingTime_min > 0.) && (!shutsOff()));

    // time elapsed in this function
    double elapsedTime_min = stepTime_min - remainingTime_min;
    if (elapsedTime_min > 0.)
    {
        input_BTUperHr /= elapsedTime_min;
        cap_BTUperHr /= elapsedTime_min;
        cop /= elapsedTime_min;
        hpwh->condenserInlet_C /= elapsedTime_min;
        hpwh->condenserOutlet_C /= elapsedTime_min;
    }

    LOG_INFO(hpwh, fmt::format("final remaining time: {}", remainingTime_min));

    // return the elapsed time
    return elapsedTime_min;
}

void HPWH::HeatSource::setupAsResistiveElement(int node,
                                               double Watts,
                                               int condensitySize /* = CONDENSITY_SIZE*/)
{

    isOn = false;
    isVIP = false;
    condensity = std::vector<double>(condensitySize, 0.);
    condensity[node] = 1;

    perfMap.reserve(2);

    perfMap.push_back({
        50,                // Temperature (T_F)
        {Watts, 0.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
        {1.0, 0.0, 0.0}    // COP Coefficients (COP_coeffs)
    });

    perfMap.push_back({
        67,                // Temperature (T_F)
        {Watts, 0.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
        {1.0, 0.0, 0.0}    // COP Coefficients (COP_coeffs)
    });

    configuration = CONFIG_SUBMERGED; // immersed in tank

    typeOfHeatSource = TYPE_resistance;
}

void HPWH::HeatSource::addTurnOnLogic(std::shared_ptr<HeatingLogic> logic)
{
    turnOnLogicSet.push_back(logic);
}

void HPWH::HeatSource::addShutOffLogic(std::shared_ptr<HeatingLogic> logic)
{
    shutOffLogicSet.push_back(logic);
}

void HPWH::HeatSource::clearAllTurnOnLogic() { this->turnOnLogicSet.clear(); }

void HPWH::HeatSource::clearAllShutOffLogic() { this->shutOffLogicSet.clear(); }

void HPWH::HeatSource::clearAllLogic()
{
    clearAllTurnOnLogic();
    clearAllShutOffLogic();
}

void HPWH::HeatSource::changeResistanceWatts(double watts)
{
    for (auto& perfP : perfMap)
    {
        perfP.inputPower_coeffs[0] = watts;
    }
}
