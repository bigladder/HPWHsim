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
    , maxOut_at_LowT({{100, Units::C}, {-273.15, Units::C}})
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

    shrinkage_dT = hSource.shrinkage_dT;

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
        if (isEngaged() && (heatSourceAmbientT < minT - hysteresis_dT))
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
    double frac = 1.;

    for (int i = 0; i < (int)shutOffLogicSet.size(); i++)
    {
        double fracTemp = shutOffLogicSet[i]->getFractToMeetComparisonExternal();
        frac = fracTemp < frac ? fracTemp : frac;
    }
    return frac;
}

void HPWH::HeatSource::addHeat(Temp_t externalT, Time_t availableTime)
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
            Energy_t nodeCap(outputPower(Units::W) * availableTime(Units::s) * heatDistribution[i],
                             Units::J);
            if (nodeCap > 0.)
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
        Energy_t cap(outputPower(Units::W) * availableTime(Units::s), Units::J);
        double fac = (cap > 0.) ? (1. - (leftoverCap / cap)) : 0.;
        runtime = fac * availableTime;

        if (runtime(Units::min) < -TOL_MINVALUE)
        {
            send_error(fmt::format("Negative runtime: {:g} min", availableTime(Units::min)));
        }
        break;
    }

    case CONFIG_EXTERNAL:
        // Else the heat source is external. SANCO2 system is only current example
        // capacity is calculated internal to this functio
        //  n, and cap/input_BTUperHr, cop are outputs
        if (isMultipass)
        {
            runtime = addHeatExternalMP(externalT, availableTime, outputPower, inputPower, cop);
        }
        else
        {
            runtime = addHeatExternal(externalT, availableTime, outputPower, inputPower, cop);
        }
        break;
    }

    // Write the input & output energy
    energyInput += Energy_t(inputPower(Units::W) * runtime(Units::s), Units::J);
    energyOutput += Energy_t(outputPower(Units::W) * runtime(Units::s), Units::J);
}

// private HPWH::HeatSource functions
void HPWH::HeatSource::sortPerformanceMap()
{
    std::sort(perfMap.begin(),
              perfMap.end(),
              [](const HPWH::PerfPoint& a, const HPWH::PerfPoint& b) -> bool { return a.T < b.T; });
}

HPWH::Temp_t HPWH::HeatSource::getTankT() const
{

    TempVect_t resampledTankTs = resample(getCondensitySize(), hpwh->tankTs);

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
                                   Temp_t tempSetpointT,
                                   Power_t& inputPower,
                                   Power_t& outputPower,
                                   double& cop)
{

    // Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate
    // a secondary heat exchange in line with the compressor
    Temp_t effCondenserT = {
        condenserT(Units::C) + secondaryHeatExchanger.coldSideOffset_dT(Units::dC), Units::C};
    Temp_t effOutletT = {
        tempSetpointT(Units::C) + secondaryHeatExchanger.hotSideOffset_dT(Units::dC), Units::C};

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
            inputPower = 0.;
            cop = 0.;
        }
        else if (perfMap.size() > 1)
        {
            size_t i_prev = 0;
            size_t i_next = 1;
            // auto& pPrev = perfMap[i_prev].inputPower_coeffs;
            for (size_t i = 0; i < perfMap.size(); ++i)
            {
                if (externalT < perfMap[i].T)
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

            /// auto testV = scaleVector(perfMap[0].inputPower_coeffs, 3.);
            // auto test_x = expandSeries(pNext, 3.);

            // std::cout<< test_x;
            // auto pNext = pNext;
            // pNext = perfMap[i_next].inputPower_coeffs;
            //  Calculate COP and Input Power at each of the two reference temepratures
            double COP_T1 = expandSeries(perfMap[i_prev].COP_coeffs,
                                         effCondenserT); // cop at ambient temperature T1

            double COP_T2 = expandSeries(perfMap[i_next].COP_coeffs,
                                         effCondenserT); // cop at ambient temperature T2//

            Power_t inputPower_T1 =
                expandSeries(perfMap[i_prev].inputPower_coeffs,
                             effCondenserT); // input power at ambient temperature T1

            Power_t inputPower_T2 =
                expandSeries(perfMap[i_next].inputPower_coeffs,
                             effCondenserT); // input power at ambient temperature T2

            // Interpolate to get COP and input power at the current ambient temperature
            inputPower = linearInterp(
                externalT, perfMap[i_prev].T, perfMap[i_next].T, inputPower_T1, inputPower_T2);

            cop = linearInterp(externalT, perfMap[i_prev].T, perfMap[i_next].T, COP_T1, COP_T2);
        }
        else
        { // perfMap.size() == 1 or we've got an issue.
            if (externalT > perfMap[0].T)
            {
                // extrapolate = true;
                if (extrapolationMethod == EXTRAP_NEAREST)
                {
                    externalT = perfMap[0].T;
                }
            }

            inputPower =
                regressedMethod(perfMap[0].inputPower_coeffs, externalT, effOutletT, effCondenserT);

            cop = regressedMethod(perfMap[0].COP_coeffs, externalT, effOutletT, effCondenserT);
        }
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT);
    }

    outputPower = cop * inputPower;

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
        send_warning("Warning: COP is Negative!");
    }
    if (cop < 1.)
    {
        send_warning("Warning: COP is Less than 1!");
    }
}

