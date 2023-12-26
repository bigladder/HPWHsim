/*
Copyright (c) 2014-2016 Ecotope Inc.
All rights reserved.



Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:



* Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.



* Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.



* Neither the name of the copyright holders nor the names of its contributors
may be used to endorse or promote products derived from this software
without specific prior written permission from the copyright holders.



THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "HPWH.hh"
#include <btwxt/btwxt.h>

#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>

using std::cout;
using std::endl;
using std::string;

const float HPWH::DENSITYWATER_kgperL = 0.995f;
const float HPWH::KWATER_WpermC = 0.62f;
const float HPWH::CPWATER_kJperkgC = 4.180f;
const float HPWH::TOL_MINVALUE = 0.0001f;
const float HPWH::UNINITIALIZED_LOCATIONTEMP = -500.f;
const float HPWH::ASPECTRATIO = 4.75f;

const double HPWH::MAXOUTLET_R134A = F_TO_C(160.);
const double HPWH::MAXOUTLET_R410A = F_TO_C(140.);
const double HPWH::MAXOUTLET_R744 = F_TO_C(190.);
const double HPWH::MINSINGLEPASSLIFT = dF_TO_dC(15.);

//-----------------------------------------------------------------------------
///	@brief	Samples a std::vector to extract a single value spanning the fractional
///			coordinate range from frac_begin to frac_end.
/// @note	Bounding fractions are clipped or swapped, if needed.
/// @param[in]	sampleValues	Contains values to be sampled
///	@param[in]	beginFraction		Lower (left) bounding fraction (0 to 1)
///	@param[in]	endFraction			Upper (right) bounding fraction (0 to 1)
/// @return	Resampled value; 0 if undefined.
//-----------------------------------------------------------------------------
double
getResampledValue(const std::vector<double>& sampleValues, double beginFraction, double endFraction)
{
    if (beginFraction > endFraction)
        std::swap(beginFraction, endFraction);
    if (beginFraction < 0.)
        beginFraction = 0.;
    if (endFraction > 1.)
        endFraction = 1.;

    double nNodes = static_cast<double>(sampleValues.size());
    auto beginIndex = static_cast<std::size_t>(beginFraction * nNodes);

    double previousFraction = beginFraction;
    double nextFraction = previousFraction;

    double totValueWeight = 0.;
    double totWeight = 0.;
    for (std::size_t index = beginIndex; nextFraction < endFraction; ++index)
    {
        nextFraction = static_cast<double>(index + 1) / nNodes;
        if (nextFraction > endFraction)
        {
            nextFraction = endFraction;
        }
        double weight = nextFraction - previousFraction;
        totValueWeight += weight * sampleValues[index];
        totWeight += weight;
        previousFraction = nextFraction;
    }
    double resampled_value = 0.;
    if (totWeight > 0.)
        resampled_value = totValueWeight / totWeight;
    return resampled_value;
}

//-----------------------------------------------------------------------------
///	@brief	Replaces the values in a std::vector by resampling another std::vector of
///			arbitrary size.
/// @param[in,out]	values			Contains values to be replaced
///	@param[in]		sampleValues	Contains values to replace with
/// @return	Success: true; Failure: false
//-----------------------------------------------------------------------------
bool resample(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    if (sampleValues.empty())
        return false;
    double actualSize = static_cast<double>(values.size());
    double sizeRatio = static_cast<double>(sampleValues.size()) / actualSize;
    auto binSize = static_cast<std::size_t>(1. / sizeRatio);
    double beginFraction = 0., endFraction;
    std::size_t index = 0;
    while (index < actualSize)
    {
        auto value = static_cast<double>(index);
        auto sampleIndex = static_cast<std::size_t>(floor(value * sizeRatio));
        if (sampleIndex + 1. < (value + 1.) * sizeRatio)
        { // General case: no binning possible
            endFraction = static_cast<double>(index + 1) / actualSize;
            values[index] = getResampledValue(sampleValues, beginFraction, endFraction);
            ++index;
        }
        else
        { // Special case: direct copy a single value to a bin
            std::size_t beginIndex = index;
            std::size_t adjustedBinSize = binSize;
            if (binSize > 1)
            { // Find beginning of bin and number to copy
                beginIndex = static_cast<std::size_t>(ceil(sampleIndex / sizeRatio));
                adjustedBinSize = static_cast<std::size_t>(floor((sampleIndex + 1) / sizeRatio) -
                                                           ceil(sampleIndex / sizeRatio));
            }
            std::fill_n(values.begin() + beginIndex, adjustedBinSize, sampleValues[sampleIndex]);
            index = beginIndex + adjustedBinSize;
            endFraction = static_cast<double>(index) / actualSize;
        }
        beginFraction = endFraction;
    }
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Resample an extensive property (e.g., heat)
///	@note	See definition of int resample.
//-----------------------------------------------------------------------------
bool resampleExtensive(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    if (resample(values, sampleValues))
    {
        double scale =
            static_cast<double>(sampleValues.size()) / static_cast<double>(values.size());
        for (auto& value : values)
            value *= scale;
        return true;
    }
    return false;
}

double expitFunc(double x, double offset)
{
    double val;
    val = 1 / (1 + exp(x - offset));
    return val;
}

void normalize(std::vector<double>& distribution)
{
    size_t N = distribution.size();

    bool normalization_needed = true;

    // Need to renormalize if negligible elements are zeroed.
    while (normalization_needed)
    {
        normalization_needed = false;
        double sum_tmp = 0.;
        for (size_t i = 0; i < N; i++)
        {
            sum_tmp += distribution[i];
        }
        if (sum_tmp > 0.)
        {
            for (size_t i = 0; i < N; i++)
            {
                distribution[i] /= sum_tmp;
                // this gives a very slight speed improvement (milliseconds per simulated year)
                if (distribution[i] < HPWH::TOL_MINVALUE)
                {
                    if (distribution[i] > 0.)
                    {
                        normalization_needed = true;
                    }
                    distribution[i] = 0.;
                }
            }
        }
        else
        {
            for (size_t i = 0; i < N; i++)
            {
                distribution[i] = 0.;
            }
        }
    }
}

//-----------------------------------------------------------------------------
///	@brief	Finds the lowest tank node with non-zero weighting
/// @param[in]	nodeDist	weighting to be applied
/// @param[in]	numTankNodes	number of nodes in tank
/// @returns	index of lowest tank node
//-----------------------------------------------------------------------------
int findLowestNode(const std::vector<double>& nodeDist, const int numTankNodes)
{
    int lowest = 0;
    const int distSize = static_cast<int>(nodeDist.size());
    double nodeRatio = static_cast<double>(numTankNodes) / distSize;

    for (auto j = 0; j < distSize; ++j)
    {
        if (nodeDist[j] > 0.)
        {
            lowest = static_cast<int>(nodeRatio * j);
            break;
        }
    }

    return lowest;
}

//-----------------------------------------------------------------------------
///	@brief	Calculates a width parameter for a thermal distribution
/// @param[in]	nodeDist		original distribution from which theraml distribution
///								is derived
/// @returns	width parameter (in degC)
//-----------------------------------------------------------------------------
double findShrinkageT_C(const std::vector<double>& nodeDist)
{
    double alphaT_C = 1., betaT_C = 2.;
    double condentropy = 0.;
    for (std::size_t iNode = 0; iNode < nodeDist.size(); ++iNode)
    {
        double dist = nodeDist[iNode];
        if (dist > 0.)
        {
            condentropy -= dist * log(dist);
        }
    }
    // condentropy shifts as ln(# of condensity nodes)
    double size_factor = static_cast<double>(nodeDist.size()) / HPWH::CONDENSITY_SIZE;
    double standard_condentropy = condentropy - log(size_factor);

    return alphaT_C + standard_condentropy * betaT_C;
}

//-----------------------------------------------------------------------------
///	@brief	Calculates a thermal distribution for heat distribution.
/// @note	Fails if all nodeTemp_C values exceed setpointT_C
/// @param[out]	thermalDist		resulting thermal distribution; does not require pre-allocation
/// @param[in]	shrinkageT_C	width of distribution
/// @param[in]	lowestNode		index of lowest non-zero contribution
/// @param[in]	nodeTemp_C		node temperatures
/// @param[in]	setpointT_C		distribution parameter
//-----------------------------------------------------------------------------
void calcThermalDist(std::vector<double>& thermalDist,
                     const double shrinkageT_C,
                     const int lowestNode,
                     const std::vector<double>& nodeT_C,
                     const double setpointT_C)
{

    thermalDist.resize(nodeT_C.size());

    // Populate the vector of heat distribution
    double totDist = 0.;
    for (int i = 0; i < static_cast<int>(nodeT_C.size()); i++)
    {
        double dist = 0.;
        if (i >= lowestNode)
        {
            double Toffset_C = 5.0 / 1.8;   // 5 degF
            double offset = Toffset_C / 1.; // should be dimensionless
            dist = expitFunc((nodeT_C[i] - nodeT_C[lowestNode]) / shrinkageT_C, offset);
            dist *= (setpointT_C - nodeT_C[i]);
            if (dist < 0.)
                dist = 0.;
        }
        thermalDist[i] = dist;
        totDist += dist;
    }

    if (totDist > 0.)
    {
        normalize(thermalDist);
    }
    else
    {
        thermalDist.assign(thermalDist.size(), 1. / static_cast<double>(thermalDist.size()));
    }
}

void HPWH::setMinutesPerStep(const double minutesPerStep_in)
{
    minutesPerStep = minutesPerStep_in;
    secondsPerStep = sec_per_min * minutesPerStep;
    hoursPerStep = minutesPerStep / min_per_hr;
};

// public HPWH functions
HPWH::HPWH() : messageCallback(NULL), messageCallbackContextPtr(NULL), hpwhVerbosity(VRB_silent)
{
    setAllDefaults();
};

void HPWH::setAllDefaults()
{
    tankTemps_C.clear();
    nextTankTemps_C.clear();
    heatSources.clear();

    simHasFailed = true;
    isHeating = false;
    setpointFixed = false;
    tankSizeFixed = true;
    canScale = false;
    member_inletT_C = HPWH_ABORT;
    currentSoCFraction = 1.;
    doTempDepression = false;
    locationTemperature_C = UNINITIALIZED_LOCATIONTEMP;
    mixBelowFractionOnDraw = 1. / 3.;
    doInversionMixing = true;
    doConduction = true;
    inletHeight = 0;
    inlet2Height = 0;
    fittingsUA_kJperHrC = 0.;
    prevDRstatus = DR_ALLOW;
    timerLimitTOT = 60.;
    timerTOT = 0.;
    usesSoCLogic = false;
    setMinutesPerStep(1.0);
    hasHeatExchanger = false;
    heatExchangerEffectiveness = 0.9;
}

HPWH::HPWH(const HPWH& hpwh) { *this = hpwh; }

HPWH& HPWH::operator=(const HPWH& hpwh)
{
    if (this == &hpwh)
    {
        return *this;
    }

    simHasFailed = hpwh.simHasFailed;

    hpwhVerbosity = hpwh.hpwhVerbosity;

    // these should actually be the same pointers
    messageCallback = hpwh.messageCallback;
    messageCallbackContextPtr = hpwh.messageCallbackContextPtr;

    isHeating = hpwh.isHeating;

    heatSources = hpwh.heatSources;
    for (auto& heatSource : heatSources)
    {
        heatSource.hpwh = this;
        // HeatSource assignment will fail (causing the simulation to fail) if a
        // HeatSource has backups/companions.
        // This could be dealt with in this function (tricky), but the HeatSource
        // assignment can't know where its backup/companion pointer goes so it either
        // fails or silently does something that's not at all useful.
        // I prefer it to fail.  -NDK  1/2016
    }

    tankVolume_L = hpwh.tankVolume_L;
    tankUA_kJperHrC = hpwh.tankUA_kJperHrC;
    fittingsUA_kJperHrC = hpwh.fittingsUA_kJperHrC;

    setpoint_C = hpwh.setpoint_C;

    tankTemps_C = hpwh.tankTemps_C;
    nextTankTemps_C = hpwh.nextTankTemps_C;

    inletHeight = hpwh.inletHeight;
    inlet2Height = hpwh.inlet2Height;

    outletTemp_C = hpwh.outletTemp_C;
    condenserInlet_C = hpwh.condenserInlet_C;
    condenserOutlet_C = hpwh.condenserOutlet_C;
    externalVolumeHeated_L = hpwh.externalVolumeHeated_L;
    energyRemovedFromEnvironment_kWh = hpwh.energyRemovedFromEnvironment_kWh;
    standbyLosses_kWh = hpwh.standbyLosses_kWh;

    tankMixesOnDraw = hpwh.tankMixesOnDraw;
    mixBelowFractionOnDraw = hpwh.mixBelowFractionOnDraw;

    doTempDepression = hpwh.doTempDepression;

    doInversionMixing = hpwh.doInversionMixing;
    doConduction = hpwh.doConduction;

    locationTemperature_C = hpwh.locationTemperature_C;

    prevDRstatus = hpwh.prevDRstatus;
    timerLimitTOT = hpwh.timerLimitTOT;

    usesSoCLogic = hpwh.usesSoCLogic;

    nodeVolume_L = hpwh.nodeVolume_L;
    nodeHeight_m = hpwh.nodeHeight_m;
    fracAreaTop = hpwh.fracAreaTop;
    fracAreaSide = hpwh.fracAreaSide;
    return *this;
}

HPWH::~HPWH() {}

int HPWH::runOneStep(double drawVolume_L,
                     double tankAmbientT_C,
                     double heatSourceAmbientT_C,
                     DRMODES DRstatus,
                     double inletVol2_L,
                     double inletT2_C,
                     std::vector<double>* extraHeatDist_W)
{
    // returns 0 on successful completion, HPWH_ABORT on failure

    // check for errors
    if (doTempDepression == true && minutesPerStep != 1)
    {
        msg("minutesPerStep must equal one for temperature depression to work.  \n");
        simHasFailed = true;
        return HPWH_ABORT;
    }

    if ((DRstatus & (DR_TOO | DR_TOT)))
    {
        if (hpwhVerbosity >= VRB_typical)
        {
            msg("DR_TOO | DR_TOT use conflicting logic sets. The logic will follow a DR_TOT scheme "
                " \n");
        }
    }

    if (hpwhVerbosity >= VRB_typical)
    {
        msg("Beginning runOneStep.  \nTank Temps: ");
        printTankTemps();
        msg("Step Inputs: InletT_C:  %.2lf, drawVolume_L:  %.2lf, tankAmbientT_C:  %.2lf, "
            "heatSourceAmbientT_C:  %.2lf, DRstatus:  %d, minutesPerStep:  %.2lf \n",
            member_inletT_C,
            drawVolume_L,
            tankAmbientT_C,
            heatSourceAmbientT_C,
            DRstatus,
            minutesPerStep);
    }
    // is the failure flag is set, don't run
    if (simHasFailed)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("simHasFailed is set, aborting.  \n");
        }
        return HPWH_ABORT;
    }

    // reset the output variables
    outletTemp_C = 0.;
    condenserInlet_C = 0.;
    condenserOutlet_C = 0.;
    externalVolumeHeated_L = 0.;
    energyRemovedFromEnvironment_kWh = 0.;
    standbyLosses_kWh = 0.;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = 0;
        heatSources[i].energyInput_kWh = 0.;
        heatSources[i].energyOutput_kWh = 0.;
    }
    extraEnergyInput_kWh = 0.;

    // if you are doing temp. depression, set tank and heatSource ambient temps
    // to the tracked locationTemperature
    double temperatureGoal = tankAmbientT_C;
    if (doTempDepression)
    {
        if (locationTemperature_C == UNINITIALIZED_LOCATIONTEMP)
        {
            locationTemperature_C = tankAmbientT_C;
        }
        tankAmbientT_C = locationTemperature_C;
        heatSourceAmbientT_C = locationTemperature_C;
    }

    // process draws and standby losses
    updateTankTemps(drawVolume_L, member_inletT_C, tankAmbientT_C, inletVol2_L, inletT2_C);

    updateSoCIfNecessary();

    // First Logic DR checks //////////////////////////////////////////////////////////////////

    // If the DR signal includes a top off but the previous signal did not, then top it off!
    if ((DRstatus & DR_LOC) != 0 && (DRstatus & DR_LOR) != 0)
    {
        turnAllHeatSourcesOff(); // turns off isheating
        if (hpwhVerbosity >= VRB_emetic)
        {
            msg("DR_LOC | DR_LOC everything off, DRstatus = %i \n", DRstatus);
        }
    }
    else
    { // do normal check
        if (((DRstatus & DR_TOO) != 0 || (DRstatus & DR_TOT) != 0) && timerTOT == 0)
        {

            // turn on the compressor and last resistance element.
            if (hasACompressor())
            {
                heatSources[compressorIndex].engageHeatSource(DRstatus);
            }
            if (lowestElementIndex >= 0)
            {
                heatSources[lowestElementIndex].engageHeatSource(DRstatus);
            }

            if (hpwhVerbosity >= VRB_emetic)
            {
                msg("TURNED ON DR_TOO engaged compressor and lowest resistance element, DRstatus = "
                    "%i \n",
                    DRstatus);
            }
        }

        // do HeatSource choice
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (hpwhVerbosity >= VRB_emetic)
            {
                msg("Heat source choice:\theatsource %d can choose from %lu turn on logics and %lu "
                    "shut off logics\n",
                    i,
                    heatSources[i].turnOnLogicSet.size(),
                    heatSources[i].shutOffLogicSet.size());
            }
            if (isHeating == true)
            {
                // check if anything that is on needs to turn off (generally for lowT cutoffs)
                // things that just turn on later this step are checked for this in shouldHeat
                if (heatSources[i].isEngaged() && heatSources[i].shutsOff())
                {
                    heatSources[i].disengageHeatSource();
                    // check if the backup heat source would have to shut off too
                    if (heatSources[i].backupHeatSource != NULL &&
                        heatSources[i].backupHeatSource->shutsOff() != true)
                    {
                        // and if not, go ahead and turn it on
                        heatSources[i].backupHeatSource->engageHeatSource(DRstatus);
                    }
                }

                // if there's a priority HeatSource (e.g. upper resistor) and it needs to
                // come on, then turn  off and start it up
                if (heatSources[i].isVIP)
                {
                    if (hpwhVerbosity >= VRB_emetic)
                    {
                        msg("\tVIP check");
                    }
                    if (heatSources[i].shouldHeat())
                    {
                        if (shouldDRLockOut(heatSources[i].typeOfHeatSource, DRstatus))
                        {
                            if (hasACompressor())
                            {
                                heatSources[compressorIndex].engageHeatSource(DRstatus);
                                break;
                            }
                        }
                        else
                        {
                            turnAllHeatSourcesOff();
                            heatSources[i].engageHeatSource(DRstatus);
                            // stop looking if the VIP needs to run
                            break;
                        }
                    }
                }
            }
            // if nothing is currently on, then check if something should come on
            else /* (isHeating == false) */
            {
                if (heatSources[i].shouldHeat())
                {
                    heatSources[i].engageHeatSource(DRstatus);
                    // engaging a heat source sets isHeating to true, so this will only trigger once
                }
            }

        } // end loop over heat sources

        if (hpwhVerbosity >= VRB_emetic)
        {
            msg("after heat source choosing:  ");
            for (int i = 0; i < getNumHeatSources(); i++)
            {
                msg("heat source %d: %d \t", i, heatSources[i].isEngaged());
            }
            msg("\n");
        }

        // do heating logic
        double minutesToRun = minutesPerStep;
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            // check/apply lock-outs
            if (hpwhVerbosity >= VRB_emetic)
            {
                msg("Checking lock-out logic for heat source %d:\n", i);
            }
            if (shouldDRLockOut(heatSources[i].typeOfHeatSource, DRstatus))
            {
                heatSources[i].lockOutHeatSource();
                if (hpwhVerbosity >= VRB_emetic)
                {
                    msg("Locked out heat source, DRstatus = %i\n", DRstatus);
                }
            }
            else
            {
                // locks or unlocks the heat source
                heatSources[i].toLockOrUnlock(heatSourceAmbientT_C);
            }
            if (heatSources[i].isLockedOut() && heatSources[i].backupHeatSource == NULL)
            {
                heatSources[i].disengageHeatSource();
                if (hpwhVerbosity >= HPWH::VRB_emetic)
                {
                    msg("\nWARNING: lock-out triggered, but no backupHeatSource defined. "
                        "Simulation will continue will lock out the heat source.");
                }
            }

            // going through in order, check if the heat source is on
            if (heatSources[i].isEngaged())
            {

                HeatSource* heatSourcePtr;
                if (heatSources[i].isLockedOut() && heatSources[i].backupHeatSource != NULL)
                {

                    // Check that the backup isn't locked out too or already engaged then it will
                    // heat on its own.
                    if (heatSources[i].backupHeatSource->toLockOrUnlock(heatSourceAmbientT_C) ||
                        shouldDRLockOut(heatSources[i].backupHeatSource->typeOfHeatSource,
                                        DRstatus) || //){
                        heatSources[i].backupHeatSource->isEngaged())
                    {
                        continue;
                    }
                    // Don't turn the backup electric resistance heat source on if the VIP
                    // resistance element is on .
                    else if (VIPIndex >= 0 && heatSources[VIPIndex].isOn &&
                             heatSources[i].backupHeatSource->isAResistance())
                    {
                        if (hpwhVerbosity >= VRB_typical)
                        {
                            msg("Locked out back up heat source AND the engaged heat source %i, "
                                "DRstatus = %i\n",
                                i,
                                DRstatus);
                        }
                        continue;
                    }
                    else
                    {
                        heatSourcePtr = heatSources[i].backupHeatSource;
                    }
                }
                else
                {
                    heatSourcePtr = &heatSources[i];
                }

                addHeatParent(heatSourcePtr, heatSourceAmbientT_C, minutesToRun);

                // if it finished early. i.e. shuts off early like if the heatsource met setpoint or
                // maxed out
                if (heatSourcePtr->runtime_min < minutesToRun)
                {
                    // debugging message handling
                    if (hpwhVerbosity >= VRB_emetic)
                    {
                        msg("done heating! runtime_min minutesToRun %.2lf %.2lf\n",
                            heatSourcePtr->runtime_min,
                            minutesToRun);
                    }

                    // subtract time it ran and turn it off
                    minutesToRun -= heatSourcePtr->runtime_min;
                    heatSources[i].disengageHeatSource();
                    // and if there's a heat source that follows this heat source (regardless of
                    // lockout) that's able to come on,
                    if (heatSources[i].followedByHeatSource != NULL &&
                        heatSources[i].followedByHeatSource->shutsOff() == false)
                    {
                        // turn it on
                        heatSources[i].followedByHeatSource->engageHeatSource(DRstatus);
                    }
                    // or if there heat source can't produce hotter water (i.e. it's maxed out) and
                    // the tank still isn't at setpoint. the compressor should get locked out when
                    // the maxedOut is true but have to run the resistance first during this
                    // timestep to make sure tank is above the max temperature for the compressor.
                    else if (heatSources[i].maxedOut() && heatSources[i].backupHeatSource != NULL)
                    {

                        // Check that the backup isn't locked out or already engaged then it will
                        // heat or already heated on it's own.
                        if (!heatSources[i].backupHeatSource->toLockOrUnlock(
                                heatSourceAmbientT_C) && // If not locked out
                            !shouldDRLockOut(heatSources[i].backupHeatSource->typeOfHeatSource,
                                             DRstatus) && // and not DR locked out
                            !heatSources[i].backupHeatSource->isEngaged())
                        { // and not already engaged

                            HeatSource* backupHeatSourcePtr = heatSources[i].backupHeatSource;
                            // turn it on
                            backupHeatSourcePtr->engageHeatSource(DRstatus);
                            // add heat if it hasn't heated up this whole minute already
                            if (backupHeatSourcePtr->runtime_min >= 0.)
                            {
                                addHeatParent(backupHeatSourcePtr,
                                              heatSourceAmbientT_C,
                                              minutesToRun - backupHeatSourcePtr->runtime_min);
                            }
                        }
                    }
                }
            } // heat source not engaged
        }     // end while iHS heat source
    }
    if (areAllHeatSourcesOff() == true)
    {
        isHeating = false;
    }
    // If there's extra user defined heat to add -> Add extra heat!
    if (extraHeatDist_W != NULL && (*extraHeatDist_W).size() != 0)
    {
        addExtraHeat(*extraHeatDist_W);
        updateSoCIfNecessary();
    }

    // track the depressed local temperature
    if (doTempDepression)
    {
        bool compressorRan = false;
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isEngaged() && !heatSources[i].isLockedOut() &&
                heatSources[i].depressesTemperature)
            {
                compressorRan = true;
            }
        }

        if (compressorRan)
        {
            temperatureGoal -= maxDepression_C; // hardcoded 4.5 degree total drop - from
                                                // experimental data. Changed to an input
        }
        else
        {
            // otherwise, do nothing, we're going back to ambient
        }

        // shrink the gap by the same percentage every minute - that gives us
        // exponential behavior the percentage was determined by a fit to
        // experimental data - 9.4 minute half life and 4.5 degree total drop
        // minus-equals is important, and fits with the order of locationTemperature
        // and temperatureGoal, so as to not use fabs() and conditional tests
        locationTemperature_C -= (locationTemperature_C - temperatureGoal) * (1 - 0.9289);
    }

    // settle outputs

    // outletTemp_C and standbyLosses_kWh are taken care of in updateTankTemps

    // sum energyRemovedFromEnvironment_kWh for each heat source;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        energyRemovedFromEnvironment_kWh +=
            (heatSources[i].energyOutput_kWh - heatSources[i].energyInput_kWh);
    }

    // cursory check for inverted temperature profile
    if (tankTemps_C[getNumNodes() - 1] < tankTemps_C[0])
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The top of the tank is cooler than the bottom.  \n");
        }
    }

    // Handle DR timer
    prevDRstatus = DRstatus;
    // DR check for TOT to increase timer.
    timerTOT += minutesPerStep;
    // Restart the time if we're over the limit or the command is not a top off.
    if ((DRstatus & DR_TOT) != 0 && timerTOT >= timerLimitTOT)
    {
        resetTopOffTimer();
    }
    else if ((DRstatus & DR_TOO) == 0 && (DRstatus & DR_TOT) == 0)
    {
        resetTopOffTimer();
    }

    if (simHasFailed)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The simulation has encountered an error.  \n");
        }
        return HPWH_ABORT;
    }

    if (hpwhVerbosity >= VRB_typical)
    {
        msg("Ending runOneStep.  \n\n\n\n");
    }

    return 0; // successful completion of the step returns 0
} // end runOneStep

