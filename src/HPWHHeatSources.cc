/*
 * Implementation of class HPWH::HeatSource
 */

#include <algorithm>
#include <regex>

// vendor
#include <btwxt/btwxt.h>

#include "HPWH.hh"

// public HPWH::HeatSource functions
HPWH::HeatSource::HeatSource(
    const std::string& name_in,
    HPWH* hpwh_in,
    const std::shared_ptr<Courier::Courier> courier_in /*std::make_shared<DefaultCourier>()*/)
    : Sender("HeatSource", name_in, courier_in)
    , hpwh(hpwh_in)
    , isOn(false)
    , lockedOut(false)
    , doDefrost(false)
    , backupHeatSource(NULL)
    , companionHeatSource(NULL)
    , followedByHeatSource(NULL)
    , useBtwxtGrid(false)
    , standbyLogic(NULL)
    , maxOut_at_LowT {Temp_t(100, Units::C), Temp_t(-273.15, Units::C)}
    , secondaryHeatExchanger {0., 0., 0.}
    , minT(-273.15, Units::C)
    , maxT(100, Units::C)
    , maxSetpointT(100., Units::C)
    , hysteresis_dT(0., Units::dC)
    , airflowFreedom(1.0)
    , externalInletHeight(-1)
    , externalOutletHeight(-1)
    , mpFlowRate(0.)
    , typeOfHeatSource(TYPE_none)
    , isMultipass(true)
    , extrapolationMethod(EXTRAP_LINEAR)
{
    parent_pointer = hpwh;
}

HPWH::HeatSource::HeatSource(const HeatSource& hSource) : Sender(hSource) { *this = hSource; }

HPWH::HeatSource& HPWH::HeatSource::operator=(const HeatSource& hSource)
{
    if (this == &hSource)
    {
        return *this;
    }
    Sender::operator=(hSource);
    hpwh = hSource.hpwh;

    typeOfHeatSource = hSource.typeOfHeatSource;
    isOn = hSource.isOn;
    lockedOut = hSource.lockedOut;
    doDefrost = hSource.doDefrost;

    runtime = hSource.runtime;
    energyInput = hSource.energyInput;
    energyOutput = hSource.energyOutput;

    isVIP = hSource.isVIP;

    if (hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL ||
        hSource.followedByHeatSource != NULL)
    {
        send_error("HeatSources cannot be copied if they contain pointers to other HeatSources.");
    }
    else
    {
        companionHeatSource = NULL;
        backupHeatSource = NULL;
        followedByHeatSource = NULL;
    }

    condensity = hSource.condensity;

    shrinkageT = hSource.shrinkageT;

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
    hysteresis_dT = hSource.hysteresis_dT;
    maxSetpointT = hSource.maxSetpointT;

    depressesTemperature = hSource.depressesTemperature;
    airflowFreedom = hSource.airflowFreedom;

    configuration = hSource.configuration;
    typeOfHeatSource = hSource.typeOfHeatSource;
    isMultipass = hSource.isMultipass;
    mpFlowRate = hSource.mpFlowRate;

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

bool HPWH::HeatSource::shouldLockOut(Temp_t heatSourceAmbientT) const
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
        if (isEngaged() && heatSourceAmbientT < minT - hysteresis_dT)
        {
            lock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && heatSourceAmbientT < minT)
        {
            lock = true;
        }

        // when the "external" temperature is too warm - typically used for resistance lockout
        // when running, use hysteresis
        if (isEngaged() && heatSourceAmbientT > maxT + hysteresis_dT)
        {
            lock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && heatSourceAmbientT > maxT)
        {
            lock = true;
        }

        if (maxedOut())
        {
            lock = true;
            send_warning(fmt::format("lock-out: condenser water temperature above max: {:0.2f}",
                                     maxSetpointT(Units::C)));
        }

        return lock;
    }
}

bool HPWH::HeatSource::shouldUnlock(Temp_t heatSourceAmbientT) const
{

    // if it's already unlocked, keep it unlocked
    if (!isLockedOut())
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
        if (isEngaged() && (heatSourceAmbientT > minT + hysteresis_dT) &&
            (heatSourceAmbientT < maxT - hysteresis_dT))
        {
            unlock = true;
        }
        // when not running, don't use hysteresis
        else if (!isEngaged() && (heatSourceAmbientT > minT) && (heatSourceAmbientT < maxT))
        {
            unlock = true;
        }
        return unlock;
    }
}