void HPWH::HeatSource::getCapacityMP(
    Temp_t externalT, Temp_t condenserT, Power_t& inputPower, Power_t& outputPower, double& cop)
{
    bool resDefrostHeatingOn = false;

    Temp_t effCondenserT = {
        condenserT(Units::C) + secondaryHeatExchanger.coldSideOffset_dT(Units::dC), Units::C};

    // Check if we have resistance elements to turn on for defrost and add the constant lift.
    if (resDefrost.inputPwr > 0)
    {
        if (externalT < resDefrost.onBelowT)
        {
            externalT = {externalT(Units::C) + resDefrost.constLift_dT(Units::dC), Units::C};
            resDefrostHeatingOn = true;
        }
    }

    if (useBtwxtGrid)
    {
        TempVect_t target = {{externalT(Units::C), effCondenserT(Units::C)}, Units::C};
        btwxtInterp(inputPower, cop, target);
    }
    else
    {
        // Get bounding performance map points for interpolation/extrapolation
        if (externalT > perfMap[0].T)
        {
            if (extrapolationMethod == EXTRAP_NEAREST)
            {
                externalT = perfMap[0].T;
            }
        }

        // Const Tair Tin Tair2 Tin2 TairTin
        inputPower = regressedMethodMP(perfMap[0].inputPower_coeffs, externalT, effCondenserT);
        cop = regressedMethodMP(perfMap[0].COP_coeffs, externalT, effCondenserT);
    }

    if (doDefrost)
    {
        // adjust COP by the defrost factor
        defrostDerate(cop, externalT);
    }

    outputPower = cop * inputPower;

    // For accounting add the resistance defrost to the input energy
    if (resDefrostHeatingOn)
    {
        inputPower += resDefrost.inputPwr;
    }
}

void HPWH::HeatSource::setupDefrostMap(double derate35 /*=0.8865*/)
{
    doDefrost = true;
    defrostMap.reserve(3);
    defrostMap.push_back({{17., Units::F}, 1.});
    defrostMap.push_back({{35., Units::F}, derate35});
    defrostMap.push_back({{47., Units::F}, 1.});
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
    derate_factor = linearInterp(airT,
                                 defrostMap[i_prev].T,
                                 defrostMap[i_prev + 1].T,
                                 defrostMap[i_prev].derate_fraction,
                                 defrostMap[i_prev + 1].derate_fraction);
    to_derate *= derate_factor;
}

void HPWH::HeatSource::btwxtInterp(Power_t& inputPower, double& cop, TempVect_t& target)
{
    std::vector<double> result = perfRGI->get_values_at_target(target);
    inputPower = result[0];
    cop = result[1];
}