int HPWH::runNSteps(int N,
                    double* inletT_C,
                    double* drawVolume_L,
                    double* tankAmbientT_C,
                    double* heatSourceAmbientT_C,
                    DRMODES* DRstatus)
{
    // returns 0 on successful completion, HPWH_ABORT on failure

    // these are all the accumulating variables we'll need
    double energyRemovedFromEnvironment_kWh_SUM = 0;
    double standbyLosses_kWh_SUM = 0;
    double outletTemp_C_AVG = 0;
    double totalDrawVolume_L = 0;
    std::vector<double> heatSources_runTimes_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyInputs_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyOutputs_SUM(getNumHeatSources());

    if (hpwhVerbosity >= VRB_typical)
    {
        msg("Begin runNSteps.  \n");
    }
    // run the sim one step at a time, accumulating the outputs as you go
    for (int i = 0; i < N; i++)
    {
        runOneStep(
            inletT_C[i], drawVolume_L[i], tankAmbientT_C[i], heatSourceAmbientT_C[i], DRstatus[i]);

        if (simHasFailed)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("RunNSteps has encountered an error on step %d of N and has ceased running.  "
                    "\n",
                    i + 1);
            }
            return HPWH_ABORT;
        }

        energyRemovedFromEnvironment_kWh_SUM += energyRemovedFromEnvironment_kWh;
        standbyLosses_kWh_SUM += standbyLosses_kWh;

        outletTemp_C_AVG += outletTemp_C * drawVolume_L[i];
        totalDrawVolume_L += drawVolume_L[i];

        for (int j = 0; j < getNumHeatSources(); j++)
        {
            heatSources_runTimes_SUM[j] += getNthHeatSourceRunTime(j);
            heatSources_energyInputs_SUM[j] += getNthHeatSourceEnergyInput(j);
            heatSources_energyOutputs_SUM[j] += getNthHeatSourceEnergyOutput(j);
        }

        // print minutely output
        if (hpwhVerbosity == VRB_minuteOut)
        {
            msg("%f,%f,%f,", tankAmbientT_C[i], drawVolume_L[i], inletT_C[i]);
            for (int j = 0; j < getNumHeatSources(); j++)
            {
                msg("%f,%f,", getNthHeatSourceEnergyInput(j), getNthHeatSourceEnergyOutput(j));
            }

            std::vector<double> displayTemps_C(10);
            resampleIntensive(displayTemps_C, tankTemps_C);
            bool first = true;
            for (auto& displayTemp : displayTemps_C)
            {
                if (first)
                    first = false;
                else
                    msg(",");

                msg("%f", displayTemp);
            }

            for (int k = 1; k < 7; ++k)
            {
                if (first)
                    first = false;
                else
                    msg(",");

                msg("%f", getNthSimTcouple(k, 6));
            }

            msg("\n");
        }
    }
    // finish weighted avg. of outlet temp by dividing by the total drawn volume
    outletTemp_C_AVG /= totalDrawVolume_L;

    // now, reassign all of the accumulated values to their original spots
    energyRemovedFromEnvironment_kWh = energyRemovedFromEnvironment_kWh_SUM;
    standbyLosses_kWh = standbyLosses_kWh_SUM;
    outletTemp_C = outletTemp_C_AVG;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = heatSources_runTimes_SUM[i];
        heatSources[i].energyInput_kWh = heatSources_energyInputs_SUM[i];
        heatSources[i].energyOutput_kWh = heatSources_energyOutputs_SUM[i];
    }

    if (hpwhVerbosity >= VRB_typical)
    {
        msg("Ending runNSteps.  \n\n\n\n");
    }
    return 0;
}

void HPWH::addHeatParent(HeatSource* heatSourcePtr,
                         double heatSourceAmbientT_C,
                         double minutesToRun)
{

    double tempSetpoint_C = -273.15;

    // Check the air temprature and setpoint against maxOut_at_LowT
    if (heatSourcePtr->isACompressor())
    {
        if (heatSourceAmbientT_C <= heatSourcePtr->maxOut_at_LowT.airT_C &&
            setpoint_C >= heatSourcePtr->maxOut_at_LowT.outT_C)
        {
            tempSetpoint_C = setpoint_C; // Store setpoint
            setSetpoint(heatSourcePtr->maxOut_at_LowT
                            .outT_C); // Reset to new setpoint as this is used in the add heat calc
        }
    }
    // and add heat if it is
    heatSourcePtr->addHeat(heatSourceAmbientT_C, minutesToRun);

    // Change the setpoint back to what it was pre-compressor depression
    if (tempSetpoint_C > -273.15)
    {
        setSetpoint(tempSetpoint_C);
    }
}

void HPWH::setVerbosity(VERBOSITY hpwhVrb) { hpwhVerbosity = hpwhVrb; }
void HPWH::setMessageCallback(void (*callbackFunc)(const string message, void* contextPtr),
                              void* contextPtr)
{
    messageCallback = callbackFunc;
    messageCallbackContextPtr = contextPtr;
}
void HPWH::sayMessage(const string message) const
{
    if (messageCallback != NULL)
    {
        (*messageCallback)(message, messageCallbackContextPtr);
    }
    else
    {
        std::cout << message;
    }
}
void HPWH::msg(const char* fmt, ...) const
{
    va_list ap;
    va_start(ap, fmt);
    msgV(fmt, ap);
}
void HPWH::msgV(const char* fmt, va_list ap /*=NULL*/) const
{
    char outputString[MAXOUTSTRING];

    const char* p;
    if (ap)
    {
#if defined(_MSC_VER)
        vsprintf_s<MAXOUTSTRING>(outputString, fmt, ap);
#else
        vsnprintf(outputString, MAXOUTSTRING, fmt, ap);
#endif
        p = outputString;
    }
    else
    {
        p = fmt;
    }
    sayMessage(p);
} // HPWH::msgV

void HPWH::printHeatSourceInfo()
{
    std::stringstream ss;
    double runtime = 0, outputVar = 0;

    ss << std::left;
    ss << std::fixed;
    ss << std::setprecision(2);
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        ss << "heat source " << i << ": " << isNthHeatSourceRunning(i) << "\t\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        ss << "input energy kwh: " << std::setw(7) << getNthHeatSourceEnergyInput(i) << "\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        runtime = getNthHeatSourceRunTime(i);
        if (runtime != 0)
        {
            outputVar = getNthHeatSourceEnergyInput(i) / (runtime / 60.0);
        }
        else
        {
            outputVar = 0;
        }
        ss << "input power kw: " << std::setw(7) << outputVar << "\t\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        ss << "output energy kwh: " << std::setw(7) << getNthHeatSourceEnergyOutput(i, UNITS_KWH)
           << "\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        runtime = getNthHeatSourceRunTime(i);
        if (runtime != 0)
        {
            outputVar = getNthHeatSourceEnergyOutput(i, UNITS_KWH) / (runtime / 60.0);
        }
        else
        {
            outputVar = 0;
        }
        ss << "output power kw: " << std::setw(7) << outputVar << "\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        ss << "run time min: " << std::setw(7) << getNthHeatSourceRunTime(i) << "\t\t";
    }
    ss << endl << endl << endl;

    msg(ss.str().c_str());
}

void HPWH::printTankTemps()
{
    std::stringstream ss;

    ss << std::left;

    for (int i = 0; i < getNumNodes(); i++)
    {
        ss << std::setw(9) << getTankNodeTemp(i) << " ";
    }
    ss << endl;

    msg(ss.str().c_str());
}

// public members to write to CSV file
int HPWH::WriteCSVHeading(FILE* outFILE, const char* preamble, int nTCouples, int options) const
{

    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    fprintf(outFILE, "%s", preamble);

    fprintf(outFILE, "%s", "DRstatus");

    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        fprintf(outFILE, ",h_src%dIn (Wh),h_src%dOut (Wh)", iHS + 1, iHS + 1);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        fprintf(outFILE, ",tcouple%d (%s)", iTC + 1, doIP ? "F" : "C");
    }

    fprintf(outFILE, ",toutlet (%s)", doIP ? "F" : "C");

    fprintf(outFILE, "\n");

    return 0;
}

int HPWH::WriteCSVRow(FILE* outFILE, const char* preamble, int nTCouples, int options) const
{

    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    fprintf(outFILE, "%s", preamble);

    fprintf(outFILE, "%i", prevDRstatus);

    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        fprintf(outFILE,
                ",%0.2f,%0.2f",
                getNthHeatSourceEnergyInput(iHS, UNITS_KWH) * 1000.,
                getNthHeatSourceEnergyOutput(iHS, UNITS_KWH) * 1000.);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        fprintf(outFILE, ",%0.2f", getNthSimTcouple(iTC + 1, nTCouples, doIP ? UNITS_F : UNITS_C));
    }

    if (options & HPWH::CSVOPT_IS_DRAWING)
    {
        fprintf(outFILE, ",%0.2f", doIP ? C_TO_F(outletTemp_C) : outletTemp_C);
    }
    else
    {
        fprintf(outFILE, ",");
    }

    fprintf(outFILE, "\n");

    return 0;
}

bool HPWH::isSetpointFixed() const { return setpointFixed; }

int HPWH::setSetpoint(double newSetpoint, UNITS units /*=UNITS_C*/)
{

    double newSetpoint_C, temp;
    string why;
    if (units == UNITS_C)
    {
        newSetpoint_C = newSetpoint;
    }
    else if (units == UNITS_F)
    {
        newSetpoint_C = F_TO_C(newSetpoint);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for setSetpoint.  \n");
        }
        return HPWH_ABORT;
    }
    if (!isNewSetpointPossible(newSetpoint_C, temp, why))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Unwilling to set this setpoint for the currently selected model, max setpoint is "
                "%f C. %s\n",
                temp,
                why.c_str());
        }
        return HPWH_ABORT;
    }

    setpoint_C = newSetpoint_C;

    return 0;
}

double HPWH::getSetpoint(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return setpoint_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(setpoint_C);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getSetpoint. \n");
        }
        return HPWH_ABORT;
    }
}

double HPWH::getMaxCompressorSetpoint(UNITS units /*=UNITS_C*/) const
{

    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Unit does not have a compressor \n");
        }
        return HPWH_ABORT;
    }

    double returnVal = heatSources[compressorIndex].maxSetpoint_C;
    if (units == UNITS_C)
    {
        returnVal = returnVal;
    }
    else if (units == UNITS_F)
    {
        returnVal = C_TO_F(returnVal);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getMaxCompressorSetpoint. \n");
        }
        return HPWH_ABORT;
    }
    return returnVal;
}