bool HPWH::HeatSource::toLockOrUnlock(Temp_t heatSourceAmbientT)
{
    if (shouldLockOut(heatSourceAmbientT))
    {
        lockOutHeatSource();
    }
    if (shouldUnlock(heatSourceAmbientT))
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
            break;
        }

    } // end loop over set of logic conditions

    // if everything else wants it to come on, but if it would shut off anyways don't turn it on
    if (shouldEngage && shutsOff())
    {
        shouldEngage = false;
    }

    return shouldEngage;
}

bool HPWH::HeatSource::shutsOff() const
{
    bool shutOff = false;

    if (hpwh->tankTs[0] >= hpwh->setpointT)
    {
        shutOff = true;
        return shutOff;
    }

    for (int i = 0; i < (int)shutOffLogicSet.size(); i++)
    {
        double average = shutOffLogicSet[i]->getTankValue();
        double comparison = shutOffLogicSet[i]->getComparisonValue();

        if (shutOffLogicSet[i]->compare(average, comparison))
        {
            shutOff = true;
        }
    }

    return shutOff;
}

bool HPWH::HeatSource::maxedOut() const
{
    bool maxed = false;

    // If the heat source can't produce water at the setpoint and the control logics are saying to
    // shut off
    if (hpwh->setpointT > maxSetpointT)
    {
        if (hpwh->tankTs[0] >= maxSetpointT || shutsOff())
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
        fracTemp = shutOffLogicSet[i]->getFractToMeetComparisonExternal();
        frac = fracTemp < frac ? fracTemp : frac;
    }
    return frac;
}

void HPWH::HeatSource::addHeat(Temp_t externalT, Time_t timeToRun)
{
    Power_t inputPower = 0., outputPower = 0.;
    double cop = 0.;

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
            hpwh->condenserInletT = getTankT();
            getCapacity(externalT, getTankT(), inputPower, outputPower, cop);
        }
        else
        {
            getCapacity(externalT, getTankT(), inputPower, outputPower, cop);
        }
        // the loop over nodes here is intentional - essentially each node that has
        // some amount of heatDistribution acts as a separate resistive element
        // maybe start from the top and go down?  test this with graphs

        // set the leftover capacity to 0
        Energy_t leftoverCap = 0.;
        for (int i = hpwh->getNumNodes() - 1; i >= 0; i--)
        {
            // for(int i = 0; i < hpwh->numNodes; i++){
            Energy_t nodeCap(outputPower(Units::W) * timeToRun(Units::s) * heatDistribution[i], Units::J);
            if (nodeCap != 0.)
            {
                Energy_t heatToAdd = nodeCap + leftoverCap;
                // add leftoverCap to the next run, and keep passing it on
                leftoverCap = hpwh->addHeatAboveNode(heatToAdd, i, maxSetpointT);
            }
        }

        if (isACompressor())
        { // outlet temperature is the condenser temperature after heat has been added
            hpwh->condenserOutletT = getTankT();
        }

        // after you've done everything, any leftover capacity is time that didn't run
        Energy_t cap(outputPower(Units::W) * timeToRun(Units::s), Units::J);
        runtime = (1. - (leftoverCap / cap)) * timeToRun;

        if (timeToRun(Units::min) < -TOL_MINVALUE)
        {
            send_error(fmt::format("Negative runtime: {:g} min", timeToRun(Units::min)));
        }
        break;
    }

    case CONFIG_EXTERNAL:
        // Else the heat source is external. SANCO2 system is only current example
        // capacity is calculated internal to this functio
        //  n, and cap/input_BTUperHr, cop are outputs
        if (isMultipass)
        {
            runtime =
                addHeatExternalMP(externalT, timeToRun, outputPower, inputPower, cop);
        }
        else
        {
            runtime =
                addHeatExternal(externalT, timeToRun, outputPower, inputPower, cop);
        }
        break;
    }

    // Write the input & output energy
    energyInput += Energy_t(runtime(Units::s) * inputPower(Units::W), Units::J);
    energyOutput += Energy_t(runtime(Units::s) * outputPower(Units::W), Units::J);
}

// private HPWH::HeatSource functions
void HPWH::HeatSource::sortPerformanceMap()
{
    std::sort(perfMap.begin(),
              perfMap.end(),
              [](const HPWH::PerfPoint& a, const HPWH::PerfPoint& b) -> bool
              { return a.T < b.T; });
}