void HPWH::HeatSource::calcHeatDist(std::vector<double>& heatDistribution)
{
    // Populate the vector of heat distribution
    if (configuration == CONFIG_SUBMERGED)
    {
        heatDistribution = resampleExtensive(hpwh->getNumNodes(), condensity);
    }
    else if (configuration == CONFIG_WRAPPED)
    { // Wrapped around the tank, send through the logistic function
        heatDistribution = calcThermalDist(shrinkage_dT, lowestNode, hpwh->tankTs, hpwh->setpointT);
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
HPWH::Time_t HPWH::HeatSource::addHeatExternal(
    Temp_t externalT, Time_t duration, Power_t& outputPower, Power_t& inputPower, double& cop)
{
    inputPower = 0.;
    outputPower = 0.;
    cop = 0.;

    Temp_t tempSetpointT = std::min(maxSetpointT, hpwh->setpointT);
    Time_t remainingTime = duration;

    Energy_t inputEnergy = 0.;
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
            heatingTime *= (neededHeat / heatingCapacity);
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
            hpwh->tankTs[nodeIndex] = (1. - nodeFrac) * hpwh->tankTs[nodeIndex] + nodeFrac * mixT_C;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        Energy_t input_dE(
            (tempInputPower(Units::kW) + secondaryHeatExchanger.extraPumpPower(Units::kW)) *
                heatingTime(Units::s),
            Units::kJ);
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
    outputPower = 0.;
    cop = 0.;

    Energy_t inputEnergy = 0.;
    Energy_t outputEnergy = 0.;
    double condenserInletT_sum = 0.;
    double condenserOutletT_sum = 0.;

    Time_t remainingTime = duration;
    do
    {
        // find node fraction to heat in remaining time
        double nodeFrac =
            mpFlowRate(Units::L_per_s) * remainingTime(Units::s) / hpwh->nodeVolume(Units::L);
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
        getCapacityMP(externalT, externalOutletT, tempInputPower, tempOutputPower, temp_cop);

        Power_t heatingPower = tempOutputPower;

        // temperature increase at this power and flow rate
        Temp_d_t delta_dT = {heatingPower(Units::kW) / (mpFlowRate(Units::L_per_s) *
                                                        CPWATER_kJ_per_kgC * DENSITYWATER_kg_per_L),
                             Units::dC};

        // find target temperature
        Temp_t targetT = {externalOutletT(Units::C) + delta_dT(Units::dC), Units::C};

        // maximum heat that can be added in remaining time
        Energy_t heatingCapacity = {heatingPower(Units::kW) * remainingTime(Units::s), Units::kJ};

        // heat needed to raise temperature of one node by deltaT_C
        // Energy_t nodeHeat = {hpwh->nodeCp(Units::kJ_per_C) * delta_dT(Units::dC), Units::kJ};
        Energy_t nodeHeat = {heatingPower(Units::kW) * hpwh->nodeVolume(Units::L) /
                                 mpFlowRate(Units::L_per_s),
                             Units::kJ};

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
            hpwh->tankTs[nodeIndex] = (1. - nodeFrac) * hpwh->tankTs[nodeIndex] + nodeFrac * mixT;
        }

        hpwh->mixTankInversions();
        hpwh->updateSoCIfNecessary();

        // track outputs weighted by the time run
        // pump power added to approximate a secondary heat exchange in line with the compressor
        Energy_t dE_in = {
            (tempInputPower(Units::kW) + secondaryHeatExchanger.extraPumpPower(Units::kW)) *
                heatingTime(Units::s),
            Units::kJ};
        inputEnergy += dE_in;

        Energy_t dE_out = {tempOutputPower(Units::kW) * heatingTime(Units::s), Units::kJ};
        outputEnergy += dE_out;

        cop += temp_cop * heatingTime;

        hpwh->externalVolumeHeated += nodeFrac * hpwh->nodeVolume;

        // continue until time expired or cutoff condition met
    } while ((remainingTime > 0.) && (!shutsOff()));

    // time elapsed in this function
    Time_t elapsedTime = duration - remainingTime;
    if (elapsedTime > 0.)
    {
        inputPower = {inputEnergy(Units::kJ) / elapsedTime(Units::s), Units::kW};
        outputPower = {outputEnergy(Units::kJ) / elapsedTime(Units::s), Units::kW};
        cop /= elapsedTime;
        hpwh->condenserInletT = condenserInletT_sum / elapsedTime;
        hpwh->condenserOutletT = condenserOutletT_sum / elapsedTime;
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
        {50, Units::F},                          // Temperature (T_F)
        {{power(Units::W), 0.0, 0.0}, Units::W}, // Input Power Coefficients (inputPower_coeffs)
        {1.0, 0.0, 0.0}                          // COP Coefficients (COP_coeffs)
    });

    perfMap.push_back({
        {67, Units::F},                          // Temperature (T_F)
        {{power(Units::W), 0.0, 0.0}, Units::W}, // Input Power Coefficients (inputPower_coeffs)
        {1.0, 0.0, 0.0}                          // COP Coefficients (COP_coeffs)
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