bool HPWH::isNewSetpointPossible(double newSetpoint,
                                 double& maxAllowedSetpoint,
                                 string& why,
                                 UNITS units /*=UNITS_C*/) const
{
    double newSetpoint_C;
    double maxAllowedSetpoint_C = -273.15;
    if (units == UNITS_C)
    {
        newSetpoint_C = newSetpoint;
    }
    else if (units == UNITS_F)
    {
        newSetpoint_C = F_TO_C(newSetpoint);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for isNewSetpointPossible. \n");
        }
        return false;
    }
    bool returnVal = false;

    if (isSetpointFixed())
    {
        returnVal = (newSetpoint == setpoint_C);
        maxAllowedSetpoint_C = setpoint_C;
        if (!returnVal)
        {
            why = "The set point is fixed for the currently selected model.";
        }
    }
    else
    {

        if (hasACompressor())
        { // If there's a compressor lets check the new setpoint against the compressor's max
          // setpoint

            maxAllowedSetpoint_C =
                heatSources[compressorIndex].maxSetpoint_C -
                heatSources[compressorIndex].secondaryHeatExchanger.hotSideTemperatureOffset_dC;

            if (newSetpoint_C > maxAllowedSetpoint_C && lowestElementIndex == -1)
            {
                why = "The compressor cannot meet the setpoint temperature and there is no "
                      "resistance backup.";
                returnVal = false;
            }
            else
            {
                returnVal = true;
            }
        }
        if (lowestElementIndex >= 0)
        { // If there's a resistance element lets check the new setpoint against the its max
          // setpoint
            maxAllowedSetpoint_C = heatSources[lowestElementIndex].maxSetpoint_C;

            if (newSetpoint_C > maxAllowedSetpoint_C)
            {
                why = "The resistance elements cannot produce water this hot.";
                returnVal = false;
            }
            else
            {
                returnVal = true;
            }
        }
        else if (lowestElementIndex == -1 && !hasACompressor())
        { // There are no heat sources here!
            if (hpwhModel == MODELS_StorageTank)
            {
                returnVal = true; // The one pass the storage tank doesn't have any heating elements
                                  // so sure change the setpoint it does nothing!
            }
            else
            {
                why = "There aren't any heat sources to check the new setpoint against!";
                returnVal = false;
            }
        }
    }

    if (units == UNITS_C)
    {
        maxAllowedSetpoint = maxAllowedSetpoint_C;
    }
    else if (units == UNITS_F)
    {
        maxAllowedSetpoint = C_TO_F(maxAllowedSetpoint_C);
    }
    return returnVal;
}

double HPWH::calcSoCFraction(double tMains_C, double tMinUseful_C, double tMax_C) const
{
    // Note that volume is ignored in here since with even nodes it cancels out of the SoC
    // fractional equation
    if (tMains_C >= tMinUseful_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("tMains_C is greater than or equal tMinUseful_C. \n");
        }
        return HPWH_ABORT;
    }
    if (tMinUseful_C > tMax_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("tMinUseful_C is greater tMax_C. \n");
        }
        return HPWH_ABORT;
    }

    double chargeEquivalent = 0.;
    for (auto& T : tankTemps_C)
    {
        chargeEquivalent += getChargePerNode(tMains_C, tMinUseful_C, T);
    }
    double maxSoC = getNumNodes() * getChargePerNode(tMains_C, tMinUseful_C, tMax_C);
    return chargeEquivalent / maxSoC;
}

double HPWH::getSoCFraction() const { return currentSoCFraction; }

void HPWH::calcAndSetSoCFraction()
{
    double newSoCFraction = -1.;

    std::shared_ptr<SoCBasedHeatingLogic> logicSoC =
        std::dynamic_pointer_cast<SoCBasedHeatingLogic>(
            heatSources[compressorIndex].turnOnLogicSet[0]);
    newSoCFraction = calcSoCFraction(logicSoC->getMainsT_C(), logicSoC->getTempMinUseful_C());

    currentSoCFraction = newSoCFraction;
}

double HPWH::getChargePerNode(double tCold, double tMix, double tHot) const
{
    if (tHot < tMix)
    {
        return 0.;
    }
    return (tHot - tCold) / (tMix - tCold);
}

double HPWH::getMinOperatingTemp(UNITS units /*=UNITS_C*/) const
{
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("No compressor found in this HPWH. \n");
        }
        return HPWH_ABORT;
    }
    if (units == UNITS_C)
    {
        return heatSources[compressorIndex].minT;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(heatSources[compressorIndex].minT);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getMinOperatingTemp.\n");
        }
        return HPWH_ABORT;
    }
}

int HPWH::resetTankToSetpoint() { return setTankToTemperature(setpoint_C); }

int HPWH::setTankToTemperature(double temp_C) { return setTankLayerTemperatures({temp_C}); }

//-----------------------------------------------------------------------------
///	@brief	Assigns new temps provided from a std::vector to tankTemps_C.
/// @param[in]	setTankTemps	new tank temps (arbitrary non-zero size)
///	@param[in]	units          temp units in setTankTemps (default = UNITS_C)
/// @return	Success: 0; Failure: HPWH_ABORT
//-----------------------------------------------------------------------------
int HPWH::setTankLayerTemperatures(std::vector<double> setTankTemps, const UNITS units)
{
    if ((units != UNITS_C) && (units != UNITS_F))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for setSetpoint.  \n");
        }
        return HPWH_ABORT;
    }

    std::size_t numSetNodes = setTankTemps.size();
    if (numSetNodes == 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("No temperatures provided.\n");
        }
        return HPWH_ABORT;
    }

    // convert setTankTemps to °C, if necessary
    if (units == UNITS_F)
        for (auto& T : setTankTemps)
            T = F_TO_C(T);

    // set node temps
    if (!resampleIntensive(tankTemps_C, setTankTemps))
        return HPWH_ABORT;

    return 0;
}

void HPWH::getTankTemps(std::vector<double>& tankTemps) { tankTemps = tankTemps_C; }

int HPWH::setAirFlowFreedom(double fanFraction)
{
    if (fanFraction < 0 || fanFraction > 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to set the fan fraction outside of bounds.  \n");
        }
        simHasFailed = true;
        return HPWH_ABORT;
    }
    else
    {
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isACompressor())
            {
                heatSources[i].airflowFreedom = fanFraction;
            }
        }
    }
    return 0;
}

int HPWH::setDoTempDepression(bool doTempDepress)
{
    this->doTempDepression = doTempDepress;
    return 0;
}

int HPWH::setTankSize_adjustUA(double HPWH_size,
                               UNITS units /*=UNITS_L*/,
                               bool forceChange /*=false*/)
{
    // Uses the UA before the function is called and adjusts the A part of the UA to match the input
    // volume given getTankSurfaceArea().
    double HPWH_size_L;
    double oldA = getTankSurfaceArea(UNITS_FT2);

    if (units == UNITS_L)
    {
        HPWH_size_L = HPWH_size;
    }
    else if (units == UNITS_GAL)
    {
        HPWH_size_L = GAL_TO_L(HPWH_size);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for setTankSize_adjustUA.  \n");
        }
        return HPWH_ABORT;
    }
    setTankSize(HPWH_size_L, UNITS_L, forceChange);
    setUA(tankUA_kJperHrC / oldA * getTankSurfaceArea(UNITS_FT2), UNITS_kJperHrC);
    return 0;
}

/*static*/ double
HPWH::getTankSurfaceArea(double vol, UNITS volUnits /*=UNITS_L*/, UNITS surfAUnits /*=UNITS_FT2*/)
{
    // returns tank surface area, old defualt was in ft2
    // Based off 88 insulated storage tanks currently available on the market from Sanden, AOSmith,
    // HTP, Rheem, and Niles. Corresponds to the inner tank with volume tankVolume_L with the
    // assumption that the aspect ratio is the same as the outer dimenisions of the whole unit.
    double radius = getTankRadius(vol, volUnits, UNITS_FT);

    double value = 2. * 3.14159 * pow(radius, 2) * (ASPECTRATIO + 1.);

    if (value >= 0.)
    {
        if (surfAUnits == UNITS_M2)
            value = FT2_TO_M2(value);
        else if (surfAUnits != UNITS_FT2)
            value = -1.;
    }
    return value;
}

double HPWH::getTankSurfaceArea(UNITS units /*=UNITS_FT2*/) const
{
    // returns tank surface area, old defualt was in ft2
    // Based off 88 insulated storage tanks currently available on the market from Sanden, AOSmith,
    // HTP, Rheem, and Niles. Corresponds to the inner tank with volume tankVolume_L with the
    // assumption that the aspect ratio is the same as the outer dimenisions of the whole unit.
    double value = getTankSurfaceArea(tankVolume_L, UNITS_L, units);
    if (value < 0.)
    {
        if (hpwhVerbosity >= VRB_reluctant)
            msg("Incorrect unit specification for getTankSurfaceArea.  \n");
        value = HPWH_ABORT;
    }
    return value;
}

/*static*/ double
HPWH::getTankRadius(double vol, UNITS volUnits /*=UNITS_L*/, UNITS radiusUnits /*=UNITS_FT*/)
{ // returns tank radius, ft for use in calculation of heat loss in the bottom and top of the tank.
    // Based off 88 insulated storage tanks currently available on the market from Sanden, AOSmith,
    // HTP, Rheem, and Niles, assumes the aspect ratio for the outer measurements is the same is the
    // actual tank.
    double volft3 = volUnits == UNITS_L     ? L_TO_FT3(vol)
                    : volUnits == UNITS_GAL ? L_TO_FT3(GAL_TO_L(vol))
                                            : -1.;

    double value = -1.;
    if (volft3 >= 0.)
    {
        value = pow(volft3 / 3.14159 / ASPECTRATIO, 1. / 3.);
        if (radiusUnits == UNITS_M)
            value = FT_TO_M(value);
        else if (radiusUnits != UNITS_FT)
            value = -1.;
    }
    return value;
}

double HPWH::getTankRadius(UNITS units /*=UNITS_FT*/) const
{
    // returns tank radius, ft for use in calculation of heat loss in the bottom and top of the
    // tank. Based off 88 insulated storage tanks currently available on the market from Sanden,
    // AOSmith, HTP, Rheem, and Niles, assumes the aspect ratio for the outer measurements is the
    // same is the actual tank.

    double value = getTankRadius(tankVolume_L, UNITS_L, units);

    if (value < 0.)
    {
        if (hpwhVerbosity >= VRB_reluctant)
            msg("Incorrect unit specification for getTankRadius.  \n");
        value = HPWH_ABORT;
    }
    return value;
}

bool HPWH::isTankSizeFixed() const { return tankSizeFixed; }

int HPWH::setTankSize(double HPWH_size, UNITS units /*=UNITS_L*/, bool forceChange /*=false*/)
{
    if (isTankSizeFixed() && !forceChange)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not change the tank size for your currently selected model.  \n");
        }
        return HPWH_ABORT;
    }
    if (HPWH_size <= 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to set the tank volume outside of bounds.  \n");
        }
        simHasFailed = true;
        return HPWH_ABORT;
    }
    else
    {
        if (units == UNITS_L)
        {
            this->tankVolume_L = HPWH_size;
        }
        else if (units == UNITS_GAL)
        {
            this->tankVolume_L = (GAL_TO_L(HPWH_size));
        }
        else
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Incorrect unit specification for setTankSize.  \n");
            }
            return HPWH_ABORT;
        }
    }

    calcSizeConstants();

    return 0;
}
int HPWH::setDoInversionMixing(bool doInversionMixing_in)
{
    doInversionMixing = doInversionMixing_in;
    return 0;
}
int HPWH::setDoConduction(bool doConduction_in)
{
    doConduction = doConduction_in;
    return 0;
}

int HPWH::setUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
{
    if (units == UNITS_kJperHrC)
    {
        tankUA_kJperHrC = UA;
    }
    else if (units == UNITS_BTUperHrF)
    {
        tankUA_kJperHrC = UAf_TO_UAc(UA);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for setUA.  \n");
        }
        return HPWH_ABORT;
    }
    return 0;
}

int HPWH::getUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    UA = tankUA_kJperHrC;
    if (units == UNITS_kJperHrC)
    {
        // UA is already in correct units
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UA / UAf_TO_UAc(1.);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getUA.  \n");
        }
        UA = -1.;
        return HPWH_ABORT;
    }
    return 0;
}

int HPWH::setFittingsUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
{
    if (units == UNITS_kJperHrC)
    {
        fittingsUA_kJperHrC = UA;
    }
    else if (units == UNITS_BTUperHrF)
    {
        fittingsUA_kJperHrC = UAf_TO_UAc(UA);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for setFittingsUA.  \n");
        }
        return HPWH_ABORT;
    }
    return 0;
}
int HPWH::getFittingsUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    UA = fittingsUA_kJperHrC;
    if (units == UNITS_kJperHrC)
    {
        // UA is already in correct units
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UA / UAf_TO_UAc(1.);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getUA.  \n");
        }
        UA = -1.;
        return HPWH_ABORT;
    }
    return 0;
}

int HPWH::setInletByFraction(double fractionalHeight)
{
    return setNodeNumFromFractionalHeight(fractionalHeight, inletHeight);
}
int HPWH::setInlet2ByFraction(double fractionalHeight)
{
    return setNodeNumFromFractionalHeight(fractionalHeight, inlet2Height);
}

int HPWH::setExternalInletHeightByFraction(double fractionalHeight)
{
    return setExternalPortHeightByFraction(fractionalHeight, 1);
}
int HPWH::setExternalOutletHeightByFraction(double fractionalHeight)
{
    return setExternalPortHeightByFraction(fractionalHeight, 2);
}

int HPWH::setExternalPortHeightByFraction(double fractionalHeight, int whichExternalPort)
{
    if (!hasExternalHeatSource())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Does not have an external heat source \n");
        }
        return HPWH_ABORT;
    }

    int returnVal = 0;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            if (whichExternalPort == 1)
            {
                returnVal = setNodeNumFromFractionalHeight(fractionalHeight,
                                                           heatSources[i].externalInletHeight);
            }
            else
            {
                returnVal = setNodeNumFromFractionalHeight(fractionalHeight,
                                                           heatSources[i].externalOutletHeight);
            }

            if (returnVal == HPWH_ABORT)
            {
                return returnVal;
            }
        }
    }
    return returnVal;
}

int HPWH::setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum)
{
    if (fractionalHeight > 1. || fractionalHeight < 0.)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Out of bounds fraction for setInletByFraction \n");
        }
        return HPWH_ABORT;
    }

    int node = (int)std::floor(getNumNodes() * fractionalHeight);
    inletNum = (node == getNumNodes()) ? getIndexTopNode() : node;

    return 0;
}

int HPWH::getExternalInletHeight() const
{
    if (!hasExternalHeatSource())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Does not have an external heat source \n");
        }
        return HPWH_ABORT;
    }
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return heatSources[i].externalInletHeight; // Return the first one since all external
                                                       // sources have some ports
        }
    }
    return HPWH_ABORT;
}
int HPWH::getExternalOutletHeight() const
{
    if (!hasExternalHeatSource())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Does not have an external heat source \n");
        }
        return HPWH_ABORT;
    }
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return heatSources[i].externalOutletHeight; // Return the first one since all external
                                                        // sources have some ports
        }
    }
    return HPWH_ABORT;
}

int HPWH::setTimerLimitTOT(double limit_min)
{
    if (limit_min > 24. * 60. || limit_min < 0.)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Out of bounds time limit for setTimerLimitTOT \n");
        }
        return HPWH_ABORT;
    }

    timerLimitTOT = limit_min;

    return 0;
}

double HPWH::getTimerLimitTOT_minute() const { return timerLimitTOT; }

int HPWH::getInletHeight(int whichInlet) const
{
    if (whichInlet == 1)
    {
        return inletHeight;
    }
    else if (whichInlet == 2)
    {
        return inlet2Height;
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Invalid inlet chosen in getInletHeight \n");
        }
        return HPWH_ABORT;
    }
}

int HPWH::setMaxTempDepression(double maxDepression, UNITS units /*=UNITS_C*/)
{
    if (units == UNITS_C)
    {
        maxDepression_C = maxDepression;
    }
    else if (units == UNITS_F)
    {
        maxDepression_C = F_TO_C(maxDepression);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for max Temp Depression.  \n");
        }
        return HPWH_ABORT;
    }
    return 0;
}

bool HPWH::hasEnteringWaterHighTempShutOff(int heatSourceIndex)
{
    bool retVal = false;
    if (heatSourceIndex >= getNumHeatSources() || heatSourceIndex < 0)
    {
        return retVal;
    }
    if (heatSources[heatSourceIndex].shutOffLogicSet.size() == 0)
    {
        return retVal;
    }

    for (std::shared_ptr<HeatingLogic> shutOffLogic : heatSources[heatSourceIndex].shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            retVal = true;
            break;
        }
    }
    return retVal;
}

int HPWH::setEnteringWaterHighTempShutOff(double highTemp,
                                          bool tempIsAbsolute,
                                          int heatSourceIndex,
                                          UNITS unit /*=UNITS_C*/)
{
    if (!hasEnteringWaterHighTempShutOff(heatSourceIndex))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to acess a heating logic that does not exist.  \n");
        }
        return HPWH_ABORT;
    }

    double highTemp_C;
    if (unit == UNITS_C)
    {
        highTemp_C = highTemp;
    }
    else if (unit == UNITS_F)
    {
        highTemp_C = F_TO_C(highTemp);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for set Entering Water High Temp Shut Off.  \n");
        }
        return HPWH_ABORT;
    }

    bool highTempIsNotValid = false;
    if (tempIsAbsolute)
    {
        // check differnce with setpoint
        if (setpoint_C - highTemp_C < MINSINGLEPASSLIFT)
        {
            highTempIsNotValid = true;
        }
    }
    else
    {
        if (highTemp_C < MINSINGLEPASSLIFT)
        {
            highTempIsNotValid = true;
        }
    }
    if (highTempIsNotValid)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("High temperature shut off is too close to the setpoint, excpected a minimum "
                "difference of %.2lf.\n",
                MINSINGLEPASSLIFT);
        }
        return HPWH_ABORT;
    }

    for (std::shared_ptr<HeatingLogic> shutOffLogic : heatSources[heatSourceIndex].shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            std::dynamic_pointer_cast<TempBasedHeatingLogic>(shutOffLogic)
                ->setDecisionPoint(highTemp_C, tempIsAbsolute);
            break;
        }
    }
    return 0;
}

int HPWH::setTargetSoCFraction(double target)
{
    if (!isSoCControlled())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not set target state of charge if HPWH is not using state of charge "
                "controls.");
        }
        return HPWH_ABORT;
    }
    if (target < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not set a negative target state of charge.");
        }
        return HPWH_ABORT;
    }

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].shutOffLogicSet)
        {
            if (!logic->getIsEnteringWaterHighTempShutoff())
            {
                logic->setDecisionPoint(target);
            }
        }
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].turnOnLogicSet)
        {
            logic->setDecisionPoint(target);
        }
    }
    return 0;
}

bool HPWH::isSoCControlled() const { return usesSoCLogic; }

bool HPWH::canUseSoCControls()
{
    bool retVal = true;
    if (getCompressorCoilConfig() != HPWH::HeatSource::CONFIG_EXTERNAL)
    {
        retVal = false;
    }
    return retVal;
}