HPWH::Temp_t HPWH::HeatSource::getTankT() const
{

    std::vector<Temp_t> resampledTankTs(getCondensitySize());
    resample(resampledTankTs, hpwh->tankTs);

    double tankT = 0.;

    std::size_t j = 0;
    for (auto& resampledNodeT : resampledTankTs)
    {
        tankT += condensity[j] * resampledNodeT;
        // Note that condensity is normalized.
        ++j;
    }

    return tankT;
}


void HPWH::HeatSource::getCapacity(Temp_t externalT,
                                   Temp_t condenserT,
                                   Temp_t setpointT,
                                   Power_t& inputPower,
                                   Power_t& outputPower,
                                   double& cop)
{


    // Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate
    // a secondary heat exchange in line with the compressor
    double effCondenserT = condenserT + secondaryHeatExchanger.coldSideOffset_dT;
    double effOutletT = setpointT + secondaryHeatExchanger.hotSideOffset_dT;

    // Get bounding performance map points for interpolation/extrapolation
    // bool extrapolate = false;

    if (useBtwxtGrid)
    {
        TempVect_t target({externalT, effOutletT, effCondenserT});
        btwxtInterp(inputPower, cop, target);
    }
    else
    {
        if (perfMap.empty())
        { // Avoid using empty perfMap
            input_kW = 0.;
            cop = 0.;
        }
        else if (perfMap.size() > 1)
        {
            size_t i_prev = 0;
            size_t i_next = 1;
            for (size_t i = 0; i < perfMap.size(); ++i)
            {
                if (externalT_C < perfMap[i].T_C)
                {
                    if (i == 0)
                    {
                        // extrapolate = true;
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
                        // extrapolate = true;
                        i_prev = i - 1;
                        i_next = i;
                        break;
                    }
                }
            }

            // Calculate COP and Input Power at each of the two reference temepratures
            double COP_T1 = // cop at ambient temperature T1
                expandSeries(perfMap[i_prev].COP_coeffs, effCondenserT_C);

            double COP_T2 = // cop at ambient temperature T2
                expandSeries(perfMap[i_next].COP_coeffs, effCondenserT_C);

            double inputPower_T1_kW = // input power at ambient temperature T1
                expandSeries(perfMap[i_prev].inputPower_coeffs_kW, effCondenserT_C);

            double inputPower_T2_kW = // input power at ambient temperature T2
                expandSeries(perfMap[i_next].inputPower_coeffs_kW, effCondenserT_C);

            // Interpolate to get COP and input power at the current ambient temperature
            linearInterp(input_kW,
                         externalT_C,
                         perfMap[i_prev].T_C,
                         perfMap[i_next].T_C,
                         inputPower_T1_kW,
                         inputPower_T2_kW);

            linearInterp(
                cop, externalT_C, perfMap[i_prev].T_C, perfMap[i_next].T_C, COP_T1, COP_T2);
        }
        else
        { // perfMap.size() == 1 or we've got an issue.
            if (externalT_C > perfMap[0].T_C)
            {
                // extrapolate = true;
                if (extrapolationMethod == EXTRAP_NEAREST)
                {
                    externalT_C = perfMap[0].T_C;
                }
            }

            regressedMethod(input_kW,
                            perfMap[0].inputPower_coeffs_kW,
                            externalT_C,
                            effOutletT_C,
                            effCondenserT_C);

            regressedMethod(cop, perfMap[0].COP_coeffs, externalT_C, effOutletT_C, effCondenserT_C);
        }
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT_C);
    }

    cap_kW = cop * input_kW;

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
    if (hpwh->hpwhVerbosity >= VRB_typical)
    {
        hpwh->msg("cop: %.2lf \tinput_kW: %.2lf \tcap_kW: %.2lf \n", cop, input_kW, cap_kW);
        if (cop < 0.)
        {
            hpwh->msg(" Warning: COP is Negative! \n");
        }
        if (cop < 1.)
        {
            hpwh->msg(" Warning: COP is Less than 1! \n");
        }
    }
}