int HPWH::switchToSoCControls(double targetSoC,
                              double hysteresisFraction /*= 0.05*/,
                              double tempMinUseful /*= 43.333*/,
                              bool constantMainsT /*= false*/,
                              double mainsT /*= 18.333*/,
                              UNITS tempUnit /*= UNITS_C*/)
{
    if (!canUseSoCControls())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Cannot set up state of charge controls for integrated or wrapped HPWHs.\n");
        }
        return HPWH_ABORT;
    }

    double tempMinUseful_C, mainsT_C;
    if (tempUnit == UNITS_C)
    {
        tempMinUseful_C = tempMinUseful;
        mainsT_C = mainsT;
    }
    else if (tempUnit == UNITS_F)
    {
        tempMinUseful_C = F_TO_C(tempMinUseful);
        mainsT_C = F_TO_C(mainsT);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for set Enterinh Water High Temp Shut Off.\n");
        }
        return HPWH_ABORT;
    }

    if (mainsT_C >= tempMinUseful_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The mains temperature can't be equal to or greater than the minimum useful "
                "temperature.\n");
        }
        return HPWH_ABORT;
    }

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].clearAllTurnOnLogic();

        heatSources[i].shutOffLogicSet.erase(
            std::remove_if(heatSources[i].shutOffLogicSet.begin(),
                           heatSources[i].shutOffLogicSet.end(),
                           [&](const auto logic) -> bool
                           { return !logic->getIsEnteringWaterHighTempShutoff(); }),
            heatSources[i].shutOffLogicSet.end());

        heatSources[i].shutOffLogicSet.push_back(shutOffSoC("SoC Shut Off",
                                                            targetSoC,
                                                            hysteresisFraction,
                                                            tempMinUseful_C,
                                                            constantMainsT,
                                                            mainsT_C));
        heatSources[i].turnOnLogicSet.push_back(turnOnSoC("SoC Turn On",
                                                          targetSoC,
                                                          hysteresisFraction,
                                                          tempMinUseful_C,
                                                          constantMainsT,
                                                          mainsT_C));
    }

    usesSoCLogic = true;

    return 0;
}

std::shared_ptr<HPWH::SoCBasedHeatingLogic> HPWH::turnOnSoC(string desc,
                                                            double targetSoC,
                                                            double hystFract,
                                                            double tempMinUseful_C,
                                                            bool constantMainsT,
                                                            double mainsT_C)
{
    return std::make_shared<SoCBasedHeatingLogic>(
        desc, targetSoC, this, -hystFract, tempMinUseful_C, constantMainsT, mainsT_C);
}

std::shared_ptr<HPWH::SoCBasedHeatingLogic> HPWH::shutOffSoC(string desc,
                                                             double targetSoC,
                                                             double hystFract,
                                                             double tempMinUseful_C,
                                                             bool constantMainsT,
                                                             double mainsT_C)
{
    return std::make_shared<SoCBasedHeatingLogic>(desc,
                                                  targetSoC,
                                                  this,
                                                  hystFract,
                                                  tempMinUseful_C,
                                                  constantMainsT,
                                                  mainsT_C,
                                                  std::greater<double>());
}

//-----------------------------------------------------------------------------
///	@brief	Builds a vector of logic node weights referred to a fixed number of
/// nodes given by LOGIC_NODE_SIZE.
/// @param[in]	bottomFraction	Lower bounding fraction (0 to 1)
///	@param[in]	topFraction		Upper bounding fraction (0 to 1)
/// @return	vector of node weights
//-----------------------------------------------------------------------------
std::vector<HPWH::NodeWeight> HPWH::getNodeWeightRange(double bottomFraction, double topFraction)
{
    std::vector<NodeWeight> nodeWeights;
    if (topFraction < bottomFraction)
        std::swap(bottomFraction, topFraction);
    auto bottomIndex = static_cast<std::size_t>(bottomFraction * LOGIC_NODE_SIZE);
    auto topIndex = static_cast<std::size_t>(topFraction * LOGIC_NODE_SIZE);
    for (auto index = bottomIndex; index < topIndex; ++index)
    {
        nodeWeights.emplace_back(static_cast<int>(index) + 1);
    }
    return nodeWeights;
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::wholeTank(double decisionPoint,
                                                             const UNITS units /* = UNITS_C */,
                                                             const bool absolute /* = false */)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "whole tank", nodeWeights, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird_absolute(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third absolute", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondThird(double decisionPoint,
                                                               const UNITS units /* = UNITS_C */,
                                                               const bool absolute /* = false */)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(1. / 3., 2. / 3.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second third", nodeWeights, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomThird(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom third", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth_absolute(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth absolute", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::thirdSixth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(2. / 6., 3. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "third sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fourthSixth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(3. / 6., 4. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fourth sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fifth sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomHalf(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom half", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfth(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::standby(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_NODE_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "standby", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topNodeMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_NODE_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top node", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic>
HPWH::bottomNodeMaxTemp(double decisionPoint, bool isEnteringWaterHighTempShutoff /*=false*/)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(0); // uses very bottom computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom node",
                                                         nodeWeights,
                                                         decisionPoint,
                                                         this,
                                                         true,
                                                         std::greater<double>(),
                                                         isEnteringWaterHighTempShutoff);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfthMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThirdMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixthMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixthMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixthMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixthMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largeDraw(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 4.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "large draw", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largerDraw(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "larger draw", nodeWeights, decisionPoint, this, true);
}

void HPWH::setNumNodes(const std::size_t num_nodes)
{
    tankTemps_C.resize(num_nodes);
    nextTankTemps_C.resize(num_nodes);
}

int HPWH::getNumNodes() const { return static_cast<int>(tankTemps_C.size()); }

int HPWH::getIndexTopNode() const { return getNumNodes() - 1; }

double HPWH::getTankNodeTemp(int nodeNum, UNITS units /*=UNITS_C*/) const
{
    if (tankTemps_C.empty())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the temperature of a tank node that does not exist.  "
                "\n");
        }
        return double(HPWH_ABORT);
    }
    else
    {
        double result = tankTemps_C[nodeNum];
        // if (result == double(HPWH_ABORT)) { can't happen?
        //	return result;
        // }
        if (units == UNITS_C)
        {
            return result;
        }
        else if (units == UNITS_F)
        {
            return C_TO_F(result);
        }
        else
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Incorrect unit specification for getTankNodeTemp.  \n");
            }
            return double(HPWH_ABORT);
        }
    }
}

double HPWH::getNthSimTcouple(int iTCouple, int nTCouple, UNITS units /*=UNITS_C*/) const
{
    if (iTCouple > nTCouple || iTCouple < 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access a simulated thermocouple that does not exist.  \n");
        }
        return double(HPWH_ABORT);
    }
    double beginFraction = static_cast<double>(iTCouple - 1.) / static_cast<double>(nTCouple);
    double endFraction = static_cast<double>(iTCouple) / static_cast<double>(nTCouple);

    double simTcoupleTemp_C = getResampledValue(tankTemps_C, beginFraction, endFraction);
    if (units == UNITS_C)
    {
        return simTcoupleTemp_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(simTcoupleTemp_C);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getNthSimTcouple.  \n");
        }
        return double(HPWH_ABORT);
    }
}

int HPWH::getNumHeatSources() const { return static_cast<int>(heatSources.size()); }

int HPWH::getCompressorIndex() const { return compressorIndex; }

int HPWH::getNumResistanceElements() const
{
    int count = 0;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        count += heatSources[i].isAResistance() ? 1 : 0;
    }
    return count;
}

double HPWH::getCompressorCapacity(double airTemp /*=19.722*/,
                                   double inletTemp /*=14.444*/,
                                   double outTemp /*=57.222*/,
                                   UNITS pwrUnit /*=UNITS_KW*/,
                                   UNITS tempUnit /*=UNITS_C*/)
{
    // calculate capacity btu/hr, input btu/hr, and cop
    double capTemp_BTUperHr, inputTemp_BTUperHr, copTemp; // temporary variables
    double airTemp_C, inletTemp_C, outTemp_C;

    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor.  \n");
        }
        return double(HPWH_ABORT);
    }

    if (tempUnit == UNITS_C)
    {
        airTemp_C = airTemp;
        inletTemp_C = inletTemp;
        outTemp_C = outTemp;
    }
    else if (tempUnit == UNITS_F)
    {
        airTemp_C = F_TO_C(airTemp);
        inletTemp_C = F_TO_C(inletTemp);
        outTemp_C = F_TO_C(outTemp);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for temperatures in getCompressorCapacity.  \n");
        }
        return double(HPWH_ABORT);
    }

    if (airTemp_C < heatSources[compressorIndex].minT ||
        airTemp_C > heatSources[compressorIndex].maxT)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The compress does not operate at the specified air temperature. \n");
        }
        return double(HPWH_ABORT);
    }

    double maxAllowedSetpoint_C =
        heatSources[compressorIndex].maxSetpoint_C -
        heatSources[compressorIndex].secondaryHeatExchanger.hotSideTemperatureOffset_dC;
    if (outTemp_C > maxAllowedSetpoint_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Inputted outlet temperature of the compressor is higher than can be produced.");
        }
        return double(HPWH_ABORT);
    }

    if (heatSources[compressorIndex].isExternalMultipass())
    {
        double averageTemp_C = (outTemp_C + inletTemp_C) / 2.;
        heatSources[compressorIndex].getCapacityMP(
            airTemp_C, averageTemp_C, inputTemp_BTUperHr, capTemp_BTUperHr, copTemp);
    }
    else
    {
        heatSources[compressorIndex].getCapacity(
            airTemp_C, inletTemp_C, outTemp_C, inputTemp_BTUperHr, capTemp_BTUperHr, copTemp);
    }

    double outputCapacity = capTemp_BTUperHr;
    if (pwrUnit == UNITS_KW)
    {
        outputCapacity = BTU_TO_KWH(capTemp_BTUperHr);
    }
    else if (pwrUnit != UNITS_BTUperHr)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for capacity in getCompressorCapacity.  \n");
        }
        return double(HPWH_ABORT);
    }

    return outputCapacity;
}

double HPWH::getNthHeatSourceEnergyInput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // energy used by the heat source is positive - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the energy input of a heat source that does not "
                "exist.  \n");
        }
        return double(HPWH_ABORT);
    }

    if (units == UNITS_KWH)
    {
        return heatSources[N].energyInput_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(heatSources[N].energyInput_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(heatSources[N].energyInput_kWh);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getNthHeatSourceEnergyInput.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getNthHeatSourceEnergyOutput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // returns energy from the heat source into the water - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the energy output of a heat source that does not "
                "exist.  \n");
        }
        return double(HPWH_ABORT);
    }

    if (units == UNITS_KWH)
    {
        return heatSources[N].energyOutput_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(heatSources[N].energyOutput_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(heatSources[N].energyOutput_kWh);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getNthHeatSourceEnergyInput.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getNthHeatSourceRunTime(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the run time of a heat source that does not exist.  "
                "\n");
        }
        return double(HPWH_ABORT);
    }
    return heatSources[N].runtime_min;
}

int HPWH::isNthHeatSourceRunning(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the status of a heat source that does not exist.  "
                "\n");
        }
        return HPWH_ABORT;
    }
    if (heatSources[N].isEngaged())
    {
        return 1;
    }
    else
    {
        return 0;
    }
}

HPWH::HEATSOURCE_TYPE HPWH::getNthHeatSourceType(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the type of a heat source that does not exist.  \n");
        }
        return HEATSOURCE_TYPE(HPWH_ABORT);
    }
    return heatSources[N].typeOfHeatSource;
}

double HPWH::getTankSize(UNITS units /*=UNITS_L*/) const
{
    if (units == UNITS_L)
    {
        return tankVolume_L;
    }
    else if (units == UNITS_GAL)
    {
        return L_TO_GAL(tankVolume_L);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getTankSize.  \n");
        }
        return HPWH_ABORT;
    }
}

double HPWH::getOutletTemp(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return outletTemp_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(outletTemp_C);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getOutletTemp.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getCondenserWaterInletTemp(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return condenserInlet_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(condenserInlet_C);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getCondenserWaterInletTemp.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getCondenserWaterOutletTemp(UNITS units /*=UNITS_C*/) const
{
    if (units == UNITS_C)
    {
        return condenserOutlet_C;
    }
    else if (units == UNITS_F)
    {
        return C_TO_F(condenserOutlet_C);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getCondenserWaterInletTemp.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getExternalVolumeHeated(UNITS units /*=UNITS_L*/) const
{
    if (units == UNITS_L)
    {
        return externalVolumeHeated_L;
    }
    else if (units == UNITS_GAL)
    {
        return L_TO_GAL(externalVolumeHeated_L);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getExternalVolumeHeated.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getEnergyRemovedFromEnvironment(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the space to the water is the positive direction
    if (units == UNITS_KWH)
    {
        return energyRemovedFromEnvironment_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(energyRemovedFromEnvironment_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(energyRemovedFromEnvironment_kWh);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getEnergyRemovedFromEnvironment.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getStandbyLosses(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the water to the space is the positive direction
    if (units == UNITS_KWH)
    {
        return standbyLosses_kWh;
    }
    else if (units == UNITS_BTU)
    {
        return KWH_TO_BTU(standbyLosses_kWh);
    }
    else if (units == UNITS_KJ)
    {
        return KWH_TO_KJ(standbyLosses_kWh);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getStandbyLosses.  \n");
        }
        return double(HPWH_ABORT);
    }
}

double HPWH::getTankHeatContent_kJ() const
{
    // returns tank heat content relative to 0 C using kJ

    // get average tank temperature
    double avgTemp = 0.0;
    for (int i = 0; i < getNumNodes(); i++)
    {
        avgTemp += tankTemps_C[i];
    }
    avgTemp /= getNumNodes();

    double totalHeat = avgTemp * DENSITYWATER_kgperL * CPWATER_kJperkgC * tankVolume_L;
    return totalHeat;
}

double HPWH::getLocationTemp_C() const { return locationTemperature_C; }

int HPWH::getHPWHModel() const { return hpwhModel; }
int HPWH::getCompressorCoilConfig() const
{
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor.  \n");
        }
        return HPWH_ABORT;
    }
    return heatSources[compressorIndex].configuration;
}
bool HPWH::isCompressorMultipass() const
{
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor.  \n");
        }
        return HPWH_ABORT;
    }
    return heatSources[compressorIndex].isMultipass;
}
bool HPWH::isCompressoExternalMultipass() const
{
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor.  \n");
        }
        return HPWH_ABORT;
    }
    return heatSources[compressorIndex].isExternalMultipass();
}

bool HPWH::hasACompressor() const { return compressorIndex >= 0; }

bool HPWH::hasExternalHeatSource() const
{
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return true;
        }
    }
    return false;
}

double HPWH::getExternalMPFlowRate(UNITS units /*=UNITS_GPM*/) const
{
    if (!isCompressoExternalMultipass())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Does not have an external multipass heat source \n");
        }
        return HPWH_ABORT;
    }

    if (units == HPWH::UNITS_LPS)
    {
        return heatSources[compressorIndex].mpFlowRate_LPS;
    }
    else if (units == HPWH::UNITS_GPM)
    {
        return LPS_TO_GPM(heatSources[compressorIndex].mpFlowRate_LPS);
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for getExternalMPFlowRate.  \n");
        }
        return (double)HPWH_ABORT;
    }
}

double HPWH::getCompressorMinRuntime(UNITS units /*=UNITS_MIN*/) const
{

    if (hasACompressor())
    {
        double min_minutes = 10.;

        if (units == UNITS_MIN)
        {
            return min_minutes;
        }
        else if (units == UNITS_SEC)
        {
            return MIN_TO_SEC(min_minutes);
        }
        else if (units == UNITS_HR)
        {
            return MIN_TO_HR(min_minutes);
        }
        else
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Incorrect unit specification for getCompressorMinRunTime.  \n");
            }
            return (double)HPWH_ABORT;
        }
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("This HPWH has no compressor.  \n");
        }
        return (double)HPWH_ABORT;
    }
}

int HPWH::getSizingFractions(double& aquaFract, double& useableFract) const
{
    double aFract = 1.;
    double useFract = 1.;

    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor. \n");
        }
        return HPWH_ABORT;
    }
    else if (usesSoCLogic)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model uses SOC control logic and does not have a definition for sizing "
                "fractions. \n");
        }
        return HPWH_ABORT;
    }

    // Every compressor must have at least one on logic
    for (std::shared_ptr<HeatingLogic> onLogic : heatSources[compressorIndex].turnOnLogicSet)
    {
        double tempA;

        if (hpwhVerbosity >= VRB_emetic)
        {
            msg("\tturnon logic: %s ", onLogic->description.c_str());
        }
        tempA = onLogic->nodeWeightAvgFract(); // if standby logic will return 1
        aFract = tempA < aFract ? tempA : aFract;
    }
    aquaFract = aFract;

    // Compressors don't need to have an off logic
    if (heatSources[compressorIndex].shutOffLogicSet.size() != 0)
    {
        for (std::shared_ptr<HeatingLogic> offLogic : heatSources[compressorIndex].shutOffLogicSet)
        {

            double tempUse;

            if (hpwhVerbosity >= VRB_emetic)
            {
                msg("\tshutsOff logic: %s ", offLogic->description.c_str());
            }
            if (offLogic->description == "large draw" || offLogic->description == "larger draw")
            {
                tempUse =
                    1.; // These logics are just for checking if there's a big draw to switch to RE
            }
            else
            {
                tempUse = 1. - offLogic->nodeWeightAvgFract();
            }
            useFract =
                tempUse < useFract ? tempUse : useFract; // Will return the smallest case of the
                                                         // useable fraction for multiple off logics
        }
        useableFract = useFract;
    }
    else
    {
        if (hpwhVerbosity >= VRB_emetic)
        {
            msg("\no shutoff logics present");
        }
        useableFract = 1.;
    }

    // Check if double's are approximately equally and adjust the relationship so it follows the
    // relationship we expect. The tolerance plays with 0.1 mm in position if the tank is 1m tall...
    double temp = 1. - useableFract;
    if (aboutEqual(aquaFract, temp))
    {
        useableFract = 1. - aquaFract + TOL_MINVALUE;
    }

    return 0;
}

bool HPWH::isHPWHScalable() const { return canScale; }

int HPWH::setScaleHPWHCapacityCOP(double scaleCapacity /*=1.0*/, double scaleCOP /*=1.0*/)
{
    if (!isHPWHScalable())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not scale the HPWH Capacity or COP  \n");
        }
        return HPWH_ABORT;
    }
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor.  \n");
        }
        return HPWH_ABORT;
    }
    if (scaleCapacity <= 0 || scaleCOP <= 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not scale the HPWH Capacity or COP to 0 or less than 0 \n");
        }
        return HPWH_ABORT;
    }

    for (auto& perfP : heatSources[compressorIndex].perfMap)
    {
        if (scaleCapacity != 1.)
        {
            std::transform(
                perfP.inputPower_coeffs.begin(),
                perfP.inputPower_coeffs.end(),
                perfP.inputPower_coeffs.begin(),
                std::bind(std::multiplies<double>(), std::placeholders::_1, scaleCapacity));
        }
        if (scaleCOP != 1.)
        {
            std::transform(perfP.COP_coeffs.begin(),
                           perfP.COP_coeffs.end(),
                           perfP.COP_coeffs.begin(),
                           std::bind(std::multiplies<double>(), std::placeholders::_1, scaleCOP));
        }
    }

    return 0;
}

int HPWH::setCompressorOutputCapacity(double newCapacity,
                                      double airTemp /*=19.722*/,
                                      double inletTemp /*=14.444*/,
                                      double outTemp /*=57.222*/,
                                      UNITS pwrUnit /*=UNITS_KW*/,
                                      UNITS tempUnit /*=UNITS_C*/)
{

    double oldCapacity = getCompressorCapacity(airTemp, inletTemp, outTemp, pwrUnit, tempUnit);
    if (oldCapacity == double(HPWH_ABORT))
    {
        return HPWH_ABORT;
    }

    double scale = newCapacity / oldCapacity;
    return setScaleHPWHCapacityCOP(scale, 1.); // Scale the compressor capacity
}

int HPWH::setResistanceCapacity(double power, int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (!isHPWHScalable())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not scale the resistance elements \n");
        }
        return HPWH_ABORT;
    }
    if (getNumResistanceElements() == 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("There are no resistance elements to set capacity for \n");
        }
        return HPWH_ABORT;
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Out of bounds value for which in setResistanceCapacity()\n");
        }
        return HPWH_ABORT;
    }
    if (power < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not have a negative input power \n");
        }
        return HPWH_ABORT;
    }
    // Unit conversion
    double watts;
    if (pwrUnit == UNITS_KW)
    {
        watts = power * 1000; // kW to W
    }
    else if (pwrUnit == UNITS_BTUperHr)
    {
        watts = BTU_TO_KWH(power) * 1000; // BTU to kW then kW to W
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for capacity in setResistanceCapacity.  \n");
        }
        return HPWH_ABORT;
    }

    // Whew so many checks...
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                heatSources[i].changeResistanceWatts(watts);
            }
        }
    }
    else
    {
        heatSources[resistanceHeightMap[which].index].changeResistanceWatts(watts);

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                heatSources[resistanceHeightMap[i].index].changeResistanceWatts(watts);
            }
        }
    }

    return 0;
}

double HPWH::getResistanceCapacity(int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (getNumResistanceElements() == 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("There are no resistance elements to return capacity for \n");
        }
        return HPWH_ABORT;
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Out of bounds value for which in getResistanceCapacity()\n");
        }
        return HPWH_ABORT;
    }

    double returnPower = 0;
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                returnPower += heatSources[i].perfMap[0].inputPower_coeffs[0];
            }
        }
    }
    else
    {
        // get the power from "which" element by height
        returnPower +=
            heatSources[resistanceHeightMap[which].index].perfMap[0].inputPower_coeffs[0];

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                returnPower +=
                    heatSources[resistanceHeightMap[i].index].perfMap[0].inputPower_coeffs[0];
            }
        }
    }

    // Unit conversion
    if (pwrUnit == UNITS_KW)
    {
        returnPower /= 1000.; // W to KW
    }
    else if (pwrUnit == UNITS_BTUperHr)
    {
        returnPower = KWH_TO_BTU(returnPower / 1000.); // W to BTU/hr
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Incorrect unit specification for capacity in getResistanceCapacity.  \n");
        }
        return HPWH_ABORT;
    }

    return returnPower;
}

int HPWH::getResistancePosition(int elementIndex) const
{

    if (elementIndex < 0 || elementIndex > getNumHeatSources() - 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Out of bounds value for which in getResistancePosition\n");
        }
        return HPWH_ABORT;
    }

    if (!heatSources[elementIndex].isAResistance())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("This index is not a resistance element\n");
        }
        return HPWH_ABORT;
    }

    for (int i = 0; i < heatSources[elementIndex].getCondensitySize(); i++)
    {
        if (heatSources[elementIndex].condensity[i] == 1)
        { // res elements have a condenstiy of 1 at a specific node
            return i;
        }
    }
    return HPWH_ABORT;
}

// the privates
void HPWH::updateTankTemps(double drawVolume_L,
                           double inletT_C,
                           double tankAmbientT_C,
                           double inletVol2_L,
                           double inletT2_C)
{

    outletTemp_C = 0.;

    /////////////////////////////////////////////////////////////////////////////////////////////////
    if (drawVolume_L > 0.)
    {

        // calculate how many nodes to draw (wholeNodesToDraw), and the remainder (drawFraction)
        if (inletVol2_L > drawVolume_L)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Volume in inlet 2 is greater than the draw volume.  \n");
            }
            simHasFailed = true;
            return;
        }

        // Check which inlet is higher;
        int highInletH;
        double highInletV, highInletT;
        int lowInletH;
        double lowInletT, lowInletV;
        if (inletHeight > inlet2Height)
        {
            highInletH = inletHeight;
            highInletV = drawVolume_L - inletVol2_L;
            highInletT = inletT_C;
            lowInletH = inlet2Height;
            lowInletT = inletT2_C;
            lowInletV = inletVol2_L;
        }
        else
        {
            highInletH = inlet2Height;
            highInletV = inletVol2_L;
            highInletT = inletT2_C;
            lowInletH = inletHeight;
            lowInletT = inletT_C;
            lowInletV = drawVolume_L - inletVol2_L;
        }

        if (hasHeatExchanger)
        { // heat-exchange models

            const double densityTank_kgperL = DENSITYWATER_kgperL;
            const double CpTank_kJperkgC = CPWATER_kJperkgC;

            double C_Node_kJperC = CpTank_kJperkgC * densityTank_kgperL * nodeVolume_L;
            double C_draw_kJperC = CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVolume_L;

            outletTemp_C = inletT_C;
            for (auto& nodeTemp : tankTemps_C)
            {
                double maxHeatExchange_kJ = C_draw_kJperC * (nodeTemp - outletTemp_C);
                double heatExchange_kJ = nodeHeatExchangerEffectiveness * maxHeatExchange_kJ;

                nodeTemp -= heatExchange_kJ / C_Node_kJperC;
                outletTemp_C += heatExchange_kJ / C_draw_kJperC;
            }
        }
        else
        {
            // calculate how many nodes to draw (drawVolume_N)
            double drawVolume_N = drawVolume_L / nodeVolume_L;
            if (drawVolume_L > tankVolume_L)
            {
                // if (hpwhVerbosity >= VRB_reluctant) {
                //	//msg("WARNING: Drawing more than the tank volume in one step is undefined
                //behavior.  Terminating simulation.  \n"); 	msg("WARNING: Drawing more than the tank
                //volume in one step is undefined behavior.  Continuing simulation at your own risk.
                //\n");
                // }
                // simHasFailed = true;
                // return;
                for (int i = 0; i < getNumNodes(); i++)
                {
                    outletTemp_C += tankTemps_C[i];
                    tankTemps_C[i] =
                        (inletT_C * (drawVolume_L - inletVol2_L) + inletT2_C * inletVol2_L) /
                        drawVolume_L;
                }
                outletTemp_C = (outletTemp_C / getNumNodes() * tankVolume_L +
                                tankTemps_C[0] * (drawVolume_L - tankVolume_L)) /
                               drawVolume_L * drawVolume_N;

                drawVolume_N = 0.;
            }

            while (drawVolume_N > 0)
            {

                // Draw one node at a time
                double drawFraction = drawVolume_N > 1. ? 1. : drawVolume_N;
                double nodeInletTV = 0.;

                // add temperature for outletT average
                outletTemp_C += drawFraction * tankTemps_C[getNumNodes() - 1];

                double cumInletFraction = 0.;
                for (int i = getNumNodes() - 1; i >= lowInletH; i--)
                {

                    // Reset inlet inputs at this node.
                    double nodeInletFraction = 0.;
                    nodeInletTV = 0.;

                    // Sum of all inlets Vi*Ti at this node
                    if (i == highInletH)
                    {
                        nodeInletTV += highInletV * drawFraction / drawVolume_L * highInletT;
                        nodeInletFraction += highInletV * drawFraction / drawVolume_L;
                    }
                    if (i == lowInletH)
                    {
                        nodeInletTV += lowInletV * drawFraction / drawVolume_L * lowInletT;
                        nodeInletFraction += lowInletV * drawFraction / drawVolume_L;

                        break; // if this is the bottom inlet break out of the four loop and use the
                               // boundary condition equation.
                    }

                    // Look at the volume and temperature fluxes into this node
                    tankTemps_C[i] = (1. - (drawFraction - cumInletFraction)) * tankTemps_C[i] +
                                     nodeInletTV +
                                     (drawFraction - (cumInletFraction + nodeInletFraction)) *
                                         tankTemps_C[i - 1];

                    cumInletFraction += nodeInletFraction;
                }

                // Boundary condition equation because it shouldn't take anything from tankTemps_C[i
                // - 1] but it also might not exist.
                tankTemps_C[lowInletH] =
                    (1. - (drawFraction - cumInletFraction)) * tankTemps_C[lowInletH] + nodeInletTV;

                drawVolume_N -= drawFraction;

                mixTankInversions();
            }

            // fill in average outlet T - it is a weighted averaged, with weights == nodes drawn
            outletTemp_C /= (drawVolume_L / nodeVolume_L);
        }

        // Account for mixing at the bottom of the tank
        if (tankMixesOnDraw && drawVolume_L > 0.)
        {
            int mixedBelowNode = (int)(getNumNodes() * mixBelowFractionOnDraw);
            mixTankNodes(0, mixedBelowNode, 3.0);
        }

    } // end if(draw_volume_L > 0)

    // Initialize newTankTemps_C
    nextTankTemps_C = tankTemps_C;

    double standbyLossesBottom_kJ = 0.;
    double standbyLossesTop_kJ = 0.;
    double standbyLossesSides_kJ = 0.;

    // Standby losses from the top and bottom of the tank
    {
        auto standbyLossRate_kJperHrC = tankUA_kJperHrC * fracAreaTop;

        standbyLossesBottom_kJ =
            standbyLossRate_kJperHrC * hoursPerStep * (tankTemps_C[0] - tankAmbientT_C);
        standbyLossesTop_kJ = standbyLossRate_kJperHrC * hoursPerStep *
                              (tankTemps_C[getNumNodes() - 1] - tankAmbientT_C);

        nextTankTemps_C.front() -= standbyLossesBottom_kJ / nodeCp_kJperC;
        nextTankTemps_C.back() -= standbyLossesTop_kJ / nodeCp_kJperC;
    }

    // Standby losses from the sides of the tank
    {
        auto standbyLossRate_kJperHrC =
            (tankUA_kJperHrC * fracAreaSide + fittingsUA_kJperHrC) / getNumNodes();
        for (int i = 0; i < getNumNodes(); i++)
        {
            double standbyLosses_kJ =
                standbyLossRate_kJperHrC * hoursPerStep * (tankTemps_C[i] - tankAmbientT_C);
            standbyLossesSides_kJ += standbyLosses_kJ;

            nextTankTemps_C[i] -= standbyLosses_kJ / nodeCp_kJperC;
        }
    }

    // Heat transfer between nodes
    if (doConduction)
    {

        // Get the "constant" tau for the stability condition and the conduction calculation
        const double tau = KWATER_WpermC /
                           ((CPWATER_kJperkgC * 1000.0) * (DENSITYWATER_kgperL * 1000.0) *
                            (nodeHeight_m * nodeHeight_m)) *
                           secondsPerStep;
        if (tau > 0.5)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("The stability condition for conduction has failed, these results are going to "
                    "be interesting!\n");
            }
            simHasFailed = true;
            return;
        }

        // End nodes
        if (getNumNodes() > 1)
        { // inner edges of top and bottom nodes
            nextTankTemps_C.front() += 2. * tau * (tankTemps_C[1] - tankTemps_C.front());
            nextTankTemps_C.back() +=
                2. * tau * (tankTemps_C[getNumNodes() - 2] - tankTemps_C.back());
        }

        // Internal nodes
        for (int i = 1; i < getNumNodes() - 1; i++)
        {
            nextTankTemps_C[i] +=
                2. * tau * (tankTemps_C[i + 1] - 2. * tankTemps_C[i] + tankTemps_C[i - 1]);
        }
    }

    // Update tankTemps_C
    tankTemps_C = nextTankTemps_C;

    standbyLosses_kWh +=
        KJ_TO_KWH(standbyLossesBottom_kJ + standbyLossesTop_kJ + standbyLossesSides_kJ);

    // check for inverted temperature profile
    mixTankInversions();

} // end updateTankTemps

void HPWH::updateSoCIfNecessary()
{
    if (usesSoCLogic)
    {
        calcAndSetSoCFraction();
    }
}

// Inversion mixing modeled after bigladder EnergyPlus code PK
void HPWH::mixTankInversions()
{
    bool hasInversion;
    const double volumePerNode_L = tankVolume_L / getNumNodes();
    // int numdos = 0;
    if (doInversionMixing)
    {
        do
        {
            hasInversion = false;
            // Start from the top and check downwards
            for (int i = getNumNodes() - 1; i > 0; i--)
            {
                if (tankTemps_C[i] < tankTemps_C[i - 1])
                {
                    // Temperature inversion!
                    hasInversion = true;

                    // Mix this inversion mixing temperature by averaging all of the inverted nodes
                    // together together.
                    double Tmixed = 0.0;
                    double massMixed = 0.0;
                    int m;
                    for (m = i; m >= 0; m--)
                    {
                        Tmixed += tankTemps_C[m] * (volumePerNode_L * DENSITYWATER_kgperL);
                        massMixed += (volumePerNode_L * DENSITYWATER_kgperL);
                        if ((m == 0) || (Tmixed / massMixed > tankTemps_C[m - 1]))
                        {
                            break;
                        }
                    }
                    Tmixed /= massMixed;

                    // Assign the tank temps from i to k
                    for (int k = i; k >= m; k--)
                        tankTemps_C[k] = Tmixed;
                }
            }

        } while (hasInversion);
    }
}

//-----------------------------------------------------------------------------
///	@brief	Adds heat amount qAdd_kJ at and above the node with index nodeNum.
///			Returns unused heat to prevent exceeding maximum or setpoint.
/// @note	Moved from HPWH::HeatSource
/// @param[in]	qAdd_kJ		Amount of heat to add
///	@param[in]	nodeNum		Lowest node at which to add heat
/// @param[in]	maxT_C		Maximum allowable temperature to maintain
//-----------------------------------------------------------------------------
double HPWH::addHeatAboveNode(double qAdd_kJ, int nodeNum, const double maxT_C)
{

    // Do not exceed maxT_C or setpoint
    double maxHeatToT_C = std::min(maxT_C, setpoint_C);

    if (hpwhVerbosity >= VRB_emetic)
    {
        msg("node %2d   cap_kwh %.4lf \n", nodeNum, KJ_TO_KWH(qAdd_kJ));
    }

    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (tankTemps_C[i] != tankTemps_C[i + 1])
        {
            break;
        }
        else
        {
            numNodesToHeat++;
        }
    }

    while ((qAdd_kJ > 0.) && (nodeNum + numNodesToHeat - 1 < getNumNodes()))
    {

        // assume there is another node above the equal-temp nodes
        int targetTempNodeNum = nodeNum + numNodesToHeat;

        double heatToT_C;
        if (targetTempNodeNum > (getNumNodes() - 1))
        {
            // no nodes above the equal-temp nodes; target temperature is the maximum
            heatToT_C = maxHeatToT_C;
        }
        else
        {
            heatToT_C = tankTemps_C[targetTempNodeNum];
            if (heatToT_C > maxHeatToT_C)
            {
                // Ensure temperature does not exceed maximum
                heatToT_C = maxHeatToT_C;
            }
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        double qIncrement_kJ = nodeCp_kJperC * numNodesToHeat * (heatToT_C - tankTemps_C[nodeNum]);

        if (qIncrement_kJ > qAdd_kJ)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT_C = tankTemps_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                tankTemps_C[nodeNum + j] = heatToT_C;
            }
            qAdd_kJ = 0.;
        }
        else if (qIncrement_kJ > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                tankTemps_C[nodeNum + j] = heatToT_C;
            qAdd_kJ -= qIncrement_kJ;
        }
        numNodesToHeat++;
    }

    // return any unused heat
    return qAdd_kJ;
}

//-----------------------------------------------------------------------------
///	@brief	Adds extra heat amount qAdd_kJ at and above the node with index nodeNum.
/// 		Does not limit final temperatures.
/// @param[in]	qAdd_kJ				Amount of heat to add
///	@param[in]	nodeNum				Lowest node at which to add heat
//-----------------------------------------------------------------------------
void HPWH::addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum)
{

    if (hpwhVerbosity >= VRB_emetic)
    {
        msg("node %2d   cap_kwh %.4lf \n", nodeNum, KJ_TO_KWH(qAdd_kJ));
    }

    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (tankTemps_C[i] != tankTemps_C[i + 1])
        {
            break;
        }
        else
        {
            numNodesToHeat++;
        }
    }

    while ((qAdd_kJ > 0.) && (nodeNum + numNodesToHeat - 1 < getNumNodes()))
    {

        // assume there is another node above the equal-temp nodes
        int targetTempNodeNum = nodeNum + numNodesToHeat;

        double heatToT_C;
        if (targetTempNodeNum > (getNumNodes() - 1))
        {
            // no nodes above the equal-temp nodes; target temperature limited by the heat available
            heatToT_C = tankTemps_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
        }
        else
        {
            heatToT_C = tankTemps_C[targetTempNodeNum];
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        double qIncrement_kJ = nodeCp_kJperC * numNodesToHeat * (heatToT_C - tankTemps_C[nodeNum]);

        if (qIncrement_kJ > qAdd_kJ)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT_C = tankTemps_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                tankTemps_C[nodeNum + j] = heatToT_C;
            }
            qAdd_kJ = 0.;
        }
        else if (qIncrement_kJ > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                tankTemps_C[nodeNum + j] = heatToT_C;
            qAdd_kJ -= qIncrement_kJ;
        }
        numNodesToHeat++;
    }
}