void HPWH::HeatSource::getCapacityMP(
    double externalT_C, double condenserT_C, double& input_kW, double& cap_kW, double& cop)
{
    bool resDefrostHeatingOn = false;

    double effCondenserT_C = condenserT_C + secondaryHeatExchanger.coldSideOffsetT_C;

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr_kW > 0)
    {
        if (externalT_C < resDefrost.onBelowT_C)
        {
            externalT_C += resDefrost.constLiftT_C;
            resDefrostHeatingOn = true;
        }
    }

    if (useBtwxtGrid)
    {
        std::vector<double> target {externalT_C, effCondenserT_C};
        btwxtInterp(input_kW, cop, target);
    }
    else
    {
        // Get bounding performance map points for interpolation/extrapolation
        if (externalT_C > perfMap[0].T_C)
        {
            if (extrapolationMethod == EXTRAP_NEAREST)
            {
                externalT_C = perfMap[0].T_C;
            }
        }

        // Const Tair Tin Tair2 Tin2 TairTin
        regressedMethodMP(input_kW, perfMap[0].inputPower_coeffs_kW, externalT_C, effCondenserT_C);
        regressedMethodMP(cop, perfMap[0].COP_coeffs, externalT_C, effCondenserT_C);
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT_C);
    }

    cap_kW = cop * input_kW;

    // For accounting add the resistance defrost to the input energy
    if (resDefrostHeatingOn)
    {
        input_kW += resDefrost.inputPwr_kW;
    }
}

void HPWH::HeatSource::setupDefrostMap(double derate35 /*=0.8865*/)
{
    doDefrost = true;
    defrostMap.reserve(3);
    defrostMap.push_back({F_TO_C(17.), 1.});
    defrostMap.push_back({F_TO_C(35.), derate35});
    defrostMap.push_back({F_TO_C(47.), 1.});
}