//-----------------------------------------------------------------------------
///	@brief	Modifies a heat distribution using a thermal distribution.
/// @param[in,out]	heatDistribution_W		The distribution to be modified
//-----------------------------------------------------------------------------
void HPWH::modifyHeatDistribution(std::vector<double>& heatDistribution_W)
{
    double totalHeat_W = 0.;
    for (auto& heatDist_W : heatDistribution_W)
        totalHeat_W += heatDist_W;

    if (totalHeat_W == 0.)
        return;

    for (auto& heatDist_W : heatDistribution_W)
        heatDist_W /= totalHeat_W;

    double shrinkageT_C = findShrinkageT_C(heatDistribution_W);
    int lowestNode = findLowestNode(heatDistribution_W, getNumNodes());

    std::vector<double> modHeatDistribution_W;
    calcThermalDist(modHeatDistribution_W, shrinkageT_C, lowestNode, tankTemps_C, setpoint_C);

    heatDistribution_W = modHeatDistribution_W;
    for (auto& heatDist_W : heatDistribution_W)
        heatDist_W *= totalHeat_W;
}

//-----------------------------------------------------------------------------
///	@brief	Adds extra heat to tank.
/// @param[in]	extraHeatDist_W		A distribution of extra heat to add
//-----------------------------------------------------------------------------
void HPWH::addExtraHeat(std::vector<double>& extraHeatDist_W)
{

    auto modHeatDistribution_W = extraHeatDist_W;
    modifyHeatDistribution(modHeatDistribution_W);

    std::vector<double> heatDistribution_W(getNumNodes());
    resampleExtensive(heatDistribution_W, modHeatDistribution_W);

    // Unnecessary unit conversions used here to match former method
    double tot_qAdded_BTUperHr = 0.;
    for (int i = getNumNodes() - 1; i >= 0; i--)
    {
        if (heatDistribution_W[i] != 0)
        {
            double qAdd_BTUperHr = KWH_TO_BTU(heatDistribution_W[i] / 1000.);
            double qAdd_KJ = BTU_TO_KJ(qAdd_BTUperHr * minutesPerStep / min_per_hr);
            addExtraHeatAboveNode(qAdd_KJ, i);
            tot_qAdded_BTUperHr += qAdd_BTUperHr;
        }
    }
    // Write the input & output energy
    extraEnergyInput_kWh = BTU_TO_KWH(tot_qAdded_BTUperHr * minutesPerStep / min_per_hr);
}

///////////////////////////////////////////////////////////////////////////////////

void HPWH::turnAllHeatSourcesOff()
{
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].disengageHeatSource();
    }
    isHeating = false;
}

bool HPWH::areAllHeatSourcesOff() const
{
    bool allOff = true;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isEngaged() == true)
        {
            allOff = false;
        }
    }
    return allOff;
}

double HPWH::tankAvg_C(const std::vector<HPWH::NodeWeight> nodeWeights) const
{
    double sum = 0;
    double totWeight = 0;

    std::vector<double> resampledTankTemps(LOGIC_NODE_SIZE);
    resample(resampledTankTemps, tankTemps_C);

    for (auto& nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum == 0)
        { // bottom node only
            sum += tankTemps_C.front() * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
        else if (nodeWeight.nodeNum > LOGIC_NODE_SIZE)
        { // top node only
            sum += tankTemps_C.back() * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
        else
        { // general case; sum over all weighted nodes
            sum += resampledTankTemps[static_cast<std::size_t>(nodeWeight.nodeNum - 1)] *
                   nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
    }
    return sum / totWeight;
}

void HPWH::mixTankNodes(int mixedAboveNode, int mixedBelowNode, double mixFactor)
{
    double ave = 0.;
    double numAvgNodes = (double)(mixedBelowNode - mixedAboveNode);
    for (int i = mixedAboveNode; i < mixedBelowNode; i++)
    {
        ave += tankTemps_C[i];
    }
    ave /= numAvgNodes;

    for (int i = mixedAboveNode; i < mixedBelowNode; i++)
    {
        tankTemps_C[i] += ((ave - tankTemps_C[i]) / mixFactor);
        // tankTemps_C[i] = tankTemps_C[i] * (1.0 - 1.0 / mixFactor) + ave / mixFactor;
    }
}

void HPWH::calcSizeConstants()
{
    // calculate conduction between the nodes AND heat loss by node with top and bottom having
    // greater surface area. model uses explicit finite difference to find conductive heat exchange
    // between the tank nodes with the boundary conditions on the top and bottom node being the
    // fraction of UA that corresponds to the top and bottom of the tank. The assumption is that the
    // aspect ratio is the same for all tanks and is the same for the outside measurements of the
    // unit and the inner water tank.
    const double tankRad_m = getTankRadius(UNITS_M);
    const double tankHeight_m = ASPECTRATIO * tankRad_m;

    nodeVolume_L = tankVolume_L / getNumNodes();
    nodeMass_kg = nodeVolume_L * DENSITYWATER_kgperL;
    nodeCp_kJperC = nodeMass_kg * CPWATER_kJperkgC;
    nodeHeight_m = tankHeight_m / getNumNodes();

    // The fraction of UA that is on the top or the bottom of the tank. So 2 * fracAreaTop +
    // fracAreaSide is the total tank area.
    fracAreaTop = tankRad_m / (2.0 * (tankHeight_m + tankRad_m));

    // fracAreaSide is the faction of the area of the cylinder that's not the top or bottom.
    fracAreaSide = tankHeight_m / (tankHeight_m + tankRad_m);

    /// Single-node heat-exchange effectiveness
    nodeHeatExchangerEffectiveness =
        1. - pow(1. - heatExchangerEffectiveness, 1. / static_cast<double>(getNumNodes()));
}

void HPWH::calcDerivedValues()
{
    // condentropy/shrinkage and lowestNode are now in calcDerivedHeatingValues()
    calcDerivedHeatingValues();

    calcSizeConstants();

    mapResRelativePosToHeatSources();

    // heat source ability to depress temp
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isACompressor())
        {
            heatSources[i].depressesTemperature = true;
        }
        else if (heatSources[i].isAResistance())
        {
            heatSources[i].depressesTemperature = false;
        }
    }
}

void HPWH::calcDerivedHeatingValues()
{
    static char outputString[MAXOUTSTRING]; // this is used for debugging outputs

    // find condentropy/shrinkage
    for (int i = 0; i < getNumHeatSources(); ++i)
    {
        heatSources[i].Tshrinkage_C = findShrinkageT_C(heatSources[i].condensity);

        if (hpwhVerbosity >= VRB_emetic)
        {
            msg(outputString, "Heat Source %d \n", i);
            msg(outputString, "shrinkage %.2lf \n\n", heatSources[i].Tshrinkage_C);
        }
    }

    // find lowest node
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].lowestNode = findLowestNode(heatSources[i].condensity, getNumNodes());

        if (hpwhVerbosity >= VRB_emetic)
        {
            msg(outputString, "Heat Source %d \n", i);
            msg(outputString, " lowest : %d \n", heatSources[i].lowestNode);
        }
    }

    // define condenser index and lowest resistance element index
    compressorIndex = -1;     // Default = No compressor
    lowestElementIndex = -1;  // Default = No resistance elements
    highestElementIndex = -1; // Default = No resistance elements
    VIPIndex = -1;            // Default = No VIP element
    double lowestPos = 1.;
    double highestPos = 0.; // -1 to make sure a an element on the bottom can still be identified.
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isACompressor())
        {
            compressorIndex = i; // NOTE: Maybe won't work with multiple compressors (last
                                 // compressor will be used)
        }
        else if (heatSources[i].isAResistance())
        {
            // Gets VIP element index
            if (heatSources[i].isVIP)
            {
                if (VIPIndex == -1)
                {
                    VIPIndex = i;
                }
                else
                {
                    if (hpwhVerbosity >= VRB_minuteOut)
                    {
                        msg("More than one resistance element is assigned to VIP");
                    };
                }
            }
            int condensitySize = heatSources[i].getCondensitySize();
            for (int j = 0; j < condensitySize; ++j)
            {
                double pos = static_cast<double>(j) / condensitySize;
                if ((heatSources[i].condensity[j] > 0.) && (pos < lowestPos))
                {
                    lowestElementIndex = i;
                    lowestPos = pos;
                }
                if ((heatSources[i].condensity[j] > 0.) && (pos >= highestPos))
                {
                    highestElementIndex = i;
                    highestPos = pos;
                }
            }
        }
    }
    if (hpwhVerbosity >= VRB_emetic)
    {
        msg(outputString, " compressorIndex : %d \n", compressorIndex);
        msg(outputString, " lowestElementIndex : %d \n", lowestElementIndex);
        msg(outputString, " highestElementIndex : %d \n", highestElementIndex);
    }
    if (hpwhVerbosity >= VRB_emetic)
    {
        msg(outputString, " VIPIndex : %d \n", VIPIndex);
    }

    // heat source ability to depress temp
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isACompressor())
        {
            heatSources[i].depressesTemperature = true;
        }
        else if (heatSources[i].isAResistance())
        {
            heatSources[i].depressesTemperature = false;
        }
    }
}

void HPWH::mapResRelativePosToHeatSources()
{
    resistanceHeightMap.clear();
    resistanceHeightMap.reserve(getNumResistanceElements());

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isAResistance())
        {
            resistanceHeightMap.push_back({i, getResistancePosition(i)});
        }
    }

    // Sort by height, low to high
    std::sort(resistanceHeightMap.begin(),
              resistanceHeightMap.end(),
              [](const HPWH::resPoint& a, const HPWH::resPoint& b) -> bool
              {
                  return a.position < b.position; // (5 < 5)      // evaluates to false
              });
}

// Used to check a few inputs after the initialization of a tank model from a preset or a file.
int HPWH::checkInputs()
{
    int returnVal = 0;
    // use a returnVal so that all checks are processed and error messages written

    if (getNumHeatSources() <= 0 && hpwhModel != MODELS_StorageTank)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You must have at least one HeatSource.\n");
        }
        returnVal = HPWH_ABORT;
    }

    double condensitySum;
    // loop through all heat sources to check each for malconfigurations
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        // check the heat source type to make sure it has been set
        if (heatSources[i].typeOfHeatSource == TYPE_none)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Heat source %d does not have a specified type.  Initialization failed.\n", i);
            }
            returnVal = HPWH_ABORT;
        }
        // check to make sure there is at least one onlogic or parent with onlogic
        int parent = heatSources[i].findParent();
        if (heatSources[i].turnOnLogicSet.size() == 0 &&
            (parent == -1 || heatSources[parent].turnOnLogicSet.size() == 0))
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("You must specify at least one logic to turn on the element or the element "
                    "must be set as a backup for another heat source with at least one logic.");
            }
            returnVal = HPWH_ABORT;
        }

        // Validate on logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].turnOnLogicSet)
        {
            if (!logic->isValid())
            {
                returnVal = HPWH_ABORT;
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("On logic at index %i is invalid", i);
                }
            }
        }
        // Validate off logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].shutOffLogicSet)
        {
            if (!logic->isValid())
            {
                returnVal = HPWH_ABORT;
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Off logic at index %i is invalid", i);
                }
            }
        }

        // check is condensity sums to 1
        condensitySum = 0;

        for (int j = 0; j < heatSources[i].getCondensitySize(); j++)
            condensitySum += heatSources[i].condensity[j];
        if (fabs(condensitySum - 1.0) > 1e-6)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("The condensity for heatsource %d does not sum to 1.  \n", i);
                msg("It sums to %f \n", condensitySum);
            }
            returnVal = HPWH_ABORT;
        }
        // check that air flows are all set properly
        if (heatSources[i].airflowFreedom > 1.0 || heatSources[i].airflowFreedom <= 0.0)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("The airflowFreedom must be between 0 and 1 for heatsource %d.  \n", i);
            }
            returnVal = HPWH_ABORT;
        }

        if (heatSources[i].isACompressor())
        {
            if (heatSources[i].doDefrost)
            {
                if (heatSources[i].defrostMap.size() < 3)
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Defrost logic set to true but no valid defrost map of length 3 or "
                            "greater set. \n");
                    }
                    returnVal = HPWH_ABORT;
                }
                if (heatSources[i].configuration != HeatSource::CONFIG_EXTERNAL)
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Defrost is only simulated for external compressors. \n");
                    }
                    returnVal = HPWH_ABORT;
                }
            }
        }
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {

            if (heatSources[i].shutOffLogicSet.size() != 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("External heat sources can only have one shut off logic. \n ");
                }
                returnVal = HPWH_ABORT;
            }
            if (0 > heatSources[i].externalOutletHeight ||
                heatSources[i].externalOutletHeight > getNumNodes() - 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("External heat sources need an external outlet height within the bounds "
                        "from from 0 to numNodes-1. \n");
                }
                returnVal = HPWH_ABORT;
            }
            if (0 > heatSources[i].externalInletHeight ||
                heatSources[i].externalInletHeight > getNumNodes() - 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("External heat sources need an external inlet height within the bounds "
                        "from from 0 to numNodes-1. \n");
                }
                returnVal = HPWH_ABORT;
            }
        }
        else
        {
            if (heatSources[i].secondaryHeatExchanger.extraPumpPower_W != 0 ||
                heatSources[i].secondaryHeatExchanger.extraPumpPower_W)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Heatsource %d is not an external heat source but has an external "
                        "secondary heat exchanger. \n",
                        i);
                }
                returnVal = HPWH_ABORT;
            }
        }

        // Check performance map
        // perfGrid and perfGridValues, and the length of vectors in perfGridValues are equal and
        // that ;
        if (heatSources[i].useBtwxtGrid)
        {
            // If useBtwxtGrid is true that the perfMap is empty
            if (heatSources[i].perfMap.size() != 0)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Using the grid lookups but a regression based perforamnce map is given "
                        "\n");
                }
                returnVal = HPWH_ABORT;
            }

            // Check length of vectors in perfGridValue are equal
            if (heatSources[i].perfGridValues[0].size() !=
                    heatSources[i].perfGridValues[1].size() &&
                heatSources[i].perfGridValues[0].size() != 0)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("When using grid lookups for perfmance the vectors in perfGridValues must "
                        "be the same length. \n");
                }
                returnVal = HPWH_ABORT;
            }

            // Check perfGrid's vectors lengths multiplied together == the perfGridValues vector
            // lengths
            size_t expLength = 1;
            for (const auto& v : heatSources[i].perfGrid)
            {
                expLength *= v.size();
            }
            if (expLength != heatSources[i].perfGridValues[0].size())
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("When using grid lookups for perfmance the vectors in perfGridValues must "
                        "be the same length. \n");
                }
                returnVal = HPWH_ABORT;
            }
        }
        else
        {
            // Check that perfmap only has 1 point if config_external and multipass
            if (heatSources[i].isExternalMultipass() && heatSources[i].perfMap.size() != 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("External multipass heat sources must have a perfMap of only one point "
                        "with regression equations. \n");
                }
                returnVal = HPWH_ABORT;
            }
        }
    }

    // Check that the on logic and off logics are ordered properly
    if (hasACompressor())
    {
        double aquaF = 0., useF = 1.;
        getSizingFractions(aquaF, useF);
        if (aquaF < (1. - useF))
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("The relationship between the on logic and off logic is not supported. The off "
                    "logic is beneath the on logic.");
            }
            returnVal = HPWH_ABORT;
        }
    }

    double maxTemp;
    string why;
    double tempSetpoint = setpoint_C;
    if (!isNewSetpointPossible(tempSetpoint, maxTemp, why))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Cannot set new setpoint. %s", why.c_str());
        }
        returnVal = HPWH_ABORT;
    }

    // Check if the UA is out of bounds
    if (tankUA_kJperHrC < 0.0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The tankUA_kJperHrC is less than 0 for a HPWH, it must be greater than 0, "
                "tankUA_kJperHrC is: %f  \n",
                tankUA_kJperHrC);
        }
        returnVal = HPWH_ABORT;
    }

    // Check single-node heat-exchange effectiveness validity
    if (heatExchangerEffectiveness > 1.)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Heat-exchanger effectiveness cannot exceed 1.\n");
        }
        returnVal = HPWH_ABORT;
    }

    // if there's no failures, return 0
    return returnVal;
}

bool HPWH::shouldDRLockOut(HEATSOURCE_TYPE hs, DRMODES DR_signal) const
{

    if (hs == TYPE_compressor && (DR_signal & DR_LOC) != 0)
    {
        return true;
    }
    else if (hs == TYPE_resistance && (DR_signal & DR_LOR) != 0)
    {
        return true;
    }
    return false;
}

void HPWH::resetTopOffTimer() { timerTOT = 0.; }

//-----------------------------------------------------------------------------
///	@brief	Checks whether energy is balanced during a simulation step.
/// @note	Used in test/main.cc
/// @param[in]	drawVol_L				Water volume drawn during simulation step
///	@param[in]	prevHeatContent_kJ		Heat content of tank prior to simulation step
///	@param[in]	fracEnergyTolerance		Fractional tolerance for energy imbalance
/// @return	true if balanced; false otherwise.
//-----------------------------------------------------------------------------
bool HPWH::isEnergyBalanced(const double drawVol_L,
                            const double prevHeatContent_kJ,
                            const double fracEnergyTolerance /* = 0.001 */)
{
    // Check energy balancing.
    double qInElectrical_kJ = 0.;
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        qInElectrical_kJ += getNthHeatSourceEnergyInput(iHS, UNITS_KJ);
    }

    double qInExtra_kJ = KWH_TO_KJ(extraEnergyInput_kWh);
    double qInHeatSourceEnviron_kJ = getEnergyRemovedFromEnvironment(UNITS_KJ);
    double qOutTankEnviron_kJ = KWH_TO_KJ(standbyLosses_kWh);
    double qOutWater_kJ = drawVol_L * (outletTemp_C - member_inletT_C) * DENSITYWATER_kgperL *
                          CPWATER_kJperkgC; // assumes only one inlet
    double expectedTankHeatContent_kJ =
        prevHeatContent_kJ        // previous heat content
        + qInElectrical_kJ        // electrical energy delivered to heat sources
        + qInExtra_kJ             // extra energy delivered to heat sources
        + qInHeatSourceEnviron_kJ // heat extracted from environment by condenser
        - qOutTankEnviron_kJ      // heat released from tank to environment
        - qOutWater_kJ;           // heat expelled to outlet by water flow

    double qBal_kJ = getTankHeatContent_kJ() - expectedTankHeatContent_kJ;

    double fracEnergyDiff = fabs(qBal_kJ) / std::max(prevHeatContent_kJ, 1.);
    if (fracEnergyDiff > fracEnergyTolerance)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Energy-balance error: %f kJ, %f %% \n", qBal_kJ, 100. * fracEnergyDiff);
        }
        return false;
    }
    return true;
}

#ifndef HPWH_ABRIDGED
int HPWH::HPWHinit_file(string configFile)
{

    setAllDefaults(); // reset all defaults if you're re-initilizing
    // sets simHasFailed = true; this gets cleared on successful completion of init
    // return 0 on success, HPWH_ABORT for failure

    // open file, check and report errors
    std::ifstream inputFILE;
    inputFILE.open(configFile.c_str());
    if (!inputFILE.is_open())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Input file failed to open.  \n");
        }
        return HPWH_ABORT;
    }

    // some variables that will be handy
    int heatsource, sourceNum, nTemps, tempInt;
    std::size_t num_nodes = 0, numHeatSources = 0;
    bool hasInitialTankTemp = false;
    double initalTankT_C = F_TO_C(120.);

    string tempString, units;
    double tempDouble;

    // being file processing, line by line
    string line_s;
    std::stringstream line_ss;
    string token;
    while (std::getline(inputFILE, line_s))
    {
        line_ss.clear();
        line_ss.str(line_s);

        // grab the first word, and start comparing
        line_ss >> token;
        if (token.at(0) == '#' || line_s.empty())
        {
            // if you hit a comment, skip to next line
            continue;
        }
        else if (token == "numNodes")
        {
            line_ss >> num_nodes;
        }
        else if (token == "volume")
        {
            line_ss >> tempDouble >> units;
            if (units == "gal")
                tempDouble = GAL_TO_L(tempDouble);
            else if (units == "L")
                ; // do nothing, lol
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Incorrect units specification for %s.  \n", token.c_str());
                }
                return HPWH_ABORT;
            }
            tankVolume_L = tempDouble;
        }
        else if (token == "UA")
        {
            line_ss >> tempDouble >> units;
            if (units != "kJperHrC")
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Incorrect units specification for %s.  \n", token.c_str());
                }
                return HPWH_ABORT;
            }
            tankUA_kJperHrC = tempDouble;
        }
        else if (token == "depressTemp")
        {
            line_ss >> tempString;
            if (tempString == "true")
            {
                doTempDepression = true;
            }
            else if (tempString == "false")
            {
                doTempDepression = false;
            }
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Improper value for %s\n", token.c_str());
                }
                return HPWH_ABORT;
            }
        }
        else if (token == "mixOnDraw")
        {
            line_ss >> tempString;
            if (tempString == "true")
            {
                tankMixesOnDraw = true;
            }
            else if (tempString == "false")
            {
                tankMixesOnDraw = false;
            }
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Improper value for %s\n", token.c_str());
                }
                return HPWH_ABORT;
            }
        }
        else if (token == "mixBelowFractionOnDraw")
        {
            line_ss >> tempDouble;
            if (tempDouble < 0 || tempDouble > 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Out of bounds value for %s. Should be between 0 and 1. \n", token.c_str());
                }
                return HPWH_ABORT;
            }
            mixBelowFractionOnDraw = tempDouble;
        }
        else if (token == "setpoint")
        {
            line_ss >> tempDouble >> units;
            if (units == "F")
                tempDouble = F_TO_C(tempDouble);
            else if (units == "C")
                ; // do nothing, lol
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Incorrect units specification for %s.  \n", token.c_str());
                }
                return HPWH_ABORT;
            }
            setpoint_C = tempDouble;
            // tank will be set to setpoint at end of function
        }
        else if (token == "setpointFixed")
        {
            line_ss >> tempString;
            if (tempString == "true")
                setpointFixed = true;
            else if (tempString == "false")
                setpointFixed = false;
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Improper value for %s\n", token.c_str());
                }
                return HPWH_ABORT;
            }
        }
        else if (token == "initialTankTemp")
        {
            line_ss >> tempDouble >> units;
            if (units == "F")
                tempDouble = F_TO_C(tempDouble);
            else if (units == "C")
                ;
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Incorrect units specification for %s.  \n", token.c_str());
                }
                return HPWH_ABORT;
            }
            initalTankT_C = tempDouble;
            hasInitialTankTemp = true;
        }
        else if (token == "hasHeatExchanger")
        {
            // false of this model uses heat exchange
            line_ss >> tempString;
            if (tempString == "true")
                hasHeatExchanger = true;
            else if (tempString == "false")
                hasHeatExchanger = false;
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Improper value for %s\n", token.c_str());
                }
                return HPWH_ABORT;
            }
        }
        else if (token == "heatExchangerEffectiveness")
        {
            // applies to heat-exchange models only
            line_ss >> tempDouble;
            heatExchangerEffectiveness = tempDouble;
        }
        else if (token == "verbosity")
        {
            line_ss >> token;
            if (token == "silent")
            {
                hpwhVerbosity = VRB_silent;
            }
            else if (token == "reluctant")
            {
                hpwhVerbosity = VRB_reluctant;
            }
            else if (token == "typical")
            {
                hpwhVerbosity = VRB_typical;
            }
            else if (token == "emetic")
            {
                hpwhVerbosity = VRB_emetic;
            }
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Incorrect verbosity on input.  \n");
                }
                return HPWH_ABORT;
            }
        }

        else if (token == "numHeatSources")
        {
            line_ss >> numHeatSources;
            heatSources.reserve(numHeatSources);
            for (int i = 0; i < numHeatSources; i++)
            {
                heatSources.emplace_back(this);
            }
        }
        else if (token == "heatsource")
        {
            if (numHeatSources == 0)
            {
                msg("You must specify the number of heatsources before setting their properties.  "
                    "\n");
                return HPWH_ABORT;
            }
            line_ss >> heatsource >> token;
            if (token == "isVIP")
            {
                line_ss >> tempString;
                if (tempString == "true")
                    heatSources[heatsource].isVIP = true;
                else if (tempString == "false")
                    heatSources[heatsource].isVIP = false;
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper value for %s for heat source %d\n",
                            token.c_str(),
                            heatsource);
                    }
                    return HPWH_ABORT;
                }
            }
            else if (token == "isOn")
            {
                line_ss >> tempString;
                if (tempString == "true")
                    heatSources[heatsource].isOn = true;
                else if (tempString == "false")
                    heatSources[heatsource].isOn = false;
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper value for %s for heat source %d\n",
                            token.c_str(),
                            heatsource);
                    }
                    return HPWH_ABORT;
                }
            }
            else if (token == "minT")
            {
                line_ss >> tempDouble >> units;
                if (units == "F")
                    tempDouble = F_TO_C(tempDouble);
                else if (units == "C")
                    ; // do nothing, lol
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Incorrect units specification for %s.  \n", token.c_str());
                    }
                    return HPWH_ABORT;
                }
                heatSources[heatsource].minT = tempDouble;
            }
            else if (token == "maxT")
            {
                line_ss >> tempDouble >> units;
                if (units == "F")
                    tempDouble = F_TO_C(tempDouble);
                else if (units == "C")
                    ; // do nothing, lol
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Incorrect units specification for %s.  \n", token.c_str());
                    }
                    return HPWH_ABORT;
                }
                heatSources[heatsource].maxT = tempDouble;
            }
            else if (token == "onlogic" || token == "offlogic" || token == "standbylogic")
            {
                line_ss >> tempString;
                if (tempString == "nodes")
                {
                    std::vector<int> nodeNums;
                    std::vector<double> weights;
                    std::string nextToken;
                    line_ss >> nextToken;
                    while (std::regex_match(nextToken, std::regex("\\d+")))
                    {
                        int nodeNum = std::stoi(nextToken);
                        if (nodeNum > LOGIC_NODE_SIZE + 1 || nodeNum < 0)
                        {
                            if (hpwhVerbosity >= VRB_reluctant)
                            {
                                msg("Node number for heatsource %d %s must be between 0 and %d.  "
                                    "\n",
                                    heatsource,
                                    token.c_str(),
                                    LOGIC_NODE_SIZE + 1);
                            }
                            return HPWH_ABORT;
                        }
                        nodeNums.push_back(nodeNum);
                        line_ss >> nextToken;
                    }
                    if (nextToken == "weights")
                    {
                        line_ss >> nextToken;
                        while (std::regex_match(nextToken, std::regex("-?\\d*\\.\\d+(?:e-?\\d+)?")))
                        {
                            weights.push_back(std::stod(nextToken));
                            line_ss >> nextToken;
                        }
                    }
                    else
                    {
                        for (auto n : nodeNums)
                        {
                            n += 0; // used to get rid of unused variable compiler warning
                            weights.push_back(1.0);
                        }
                    }
                    if (nodeNums.size() != weights.size())
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Number of weights for heatsource %d %s (%d) does not match number "
                                "of nodes for %s (%d).  \n",
                                heatsource,
                                token.c_str(),
                                weights.size(),
                                token.c_str(),
                                nodeNums.size());
                        }
                        return HPWH_ABORT;
                    }
                    if (nextToken != "absolute" && nextToken != "relative")
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Improper definition, \"%s\", for heat source %d %s. Should be "
                                "\"relative\" or \"absoute\".\n",
                                nextToken.c_str(),
                                heatsource,
                                token.c_str());
                        }
                        return HPWH_ABORT;
                    }
                    bool absolute = (nextToken == "absolute");
                    std::string compareStr;
                    line_ss >> compareStr >> tempDouble >> units;
                    std::function<bool(double, double)> compare;
                    if (compareStr == "<")
                        compare = std::less<double>();
                    else if (compareStr == ">")
                        compare = std::greater<double>();
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Improper comparison, \"%s\", for heat source %d %s. Should be "
                                "\"<\" or \">\".\n",
                                compareStr.c_str(),
                                heatsource,
                                token.c_str());
                        }
                        return HPWH_ABORT;
                    }
                    if (units == "F")
                    {
                        if (absolute)
                        {
                            tempDouble = F_TO_C(tempDouble);
                        }
                        else
                        {
                            tempDouble = dF_TO_dC(tempDouble);
                        }
                    }
                    else if (units == "C")
                        ; // do nothing, lol
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect units specification for %s from heatsource %d.  \n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    std::vector<NodeWeight> nodeWeights;
                    for (size_t i = 0; i < nodeNums.size(); i++)
                    {
                        nodeWeights.emplace_back(nodeNums[i], weights[i]);
                    }
                    std::shared_ptr<HPWH::TempBasedHeatingLogic> logic =
                        std::make_shared<HPWH::TempBasedHeatingLogic>(
                            "custom", nodeWeights, tempDouble, this, absolute, compare);
                    if (token == "onlogic")
                    {
                        heatSources[heatsource].addTurnOnLogic(logic);
                    }
                    else if (token == "offlogic")
                    {
                        heatSources[heatsource].addShutOffLogic(std::move(logic));
                    }
                    else
                    { // standby logic
                        heatSources[heatsource].standbyLogic =
                            std::make_shared<HPWH::TempBasedHeatingLogic>(
                                "standby logic", nodeWeights, tempDouble, this, absolute, compare);
                    }
                }
                else if (token == "onlogic")
                {
                    std::string nextToken;
                    line_ss >> nextToken;
                    bool absolute = (nextToken == "absolute");
                    if (absolute)
                    {
                        std::string compareStr;
                        line_ss >> compareStr >> tempDouble >> units;
                        std::function<bool(double, double)> compare;
                        if (compareStr == "<")
                            compare = std::less<double>();
                        else if (compareStr == ">")
                            compare = std::greater<double>();
                        else
                        {
                            if (hpwhVerbosity >= VRB_reluctant)
                            {
                                msg("Improper comparison, \"%s\", for heat source %d %s. Should be "
                                    "\"<\" or \">\".\n",
                                    compareStr.c_str(),
                                    heatsource,
                                    token.c_str());
                            }
                            return HPWH_ABORT;
                        }
                        line_ss >> tempDouble;
                    }
                    else
                    {
                        tempDouble = std::stod(nextToken);
                    }
                    line_ss >> units;
                    if (units == "F")
                    {
                        if (absolute)
                        {
                            tempDouble = F_TO_C(tempDouble);
                        }
                        else
                        {
                            tempDouble = dF_TO_dC(tempDouble);
                        }
                    }
                    else if (units == "C")
                        ; // do nothing, lol
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect units specification for %s from heatsource %d.  \n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    if (tempString == "wholeTank")
                    {
                        heatSources[heatsource].addTurnOnLogic(
                            HPWH::wholeTank(tempDouble, UNITS_C, absolute));
                    }
                    else if (tempString == "topThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::topThird(tempDouble));
                    }
                    else if (tempString == "bottomThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomThird(tempDouble));
                    }
                    else if (tempString == "standby")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::standby(tempDouble));
                    }
                    else if (tempString == "bottomSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomSixth(tempDouble));
                    }
                    else if (tempString == "secondSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::secondSixth(tempDouble));
                    }
                    else if (tempString == "thirdSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::thirdSixth(tempDouble));
                    }
                    else if (tempString == "fourthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::fourthSixth(tempDouble));
                    }
                    else if (tempString == "fifthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::fifthSixth(tempDouble));
                    }
                    else if (tempString == "topSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::topSixth(tempDouble));
                    }
                    else if (tempString == "bottomHalf")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomHalf(tempDouble));
                    }
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                        }
                        return HPWH_ABORT;
                    }
                }
                else if (token == "offlogic")
                {
                    line_ss >> tempDouble >> units;
                    if (units == "F")
                        tempDouble = F_TO_C(tempDouble);
                    else if (units == "C")
                        ; // do nothing, lol
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect units specification for %s from heatsource %d.  \n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    if (tempString == "topNodeMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::topNodeMaxTemp(tempDouble));
                    }
                    else if (tempString == "bottomNodeMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(
                            HPWH::bottomNodeMaxTemp(tempDouble));
                    }
                    else if (tempString == "bottomTwelfthMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(
                            HPWH::bottomTwelfthMaxTemp(tempDouble));
                    }
                    else if (tempString == "bottomSixthMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(
                            HPWH::bottomSixthMaxTemp(tempDouble));
                    }
                    else if (tempString == "largeDraw")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::largeDraw(tempDouble));
                    }
                    else if (tempString == "largerDraw")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::largerDraw(tempDouble));
                    }
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                        }
                        return HPWH_ABORT;
                    }
                }
            }
            else if (token == "type")
            {
                line_ss >> tempString;
                if (tempString == "resistor")
                {
                    heatSources[heatsource].typeOfHeatSource = TYPE_resistance;
                }
                else if (tempString == "compressor")
                {
                    heatSources[heatsource].typeOfHeatSource = TYPE_compressor;
                }
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                    }
                    return HPWH_ABORT;
                }
            }
            else if (token == "coilConfig")
            {
                line_ss >> tempString;
                if (tempString == "wrapped")
                {
                    heatSources[heatsource].configuration = HeatSource::CONFIG_WRAPPED;
                }
                else if (tempString == "submerged")
                {
                    heatSources[heatsource].configuration = HeatSource::CONFIG_SUBMERGED;
                }
                else if (tempString == "external")
                {
                    heatSources[heatsource].configuration = HeatSource::CONFIG_EXTERNAL;
                }
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                    }
                    return HPWH_ABORT;
                }
            }
            else if (token == "heatCycle")
            {
                line_ss >> tempString;
                if (tempString == "singlepass")
                {
                    heatSources[heatsource].isMultipass = false;
                }
                else if (tempString == "multipass")
                {
                    heatSources[heatsource].isMultipass = true;
                }
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                    }
                    return HPWH_ABORT;
                }
            }

            else if (token == "externalInlet")
            {
                line_ss >> tempInt;
                if (tempInt < num_nodes && tempInt >= 0)
                {
                    heatSources[heatsource].externalInletHeight = tempInt;
                }
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                    }
                    return HPWH_ABORT;
                }
            }
            else if (token == "externalOutlet")
            {
                line_ss >> tempInt;
                if (tempInt < num_nodes && tempInt >= 0)
                {
                    heatSources[heatsource].externalOutletHeight = tempInt;
                }
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Improper %s for heat source %d\n", token.c_str(), heatsource);
                    }
                    return HPWH_ABORT;
                }
            }

            else if (token == "condensity")
            {
                double x;
                std::vector<double> condensity;
                while (line_ss >> x)
                    condensity.push_back(x);
                heatSources[heatsource].setCondensity(condensity);
            }
            else if (token == "nTemps")
            {
                line_ss >> nTemps;
                heatSources[heatsource].perfMap.resize(nTemps);
            }
            else if (std::regex_match(token, std::regex("T\\d+")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("T(\\d+)"));
                nTemps = std::stoi(match[1].str());
                int maxTemps = (int)heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("%s specified for heatsource %d before definition of nTemps.  \n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect specification for %s from heatsource %d. nTemps, %d, is "
                                "less than %d.  \n",
                                token.c_str(),
                                heatsource,
                                maxTemps,
                                nTemps);
                        }
                        return HPWH_ABORT;
                    }
                }
                line_ss >> tempDouble >> units;
                //        if (units == "F")  tempDouble = F_TO_C(tempDouble);
                if (units == "F")
                    ;
                //        else if (units == "C") ; //do nothing, lol
                else if (units == "C")
                    tempDouble = C_TO_F(tempDouble);
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Incorrect units specification for %s from heatsource %d.  \n",
                            token.c_str(),
                            heatsource);
                    }
                    return HPWH_ABORT;
                }
                heatSources[heatsource].perfMap[nTemps - 1].T_F = tempDouble;
            }
            else if (std::regex_match(token, std::regex("(?:inPow|cop)T\\d+(?:const|lin|quad)")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("(inPow|cop)T(\\d+)(const|lin|quad)"));
                string var = match[1].str();
                nTemps = std::stoi(match[2].str());
                string coeff = match[3].str();
                int coeff_num;
                if (coeff == "const")
                {
                    coeff_num = 0;
                }
                else if (coeff == "lin")
                {
                    coeff_num = 1;
                }
                else if (coeff == "quad")
                {
                    coeff_num = 2;
                }

                int maxTemps = (int)heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("%s specified for heatsource %d before definition of nTemps.  \n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect specification for %s from heatsource %d. nTemps, %d, is "
                                "less than %d.  \n",
                                token.c_str(),
                                heatsource,
                                maxTemps,
                                nTemps);
                        }
                        return HPWH_ABORT;
                    }
                }
                line_ss >> tempDouble;

                if (var == "inPow")
                {
                    heatSources[heatsource].perfMap[nTemps - 1].inputPower_coeffs.push_back(
                        tempDouble);
                }
                else if (var == "cop")
                {
                    heatSources[heatsource].perfMap[nTemps - 1].COP_coeffs.push_back(tempDouble);
                }
            }
            else if (token == "hysteresis")
            {
                line_ss >> tempDouble >> units;
                if (units == "F")
                    tempDouble = dF_TO_dC(tempDouble);
                else if (units == "C")
                    ; // do nothing, lol
                else
                {
                    if (hpwhVerbosity >= VRB_reluctant)
                    {
                        msg("Incorrect units specification for %s from heatsource %d.  \n",
                            token.c_str(),
                            heatsource);
                    }
                    return HPWH_ABORT;
                }
                heatSources[heatsource].hysteresis_dC = tempDouble;
            }
            else if (token == "backupSource")
            {
                line_ss >> sourceNum;
                heatSources[heatsource].backupHeatSource = &heatSources[sourceNum];
            }
            else if (token == "companionSource")
            {
                line_ss >> sourceNum;
                heatSources[heatsource].companionHeatSource = &heatSources[sourceNum];
            }
            else if (token == "followedBySource")
            {
                line_ss >> sourceNum;
                heatSources[heatsource].followedByHeatSource = &heatSources[sourceNum];
            }
            else
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Improper specifier (%s) for heat source %d\n", token.c_str(), heatsource);
                }
            }

        } // end heatsource options
        else
        {
            msg("Improper keyword: %s  \n", token.c_str());
            return HPWH_ABORT;
        }

    } // end while over lines

    // take care of the non-input processing
    hpwhModel = MODELS_CustomFile;

    tankTemps_C.resize(num_nodes);

    if (hasInitialTankTemp)
        setTankToTemperature(initalTankT_C);
    else
        resetTankToSetpoint();

    nextTankTemps_C.resize(num_nodes);

    isHeating = false;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        if (heatSources[i].isOn)
        {
            isHeating = true;
        }
        heatSources[i].sortPerformanceMap();
    }

    calcDerivedValues();

    if (checkInputs() == HPWH_ABORT)
    {
        return HPWH_ABORT;
    }
    simHasFailed = false;
    return 0;
}
#endif