void HPWH::HeatSource::defrostDerate(double& to_derate, Temp_t airT)
{
    if (airT <= defrostMap[0].T || airT >= defrostMap[defrostMap.size() - 1].T)
    {
        return; // Air temperature outside bounds of the defrost map. There is no extrapolation
                // here.
    }
    double derate_factor = 1.;
    size_t i_prev = 0;
    for (size_t i = 1; i < defrostMap.size(); ++i)
    {
        if (airT <= defrostMap[i].T)
        {
            i_prev = i - 1;
            break;
        }
    }
    linearInterp(derate_factor,
                 airT_C,
                 defrostMap[i_prev].T_C,
                 defrostMap[i_prev + 1].T_C,
                 defrostMap[i_prev].derate_fraction,
                 defrostMap[i_prev + 1].derate_fraction);
    to_derate *= derate_factor;
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

template <typename U>
void HPWH::HeatSource::linearInterp(
    U& ynew, U xnew, U x0, U x1, U y0, U y1)
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

void HPWH::HeatSource::btwxtInterp(double& input_BTUperHr, double& cop, TempVect_t& target)
{
    std::vector<double> result = perfRGI->get_values_at_target(target);
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

bool HPWH::HeatSource::isACompressor() const { return typeOfHeatSource == TYPE_compressor; }

bool HPWH::HeatSource::isAResistance() const { return typeOfHeatSource == TYPE_resistance; }
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
HPWH::Time_t HPWH::HeatSource::addHeatExternal(Temp_t externalT,
                                         Time_t duration,
                                         Power_t& outputPower,
                                         Power_t& inputPower,
                                         double& cop)
{
    inputPower = 0.;
    outputPower = 0.;
    cop = 0.;

    Temp_t tempSetpointT = std::min(maxSetpointT, hpwh->setpointT);
    Time_t remainingTime = duration;

    Energy_t inputEnergy= 0.;
    Energy_t outputEnergy = 0.;
    double condenserInletT_sum = 0.;
    double condenserOutletT_sum = 0.;
    do
    {
        Power_t tempInputPower = 0., tempOutputPower = 0.;
        double temp_cop = 0.;
        auto& externalOutletT = hpwh->tankTs[externalOutletHeight];

        // how much heat is available in remaining time
        getCapacity(externalT, externalOutletT, tempInputPower, tempOutputPower, temp_cop);

        Power_t heatingPower = tempOutputPower;

        Temp_t targetT = tempSetpointT;

        // temperature increase
        Temp_d_t deltaT(targetT(Units::C) - externalOutletT(Units::C), Units::dC);

        if (deltaT <= 0.)
        {
            break;
        }

        // maximum heat that can be added in remaining time
        Energy_t heatingCapacity(heatingPower(Units::kW) * remainingTime(Units::s), Units::kJ);

        // heat for outlet node to reach target temperature
        Energy_t nodeHeat(hpwh->nodeCp(Units::kJ_per_C) * deltaT(Units::dC), Units::kJ);

        // assume one node will be heated this pass
        Energy_t neededHeat = nodeHeat;

        // reduce node fraction to heat if limited by capacity
        double nodeFrac = 1.;
        if (heatingCapacity < nodeHeat)
        {
            nodeFrac = heatingCapacity / nodeHeat;
            neededHeat = heatingCapacity;
        }

        // limit node fraction to heat by comparison criterion
        double fractToShutOff = fractToMeetComparisonExternal();
        if (fractToShutOff < nodeFrac)
        {
            nodeFrac = fractToShutOff;
            neededHeat = nodeFrac * nodeHeat;
        }

        // heat for less than remaining time if full capacity will not be used
        Time_t heatingTime = remainingTime;
        if (heatingCapacity > neededHeat)
        {
            heatingTime = (neededHeat / heatingCapacity) * heatingTime;
            remainingTime -= heatingTime;
        }
        else
        {
            remainingTime = 0.;
        }

        // track the condenser temperatures before mixing nodes
        if (isACompressor())
        {
            condenserInletT_sum += externalOutletT * heatingTime;
            condenserOutletT_sum += targetT * heatingTime;
        }

        // mix with node above from outlet to inlet
        // mix inlet water at target temperature with inlet node
        for (std::size_t nodeIndex = externalOutletHeight;
             static_cast<int>(nodeIndex) <= externalInletHeight;
             ++nodeIndex)
        {
            Temp_t& mixT_C = (static_cast<int>(nodeIndex) == externalInletHeight)
                                 ? targetT
                                 : hpwh->tankTs[nodeIndex + 1];
            hpwh->tankTs[nodeIndex] =
                (1. - nodeFrac) * hpwh->tankTs[nodeIndex] + nodeFrac * mixT_C;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        Energy_t input_dE((tempInputPower(Units::kW) + secondaryHeatExchanger.extraPumpPower(Units::kW)) *
            heatingTime(Units::s), Units::kJ);
        inputEnergy += input_dE;

        Energy_t output_dE(tempOutputPower(Units::kW) * heatingTime(Units::s));
        outputEnergy += output_dE;
        cop += temp_cop * heatingTime;

        hpwh->externalVolumeHeated = hpwh->externalVolumeHeated + nodeFrac * hpwh->nodeVolume;

        // if there's still time remaining and you haven't heated to the cutoff
        // specified in shutsOff logic, keep heating
    } while ((remainingTime > 0.) && (!shutsOff()));

    // divide outputs by sum of weight - the total time ran
    // not remainingTime_min == minutesToRun is possible
    //   must prevent divide by 0 (added 4-11-2023)
    Time_t runTime = duration - remainingTime;
    if (runTime > 0.)
    {
        inputPower = {inputEnergy(Units::kJ) / runTime(Units::s), Units::kW};
        outputPower = {outputEnergy(Units::kJ) / runTime(Units::s), Units::kW};
        cop /= runTime;
        hpwh->condenserInletT = condenserInletT_sum / runTime;
        hpwh->condenserOutletT = condenserOutletT_sum / runTime;
    }

    // return the time left
    return runTime;
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
HPWH::Time_t HPWH::HeatSource::addHeatExternalMP(Temp_t externalT,
                                           Time_t duration,
                                           Power_t& outputPower,
                                           Power_t& inputPower, //***
                                           double& cop)
{
    inputPower = 0.;
    outputPower= 0.;
    cop = 0.;

    Energy_t inputEnergy= 0.;
    Energy_t outputEnergy = 0.;
    double condenserInletT_sum = 0.;
    double condenserOutletT_sum = 0.;

    Time_t remainingTime = duration;
    do
    {
        // find node fraction to heat in remaining time
        double nodeFrac = mpFlowRate(Units::L_per_s) * duration(Units::s) / hpwh->nodeVolume(Units::L);
        if (nodeFrac > 1.)
        { // heat no more than one node each pass
            nodeFrac = 1.;
        }

        // mix the tank
        // apply nodeFrac as the degree of mixing (formerly 1.0)
        hpwh->mixTankNodes(0, hpwh->getNumNodes(), nodeFrac);

        Power_t tempInputPower = 0., tempOutputPower = 0.;
        double temp_cop = 0.;
        Temp_t& externalOutletT = hpwh->tankTs[externalOutletHeight];

        // find heating capacity
        getCapacityMP(
            externalT, externalOutletT, tempInputPower, tempOutputPower, temp_cop);

        Power_t heatingPower = inputPower;

        // temperature increase at this power and flow rate
        Temp_d_t delta_dT =
        {heatingPower(Units::kW) / (mpFlowRate(Units::L_per_s) * CPWATER_kJ_per_kgC * DENSITYWATER_kg_per_L), Units::dC};

        // find target temperature
        Temp_t targetT = externalOutletT + delta_dT;

        // maximum heat that can be added in remaining time
        Energy_t heatingCapacity = {heatingPower(Units::kW) * remainingTime(Units::s), Units::kJ};

        // heat needed to raise temperature of one node by deltaT_C
        Energy_t nodeHeat = {hpwh->nodeCp(Units::kJ_per_C) * delta_dT(Units::dC), Units::kJ};

        // heat no more than one node this step
        Time_t heatingTime = remainingTime;
        if (heatingCapacity > nodeHeat)
        {
            heatingTime *= nodeHeat / heatingCapacity;
            remainingTime -= heatingTime;
        }
        else
        {
            remainingTime = 0.;
        }

        // track the condenser temperatures before mixing nodes
        if (isACompressor())
        {
            condenserInletT_sum += externalOutletT * heatingTime;
            condenserOutletT_sum += targetT * heatingTime;
        }

        // mix with node above from outlet to inlet
        // mix inlet water at target temperature with inlet node
        for (std::size_t nodeIndex = externalOutletHeight;
             static_cast<int>(nodeIndex) <= externalInletHeight;
             ++nodeIndex)
        {
            Temp_t& mixT = (static_cast<int>(nodeIndex) == externalInletHeight)
                                 ? targetT
                                 : hpwh->tankTs[nodeIndex + 1];
            hpwh->tankTs[nodeIndex] =
                (1. - nodeFrac) * hpwh->tankTs[nodeIndex] + nodeFrac * mixT;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        inputEnergy +=
        Energy_t((tempInputPower(Units::kW) + secondaryHeatExchanger.extraPumpPower(Units::kW)) *
            heatingTime(Units::s), Units::kJ);
        outputEnergy += Energy_t(tempOutputPower(Units::kW) * heatingTime(Units::s), Units::kJ);
        cop += temp_cop * heatingTime(Units::s);

        hpwh->externalVolumeHeated += nodeFrac * hpwh->nodeVolume;

        // continue until time expired or cutoff condition met
    } while ((remainingTime > 0.) && (!shutsOff()));

    // time elapsed in this function
    Time_t elapsedTime = duration - remainingTime;
    if (elapsedTime > 0.)
    {
        inputPower = {inputEnergy(Units::kJ) / elapsedTime(Units::s), Units::kW};
        outputPower = {outputEnergy(Units::kJ) / elapsedTime(Units::s), Units::kW};
        cop /= elapsedTime(Units::s);
        hpwh->condenserInletT = condenserInletT_sum / elapsedTime;
        hpwh->condenserOutletT = condenserInletT_sum / elapsedTime;
    }

    // return the elapsed time
    return elapsedTime;
}

void HPWH::HeatSource::setupAsResistiveElement(int node,
                                               Power_t power,
                                               int condensitySize /* = CONDENSITY_SIZE*/)
{

    isOn = false;
    isVIP = false;
    condensity = std::vector<double>(condensitySize, 0.);
    condensity[node] = 1;

    perfMap.reserve(2);

    perfMap.push_back({
        {50, Units::F},                // Temperature (T_F)
        {power(Units::W), 0.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
        {1.0, 0.0, 0.0}    // COP Coefficients (COP_coeffs)
    });

    perfMap.push_back({
        {67, Units::F},                // Temperature (T_F)
        {power(Units::W), 0.0, 0.0}, // Input Power Coefficients (inputPower_coeffs)
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

void HPWH::HeatSource::changeResistancePower(const Power_t power)
{
    for (auto& perfP : perfMap)
    {
        perfP.inputPower_coeffs[0] = power;
    }
}