//-----------------------------------------------------------------------------
///	@brief	Reads a named schedule
/// @param[out]	schedule			schedule values
///	@param[in]	scheduleName		name of schedule to read
///	@param[in]	testLength_min		length of test (min)
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::readSchedule(HPWH::Schedule& schedule, std::string scheduleName, long testLength_min)
{
    int minuteHrTmp;
    bool hourInput;
    std::string line, snippet, s, minORhr;
    double valTmp;

    std::ifstream inputFile(scheduleName.c_str());

    if (hpwhVerbosity >= VRB_reluctant)
    {
        msg("Opening %s\n", scheduleName.c_str());
    }

    if (!inputFile.is_open())
    {
        return false;
    }

    inputFile >> snippet >> valTmp;

    if (snippet != "default")
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("First line of %s must specify default\n", scheduleName.c_str());
        }
        return false;
    }

    // Fill with the default value
    schedule.assign(testLength_min, valTmp);

    // Burn the first two lines
    std::getline(inputFile, line);
    std::getline(inputFile, line);

    std::stringstream ss(line); // Will parse with a stringstream
    // Grab the first token, which is the minute or hour marker
    ss >> minORhr;
    if (minORhr.empty())
    { // If nothing left in the file
        return true;
    }
    hourInput = tolower(minORhr.at(0)) == 'h';
    char c; // for commas
    while (inputFile >> minuteHrTmp >> c >> valTmp)
    {
        if (minuteHrTmp >= static_cast<int>(schedule.size()))
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Input file for %s has more minutes than test definition\n",
                    scheduleName.c_str());
            }
            return false;
        }
        // update the value
        if (!hourInput)
        {
            schedule[minuteHrTmp] = valTmp;
        }
        else if (hourInput)
        {
            for (int j = minuteHrTmp * 60; j < (minuteHrTmp + 1) * 60; j++)
            {
                schedule[j] = valTmp;
            }
        }
    }

    inputFile.close();
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Reads the control info for a test from file "testInfo.txt"
///	@param[in]	testDirectory	path to directory containing test files
///	@param[out]	controlInfo		data structure containing control info
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::readControlInfo(const std::string& testDirectory, HPWH::ControlInfo& controlInfo)
{
    std::ifstream controlFile;
    std::string fileToOpen = testDirectory + "/" + "testInfo.txt";
    // Read the test control file
    controlFile.open(fileToOpen.c_str());
    if (!controlFile.is_open())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Could not open control file %s\n", fileToOpen.c_str());
        }
        return false;
    }

    controlInfo.outputCode = 0;
    controlInfo.timeToRun_min = 0;
    controlInfo.setpointT_C = 0.;
    controlInfo.initialTankT_C = nullptr;
    controlInfo.doConduction = true;
    controlInfo.doInversionMixing = true;
    controlInfo.inletH = nullptr;
    controlInfo.tankSize_gal = nullptr;
    controlInfo.tot_limit = nullptr;
    controlInfo.useSoC = false;
    controlInfo.temperatureUnits = "C";
    controlInfo.recordMinuteData = false;
    controlInfo.recordYearData = false;
    controlInfo.modifyDraw = false;

    std::string token;
    std::string sValue;
    while (controlFile >> token >> sValue)
    {
        if (token == "setpoint")
        { // If a setpoint was specified then override the default
            controlInfo.setpointT_C = std::stod(sValue);
        }
        else if (token == "length_of_test")
        {
            controlInfo.timeToRun_min = std::stoi(sValue);
        }
        else if (token == "doInversionMixing")
        {
            controlInfo.doInversionMixing = getBool(sValue);
        }
        else if (token == "doConduction")
        {
            controlInfo.doConduction = getBool(sValue);
        }
        else if (token == "inletH")
        {
            controlInfo.inletH = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "tanksize")
        {
            controlInfo.tankSize_gal = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "tot_limit")
        {
            controlInfo.tot_limit = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "useSoC")
        {
            controlInfo.useSoC = getBool(sValue);
        }
        else if (token == "initialTankT_C")
        { // Initialize at this temperature instead of setpoint
            controlInfo.initialTankT_C = std::unique_ptr<double>(new double(std::stod(sValue)));
        }
        else if (token == "temperature_units")
        { // Initialize at this temperature instead of setpoint
            controlInfo.temperatureUnits = sValue;
        }
        else
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Unrecognized key in %s\n", token.c_str());
            }
        }
    }
    controlFile.close();

    if (controlInfo.temperatureUnits == "F")
    {
        controlInfo.setpointT_C = F_TO_C(controlInfo.setpointT_C);
        if (controlInfo.initialTankT_C != nullptr)
            *controlInfo.initialTankT_C = F_TO_C(*controlInfo.initialTankT_C);
    }

    if (controlInfo.timeToRun_min == 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Error: record length_of_test not found in testInfo.txt file\n");
        }
        return false;
    }

    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Reads the schedules for a test
///	@param[in]	testDirectory	path to directory containing test files
///	@param[in]	controlInfo		data structure containing control info
///	@param[out]	allSchedules	collection of test schedules read
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::readSchedules(const std::string& testDirectory,
                         const HPWH::ControlInfo& controlInfo,
                         std::vector<HPWH::Schedule>& allSchedules)
{
    std::vector<std::string> scheduleNames;
    scheduleNames.push_back("inletT");
    scheduleNames.push_back("draw");
    scheduleNames.push_back("ambientT");
    scheduleNames.push_back("evaporatorT");
    scheduleNames.push_back("DR");
    scheduleNames.push_back("setpoint");
    scheduleNames.push_back("SoC");

    long outputCode = controlInfo.outputCode;
    allSchedules.reserve(scheduleNames.size());
    for (auto i = 0; i < scheduleNames.size(); i++)
    {
        std::string fileToOpen = testDirectory + "/" + scheduleNames[i] + "schedule.csv";
        Schedule schedule;
        if (!readSchedule(schedule, fileToOpen, controlInfo.timeToRun_min))
        {
            if (scheduleNames[i] != "setpoint" && scheduleNames[i] != "SoC")
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("readSchedule returns an error on schedule %s\n", scheduleNames[i].c_str());
                }
                return false;
            }
        }
        allSchedules.push_back(schedule);
    }

    if (controlInfo.temperatureUnits == "F")
    {
        for (auto& T : allSchedules[0])
        {
            T = F_TO_C(T);
        }
        for (auto& T : allSchedules[2])
        {
            T = F_TO_C(T);
        }
        for (auto& T : allSchedules[3])
        {
            T = F_TO_C(T);
        }
        for (auto& T : allSchedules[5])
        {
            T = F_TO_C(T);
        }
    }

    setDoInversionMixing(controlInfo.doInversionMixing);
    setDoConduction(controlInfo.doConduction);

    if (controlInfo.setpointT_C > 0.)
    {
        if (!allSchedules[5].empty())
        {
            setSetpoint(allSchedules[5][0]); // expect this to fail sometimes
            if (controlInfo.initialTankT_C != nullptr)
                setTankToTemperature(*controlInfo.initialTankT_C);
            else
                resetTankToSetpoint();
        }
        else
        {
            setSetpoint(controlInfo.setpointT_C);
            if (controlInfo.initialTankT_C != nullptr)
                setTankToTemperature(*controlInfo.initialTankT_C);
            else
                resetTankToSetpoint();
        }
    }

    if (controlInfo.inletH != nullptr)
    {
        outputCode += setInletByFraction(*controlInfo.inletH);
    }
    if (controlInfo.tankSize_gal != nullptr)
    {
        outputCode += setTankSize(*controlInfo.tankSize_gal, HPWH::UNITS_GAL);
    }
    if (controlInfo.tot_limit != nullptr)
    {
        outputCode += setTimerLimitTOT(*controlInfo.tot_limit);
    }
    if (controlInfo.useSoC)
    {
        if (allSchedules[6].empty())
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("If useSoC is true need an SoCschedule.csv file. \n");
            }
        }
        const double soCMinTUse_C = F_TO_C(110.);
        const double soCMains_C = F_TO_C(65.);
        outputCode += switchToSoCControls(1., .05, soCMinTUse_C, true, soCMains_C);
    }

    if (outputCode != 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Control file testInfo.txt has unsettable specifics in it. \n");
        }
        return false;
    }
    return true;
}

//-----------------------------------------------------------------------------
///	@brief	Runs a simulation
///	@param[in]	testDesc		data structure containing test description
/// @param[in]	outputDirectory	destination path for test results (filename based on testDesc)
///	@param[in]	controlInfo		data structure containing control info
///	@param[in]	allSchedules	collection of test schedules
///	@param[in]	airT_C			air temperature (degC) used for temperature depression
///	@param[in]	doTempDepress	whether to apply temperature depression
///	@param[out]	testResults		data structure containing test results
/// @return		true if successful, false otherwise
//-----------------------------------------------------------------------------
bool HPWH::runSimulation(const TestDesc& testDesc,
                         const std::string& outputDirectory,
                         const HPWH::ControlInfo& controlInfo,
                         std::vector<HPWH::Schedule>& allSchedules,
                         double airT_C,
                         const bool doTempDepress,
                         TestResults& testResults)
{
    const double energyBalThreshold = 0.0001; // 0.1 %
    const int nTestTCouples = 6;

    // UEF data
    testResults = {false, 0., 0.};

    FILE* yearOutputFile = NULL;
    if (controlInfo.recordYearData)
    {
        std::string sOutputFilename = outputDirectory + "/DHW_YRLY.csv";
        if (fopen_s(&yearOutputFile, sOutputFilename.c_str(), "a+") != 0)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Could not open output file \n", sOutputFilename.c_str());
            }
            return false;
        }
    }

    FILE* minuteOutputFile = NULL;
    if (controlInfo.recordMinuteData)
    {
        std::string sOutputFilename = outputDirectory + "/" + testDesc.testName + "_" +
                                      testDesc.presetOrFile + "_" + testDesc.modelName + ".csv";
        if (fopen_s(&minuteOutputFile, sOutputFilename.c_str(), "w+") != 0)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Could not open output file \n", sOutputFilename.c_str());
            }
            return false;
        }
    }

    if (controlInfo.recordMinuteData)
    {
        string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";
        if (isCompressoExternalMultipass())
        {
            sHeader += "condenserInletT,condenserOutletT,externalVolGPM,";
        }
        if (controlInfo.useSoC)
        {
            sHeader += "targetSoCFract,soCFract,";
        }
        WriteCSVHeading(minuteOutputFile, sHeader.c_str(), nTestTCouples, CSVOPT_NONE);
    }

    double cumulativeEnergyIn_kWh[3] = {0., 0., 0.};
    double cumulativeEnergyOut_kWh[3] = {0., 0., 0.};

    bool doChangeSetpoint = (!allSchedules[5].empty()) && (!isSetpointFixed());

    // ------------------------------------- Simulate --------------------------------------- //
    if (hpwhVerbosity >= VRB_reluctant)
    {
        msg("Now Simulating %d Minutes of the Test\n", controlInfo.timeToRun_min);
    }

    // run simulation in 1-min steps
    for (int i = 0; i < controlInfo.timeToRun_min; i++)
    {

        double inletT_C = allSchedules[0][i];
        double drawVolume_L = GAL_TO_L(allSchedules[1][i]);
        double ambientT_C = doTempDepress ? airT_C : allSchedules[2][i];
        double externalT_C = allSchedules[3][i];
        HPWH::DRMODES drStatus = static_cast<HPWH::DRMODES>(int(allSchedules[4][i]));

        // change setpoint if specified in schedule
        if (doChangeSetpoint)
        {
            setSetpoint(allSchedules[5][i]); // expect this to fail sometimes
        }

        // change SoC schedule
        if (controlInfo.useSoC)
        {
            if (setTargetSoCFraction(allSchedules[6][i]) != 0)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("ERROR: Can not set the target state of charge fraction.\n");
                }
                return false;
            }
        }

        if (controlInfo.modifyDraw)
        {
            const double mixT_C = F_TO_C(125.);
            if (getSetpoint() <= mixT_C)
            { // do a simple mix down of the draw for the cold-water temperature
                drawVolume_L *= (mixT_C - inletT_C) /
                                (getTankNodeTemp(getNumNodes() - 1, HPWH::UNITS_C) - inletT_C);
            }
        }

        double inletT2_C = inletT_C;
        double drawVolume2_L = drawVolume_L;

        double previousTankHeatContent_kJ = getTankHeatContent_kJ();

        // run a step
        int runResult = runOneStep(inletT_C,     // inlet water temperature (C)
                                   drawVolume_L, // draw volume (L)
                                   ambientT_C,   // ambient Temp (C)
                                   externalT_C,  // external Temp (C)
                                   drStatus,     // DDR Status (now an enum. Fixed for now as allow)
                                   drawVolume2_L, // inlet-2 volume (L)
                                   inletT2_C,     // inlet-2 Temp (C)
                                   NULL);         // no extra heat

        if (runResult != 0)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("ERROR: Run failed.\n");
            }
            return false;
        }

        if (!isEnergyBalanced(
                drawVolume_L, inletT_C, previousTankHeatContent_kJ, energyBalThreshold))
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("WARNING: On minute %i, HPWH has an energy balance error.\n", i);
            }
            // return false; // fails on ModelTest.testREGoesTo93C.TamScalable_SP.Preset; issue in
            // addHeatExternal
        }

        // check timing
        for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
        {
            if (getNthHeatSourceRunTime(iHS) > 1.)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("ERROR: On minute %i, heat source %i ran for %i min.\n",
                        iHS,
                        getNthHeatSourceRunTime(iHS));
                }
                return false;
            }
        }

        // check flow for external MP
        if (isCompressoExternalMultipass())
        {
            double volumeHeated_gal = getExternalVolumeHeated(HPWH::UNITS_GAL);
            double mpFlowVolume_gal = getExternalMPFlowRate(HPWH::UNITS_GPM) *
                                      getNthHeatSourceRunTime(getCompressorIndex());
            if (fabs(volumeHeated_gal - mpFlowVolume_gal) > 0.000001)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("ERROR: Externally heated volumes are inconsistent! Volume Heated [gal]: "
                        "%d, mpFlowRate in 1 min [gal] %d\n",
                        volumeHeated_gal,
                        mpFlowVolume_gal);
                }
                return false;
            }
        }

        // location temperature reported with temperature depression, instead of ambient temperature
        if (doTempDepress)
        {
            ambientT_C = getLocationTemp_C();
        }

        // write minute summary
        if (controlInfo.recordMinuteData)
        {
            std::string sPreamble = std::to_string(i) + ", " + std::to_string(ambientT_C) + ", " +
                                    std::to_string(getSetpoint()) + ", " +
                                    std::to_string(allSchedules[0][i]) + ", " +
                                    std::to_string(allSchedules[1][i]) + ", ";
            // Add some more outputs for mp tests
            if (isCompressoExternalMultipass())
            {
                sPreamble += std::to_string(getCondenserWaterInletTemp()) + ", " +
                             std::to_string(getCondenserWaterOutletTemp()) + ", " +
                             std::to_string(getExternalVolumeHeated(HPWH::UNITS_GAL)) + ", ";
            }
            if (controlInfo.useSoC)
            {
                sPreamble += std::to_string(allSchedules[6][i]) + ", " +
                             std::to_string(getSoCFraction()) + ", ";
            }
            int csvOptions = CSVOPT_NONE;
            if (allSchedules[1][i] > 0.)
            {
                csvOptions |= CSVOPT_IS_DRAWING;
            }
            WriteCSVRow(minuteOutputFile, sPreamble.c_str(), nTestTCouples, csvOptions);
        }

        // accumulate energy and draw
        double energyIn_kJ = 0.;
        for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
        {
            double heatSourceEnergyIn_kJ = getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KJ);
            energyIn_kJ += heatSourceEnergyIn_kJ;

            cumulativeEnergyIn_kWh[iHS] += KJ_TO_KWH(heatSourceEnergyIn_kJ) * 1000.;
            cumulativeEnergyOut_kWh[iHS] +=
                getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH) * 1000.;
        }
        testResults.totalEnergyConsumed_kJ += energyIn_kJ;
        testResults.totalVolumeRemoved_L += drawVolume_L;
    }
    // -------------------------------------Simulation complete
    // --------------------------------------- //

    if (controlInfo.recordMinuteData)
    {
        fclose(minuteOutputFile);
    }

    // write year summary
    if (controlInfo.recordYearData)
    {
        std::string firstCol =
            testDesc.testName + "," + testDesc.presetOrFile + "," + testDesc.modelName;
        fprintf(yearOutputFile, "%s", firstCol.c_str());
        double totalEnergyIn_kWh = 0, totalEnergyOut_kWh = 0;
        for (int iHS = 0; iHS < 3; iHS++)
        {
            fprintf(yearOutputFile,
                    ",%0.0f,%0.0f",
                    cumulativeEnergyIn_kWh[iHS],
                    cumulativeEnergyOut_kWh[iHS]);
            totalEnergyIn_kWh += cumulativeEnergyIn_kWh[iHS];
            totalEnergyOut_kWh += cumulativeEnergyOut_kWh[iHS];
        }
        fprintf(yearOutputFile, ",%0.0f,%0.0f", totalEnergyIn_kWh, totalEnergyOut_kWh);
        for (int iHS = 0; iHS < 3; iHS++)
        {
            fprintf(yearOutputFile,
                    ",%0.2f",
                    cumulativeEnergyOut_kWh[iHS] / cumulativeEnergyIn_kWh[iHS]);
        }
        fprintf(yearOutputFile, ",%0.2f", totalEnergyOut_kWh / totalEnergyIn_kWh);
        fprintf(yearOutputFile, "\n");
        fclose(yearOutputFile);
    }

    testResults.passed = true;
    return true;
}
