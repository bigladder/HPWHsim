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
#include "HPWHFitter.hh"
#include <btwxt/btwxt.h>
#include <fmt/format.h>

#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>
#include <queue>

using std::cout;
using std::endl;
using std::string;

const double HPWH::DENSITYWATER_kgperL = 0.995; /// mass density of water
const double HPWH::KWATER_WpermC = 0.62;        /// thermal conductivity of water
const double HPWH::CPWATER_kJperkgC = 4.180;    /// specific heat capcity of water

const double HPWH::TOL_MINVALUE = 0.0001;
const float HPWH::UNINITIALIZED_LOCATIONTEMP = -500.f;
const float HPWH::ASPECTRATIO = 4.75f;

const double HPWH::MAXOUTLET_R134A = F_TO_C(160.);
const double HPWH::MAXOUTLET_R410A = F_TO_C(140.);
const double HPWH::MAXOUTLET_R744 = F_TO_C(190.);
const double HPWH::MINSINGLEPASSLIFT = dF_TO_dC(15.);

std::unordered_map<HPWH::FirstHourRating::Desig, std::size_t> HPWH::firstDrawClusterSizes = {
    {HPWH::FirstHourRating::Desig::VerySmall, 5},
    {HPWH::FirstHourRating::Desig::Low, 3},
    {HPWH::FirstHourRating::Desig::Medium, 3},
    {HPWH::FirstHourRating::Desig::High, 4}};

std::unordered_map<HPWH::FirstHourRating::Desig, HPWH::DrawPattern> HPWH::drawPatterns = {
    {HPWH::FirstHourRating::Desig::VerySmall,
     {{HM_TO_MIN(0, 00), 7.6, 3.8},
      {HM_TO_MIN(1, 00), 3.8, 3.8},
      {HM_TO_MIN(1, 05), 1.9, 3.8},
      {HM_TO_MIN(1, 10), 1.9, 3.8},
      {HM_TO_MIN(1, 15), 1.9, 3.8},
      {HM_TO_MIN(8, 00), 3.8, 3.8},
      {HM_TO_MIN(8, 15), 7.6, 3.8},
      {HM_TO_MIN(9, 00), 5.7, 3.8},
      {HM_TO_MIN(9, 15), 3.8, 3.8}}},

    {HPWH::FirstHourRating::Desig::Low,
     {{HM_TO_MIN(0, 00), 56.8, 6.4},
      {HM_TO_MIN(0, 30), 7.6, 3.8},
      {HM_TO_MIN(1, 00), 3.8, 3.8},
      {HM_TO_MIN(10, 30), 22.7, 6.4},
      {HM_TO_MIN(11, 30), 15.1, 6.4},
      {HM_TO_MIN(12, 00), 3.8, 3.8},
      {HM_TO_MIN(12, 45), 3.8, 3.8},
      {HM_TO_MIN(12, 50), 3.8, 3.8},
      {HM_TO_MIN(16, 15), 7.6, 3.8},
      {HM_TO_MIN(16, 45), 7.6, 6.4},
      {HM_TO_MIN(17, 00), 11.4, 6.4}}},

    {HPWH::FirstHourRating::Desig::Medium,
     {{HM_TO_MIN(0, 00), 56.8, 6.4},
      {HM_TO_MIN(0, 30), 7.6, 3.8},
      {HM_TO_MIN(1, 40), 34.1, 6.4},
      {HM_TO_MIN(10, 30), 34.1, 6.4},
      {HM_TO_MIN(11, 30), 18.9, 6.4},
      {HM_TO_MIN(12, 00), 3.8, 3.8},
      {HM_TO_MIN(12, 45), 3.8, 3.8},
      {HM_TO_MIN(12, 50), 3.8, 3.8},
      {HM_TO_MIN(16, 00), 3.8, 3.8},
      {HM_TO_MIN(16, 15), 7.6, 3.8},
      {HM_TO_MIN(16, 45), 7.6, 6.4},
      {HM_TO_MIN(17, 00), 26.5, 6.4}}},

    {HPWH::FirstHourRating::Desig::High,
     {{HM_TO_MIN(0, 00), 102, 11.4},
      {HM_TO_MIN(0, 30), 7.6, 3.8},
      {HM_TO_MIN(0, 40), 3.8, 3.8},
      {HM_TO_MIN(1, 40), 34.1, 6.4},
      {HM_TO_MIN(10, 30), 56.8, 11.4},
      {HM_TO_MIN(11, 30), 18.9, 6.4},
      {HM_TO_MIN(12, 00), 3.8, 3.8},
      {HM_TO_MIN(12, 45), 3.8, 3.8},
      {HM_TO_MIN(12, 50), 3.8, 3.8},
      {HM_TO_MIN(16, 00), 7.6, 3.8},
      {HM_TO_MIN(16, 15), 7.6, 3.8},
      {HM_TO_MIN(16, 30), 7.6, 6.4},
      {HM_TO_MIN(16, 45), 7.6, 6.4},
      {HM_TO_MIN(17, 00), 53.0, 11.4}}}};

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

//-----------------------------------------------------------------------------
///	@brief	Scales all values of a std::vector<double> by a common factor.
/// @param[in/out]	coeffs		values to be scaled
/// @param[in]	scaleFactor 	scaling factor
//-----------------------------------------------------------------------------
void scaleVector(std::vector<double>& coeffs, const double scaleFactor)
{
    if (scaleFactor != 1.)
    {
        std::transform(coeffs.begin(),
                       coeffs.end(),
                       coeffs.begin(),
                       std::bind(std::multiplies<double>(), std::placeholders::_1, scaleFactor));
    }
}

void HPWH::setMinutesPerStep(const double minutesPerStep_in)
{
    minutesPerStep = minutesPerStep_in;
    secondsPerStep = sec_per_min * minutesPerStep;
    hoursPerStep = minutesPerStep / min_per_hr;
}

// public HPWH functions
HPWH::HPWH(const std::shared_ptr<Courier::Courier>& courier, const std::string& name_in /*"hpwh"*/)
    : Sender("HPWH", name_in, courier)
{
    setAllDefaults();
}

void HPWH::setAllDefaults()
{
    tankTemps_C.clear();
    nextTankTemps_C.clear();
    heatSources.clear();

    isHeating = false;
    setpointFixed = false;
    tankSizeFixed = true;
    canScale = false;
    member_inletT_C = -1.; // invalid unit setInletT called
    haveInletT = false;
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

HPWH::HPWH(const HPWH& hpwh) : Sender("HPWH", "", hpwh.courier) { *this = hpwh; }

HPWH& HPWH::operator=(const HPWH& hpwh)
{
    if (this == &hpwh)
    {
        return *this;
    }

    Sender::operator=(hpwh);
    isHeating = hpwh.isHeating;

    heatSources = hpwh.heatSources;
    for (auto& heatSource : heatSources)
    {
        heatSource.hpwh = this;
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

HPWH::HeatSource* HPWH::addHeatSource(const std::string& name_in)
{
    heatSources.emplace_back(name_in, this, get_courier());
    return &heatSources.back();
}

void HPWH::runOneStep(double drawVolume_L,
                      double tankAmbientT_C,
                      double heatSourceAmbientT_C,
                      DRMODES DRstatus,
                      double inletVol2_L,
                      double inletT2_C,
                      std::vector<double>* extraHeatDist_W)
{

    // check for errors
    if (doTempDepression && (minutesPerStep != 1))
    {
        send_error("minutesPerStep must equal one for temperature depression to work.");
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
        }

        // do HeatSource choice
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (isHeating)
            {
                // check if anything that is on needs to turn off (generally for lowT cutoffs)
                // things that just turn on later this step are checked for this in shouldHeat
                if (heatSources[i].isEngaged() && heatSources[i].shutsOff())
                {
                    heatSources[i].disengageHeatSource();
                    // check if the backup heat source would have to shut off too
                    if ((heatSources[i].backupHeatSource != NULL) &&
                        !heatSources[i].backupHeatSource->shutsOff())
                    {
                        // and if not, go ahead and turn it on
                        heatSources[i].backupHeatSource->engageHeatSource(DRstatus);
                    }
                }

                // if there's a priority HeatSource (e.g. upper resistor) and it needs to
                // come on, then turn  off and start it up
                if (heatSources[i].isVIP)
                {
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

        // do heating logic
        double minutesToRun = minutesPerStep;
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            // check/apply lock-outs
            if (shouldDRLockOut(heatSources[i].typeOfHeatSource, DRstatus))
            {
                heatSources[i].lockOutHeatSource();
            }
            else
            {
                // locks or unlocks the heat source
                heatSources[i].toLockOrUnlock(heatSourceAmbientT_C);
            }
            if (heatSources[i].isLockedOut() && heatSources[i].backupHeatSource == NULL)
            {
                heatSources[i].disengageHeatSource();
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
                    // subtract time it ran and turn it off
                    minutesToRun -= heatSourcePtr->runtime_min;
                    heatSources[i].disengageHeatSource();
                    // and if there's a heat source that follows this heat source (regardless of
                    // lockout) that's able to come on,
                    if ((heatSources[i].followedByHeatSource != NULL) &&
                        !heatSources[i].followedByHeatSource->shutsOff())
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
                        // heat or already heated on its own.
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
    if (areAllHeatSourcesOff())
    {
        isHeating = false;
    }
    // If there's extra user defined heat to add -> Add extra heat!
    if ((extraHeatDist_W != NULL) && (extraHeatDist_W->size() != 0))
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

#if !NDEBUG
    // cursory check for inverted temperature profile
    if (tankTemps_C[getNumNodes() - 1] < tankTemps_C[0])
    {
        send_debug("The top of the tank is cooler than the bottom.");
    }
#endif

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
} // end runOneStep

void HPWH::runNSteps(int N,
                     double* inletT_C,
                     double* drawVolume_L,
                     double* tankAmbientT_C,
                     double* heatSourceAmbientT_C,
                     DRMODES* DRstatus)
{
    // these are all the accumulating variables we'll need
    double energyRemovedFromEnvironment_kWh_SUM = 0;
    double standbyLosses_kWh_SUM = 0;
    double outletTemp_C_AVG = 0;
    double totalDrawVolume_L = 0;
    std::vector<double> heatSources_runTimes_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyInputs_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyOutputs_SUM(getNumHeatSources());

    // run the sim one step at a time, accumulating the outputs as you go
    for (int i = 0; i < N; i++)
    {

        runOneStep(
            inletT_C[i], drawVolume_L[i], tankAmbientT_C[i], heatSourceAmbientT_C[i], DRstatus[i]);

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
    }
    // finish weighted avg. of outlet temp by dividing by the total drawn volume
    outletTemp_C_AVG /= totalDrawVolume_L;

    // now, reassign all accumulated values to their original spots
    energyRemovedFromEnvironment_kWh = energyRemovedFromEnvironment_kWh_SUM;
    standbyLosses_kWh = standbyLosses_kWh_SUM;
    outletTemp_C = outletTemp_C_AVG;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = heatSources_runTimes_SUM[i];
        heatSources[i].energyInput_kWh = heatSources_energyInputs_SUM[i];
        heatSources[i].energyOutput_kWh = heatSources_energyOutputs_SUM[i];
    }
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

// public members to write to CSV file
int HPWH::WriteCSVHeading(std::ofstream& outFILE,
                          const char* preamble,
                          int nTCouples,
                          int options) const
{

    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    outFILE << preamble;

    outFILE << "DRstatus";

    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",h_src{}In (Wh),h_src{}Out (Wh)", iHS + 1, iHS + 1);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        outFILE << fmt::format(",tcouple{} ({})", iTC + 1, doIP ? "F" : "C");
    }

    outFILE << fmt::format(",toutlet ({})", doIP ? "F" : "C") << std::endl;

    return 0;
}

int HPWH::WriteCSVRow(std::ofstream& outFILE,
                      const char* preamble,
                      int nTCouples,
                      int options) const
{

    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    outFILE << preamble;

    outFILE << prevDRstatus;

    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",{:0.2f},{:0.2f}",
                               getNthHeatSourceEnergyInput(iHS, UNITS_KWH) * 1000.,
                               getNthHeatSourceEnergyOutput(iHS, UNITS_KWH) * 1000.);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        outFILE << fmt::format(",{:0.2f}",
                               getNthSimTcouple(iTC + 1, nTCouples, doIP ? UNITS_F : UNITS_C));
    }

    if (options & HPWH::CSVOPT_IS_DRAWING)
    {
        outFILE << fmt::format(",{:0.2f}", doIP ? C_TO_F(outletTemp_C) : outletTemp_C);
    }
    else
    {
        outFILE << ",";
    }

    outFILE << std::endl;

    return 0;
}

int HPWH::writeRowAsCSV(std::ofstream& outFILE,
                        OutputData& outputData,
                        const CSVOPTIONS& options /* = CSVOPTIONS::CSVOPT_NONE */) const
{
    bool doIP = (options & CSVOPT_IPUNITS) != 0;

    //
    outFILE << fmt::format("{:d}", outputData.time_min);
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? C_TO_F(outputData.ambientT_C) : outputData.ambientT_C);
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? C_TO_F(outputData.setpointT_C) : outputData.setpointT_C);
    outFILE << fmt::format(",{:0.2f}", doIP ? C_TO_F(outputData.inletT_C) : outputData.inletT_C);
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? L_TO_GAL(outputData.drawVolume_L) : outputData.drawVolume_L);
    outFILE << fmt::format(",{}", static_cast<int>(outputData.drMode));

    //
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",{:0.2f},{:0.2f}",
                               outputData.h_srcIn_kWh[iHS] * 1000.,
                               outputData.h_srcOut_kWh[iHS] * 1000.);
    }

    //
    for (auto thermocoupleT_C : outputData.thermocoupleT_C)
    {
        outFILE << fmt::format(",{:0.2f}", doIP ? C_TO_F(thermocoupleT_C) : thermocoupleT_C);
    }

    //
    if (outputData.drawVolume_L > 0.)
    {
        outFILE << fmt::format(",{:0.2f}",
                               doIP ? C_TO_F(outputData.outletT_C) : outputData.outletT_C);
    }
    else
    {
        outFILE << ",";
    }

    outFILE << std::endl;
    return 0;
}

bool HPWH::isSetpointFixed() const { return setpointFixed; }

void HPWH::setSetpoint(double newSetpoint, UNITS units /*=UNITS_C*/)
{
    double newSetpoint_C = newSetpoint;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        newSetpoint_C = F_TO_C(newSetpoint);
        break;
    default:
        send_error("Invalid units.");
    }

    double maxAllowedSetpointT_C;
    string why;
    if (isNewSetpointPossible(newSetpoint_C, maxAllowedSetpointT_C, why))
    {
        setpoint_C = newSetpoint_C;
    }
    else
    {
        send_error(fmt::format("Cannot set this setpoint for the currently selected model, "
                               "max setpoint is {:0.2f} C. {}",
                               maxAllowedSetpointT_C,
                               why.c_str()));
    }
}

double HPWH::getSetpoint(UNITS units /*=UNITS_C*/) const
{
    double result = setpoint_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
    }
    return result;
}

double HPWH::getMaxCompressorSetpoint(UNITS units /*=UNITS_C*/) const
{

    if (!hasACompressor())
    {
        send_error("Unit does not have a compressor.");
    }

    double returnVal = heatSources[compressorIndex].maxSetpoint_C;
    if (units == UNITS_C)
    {
    }
    else if (units == UNITS_F)
    {
        returnVal = C_TO_F(returnVal);
    }
    else
        send_error("Invalid units.");

    return returnVal;
}

bool HPWH::isNewSetpointPossible(double newSetpoint,
                                 double& maxAllowedSetpoint,
                                 string& why,
                                 UNITS units /*=UNITS_C*/) const
{
    double newSetpoint_C = newSetpoint;
    double maxAllowedSetpoint_C = -273.15;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        newSetpoint_C = F_TO_C(newSetpoint);
        break;
    default:
        send_error("Invalid units.");
    }

    bool returnVal = false;

    if (isSetpointFixed())
    {
        returnVal = (newSetpoint_C == setpoint_C);
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
            if (model == MODELS_StorageTank)
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

    maxAllowedSetpoint = maxAllowedSetpoint_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        maxAllowedSetpoint = C_TO_F(maxAllowedSetpoint_C);
        break;
    default:
        send_error("Invalid units.");
    }

    return returnVal;
}

double HPWH::calcSoCFraction(double tMains_C, double tMinUseful_C, double tMax_C) const
{
    // Note that volume is ignored in here since with even nodes it cancels out of the SoC
    // fractional equation
    if (tMains_C >= tMinUseful_C)
    {
        send_warning("tMains_C is greater than or equal tMinUseful_C.");
    }
    if (tMinUseful_C > tMax_C)
    {
        send_warning("tMinUseful_C is greater tMax_C.");
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
        send_error("No compressor found in this HPWH.");
    }

    double result = heatSources[compressorIndex].minT;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
    }

    return result;
}

void HPWH::resetTankToSetpoint() { setTankToTemperature(setpoint_C); }

//-----------------------------------------------------------------------------
///	@brief	Assigns new temps provided from a std::vector to tankTemps_C.
/// @param[in]	setTankTemps	new tank temps (arbitrary non-zero size)
///	@param[in]	units          temp units in setTankTemps (default = UNITS_C)
//-----------------------------------------------------------------------------
void HPWH::setTankLayerTemperatures(std::vector<double> setTankTemps, const UNITS units)
{
    std::size_t numSetNodes = setTankTemps.size();
    if (numSetNodes == 0)
    {
        send_error("No temperatures provided.");
    }

    // convert setTankTemps to �C, if necessary
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        for (auto& T : setTankTemps)
            T = F_TO_C(T);
        break;
    default:
        send_error("Invalid units.");
    }

    // set node temps
    if (!resampleIntensive(tankTemps_C, setTankTemps))
    {
        send_error("Unable to resample tank temperatures.");
    }
}

void HPWH::getTankTemps(std::vector<double>& tankTemps) { tankTemps = tankTemps_C; }

void HPWH::setAirFlowFreedom(double fanFraction)
{
    if (fanFraction < 0 || fanFraction > 1)
    {
        send_error("You have attempted to set the fan fraction outside of bounds.");
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
}

void HPWH::setDoTempDepression(bool doTempDepress) { doTempDepression = doTempDepress; }

void HPWH::setTankSize_adjustUA(double HPWH_size,
                                UNITS units /*=UNITS_L*/,
                                bool forceChange /*=false*/)
{
    // Uses the UA before the function is called and adjusts the A part of the UA to match the input
    // volume given getTankSurfaceArea().
    double oldA = getTankSurfaceArea(UNITS_FT2);

    double HPWH_size_L = HPWH_size;
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        HPWH_size_L = GAL_TO_L(HPWH_size);
        break;
    default:
        send_error("Invalid units.");
    }

    setTankSize(HPWH_size_L, UNITS_L, forceChange);
    setUA(tankUA_kJperHrC / oldA * getTankSurfaceArea(UNITS_FT2), UNITS_kJperHrC);
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
        switch (surfAUnits)
        {
        case UNITS_FT2:
            break;
        case UNITS_M2:
            value = FT2_TO_M2(value);
            break;
        default:;
        }
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
        send_error("Incorrect value for getTankSurfaceArea.");
    }
    return value;
}

/*static*/ double
HPWH::getTankRadius(double vol, UNITS volUnits /*=UNITS_L*/, UNITS radiusUnits /*=UNITS_FT*/)
{ // returns tank radius, ft for use in calculation of heat loss in the bottom and top of the tank.
    // Based off 88 insulated storage tanks currently available on the market from Sanden, AOSmith,
    // HTP, Rheem, and Niles, assumes the aspect ratio for the outer measurements is the same is the
    // actual tank.
    double volft3 = L_TO_FT3(vol);
    switch (volUnits)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volft3 = L_TO_FT3(GAL_TO_L(vol));
        break;
    default:;
    }

    double value = -1.;
    if (volft3 >= 0.)
    {
        value = pow(volft3 / 3.14159 / ASPECTRATIO, 1. / 3.);
        switch (radiusUnits)
        {
        case UNITS_FT:
            break;

        case UNITS_M:
            value = FT_TO_M(value);
            break;
        default:;
        }
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
        send_error("Negative value for getTankRadius.");
    }
    return value;
}

bool HPWH::isTankSizeFixed() const { return tankSizeFixed; }

void HPWH::setTankSize(double HPWH_size, UNITS units /*=UNITS_L*/, bool forceChange /*=false*/)
{
    if (isTankSizeFixed() && !forceChange)
    {
        send_error("Can not change the tank size for your currently selected model.");
    }
    if (HPWH_size <= 0)
    {
        send_error("You have attempted to set the tank volume outside of bounds.");
    }
    else
    {
        switch (units)
        {
        case UNITS_L:
            tankVolume_L = HPWH_size;
            break;

        case UNITS_GAL:
            tankVolume_L = GAL_TO_L(HPWH_size);
            break;
        default:
            send_error("Invalid units.");
        }
    }
    calcSizeConstants();
}
void HPWH::setDoInversionMixing(bool doInversionMixing_in)
{
    doInversionMixing = doInversionMixing_in;
}

void HPWH::setDoConduction(bool doConduction_in) { doConduction = doConduction_in; }

void HPWH::setUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
{
    switch (units)
    {
    case UNITS_kJperHrC:
        tankUA_kJperHrC = UA;
        break;
    case UNITS_BTUperHrF:
        tankUA_kJperHrC = UAf_TO_UAc(UA);
        break;
    default:
        send_error("Invalid units.");
    }
}

void HPWH::getUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    switch (units)
    {
    case UNITS_kJperHrC:
        UA = tankUA_kJperHrC;
        break;
    case UNITS_BTUperHrF:
        UA = tankUA_kJperHrC / UAf_TO_UAc(1.);
        break;
    default:
        send_error("Invalid units.");
    }
}

void HPWH::setFittingsUA(double UA, UNITS units /*=UNITS_kJperHrC*/)
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
        send_error("Invalid units.");
}
void HPWH::getFittingsUA(double& UA, UNITS units /*=UNITS_kJperHrC*/) const
{
    UA = fittingsUA_kJperHrC;
    if (units == UNITS_kJperHrC)
    {
    }
    else if (units == UNITS_BTUperHrF)
    {
        UA = UA / UAf_TO_UAc(1.);
    }
    else
        send_error("Invalid units.");
}

void HPWH::setInletByFraction(double fractionalHeight)
{
    setNodeNumFromFractionalHeight(fractionalHeight, inletHeight);
}

void HPWH::setInlet2ByFraction(double fractionalHeight)
{
    setNodeNumFromFractionalHeight(fractionalHeight, inlet2Height);
}

void HPWH::setExternalInletHeightByFraction(double fractionalHeight)
{
    setExternalPortHeightByFraction(fractionalHeight, 1);
}

void HPWH::setExternalOutletHeightByFraction(double fractionalHeight)
{
    setExternalPortHeightByFraction(fractionalHeight, 2);
}

void HPWH::setExternalPortHeightByFraction(double fractionalHeight, int whichExternalPort)
{
    int heatSourceIndex;
    if (!hasExternalHeatSource(heatSourceIndex))
    {
        send_error("Does not have an external heat source.");
    }

    if (whichExternalPort == 1)
    {
        setNodeNumFromFractionalHeight(fractionalHeight,
                                       heatSources[heatSourceIndex].externalInletHeight);
    }
    else
    {
        setNodeNumFromFractionalHeight(fractionalHeight,
                                       heatSources[heatSourceIndex].externalOutletHeight);
    }
}

void HPWH::setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum)
{
    if (fractionalHeight > 1. || fractionalHeight < 0.)
    {
        send_error("Out of bounds fraction for setInletByFraction.");
    }

    int node = (int)std::floor(getNumNodes() * fractionalHeight);
    inletNum = (node == getNumNodes()) ? getIndexTopNode() : node;
}

int HPWH::getExternalInletHeight() const
{
    int heatSourceIndex;
    if (!hasExternalHeatSource(heatSourceIndex))
    {
        send_error("Does not have an external heat source.");
    }
    return heatSources[heatSourceIndex]
        .externalInletHeight; // Return the first one since all external
    // sources have some ports
}

int HPWH::getExternalOutletHeight() const
{
    int heatSourceIndex;
    if (!hasExternalHeatSource(heatSourceIndex))
    {
        send_error("Does not have an external heat source.");
    }
    return heatSources[heatSourceIndex].externalOutletHeight; // Return the first one since all
                                                              // external sources have some ports
}

void HPWH::setTimerLimitTOT(double limit_min)
{
    if (limit_min > 24. * 60. || limit_min < 0.)
    {
        send_error("Out of bounds time limit for setTimerLimitTOT.");
    }
    timerLimitTOT = limit_min;
}

double HPWH::getTimerLimitTOT_minute() const { return timerLimitTOT; }

int HPWH::getInletHeight(int whichInlet) const
{
    auto result = inletHeight;
    if (whichInlet == 1)
    {
    }
    else if (whichInlet == 2)
    {
        result = inlet2Height;
    }
    else
    {
        send_error("Invalid inlet chosen in getInletHeight.");
    }
    return result;
}

void HPWH::setMaxTempDepression(double maxDepression, UNITS units /*=UNITS_C*/)
{
    maxDepression_C = maxDepression;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        maxDepression_C = F_TO_C(maxDepression);
        break;
    default:
        send_error("Invalid units.");
    }
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

void HPWH::setEnteringWaterHighTempShutOff(double highTemp,
                                           bool tempIsAbsolute,
                                           int heatSourceIndex,
                                           UNITS units /*=UNITS_C*/)
{
    if (!hasEnteringWaterHighTempShutOff(heatSourceIndex))
    {
        send_error("You have attempted to access a heating logic that does not exist.");
    }

    double highTemp_C = highTemp;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        highTemp_C = F_TO_C(highTemp);
        break;
    default:
        send_error("Invalid units.");
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
        send_error(fmt::format("High temperature shut off is too close to the setpoint, expected "
                               "a minimum difference of {:g}",
                               MINSINGLEPASSLIFT));
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
}

void HPWH::setTargetSoCFraction(double target)
{
    if (!isSoCControlled())
    {
        send_error("Can not set target state of charge if HPWH is not using state of charge "
                   "controls.");
    }
    if (target < 0)
    {
        send_error("Can not set a negative target state of charge.");
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

void HPWH::switchToSoCControls(double targetSoC,
                               double hysteresisFraction /*= 0.05*/,
                               double tempMinUseful /*= 43.333*/,
                               bool constantMainsT /*= false*/,
                               double mainsT /*= 18.333*/,
                               UNITS tempUnits /*= UNITS_C*/)
{
    if (!canUseSoCControls())
    {
        send_error("Cannot set up state of charge controls for integrated or wrapped HPWHs.");
    }

    double tempMinUseful_C = tempMinUseful;
    double mainsT_C = mainsT;
    switch (tempUnits)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        tempMinUseful_C = F_TO_C(tempMinUseful);
        mainsT_C = F_TO_C(mainsT);
        break;
    default:
        send_error("Invalid units.");
    }

    if (mainsT_C >= tempMinUseful_C)
    {
        send_error("The mains temperature can't be equal to or greater than the minimum useful "
                   "temperature.");
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
/// nodes given by LOGIC_SIZE.
/// @param[in]	bottomFraction	Lower bounding fraction (0 to 1)
///	@param[in]	topFraction		Upper bounding fraction (0 to 1)
/// @return	vector of node weights
//-----------------------------------------------------------------------------
std::vector<HPWH::NodeWeight> HPWH::getNodeWeightRange(double bottomFraction, double topFraction)
{
    std::vector<NodeWeight> nodeWeights;
    if (topFraction < bottomFraction)
        std::swap(bottomFraction, topFraction);
    auto bottomIndex = static_cast<std::size_t>(bottomFraction * LOGIC_SIZE);
    auto topIndex = static_cast<std::size_t>(topFraction * LOGIC_SIZE);
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
    auto nodeWeights = getNodeWeightRange(0., 1.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "whole tank", nodeWeights, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird_absolute(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third absolute", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondThird(double decisionPoint,
                                                               const UNITS units /* = UNITS_C */,
                                                               const bool absolute /* = false */)
{
    auto nodeWeights = getNodeWeightRange(1. / 3., 2. / 3.);
    double decisionPoint_C = convertTempToC(decisionPoint, units, absolute);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second third", nodeWeights, decisionPoint_C, this, absolute);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomThird(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom third", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth_absolute(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth absolute", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::thirdSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 6., 3. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "third sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fourthSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(3. / 6., 4. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fourth sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fifth sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomHalf(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom half", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfth(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::standby(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "standby", nodeWeights, decisionPoint, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topNodeMaxTemp(double decisionPoint)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_SIZE + 1); // uses very top computation node
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
    auto nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThirdMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixthMaxTemp(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionPoint, this, true, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largeDraw(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 4.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "large draw", nodeWeights, decisionPoint, this, true);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largerDraw(double decisionPoint)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 2.);
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
        send_error(
            "You have attempted to access the temperature of a tank node that does not exist.");
    }

    double result = tankTemps_C[nodeNum];
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        result = C_TO_F(result);
        break;
    default:
        send_error("Invalid units.");
    }

    return result;
}

double HPWH::getNthSimTcouple(int iTCouple, int nTCouple, UNITS units /*=UNITS_C*/) const
{
    if (iTCouple > nTCouple || iTCouple < 1)
    {
        send_error("You have attempted to access a simulated thermocouple that does not exist.");
    }

    double beginFraction = static_cast<double>(iTCouple - 1.) / static_cast<double>(nTCouple);
    double endFraction = static_cast<double>(iTCouple) / static_cast<double>(nTCouple);
    double simTcoupleTemp = getResampledValue(tankTemps_C, beginFraction, endFraction);
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        simTcoupleTemp = C_TO_F(simTcoupleTemp);
        break;
    default:
        send_error("Invalid units.");
    }

    return simTcoupleTemp;
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
    double airTemp_C = airTemp;
    double inletTemp_C = inletTemp;
    double outTemp_C = outTemp;

    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }

    airTemp_C = airTemp;
    inletTemp_C = inletTemp;
    outTemp_C = outTemp;
    switch (tempUnit)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        airTemp_C = F_TO_C(airTemp);
        inletTemp_C = F_TO_C(inletTemp);
        outTemp_C = F_TO_C(outTemp);
        break;
    default:
        send_error("Invalid units.");
    }

    if (airTemp_C < heatSources[compressorIndex].minT ||
        airTemp_C > heatSources[compressorIndex].maxT)
    {
        send_error("The compress does not operate at the specified air temperature.");
    }

    double maxAllowedSetpoint_C =
        heatSources[compressorIndex].maxSetpoint_C -
        heatSources[compressorIndex].secondaryHeatExchanger.hotSideTemperatureOffset_dC;

    if (outTemp_C > maxAllowedSetpoint_C)
    {
        send_error("Inputted outlet temperature of the compressor is higher than can be produced.");
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
    switch (pwrUnit)
    {
    case UNITS_BTUperHr:
        break;
    case UNITS_KW:
        outputCapacity = BTU_TO_KWH(capTemp_BTUperHr);
        break;
    default:
        send_error("Invalid units.");
    }

    return outputCapacity;
}

double HPWH::getNthHeatSourceEnergyInput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // energy used by the heat source is positive - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the energy input of a heat source that does not exist.");
    }

    double energyInput = heatSources[N].energyInput_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energyInput = KWH_TO_BTU(heatSources[N].energyInput_kWh);
        break;
    case UNITS_KJ:
        energyInput = KWH_TO_KJ(heatSources[N].energyInput_kWh);
        break;
    default:
        send_error("Invalid units.");
    }

    return energyInput;
}

double HPWH::getNthHeatSourceEnergyOutput(int N, UNITS units /*=UNITS_KWH*/) const
{
    // returns energy from the heat source into the water - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the energy output of a heat source that does not exist.");
    }

    double energyOutput = heatSources[N].energyOutput_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energyOutput = KWH_TO_BTU(heatSources[N].energyOutput_kWh);
        break;
    case UNITS_KJ:
        energyOutput = KWH_TO_KJ(heatSources[N].energyOutput_kWh);
        break;
    default:
        send_error("Invalid units.");
    }

    return energyOutput;
}

double HPWH::getNthHeatSourceRunTime(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the run time of a heat source that does not exist.");
    }
    return heatSources[N].runtime_min;
}

int HPWH::isNthHeatSourceRunning(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error("You have attempted to access the status of a heat source that does not exist.");
    }
    int result = 0;
    if (heatSources[N].isEngaged())
    {
        result = 1;
    }
    return result;
}

int HPWH::isCompressorRunning() const { return isNthHeatSourceRunning(getCompressorIndex()); }

HPWH::HEATSOURCE_TYPE HPWH::getNthHeatSourceType(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error("You have attempted to access the type of a heat source that does not exist.");
    }
    return heatSources[N].typeOfHeatSource;
}

bool HPWH::getNthHeatSource(int N, HPWH::HeatSource*& heatSource)
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_warning("You have attempted to access the type of a heat source that does not exist.");
        return false;
    }
    heatSource = &heatSources[N];
    return true;
}

double HPWH::getTankSize(UNITS units /*=UNITS_L*/) const
{
    double volume = tankVolume_L;
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume = L_TO_GAL(tankVolume_L);
        break;
    default:
        send_error("Invalid units.");
    }
    return volume;
}

double HPWH::getExternalVolumeHeated(UNITS units /*=UNITS_L*/) const
{
    double volume = externalVolumeHeated_L;
    switch (units)
    {
    case UNITS_L:
        break;
    case UNITS_GAL:
        volume = L_TO_GAL(externalVolumeHeated_L);
        break;
    default:
        send_error("Invalid units.");
    }
    return volume;
}

double HPWH::getEnergyRemovedFromEnvironment(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the space to the water is the positive direction
    double energy = energyRemovedFromEnvironment_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energy = KWH_TO_BTU(energyRemovedFromEnvironment_kWh);
        break;
    case UNITS_KJ:
        energy = KWH_TO_KJ(energyRemovedFromEnvironment_kWh);
        break;
    default:
        send_error("Invalid units.");
    }
    return energy;
}

double HPWH::getStandbyLosses(UNITS units /*=UNITS_KWH*/) const
{
    // moving heat from the water to the space is the positive direction
    double energy = standbyLosses_kWh;
    switch (units)
    {
    case UNITS_KWH:
        break;
    case UNITS_BTU:
        energy = KWH_TO_BTU(standbyLosses_kWh);
        break;
    case UNITS_KJ:
        energy = KWH_TO_KJ(standbyLosses_kWh);
        break;
    default:
        send_error("Invalid units.");
    }
    return energy;
}

double HPWH::getOutletTemp(UNITS units /*=UNITS_C*/) const
{
    double temp = outletTemp_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        temp = C_TO_F(outletTemp_C);
        break;
    default:
        send_error("Invalid units.");
    }
    return temp;
}

double HPWH::getCondenserWaterInletTemp(UNITS units /*=UNITS_C*/) const
{
    double temp = condenserInlet_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        temp = C_TO_F(condenserInlet_C);
        break;
    default:
        send_error("Invalid units.");
    }
    return temp;
}

double HPWH::getCondenserWaterOutletTemp(UNITS units /*=UNITS_C*/) const
{
    double temp = condenserOutlet_C;
    switch (units)
    {
    case UNITS_C:
        break;
    case UNITS_F:
        temp = C_TO_F(condenserOutlet_C);
        break;
    default:
        send_error("Invalid units.");
    }
    return temp;
}

double HPWH::getLocationTemp_C() const { return locationTemperature_C; }

//-----------------------------------------------------------------------------
///	@brief	Evaluates the tank temperature averaged uniformly
/// @param[in]	nodeWeights	Discrete set of weighted nodes
/// @return	Tank temperature (C)
//-----------------------------------------------------------------------------
double HPWH::getAverageTankTemp_C() const
{
    double totalT_C = 0.;
    for (auto& T_C : tankTemps_C)
    {
        totalT_C += T_C;
    }
    return totalT_C / static_cast<double>(getNumNodes());
}

//-----------------------------------------------------------------------------
///	@brief	Evaluates the average tank temperature based on distribution.
/// @note	Distribution must have positive size and be normalized.
/// @param[in]	dist	Discrete set of distribution values
//-----------------------------------------------------------------------------
double HPWH::getAverageTankTemp_C(const std::vector<double>& dist) const
{
    std::vector<double> resampledTankTemps_C(dist.size());
    resample(resampledTankTemps_C, tankTemps_C);

    double tankT_C = 0.;

    std::size_t j = 0;
    for (auto& nodeT_C : resampledTankTemps_C)
    {
        tankT_C += dist[j] * nodeT_C;
        ++j;
    }
    return tankT_C;
}

//-----------------------------------------------------------------------------
///	@brief	Evaluates the average tank temperature based on weighted logic nodes.
/// @note	Logic nodes must be normalizable and are referred to the fixed size LOGIC_NODE_SIZE.
///         Node indices as associated with tank nodes as follows:
///         node # 0: bottom tank node only;
///         node # LOGIC_NODE_SIZE + 1: top node only;
///         nodes # 1..LOGIC_NODE_SIZE: resampled tank nodes.
/// @param[in]	nodeWeights	Discrete set of weighted nodes
/// @return	Tank temperature (C)
//-----------------------------------------------------------------------------
double HPWH::getAverageTankTemp_C(const std::vector<HPWH::NodeWeight>& nodeWeights) const
{
    double sum = 0;
    double totWeight = 0;

    std::vector<double> resampledTankTemps(LOGIC_SIZE);
    resample(resampledTankTemps, tankTemps_C);

    for (auto& nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum == 0)
        { // bottom node only
            sum += tankTemps_C.front() * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
        else if (nodeWeight.nodeNum > LOGIC_SIZE)
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

void HPWH::setTankToTemperature(double temp_C) { setTankLayerTemperatures({temp_C}); }

///////////////////////////////////////////////////////////////////////////////////

double HPWH::getTankHeatContent_kJ() const
{
    // returns tank heat content relative to 0 C using kJ
    return DENSITYWATER_kgperL * tankVolume_L * CPWATER_kJperkgC * getAverageTankTemp_C();
}

int HPWH::getModel() const { return model; }

int HPWH::getCompressorCoilConfig() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    return heatSources[compressorIndex].configuration;
}

int HPWH::isCompressorMultipass() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    return static_cast<int>(heatSources[compressorIndex].isMultipass);
}

int HPWH::isCompressorExternalMultipass() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    return static_cast<int>(heatSources[compressorIndex].isExternalMultipass());
}

bool HPWH::hasACompressor() const { return compressorIndex >= 0; }

bool HPWH::hasExternalHeatSource(int& heatSourceIndex) const
{
    for (heatSourceIndex = 0; heatSourceIndex < getNumHeatSources(); ++heatSourceIndex)
    {
        if (heatSources[heatSourceIndex].configuration == HeatSource::CONFIG_EXTERNAL)
        {
            return true;
        }
    }
    return false;
}

double HPWH::getExternalMPFlowRate(UNITS units /*=UNITS_GPM*/) const
{
    if (!isCompressorExternalMultipass())
    {
        send_error("Does not have an external multipass heat source.");
    }
    double flowRate = heatSources[compressorIndex].mpFlowRate_LPS;
    if (units == HPWH::UNITS_LPS)
    {
    }
    else if (units == HPWH::UNITS_GPM)
    {
        flowRate = LPS_TO_GPM(flowRate);
    }
    else
        send_error("Invalid units.");

    return flowRate;
}

double HPWH::getCompressorMinRuntime(UNITS units /*=UNITS_MIN*/) const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    double minimumRuntime = 10.;
    switch (units)
    {
    case UNITS_MIN:
        break;
    case UNITS_SEC:
        minimumRuntime = MIN_TO_SEC(minimumRuntime);
        break;
    case UNITS_HR:
        minimumRuntime = MIN_TO_HR(minimumRuntime);
        break;
    default:
        send_error("Invalid units.");
    }
    return minimumRuntime;
}

void HPWH::getSizingFractions(double& aquaFract, double& useableFract) const
{
    double aFract = 1.;
    double useFract = 1.;

    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    else if (usesSoCLogic)
    {
        send_error("Current model uses SOC control logic and does not have a definition for "
                   "sizing fractions.");
    }

    // Every compressor must have at least one on logic
    for (std::shared_ptr<HeatingLogic> onLogic : heatSources[compressorIndex].turnOnLogicSet)
    {
        double tempA = onLogic->nodeWeightAvgFract(); // if standby logic will return 1
        aFract = tempA < aFract ? tempA : aFract;
    }
    aquaFract = aFract;

    // Compressors don't need to have an off logic
    if (heatSources[compressorIndex].shutOffLogicSet.size() != 0)
    {
        for (std::shared_ptr<HeatingLogic> offLogic : heatSources[compressorIndex].shutOffLogicSet)
        {

            double tempUse;
            if (offLogic->description == "large draw" || offLogic->description == "larger draw")
            {
                tempUse = 1.; // for checking if there's a big draw to switch to RE
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
        useableFract = 1.;
    }

    // Check if doubles are approximately equally and adjust the relationship so it follows the
    // relationship we expect. The tolerance plays with 0.1 mm in position if the tank is 1m tall...
    double temp = 1. - useableFract;
    if (aboutEqual(aquaFract, temp))
    {
        useableFract = 1. - aquaFract + TOL_MINVALUE;
    }
}

bool HPWH::isHPWHScalable() const { return canScale; }

void HPWH::setScaleCapacityCOP(double scaleCapacity /*=1.0*/, double scaleCOP /*=1.0*/)
{
    if (!isHPWHScalable())
    {
        send_error("Cannot scale the HPWH Capacity or COP.");
    }
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    if (scaleCapacity <= 0 || scaleCOP <= 0)
    {
        send_error("Can not scale the HPWH Capacity or COP to 0 or less than 0.");
    }

    for (auto& perfP : heatSources[compressorIndex].perfMap)
    {
        scaleVector(perfP.inputPower_coeffs, scaleCapacity);
        scaleVector(perfP.COP_coeffs, scaleCOP);
    }
}

void HPWH::setCompressorOutputCapacity(double newCapacity,
                                       double airTemp /*=19.722*/,
                                       double inletTemp /*=14.444*/,
                                       double outTemp /*=57.222*/,
                                       UNITS pwrUnit /*=UNITS_KW*/,
                                       UNITS tempUnit /*=UNITS_C*/)
{

    double oldCapacity = getCompressorCapacity(airTemp, inletTemp, outTemp, pwrUnit, tempUnit);
    double scale = newCapacity / oldCapacity;
    setScaleCapacityCOP(scale, 1.); // Scale the compressor capacity
}

void HPWH::setResistanceCapacity(double power, int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (!isHPWHScalable())
    {
        send_error("Cannot scale the resistance elements.");
    }
    if (getNumResistanceElements() == 0)
    {
        send_error("There are no resistance elements.");
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        send_error("Out of bounds value for \"which\" in setResistanceCapacity().");
    }
    if (power < 0)
    {
        send_error("Can not have a negative input power.");
    }
    // Unit conversion
    double watts = 1000. * power;
    switch (pwrUnit)
    {
    case UNITS_KW:
        break;
    case UNITS_BTUperHr:
        watts = BTU_TO_KWH(watts); // kBTU/h to W
        break;
    default:
        send_error("Invalid units.");
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
}

double HPWH::getResistanceCapacity(int which /*=-1*/, UNITS pwrUnit /*=UNITS_KW*/)
{

    // Input checks
    if (getNumResistanceElements() == 0)
    {
        send_error("There are no resistance elements.");
    }
    if (which < -1 || which > getNumResistanceElements() - 1)
    {
        send_error("Out of bounds value for \"which\" in getResistanceCapacity().");
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
    returnPower /= 1000.; // W to KW
    switch (pwrUnit)
    {
    case UNITS_KW:
        break;
    case UNITS_BTUperHr:
        returnPower = KWH_TO_BTU(returnPower); // kW to BTU/h
        break;
    default:
        send_error("Invalid units.");
    }

    return returnPower;
}

int HPWH::getResistancePosition(int elementIndex) const
{
    if (elementIndex < 0 || elementIndex > getNumHeatSources() - 1)
    {
        send_error("Out of bounds value for which in getResistancePosition.");
    }

    if (!heatSources[elementIndex].isAResistance())
    {
        send_error("This index is not a resistance element.");
    }
    bool foundPosition = false;
    int position = -1;
    for (int i = 0; i < heatSources[elementIndex].getCondensitySize(); i++)
    {
        if (heatSources[elementIndex].condensity[i] > 0.)
        { // res elements have a condensity
            position = i;
            foundPosition = true;
            break;
        }
    }
    if (!foundPosition)
    {
        send_error("Invalid resistance heat source.");
    }
    return position;
}

// the privates
void HPWH::updateTankTemps(double drawVolume_L,
                           double inletT_C,
                           double tankAmbientT_C,
                           double inletVol2_L,
                           double inletT2_C)
{

    /////////////////////////////////////////////////////////////////////////////////////////////////
    if (drawVolume_L > 0.)
    {
        if (inletVol2_L > drawVolume_L)
        {
            send_error("Volume in inlet 2 is greater than the draw volume.");
        }

        // sort the inlets by height
        int highInletNodeIndex;
        double highInletT_C;
        double highInletFraction; // fraction of draw from high inlet

        int lowInletNodeIndex;
        double lowInletT_C;
        double lowInletFraction; // fraction of draw from low inlet

        if (inletHeight > inlet2Height)
        {
            highInletNodeIndex = inletHeight;
            highInletFraction = 1. - inletVol2_L / drawVolume_L;
            highInletT_C = inletT_C;
            lowInletNodeIndex = inlet2Height;
            lowInletT_C = inletT2_C;
            lowInletFraction = inletVol2_L / drawVolume_L;
        }
        else
        {
            highInletNodeIndex = inlet2Height;
            highInletFraction = inletVol2_L / drawVolume_L;
            highInletT_C = inletT2_C;
            lowInletNodeIndex = inletHeight;
            lowInletT_C = inletT_C;
            lowInletFraction = 1. - inletVol2_L / drawVolume_L;
        }

        // calculate number of nodes to draw
        double drawVolume_N = drawVolume_L / nodeVolume_L;
        double drawCp_kJperC = CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVolume_L;

        // heat-exchange models
        if (hasHeatExchanger)
        {
            outletTemp_C = inletT_C;
            for (auto& nodeT_C : tankTemps_C)
            {
                double maxHeatExchange_kJ = drawCp_kJperC * (nodeT_C - outletTemp_C);
                double heatExchange_kJ = nodeHeatExchangerEffectiveness * maxHeatExchange_kJ;

                nodeT_C -= heatExchange_kJ / nodeCp_kJperC;
                outletTemp_C += heatExchange_kJ / drawCp_kJperC;
            }
        }
        else
        {
            double remainingDrawVolume_N = drawVolume_N;
            if (drawVolume_L > tankVolume_L)
            {
                for (int i = 0; i < getNumNodes(); i++)
                {
                    outletTemp_C += tankTemps_C[i];
                    tankTemps_C[i] =
                        (inletT_C * (drawVolume_L - inletVol2_L) + inletT2_C * inletVol2_L) /
                        drawVolume_L;
                }
                outletTemp_C = (outletTemp_C / getNumNodes() * tankVolume_L +
                                tankTemps_C[0] * (drawVolume_L - tankVolume_L)) /
                               drawVolume_L * remainingDrawVolume_N;

                remainingDrawVolume_N = 0.;
            }

            double totalExpelledHeat_kJ = 0.;
            while (remainingDrawVolume_N > 0.)
            {

                // draw no more than one node at a time
                double incrementalDrawVolume_N =
                    remainingDrawVolume_N > 1. ? 1. : remainingDrawVolume_N;

                double outputHeat_kJ = nodeCp_kJperC * incrementalDrawVolume_N * tankTemps_C.back();
                totalExpelledHeat_kJ += outputHeat_kJ;
                tankTemps_C.back() -= outputHeat_kJ / nodeCp_kJperC;

                for (int i = getNumNodes() - 1; i >= 0; --i)
                {
                    // combine all inlet contributions at this node
                    double inletFraction = 0.;
                    if (i == highInletNodeIndex)
                    {
                        inletFraction += highInletFraction;
                        tankTemps_C[i] +=
                            incrementalDrawVolume_N * highInletFraction * highInletT_C;
                    }
                    if (i == lowInletNodeIndex)
                    {
                        inletFraction += lowInletFraction;
                        tankTemps_C[i] += incrementalDrawVolume_N * lowInletFraction * lowInletT_C;
                    }

                    if (i > 0)
                    {
                        double transferT_C =
                            incrementalDrawVolume_N * (1. - inletFraction) * tankTemps_C[i - 1];
                        tankTemps_C[i] += transferT_C;
                        tankTemps_C[i - 1] -= transferT_C;
                    }
                }

                remainingDrawVolume_N -= incrementalDrawVolume_N;
                mixTankInversions();
            }

            outletTemp_C = totalExpelledHeat_kJ / drawCp_kJperC;
        }

        // account for mixing at the bottom of the tank
        if (tankMixesOnDraw && drawVolume_L > 0.)
        {
            int mixedBelowNode = (int)(getNumNodes() * mixBelowFractionOnDraw);
            mixTankNodes(0, mixedBelowNode, 1. / 3.);
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
        const double tau = 2. * KWATER_WpermC /
                           ((CPWATER_kJperkgC * 1000.0) * (DENSITYWATER_kgperL * 1000.0) *
                            (nodeHeight_m * nodeHeight_m)) *
                           secondsPerStep;
        if (tau > 1.)
        {
            send_error(fmt::format("The stability condition for conduction has failed!"));
        }

        // End nodes
        if (getNumNodes() > 1)
        { // inner edges of top and bottom nodes
            nextTankTemps_C.front() += tau * (tankTemps_C[1] - tankTemps_C.front());
            nextTankTemps_C.back() += tau * (tankTemps_C[getNumNodes() - 2] - tankTemps_C.back());
        }

        // Internal nodes
        for (int i = 1; i < getNumNodes() - 1; i++)
        {
            nextTankTemps_C[i] +=
                tau * (tankTemps_C[i + 1] - 2. * tankTemps_C[i] + tankTemps_C[i - 1]);
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

                    // Mix this inversion mixing temperature by averaging all inverted nodes
                    // together.
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
        double qIncrement_kJ = numNodesToHeat * nodeCp_kJperC * (heatToT_C - tankTemps_C[nodeNum]);

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

void HPWH::mixTankNodes(int mixBottomNode, int mixBelowNode, double mixFactor)
{
    double avgT_C = 0.;
    double numAvgNodes = static_cast<double>(mixBelowNode - mixBottomNode);
    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        avgT_C += tankTemps_C[i];
    }
    avgT_C /= numAvgNodes;

    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        tankTemps_C[i] += mixFactor * (avgT_C - tankTemps_C[i]);
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
    nodeCp_kJperC = CPWATER_kJperkgC * DENSITYWATER_kgperL * nodeVolume_L;
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
    // find condentropy/shrinkage
    for (int i = 0; i < getNumHeatSources(); ++i)
    {
        heatSources[i].Tshrinkage_C = findShrinkageT_C(heatSources[i].condensity);
    }

    // find lowest node
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].lowestNode = findLowestNode(heatSources[i].condensity, getNumNodes());
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
                    send_warning("More than one resistance element is assigned to VIP.");
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
    double drawCp_kJperC =
        CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVol_L; // heat capacity of draw

    // Check energy balancing.
    double qInElectrical_kJ = 0.;
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        qInElectrical_kJ += getNthHeatSourceEnergyInput(iHS, UNITS_KJ);
    }

    double qInExtra_kJ = KWH_TO_KJ(extraEnergyInput_kWh);
    double qInHeatSourceEnviron_kJ = getEnergyRemovedFromEnvironment(UNITS_KJ);
    double qOutTankEnviron_kJ = KWH_TO_KJ(standbyLosses_kWh);
    double qOutWater_kJ =
        drawCp_kJperC * (outletTemp_C - member_inletT_C); // assumes only one inlet
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
        send_warning(
            fmt::format("Energy-balance error: {:g} kJ, {:g} %", qBal_kJ, 100. * fracEnergyDiff));
        return false;
    }
    return true;
}

bool compressorIsRunning(HPWH& hpwh)
{
    return (bool)hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex());
}

// Used to check a few inputs after the initialization of a tank model from a preset or a file.
void HPWH::checkInputs()
{
    std::queue<std::string> error_msgs = {};

    if (getNumHeatSources() <= 0 && (model != MODELS_StorageTank))
    {
        error_msgs.push("You must have at least one HeatSource.");
    }

    double condensitySum;
    // loop through all heat sources to check each for malconfigurations
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        // check the heat source type to make sure it has been set
        if (heatSources[i].typeOfHeatSource == TYPE_none)
        {
            error_msgs.push(fmt::format(
                "Heat source {} does not have a specified type.  Initialization failed.", i));
        }
        // check to make sure there is at least one onlogic or parent with onlogic
        int parent = heatSources[i].findParent();
        if (heatSources[i].turnOnLogicSet.size() == 0 &&
            (parent == -1 || heatSources[parent].turnOnLogicSet.size() == 0))
        {
            error_msgs.push(
                "You must specify at least one logic to turn on the element or the element "
                "must be set as a backup for another heat source with at least one logic.");
        }

        // Validate on logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].turnOnLogicSet)
        {
            if (!logic->isValid())
            {
                error_msgs.push(fmt::format("On logic at index {:d} is invalid.", i));
            }
        }
        // Validate off logics
        for (std::shared_ptr<HeatingLogic> logic : heatSources[i].shutOffLogicSet)
        {
            if (!logic->isValid())
            {
                error_msgs.push(fmt::format("Off logic at index {:d} is invalid.", i));
            }
        }

        // check is condensity sums to 1
        condensitySum = 0;

        for (int j = 0; j < heatSources[i].getCondensitySize(); j++)
            condensitySum += heatSources[i].condensity[j];
        if (fabs(condensitySum - 1.0) > 1e-6)
        {
            error_msgs.push(fmt::format("The condensity for heatsource {:d} does not sum to 1. "
                                        "It sums to {:g}.",
                                        i,
                                        condensitySum));
        }
        // check that air flows are all set properly
        if (heatSources[i].airflowFreedom > 1.0 || heatSources[i].airflowFreedom <= 0.0)
        {
            error_msgs.push(fmt::format(
                "\n\tThe airflowFreedom must be between 0 and 1 for heatsource {:d}.", i));
        }

        if (heatSources[i].isACompressor())
        {
            if (heatSources[i].doDefrost)
            {
                if (heatSources[i].defrostMap.size() < 3)
                {
                    error_msgs.push(
                        "Defrost logic set to true but no valid defrost map of length 3 or "
                        "greater set.");
                }
                if (heatSources[i].configuration != HeatSource::CONFIG_EXTERNAL)
                {
                    error_msgs.push("Defrost is only simulated for external compressors.");
                }
            }
        }
        if (heatSources[i].configuration == HeatSource::CONFIG_EXTERNAL)
        {

            if (heatSources[i].shutOffLogicSet.size() != 1)
            {
                error_msgs.push("External heat sources can only have one shut off logic.");
            }
            if (0 > heatSources[i].externalOutletHeight ||
                heatSources[i].externalOutletHeight > getNumNodes() - 1)
            {
                error_msgs.push(
                    "External heat sources need an external outlet height within the bounds"
                    "from from 0 to numNodes-1.");
            }
            if (0 > heatSources[i].externalInletHeight ||
                heatSources[i].externalInletHeight > getNumNodes() - 1)
            {
                error_msgs.push(
                    "External heat sources need an external inlet height within the bounds "
                    "from from 0 to numNodes-1.");
            }
        }
        else
        {
            if (heatSources[i].secondaryHeatExchanger.extraPumpPower_W != 0 ||
                heatSources[i].secondaryHeatExchanger.extraPumpPower_W)
            {
                error_msgs.push(fmt::format(
                    "Heatsource {:d} is not an external heat source but has an external "
                    "secondary heat exchanger.",
                    i));
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
                error_msgs.push(
                    "\n\tUsing the grid lookups but a regression-based performance map is given.");
            }

            // Check length of vectors in perfGridValue are equal
            if (heatSources[i].perfGridValues[0].size() !=
                    heatSources[i].perfGridValues[1].size() &&
                heatSources[i].perfGridValues[0].size() != 0)
            {
                error_msgs.push(
                    "When using grid lookups for performance the vectors in perfGridValues must "
                    "be the same length.");
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
                error_msgs.push(
                    "When using grid lookups for perfmance the vectors in perfGridValues must "
                    "be the same length.");
            }
        }
        else
        {
            // Check that perfmap only has 1 point if config_external and multipass
            if (heatSources[i].isExternalMultipass() && heatSources[i].perfMap.size() != 1)
            {
                error_msgs.push(
                    "External multipass heat sources must have a perfMap of only one point "
                    "with regression equations.");
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
            error_msgs.push(
                "The relationship between the on logic and off logic is not supported. The off "
                "logic is beneath the on logic.");
        }
    }

    double maxTemp;
    string why;
    double tempSetpoint = setpoint_C;
    if (!isNewSetpointPossible(tempSetpoint, maxTemp, why))
    {
        error_msgs.push(fmt::format("Cannot set new setpoint. ({})", why.c_str()));
    }

    // Check if the UA is out of bounds
    if (tankUA_kJperHrC < 0.0)
    {
        error_msgs.push(
            fmt::format("The tankUA_kJperHrC is less than 0 for a HPWH, it must be greater than 0, "
                        "tankUA_kJperHrC is: {:g}",
                        tankUA_kJperHrC));
    }

    // Check single-node heat-exchange effectiveness validity
    if (heatExchangerEffectiveness > 1.)
    {
        error_msgs.push("Heat-exchanger effectiveness cannot exceed 1.");
    }

    if (error_msgs.size() > 0)
    {
        while (error_msgs.size() > 1)
        {
            try
            {
                send_error(error_msgs.front());
            }
            catch (...)
            {
                error_msgs.pop();
            }
        }
        send_error(error_msgs.front());
    }
}

/* static */ bool HPWH::mapNameToPreset(const std::string& modelName, HPWH::MODELS& model)
{
    if (modelName == "Voltex60" || modelName == "AOSmithPHPT60")
    {
        model = HPWH::MODELS_AOSmithPHPT60;
    }
    else if (modelName == "Voltex80" || modelName == "AOSmith80")
    {
        model = HPWH::MODELS_AOSmithPHPT80;
    }
    else if (modelName == "GEred" || modelName == "GE")
    {
        model = HPWH::MODELS_GE2012;
    }
    else if (modelName == "SandenGAU" || modelName == "Sanden80" || modelName == "SandenGen3")
    {
        model = HPWH::MODELS_Sanden80;
    }
    else if (modelName == "Sanden120")
    {
        model = HPWH::MODELS_Sanden120;
    }
    else if (modelName == "SandenGES" || modelName == "Sanden40")
    {
        model = HPWH::MODELS_Sanden40;
    }
    else if (modelName == "AOSmithHPTU50")
    {
        model = HPWH::MODELS_AOSmithHPTU50;
    }
    else if (modelName == "AOSmithHPTU66")
    {
        model = HPWH::MODELS_AOSmithHPTU66;
    }
    else if (modelName == "AOSmithHPTU80")
    {
        model = HPWH::MODELS_AOSmithHPTU80;
    }
    else if (modelName == "AOSmithHPTS40")
    {
        model = HPWH::MODELS_AOSmithHPTS40;
    }
    else if (modelName == "AOSmithHPTS50")
    {
        model = HPWH::MODELS_AOSmithHPTS50;
    }
    else if (modelName == "AOSmithHPTS66")
    {
        model = HPWH::MODELS_AOSmithHPTS66;
    }
    else if (modelName == "AOSmithHPTS80")
    {
        model = HPWH::MODELS_AOSmithHPTS80;
    }
    else if (modelName == "AOSmithHPTU80DR")
    {
        model = HPWH::MODELS_AOSmithHPTU80_DR;
    }
    else if (modelName == "GE502014STDMode" || modelName == "GE2014STDMode")
    {
        model = HPWH::MODELS_GE2014STDMode;
    }
    else if (modelName == "GE502014" || modelName == "GE2014")
    {
        model = HPWH::MODELS_GE2014;
    }
    else if (modelName == "GE802014")
    {
        model = HPWH::MODELS_GE2014_80DR;
    }
    else if (modelName == "RheemHB50")
    {
        model = HPWH::MODELS_RheemHB50;
    }
    else if (modelName == "Stiebel220e" || modelName == "Stiebel220E")
    {
        model = HPWH::MODELS_Stiebel220E;
    }
    else if (modelName == "Generic1")
    {
        model = HPWH::MODELS_Generic1;
    }
    else if (modelName == "Generic2")
    {
        model = HPWH::MODELS_Generic2;
    }
    else if (modelName == "Generic3")
    {
        model = HPWH::MODELS_Generic3;
    }
    else if (modelName == "custom")
    {
        model = HPWH::MODELS_CustomFile;
    }
    else if (modelName == "restankRealistic")
    {
        model = HPWH::MODELS_restankRealistic;
    }
    else if (modelName == "StorageTank")
    {
        model = HPWH::MODELS_StorageTank;
    }
    else if (modelName == "BWC2020_65")
    {
        model = HPWH::MODELS_BWC2020_65;
    }
    // New Rheems
    else if (modelName == "Rheem2020Prem40")
    {
        model = HPWH::MODELS_Rheem2020Prem40;
    }
    else if (modelName == "Rheem2020Prem50")
    {
        model = HPWH::MODELS_Rheem2020Prem50;
    }
    else if (modelName == "Rheem2020Prem65")
    {
        model = HPWH::MODELS_Rheem2020Prem65;
    }
    else if (modelName == "Rheem2020Prem80")
    {
        model = HPWH::MODELS_Rheem2020Prem80;
    }
    else if (modelName == "Rheem2020Build40")
    {
        model = HPWH::MODELS_Rheem2020Build40;
    }
    else if (modelName == "Rheem2020Build50")
    {
        model = HPWH::MODELS_Rheem2020Build50;
    }
    else if (modelName == "Rheem2020Build65")
    {
        model = HPWH::MODELS_Rheem2020Build65;
    }
    else if (modelName == "Rheem2020Build80")
    {
        model = HPWH::MODELS_Rheem2020Build80;
    }
    else if (modelName == "RheemPlugInDedicated40")
    {
        model = HPWH::MODELS_RheemPlugInDedicated40;
    }
    else if (modelName == "RheemPlugInDedicated50")
    {
        model = HPWH::MODELS_RheemPlugInDedicated50;
    }
    else if (modelName == "RheemPlugInShared40")
    {
        model = HPWH::MODELS_RheemPlugInShared40;
    }
    else if (modelName == "RheemPlugInShared50")
    {
        model = HPWH::MODELS_RheemPlugInShared50;
    }
    else if (modelName == "RheemPlugInShared65")
    {
        model = HPWH::MODELS_RheemPlugInShared65;
    }
    else if (modelName == "RheemPlugInShared80")
    {
        model = HPWH::MODELS_RheemPlugInShared80;
    }
    // Large HPWH's
    else if (modelName == "AOSmithCAHP120")
    {
        model = HPWH::MODELS_AOSmithCAHP120;
    }
    else if (modelName == "ColmacCxV_5_SP")
    {
        model = HPWH::MODELS_ColmacCxV_5_SP;
    }
    else if (modelName == "ColmacCxA_10_SP")
    {
        model = HPWH::MODELS_ColmacCxA_10_SP;
    }
    else if (modelName == "ColmacCxA_15_SP")
    {
        model = HPWH::MODELS_ColmacCxA_15_SP;
    }
    else if (modelName == "ColmacCxA_20_SP")
    {
        model = HPWH::MODELS_ColmacCxA_20_SP;
    }
    else if (modelName == "ColmacCxA_25_SP")
    {
        model = HPWH::MODELS_ColmacCxA_25_SP;
    }
    else if (modelName == "ColmacCxA_30_SP")
    {
        model = HPWH::MODELS_ColmacCxA_30_SP;
    }

    else if (modelName == "ColmacCxV_5_MP")
    {
        model = HPWH::MODELS_ColmacCxV_5_MP;
    }
    else if (modelName == "ColmacCxA_10_MP")
    {
        model = HPWH::MODELS_ColmacCxA_10_MP;
    }
    else if (modelName == "ColmacCxA_15_MP")
    {
        model = HPWH::MODELS_ColmacCxA_15_MP;
    }
    else if (modelName == "ColmacCxA_20_MP")
    {
        model = HPWH::MODELS_ColmacCxA_20_MP;
    }
    else if (modelName == "ColmacCxA_25_MP")
    {
        model = HPWH::MODELS_ColmacCxA_25_MP;
    }
    else if (modelName == "ColmacCxA_30_MP")
    {
        model = HPWH::MODELS_ColmacCxA_30_MP;
    }

    else if (modelName == "RheemHPHD60")
    {
        model = HPWH::MODELS_RHEEM_HPHD60VNU_201_MP;
    }
    else if (modelName == "RheemHPHD135")
    {
        model = HPWH::MODELS_RHEEM_HPHD135VNU_483_MP;
    }
    // Nyle Single pass models
    else if (modelName == "NyleC25A_SP")
    {
        model = HPWH::MODELS_NyleC25A_SP;
    }
    else if (modelName == "NyleC60A_SP")
    {
        model = HPWH::MODELS_NyleC60A_SP;
    }
    else if (modelName == "NyleC90A_SP")
    {
        model = HPWH::MODELS_NyleC90A_SP;
    }
    else if (modelName == "NyleC185A_SP")
    {
        model = HPWH::MODELS_NyleC185A_SP;
    }
    else if (modelName == "NyleC250A_SP")
    {
        model = HPWH::MODELS_NyleC250A_SP;
    }
    else if (modelName == "NyleC60A_C_SP")
    {
        model = HPWH::MODELS_NyleC60A_C_SP;
    }
    else if (modelName == "NyleC90A_C_SP")
    {
        model = HPWH::MODELS_NyleC90A_C_SP;
    }
    else if (modelName == "NyleC185A_C_SP")
    {
        model = HPWH::MODELS_NyleC185A_C_SP;
    }
    else if (modelName == "NyleC250A_C_SP")
    {
        model = HPWH::MODELS_NyleC250A_C_SP;
    }
    // Nyle MP models
    else if (modelName == "NyleC60A_MP")
    {
        model = HPWH::MODELS_NyleC60A_MP;
    }
    else if (modelName == "NyleC90A_MP")
    {
        model = HPWH::MODELS_NyleC90A_MP;
    }
    else if (modelName == "NyleC125A_MP")
    {
        model = HPWH::MODELS_NyleC125A_MP;
    }
    else if (modelName == "NyleC185A_MP")
    {
        model = HPWH::MODELS_NyleC185A_MP;
    }
    else if (modelName == "NyleC250A_MP")
    {
        model = HPWH::MODELS_NyleC250A_MP;
    }
    else if (modelName == "NyleC60A_C_MP")
    {
        model = HPWH::MODELS_NyleC60A_C_MP;
    }
    else if (modelName == "NyleC90A_C_MP")
    {
        model = HPWH::MODELS_NyleC90A_C_MP;
    }
    else if (modelName == "NyleC125A_C_MP")
    {
        model = HPWH::MODELS_NyleC125A_C_MP;
    }
    else if (modelName == "NyleC185A_C_MP")
    {
        model = HPWH::MODELS_NyleC185A_C_MP;
    }
    else if (modelName == "NyleC250A_C_MP")
    {
        model = HPWH::MODELS_NyleC250A_C_MP;
    }
    else if (modelName == "QAHV_N136TAU_HPB_SP")
    {
        model = HPWH::MODELS_MITSUBISHI_QAHV_N136TAU_HPB_SP;
    }
    // Stack in a couple scalable models
    else if (modelName == "TamScalable_SP")
    {
        model = HPWH::MODELS_TamScalable_SP;
    }
    else if (modelName == "TamScalable_SP_2X")
    {
        model = HPWH::MODELS_TamScalable_SP_2X;
    }
    else if (modelName == "TamScalable_SP_Half")
    {
        model = HPWH::MODELS_TamScalable_SP_Half;
    }
    else if (modelName == "Scalable_MP")
    {
        model = HPWH::MODELS_Scalable_MP;
    }
    else if (modelName == "AWHSTier3Generic40")
    {
        model = HPWH::MODELS_AWHSTier3Generic40;
    }
    else if (modelName == "AWHSTier3Generic50")
    {
        model = HPWH::MODELS_AWHSTier3Generic50;
    }
    else if (modelName == "AWHSTier3Generic65")
    {
        model = HPWH::MODELS_AWHSTier3Generic65;
    }
    else if (modelName == "AWHSTier3Generic80")
    {
        model = HPWH::MODELS_AWHSTier3Generic80;
    }
    else if (modelName == "AWHSTier4Generic40") // Tier-4 Generic 40 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic40;
    }
    else if (modelName == "AWHSTier4Generic50") // Tier-4 Generic 50 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic50;
    }
    else if (modelName == "AWHSTier4Generic65") // Tier-4 Generic 65 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic65;
    }
    else if (modelName == "AWHSTier4Generic80") // Tier-4 Generic 80 gal
    {
        model = HPWH::MODELS_AWHSTier4Generic80;
    }
    else if (modelName == "AquaThermAire")
    {
        model = HPWH::MODELS_AquaThermAire;
    }
    else if (modelName == "GenericUEF217")
    {
        model = HPWH::MODELS_GenericUEF217;
    }
    else if (modelName == "BradfordWhiteAeroThermRE2H50")
    {
        model = HPWH::MODELS_BradfordWhiteAeroThermRE2H50;
    }
    else if (modelName == "BradfordWhiteAeroThermRE2H65")
    {
        model = HPWH::MODELS_BradfordWhiteAeroThermRE2H65;
    }
    else if (modelName == "BradfordWhiteAeroThermRE2H80")
    {
        model = HPWH::MODELS_BradfordWhiteAeroThermRE2H80;
    }
    else if (modelName == "LG_APHWC50")
    {
        model = HPWH::MODELS_LG_APHWC50;
    }
    else if (modelName == "LG_APHWC80")
    {
        model = HPWH::MODELS_LG_APHWC80;
    }
    else
    {
        model = HPWH::MODELS_basicIntegrated;
        cout << "Couldn't find model " << modelName << ".  Exiting...\n";
        return false;
    }
    return true;
}

/// Initializes a preset from the modelName
void HPWH::initPreset(const std::string& modelName)
{
    HPWH::MODELS targetModel;
    if (mapNameToPreset(modelName, targetModel))
    {
        initPreset(targetModel);
    }
    else
    {
        send_error("Unable to initialize model.");
    }
    name = modelName;
}

#ifndef HPWH_ABRIDGED
void HPWH::initFromFile(string modelName)
{
    std::string configFile = modelName + ".txt";

    setAllDefaults(); // reset all defaults if you're re-initializing

    // open file, check and report errors
    std::ifstream inputFILE;
    inputFILE.open(configFile.c_str());
    if (!inputFILE.is_open())
    {
        send_error("Input file failed to open.");
    }
    name = modelName;

    // some variables that will be handy
    std::size_t heatsource, sourceNum, nTemps, tempInt;
    std::size_t num_nodes = 0, numHeatSources = 0;
    bool hasInitialTankTemp = false;
    double initialTankT_C = F_TO_C(120.);

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
        token = "";
        line_ss >> token;
        if (line_s.empty() || (token.length() == 0))
        {
            continue;
        }
        if (token.at(0) == '#')
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
            tankVolume_L = tempDouble;
            if (units == "L")
                ;
            else if (units == "gal")
            {
                tankVolume_L = GAL_TO_L(tempDouble);
            }
            else
                send_warning(fmt::format("Invalid units: {}", token));
        }
        else if (token == "UA")
        {
            line_ss >> tempDouble >> units;
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
                send_error(fmt::format("Improper value for {}", token.c_str()));
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
                send_error(fmt::format("Improper value for {}.", token.c_str()));
            }
        }
        else if (token == "mixBelowFractionOnDraw")
        {
            line_ss >> tempDouble;
            if (tempDouble < 0 || tempDouble > 1)
            {
                send_error(fmt::format("Out of bounds value for {}. Should be between 0 and 1.",
                                       token.c_str()));
            }
            mixBelowFractionOnDraw = tempDouble;
        }
        else if (token == "setpoint")
        {
            line_ss >> tempDouble >> units;
            if (units == "C")
                ;
            else if (units == "F")
                tempDouble = F_TO_C(tempDouble);
            else
                send_warning(fmt::format("Invalid units: {}", token));

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
                send_error(fmt::format("Improper value for {}", token.c_str()));
            }
        }
        else if (token == "initialTankTemp")
        {
            line_ss >> tempDouble >> units;
            if (units == "C")
            {
            }
            else if (units == "F")
                tempDouble = F_TO_C(tempDouble);
            else
                send_warning(fmt::format("Invalid units: {}", token));

            initialTankT_C = tempDouble;
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
                send_error(fmt::format("Improper value for {}", token.c_str()));
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
        }

        else if (token == "numHeatSources")
        {
            line_ss >> numHeatSources;
            heatSources.reserve(numHeatSources);
            for (std::size_t i = 0; i < numHeatSources; i++)
            {
                addHeatSource(fmt::format("heat source {:d}", i));
            }
        }
        else if (token == "heatsource")
        {
            if (numHeatSources == 0)
            {
                send_error(
                    "You must specify the number of heat sources before setting their properties.");
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
                    send_error(fmt::format(
                        "Improper value for {} for heat source {:d}.", token.c_str(), heatsource));
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
                    send_error(fmt::format(
                        "Improper value for {} for heat source {:d}.", token.c_str(), heatsource));
                }
            }
            else if (token == "minT")
            {
                line_ss >> tempDouble >> units;
                heatSources[heatsource].minT = tempDouble;
                if (units == "C")
                    ;
                else if (units == "F")
                    heatSources[heatsource].minT = F_TO_C(tempDouble);
                else
                    send_warning(fmt::format("Invalid units: {}", token));
            }
            else if (token == "maxT")
            {
                line_ss >> tempDouble >> units;
                heatSources[heatsource].maxT = tempDouble;
                if (units == "C")
                    ;
                else if (units == "F")
                    heatSources[heatsource].maxT = F_TO_C(tempDouble);
                else
                    send_warning("Invalid units.");
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
                        if (nodeNum > LOGIC_SIZE + 1 || nodeNum < 0)
                        {
                            send_error(fmt::format(
                                "Node number for heat source {:d} {} must be between 0 and {:d}.",
                                heatsource,
                                token.c_str(),
                                LOGIC_SIZE + 1));
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
                        for ([[maybe_unused]] auto n : nodeNums)
                        {
                            weights.push_back(1.0);
                        }
                    }
                    if (nodeNums.size() != weights.size())
                    {
                        send_error(fmt::format(
                            "Number of weights for heatsource {:d} {} ({:d}) does not match number "
                            "of nodes for {} ({:d}).",
                            heatsource,
                            token.c_str(),
                            weights.size(),
                            token.c_str(),
                            nodeNums.size()));
                    }
                    if (nextToken != "absolute" && nextToken != "relative")
                    {
                        send_error(fmt::format(
                            "Improper definition, \"{}\", for heat source {:d} {}. Should be "
                            "\"relative\" or \"absolute\".",
                            nextToken.c_str(),
                            heatsource,
                            token.c_str()));
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
                        send_error(fmt::format(
                            "Improper comparison, \"{}\", for heat source {:d} {}. Should be "
                            "\"<\" or \">\".\n",
                            compareStr.c_str(),
                            heatsource,
                            token.c_str()));
                    }
                    if (units == "C")
                    {
                    }
                    else if (units == "F")
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
                    else
                        send_warning(fmt::format("Invalid units: {}", token));

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
                            send_error(fmt::format(
                                "Improper comparison, \"{}\", for heat source {:d} {}. Should be "
                                "\"<\" or \">\".",
                                compareStr.c_str(),
                                heatsource,
                                token.c_str()));
                        }
                        line_ss >> tempDouble;
                    }
                    else
                    {
                        tempDouble = std::stod(nextToken);
                    }

                    line_ss >> units;
                    if (units == "C")
                        ;
                    else if (units == "F")
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
                    else
                        send_warning(fmt::format("Invalid units: {}", token));

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
                        send_error(fmt::format(
                            "Improper {} for heat source {:d}", token.c_str(), heatsource));
                    }
                }
                else if (token == "offlogic")
                {
                    line_ss >> tempDouble >> units;
                    if (units == "C")
                        ;
                    else if (units == "F")
                    {
                        tempDouble = F_TO_C(tempDouble);
                    }
                    else
                        send_warning(fmt::format("Invalid units: {}", token));

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
                        send_error(fmt::format(
                            "Improper {} for heat source {:d}", token.c_str(), heatsource));
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
                    send_error(
                        fmt::format("Improper {} for heat source {:d}", token.c_str(), heatsource));
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
                    send_error(
                        fmt::format("Improper {} for heat source {:d}", token.c_str(), heatsource));
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
                    send_error(
                        fmt::format("Improper {} for heat source {:d}", token.c_str(), heatsource));
                }
            }

            else if (token == "externalInlet")
            {
                line_ss >> tempInt;
                if (tempInt < num_nodes)
                {
                    heatSources[heatsource].externalInletHeight = static_cast<int>(tempInt);
                }
                else
                {
                    send_error(
                        fmt::format("Improper {} for heat source {:d}", token.c_str(), heatsource));
                }
            }
            else if (token == "externalOutlet")
            {
                line_ss >> tempInt;
                if (tempInt < num_nodes)
                {
                    heatSources[heatsource].externalOutletHeight = static_cast<int>(tempInt);
                }
                else
                {
                    send_error(
                        fmt::format("Improper {} for heat source {:d}", token.c_str(), heatsource));
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
                std::size_t maxTemps = heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        send_error(fmt::format(
                            "{} specified for heat source {:d} before definition of nTemps.",
                            token.c_str(),
                            heatsource));
                    }
                    else
                    {
                        send_error(fmt::format(
                            "Incorrect specification for {} from heat source {:d}. nTemps, {}, is "
                            "less than {}.  \n",
                            token.c_str(),
                            heatsource,
                            maxTemps,
                            nTemps));
                    }
                }
                line_ss >> tempDouble >> units;
                if (units == "C")
                {
                    tempDouble = C_TO_F(tempDouble);
                }
                else if (units == "F")
                {
                }
                else
                    send_warning(fmt::format("Invalid units: {}", token));

                heatSources[heatsource].perfMap[nTemps - 1].T_F = tempDouble;
            }
            else if (std::regex_match(token, std::regex("(?:inPow|cop)T\\d+(?:const|lin|quad)")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("(inPow|cop)T(\\d+)(const|lin|quad)"));
                string var = match[1].str();
                nTemps = std::stoi(match[2].str());
                string coeff = match[3].str();

                /*
                // TODO: Currently relies on the coefficients being defined in the correct order
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
                */

                std::size_t maxTemps = heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        send_error(fmt::format(
                            "{} specified for heat source {:d} before definition of nTemps.",
                            token.c_str(),
                            heatsource));
                    }
                    else
                    {
                        send_error(fmt::format("Incorrect specification for {} from heat source "
                                               "{:d}. nTemps, {:d}, is "
                                               "less than {:d}.  \n",
                                               token.c_str(),
                                               heatsource,
                                               maxTemps,
                                               nTemps));
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
                if (units == "C")
                    ;
                else if (units == "F")
                {
                    tempDouble = dF_TO_dC(tempDouble);
                }
                else
                    send_warning(fmt::format("Invalid units: {}", token));

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
                send_error(fmt::format(
                    "Improper specifier ({:d}) for heat source {:g}", token.c_str(), heatsource));
            }

        } // end heatsource options
        else
        {
            send_error(fmt::format("Improper keyword: {}", token.c_str()));
        }

    } // end while over lines

    // take care of the non-input processing
    model = MODELS_CustomFile;

    tankTemps_C.resize(num_nodes);

    if (hasInitialTankTemp)
        setTankToTemperature(initialTankT_C);
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

    checkInputs();
}
#endif

//-----------------------------------------------------------------------------
///	@brief	Determine the inletT testing based on the ambientT
/// @return	inletT (C)
//-----------------------------------------------------------------------------
double HPWH::findInletT_C(double ambientT_C)
{
    const double ambientT_C_E50 = 10.;
    const double ambientT_C_UEF = 19.7; // EERE-2019-BT-TP-0032-0058, p. 40435
    const double ambientT_C_E95 = 35.;

    const double inletT_C_E50 = F_TO_C(50.0); // table
    const double inletT_C_UEF = 14.4;         // EERE-2019-BT-TP-0032-0058, p. 40433
    const double inletT_C_E95 = F_TO_C(67.0); // table

    if (ambientT_C < ambientT_C_E50)
        return inletT_C_E50;
    if (ambientT_C < ambientT_C_UEF)
    { // linear interp
        double ratio = (ambientT_C - ambientT_C_E50) / (ambientT_C_UEF - ambientT_C_E50);
        return (1. - ratio) * inletT_C_E50 + ratio * inletT_C_UEF;
    }
    if (ambientT_C < ambientT_C_E95)
    { // linear interp
        double ratio = (ambientT_C - ambientT_C_UEF) / (ambientT_C_E95 - ambientT_C_UEF);
        return (1. - ratio) * inletT_C_UEF + ratio * inletT_C_E95;
    }
    return inletT_C_E95;
}

//-----------------------------------------------------------------------------
///	@brief	Performs a draw/heat cycle to prep for test
///         Draw until heating begins, wait for recovery.
/// @note	see EERE-2019-BT-TP-0032-0058, p. 40479 (5.2.4)
//-----------------------------------------------------------------------------
void HPWH::prepForTest(TestOptions& testOptions)
{
    // apply first-hour-rating criterion EERE-2019-BT-TP-0032-0058, p. 40479
    double flowRate_Lper_min = GAL_TO_L(3.);
    if (tankVolume_L < GAL_TO_L(20.))
        flowRate_Lper_min = GAL_TO_L(1.5);

    double inletT_C = testOptions.testConfiguration.inletT_C;
    const double ambientT_C = testOptions.testConfiguration.ambientT_C;
    const double externalT_C = ambientT_C;

    if (testOptions.changeSetpoint)
    {
        setSetpoint(testOptions.setpointT_C, UNITS_C);
    }

    DRMODES drMode = DR_ALLOW;
    bool isDrawing = false;
    bool done = false;
    int step = 0;
    while (!done)
    {
        switch (step)
        {
        case 0: // start with heat off
        {
            if (!isHeating)
            {
                isDrawing = true;
                ++step;
            }
            break;
        }

        case 1: // draw until heat turns on
        {
            if (isHeating)
            {
                isDrawing = false;
                ++step;
            }
            break;
        }

        case 2: // wait for heat to turn off
        {
            if (!isHeating)
            {
                isDrawing = false;
                done = true;
            }
            break;
        }
        }

        // limit draw-volume increment to tank volume
        double incrementalDrawVolume_L = isDrawing ? flowRate_Lper_min * (1.) : 0.;
        if (incrementalDrawVolume_L > tankVolume_L)
        {
            incrementalDrawVolume_L = tankVolume_L;
        }

        runOneStep(inletT_C,                // inlet water temperature (C)
                   incrementalDrawVolume_L, // draw volume (L)
                   ambientT_C,              // ambient Temp (C)
                   externalT_C,             // external Temp (C)
                   drMode,                  // DDR Status
                   0.,                      // inlet-2 volume (L)
                   inletT_C,                // inlet-2 Temp (C)
                   NULL);                   // no extra heat
    }
}

//-----------------------------------------------------------------------------
///	@brief	Find first-hour rating designation for 24-hr test
/// @note	see EERE-2019-BT-TP-0032-0058, p. 40479 (5.3.3)
/// @param[out] firstHourRating	    contains first-hour rating designation
///	@param[in]	setpointT_C		    setpoint temperature (optional)
//-----------------------------------------------------------------------------
void HPWH::findFirstHourRating(FirstHourRating& firstHourRating, TestOptions& testOptions)
{
    double flowRate_Lper_min = GAL_TO_L(3.);
    if (tankVolume_L < GAL_TO_L(20.))
        flowRate_Lper_min = GAL_TO_L(1.5);

    const double ambientT_C = testConfiguration_UEF.ambientT_C;
    const double externalT_C = ambientT_C;
    double inletT_C = testConfiguration_UEF.inletT_C;

    if (testOptions.changeSetpoint)
    {
        setSetpoint(testOptions.setpointT_C, UNITS_C);
    }

    double tankT_C = getAverageTankTemp_C();
    double maxTankT_C = tankT_C;
    double maxOutletT_C = 0.;

    DRMODES drMode = DR_ALLOW;
    double drawVolume_L = 0.;

    firstHourRating.drawVolume_L = 0.;

    double sumOutletVolumeT_LC = 0.;
    double sumOutletVolume_L = 0.;

    double avgOutletT_C = 0.;
    double minOutletT_C = 0.;
    double prevAvgOutletT_C = 0.;
    double prevMinOutletT_C = 0.;

    bool isDrawing = false;
    bool done = false;
    int step = 0;

    prepForTest(testOptions);

    bool firstDraw = true;
    isDrawing = true;
    maxOutletT_C = 0.;
    drMode = DR_LOC;
    int elapsedTime_min = 0;
    while (!done)
    {
        // limit draw-volume increment to tank volume
        double incrementalDrawVolume_L = isDrawing ? flowRate_Lper_min * (1.) : 0.;
        if (incrementalDrawVolume_L > tankVolume_L)
        {
            incrementalDrawVolume_L = tankVolume_L;
        }

        runOneStep(inletT_C,                // inlet water temperature (C)
                   incrementalDrawVolume_L, // draw volume (L)
                   ambientT_C,              // ambient Temp (C)
                   externalT_C,             // external Temp (C)
                   drMode,                  // DDR Status
                   0.,                      // inlet-2 volume (L)
                   inletT_C,                // inlet-2 Temp (C)
                   NULL);                   // no extra heat

        tankT_C = getAverageTankTemp_C();

        switch (step)
        {
        case 0: // drawing
        {
            sumOutletVolume_L += incrementalDrawVolume_L;
            sumOutletVolumeT_LC += incrementalDrawVolume_L * outletTemp_C;

            maxOutletT_C = std::max(outletTemp_C, maxOutletT_C);
            if (outletTemp_C <
                maxOutletT_C - dF_TO_dC(15.)) // outletT has dropped by 15 degF below max T
            {
                avgOutletT_C = sumOutletVolumeT_LC / sumOutletVolume_L;
                minOutletT_C = outletTemp_C;
                if (elapsedTime_min >= 60)
                {
                    double fac = 1;
                    if (!firstDraw)
                    {
                        fac = (avgOutletT_C - prevMinOutletT_C) /
                              (prevAvgOutletT_C - prevMinOutletT_C);
                    }
                    firstHourRating.drawVolume_L += fac * drawVolume_L;
                    done = true;
                }
                else
                {
                    firstHourRating.drawVolume_L += drawVolume_L;
                    drawVolume_L = 0.;
                    isDrawing = false;
                    drMode = DR_ALLOW;
                    maxTankT_C = tankT_C;        // initialize for next pass
                    maxOutletT_C = outletTemp_C; // initialize for next pass
                    prevAvgOutletT_C = avgOutletT_C;
                    prevMinOutletT_C = minOutletT_C;
                    ++step;
                }
            }
            break;
        }

        case 1:
        {
            if (isHeating) // ensure heat is on before proceeding
            {
                ++step;
            }
            break;
        }

        case 2: // heating
        {
            if ((tankT_C > maxTankT_C) && isHeating &&
                (elapsedTime_min <
                 60)) // has not reached maxTankT, heat is on, and less than 1 hr has elpased
            {
                maxTankT_C = std::max(tankT_C, maxTankT_C);
            }
            else // start another draw
            {
                firstDraw = false;
                isDrawing = true;
                drawVolume_L = 0.;
                drMode = DR_LOC;
                step = 0; // repeat
            }
        }
        }

        drawVolume_L += incrementalDrawVolume_L;
        ++elapsedTime_min;
    }

    //
    if (firstHourRating.drawVolume_L < GAL_TO_L(18.))
    {
        firstHourRating.desig = FirstHourRating::Desig::VerySmall;
    }
    else if (firstHourRating.drawVolume_L < GAL_TO_L(51.))
    {
        firstHourRating.desig = FirstHourRating::Desig::Low;
    }
    else if (firstHourRating.drawVolume_L < GAL_TO_L(75.))
    {
        firstHourRating.desig = FirstHourRating::Desig::Medium;
    }
    else
    {
        firstHourRating.desig = FirstHourRating::Desig::High;
    }
    testOptions.desig = firstHourRating.desig; // use in 24-hr test
}

//-----------------------------------------------------------------------------
///	@brief	Performs standard 24-hr test
/// @note	see https://www.regulations.gov/document/EERE-2019-BT-TP-0032-0058 (5.4.2)
/// @param[in/out] testOptions          reporting and configuration
/// @param[out] testSummary	            contains test metrics on output
//-----------------------------------------------------------------------------
void HPWH::run24hrTest(TestOptions& testOptions, TestSummary& testSummary)
{
    // select the first draw cluster size and pattern
    auto firstDrawClusterSize = firstDrawClusterSizes[testOptions.desig];
    DrawPattern& drawPattern = drawPatterns[testOptions.desig];

    double inletT_C = testOptions.testConfiguration.inletT_C;
    const double ambientT_C = testOptions.testConfiguration.ambientT_C;
    const double externalT_C = ambientT_C;

    DRMODES drMode = DR_ALLOW;

    prepForTest(testOptions);

    std::vector<OutputData> outputDataSet;

    // idle for 1 hr
    int preTime_min = 0;
    bool heatersAreOn = false;
    while ((preTime_min < 60) || heatersAreOn)
    {
        runOneStep(inletT_C,    // inlet water temperature (C)
                   0,           // draw volume (L)
                   ambientT_C,  // ambient Temp (C)
                   externalT_C, // external Temp (C)
                   drMode,      // DDR Status
                   0.,          // inlet-2 volume (L)
                   inletT_C,    // inlet-2 Temp (C)
                   NULL);       // no extra heat

        heatersAreOn = false;
        for (auto& heatSource : heatSources)
        {
            heatersAreOn |= heatSource.isEngaged();
        }

        {
            OutputData outputData;
            outputData.time_min = preTime_min;
            outputData.ambientT_C = ambientT_C;
            outputData.setpointT_C = getSetpoint();
            outputData.inletT_C = inletT_C;
            outputData.drawVolume_L = 0.;
            outputData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                outputData.h_srcIn_kWh.push_back(getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                outputData.h_srcOut_kWh.push_back(
                    getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            for (int iTC = 0; iTC < testOptions.nTestTCouples; ++iTC)
            {
                outputData.thermocoupleT_C.push_back(
                    getNthSimTcouple(iTC + 1, testOptions.nTestTCouples, UNITS_C));
            }
            outputData.outletT_C = 0.;

            outputDataSet.push_back(outputData);
        }

        ++preTime_min;
    }

    // correct time to start test at 0 min
    for (auto& outputData : outputDataSet)
    {
        outputData.time_min -= preTime_min;
    }

    double tankT_C = getAverageTankTemp_C();
    double initialTankT_C = tankT_C;

    // used to find average draw temperatures
    double drawSumOutletVolumeT_LC = 0.;
    double drawSumInletVolumeT_LC = 0.;

    // used to find average 24-hr test temperatures
    double sumOutletVolumeT_LC = 0.;
    double sumInletVolumeT_LC = 0.;

    // first-recovery info
    testSummary.recoveryStoredEnergy_kJ = 0.;
    testSummary.recoveryDeliveredEnergy_kJ = 0.;
    testSummary.recoveryUsedEnergy_kJ = 0.;

    double deliveredEnergy_kJ = 0.; // total energy delivered to water
    testSummary.removedVolume_L = 0.;
    testSummary.usedEnergy_kJ = 0.;           // Q
    testSummary.usedFossilFuelEnergy_kJ = 0.; // total fossil-fuel energy consumed, Qf
    testSummary.usedElectricalEnergy_kJ = 0.; // total electrical energy consumed, Qe

    bool hasHeated = false;

    int endTime_min = 24 * static_cast<int>(min_per_hr);
    std::size_t iDraw = 0;
    double remainingDrawVolume_L = 0.;
    double drawVolume_L = 0.;
    double prevDrawEndTime_min = 0.;

    bool isFirstRecoveryPeriod = true;
    bool isInFirstDrawCluster = true;
    bool hasStandbyPeriodStarted = false;
    bool hasStandbyPeriodEnded = false;
    bool nextDraw = true;
    bool isDrawing = false;
    bool isDrawPatternComplete = false;

    int recoveryEndTime_min = 0;

    int standbyStartTime_min = 0;
    int standbyEndTime_min = 0;
    double standbyStartT_C = 0;
    double standbyEndT_C = 0;
    double standbyStartTankEnergy_kJ = 0.;
    double standbyEndTankEnergy_kJ = 0.;
    double standbySumTimeTankT_minC = 0.;
    double standbySumTimeAmbientT_minC = 0.;

    int noDrawTotalTime_min = 0;
    double noDrawSumTimeAmbientT_minC = 0.;

    bool inLastHour = false;
    double stepDrawVolume_L = 0.;
    for (int runTime_min = 0; runTime_min <= endTime_min; ++runTime_min)
    {
        if (inLastHour)
        {
            drMode = DR_LOC | DR_LOR;
        }

        if (nextDraw)
        {
            auto& draw = drawPattern[iDraw];
            if (runTime_min >= draw.startTime_min)
            {
                // limit draw-volume step to tank volume
                stepDrawVolume_L = draw.flowRate_Lper_min * (1.);
                if (stepDrawVolume_L > tankVolume_L)
                {
                    stepDrawVolume_L = tankVolume_L;
                }

                remainingDrawVolume_L = drawVolume_L = draw.volume_L;

                nextDraw = false;
                isDrawing = true;

                drawSumOutletVolumeT_LC = 0.;
                drawSumInletVolumeT_LC = 0.;

                if (hasStandbyPeriodStarted && (!hasStandbyPeriodEnded))
                {
                    hasStandbyPeriodEnded = true;
                    standbyEndTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
                    standbyEndT_C = tankT_C;                             // Tsu,0
                    standbyEndTime_min = runTime_min;
                }
            }
        }

        // iterate until 1) specified draw volume has been reached and 2) next draw has started
        // do not exceed specified draw volume
        if (isDrawing)
        {
            if (stepDrawVolume_L >= remainingDrawVolume_L)
            {
                stepDrawVolume_L = remainingDrawVolume_L;
                remainingDrawVolume_L = 0.;
                isDrawing = false;
                prevDrawEndTime_min = runTime_min;
            }
            else
            {
                remainingDrawVolume_L -= stepDrawVolume_L;
            }
        }
        else
        {
            remainingDrawVolume_L = stepDrawVolume_L = 0.;
            noDrawSumTimeAmbientT_minC += (1.) * ambientT_C;
            ++noDrawTotalTime_min;
        }

        // run a step
        runOneStep(inletT_C,         // inlet water temperature (C)
                   stepDrawVolume_L, // draw volume (L)
                   ambientT_C,       // ambient Temp (C)
                   externalT_C,      // external Temp (C)
                   drMode,           // DDR Status
                   0.,               // inlet-2 volume (L)
                   inletT_C,         // inlet-2 Temp (C)
                   NULL);            // no extra heat

        {
            OutputData outputData;
            outputData.time_min = runTime_min;
            outputData.ambientT_C = ambientT_C;
            outputData.setpointT_C = getSetpoint();
            outputData.inletT_C = inletT_C;
            outputData.drawVolume_L = stepDrawVolume_L;
            outputData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                outputData.h_srcIn_kWh.push_back(getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KWH));
                outputData.h_srcOut_kWh.push_back(
                    getNthHeatSourceEnergyOutput(iHS, HPWH::UNITS_KWH));
            }

            for (int iTC = 0; iTC < testOptions.nTestTCouples; ++iTC)
            {
                outputData.thermocoupleT_C.push_back(
                    getNthSimTcouple(iTC + 1, testOptions.nTestTCouples, UNITS_C));
            }
            outputData.outletT_C = outletTemp_C;

            outputDataSet.push_back(outputData);
        }

        tankT_C = getAverageTankTemp_C();
        hasHeated |= isHeating;

        drawSumOutletVolumeT_LC += stepDrawVolume_L * outletTemp_C;
        drawSumInletVolumeT_LC += stepDrawVolume_L * inletT_C;

        sumOutletVolumeT_LC += stepDrawVolume_L * outletTemp_C;
        sumInletVolumeT_LC += stepDrawVolume_L * inletT_C;

        // collect energy added to water
        double stepDrawMass_kg = DENSITYWATER_kgperL * stepDrawVolume_L;
        double stepDrawHeatCapacity_kJperC = CPWATER_kJperkgC * stepDrawMass_kg;
        deliveredEnergy_kJ += stepDrawHeatCapacity_kJperC * (outletTemp_C - inletT_C);

        // collect used-energy info
        double usedFossilFuelEnergy_kJ = 0.;
        double usedElectricalEnergy_kJ = 0.;
        for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
        {
            usedElectricalEnergy_kJ += getNthHeatSourceEnergyInput(iHS, HPWH::UNITS_KJ);
        }

        // collect 24-hr test info
        testSummary.removedVolume_L += stepDrawVolume_L;
        testSummary.usedFossilFuelEnergy_kJ += usedFossilFuelEnergy_kJ;
        testSummary.usedElectricalEnergy_kJ += usedElectricalEnergy_kJ;
        testSummary.usedEnergy_kJ += usedFossilFuelEnergy_kJ + usedElectricalEnergy_kJ;

        if (isFirstRecoveryPeriod)
        {
            testSummary.recoveryUsedEnergy_kJ += usedFossilFuelEnergy_kJ + usedElectricalEnergy_kJ;
        }

        if (!isDrawing)
        {
            if (isFirstRecoveryPeriod)
            {
                if (hasHeated && (!isHeating))
                {
                    // collect recovery info
                    isFirstRecoveryPeriod = false;

                    double tankContentMass_kg = DENSITYWATER_kgperL * tankVolume_L;
                    double tankHeatCapacity_kJperC = CPWATER_kJperkgC * tankContentMass_kg;
                    testSummary.recoveryStoredEnergy_kJ =
                        tankHeatCapacity_kJperC * (tankT_C - initialTankT_C);
                }

                if (!nextDraw)
                {
                    double meanDrawOutletT_C = drawSumOutletVolumeT_LC / drawVolume_L;
                    double meanDrawInletT_C = drawSumInletVolumeT_LC / drawVolume_L;

                    double drawMass_kg = DENSITYWATER_kgperL * drawVolume_L;
                    double drawHeatCapacity_kJperC = CPWATER_kJperkgC * drawMass_kg;

                    testSummary.recoveryDeliveredEnergy_kJ +=
                        drawHeatCapacity_kJperC * (meanDrawOutletT_C - meanDrawInletT_C);
                }
            }

            if (!hasStandbyPeriodEnded)
            {
                if (hasStandbyPeriodStarted)
                {
                    standbySumTimeTankT_minC += (1.) * tankT_C;
                    standbySumTimeAmbientT_minC += (1.) * ambientT_C;

                    if (runTime_min >= standbyStartTime_min + 8 * min_per_hr)
                    {
                        hasStandbyPeriodEnded = true;
                        standbyEndTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
                        standbyEndT_C = tankT_C;                             // Tsu,0
                        standbyEndTime_min = runTime_min;
                    }
                }
                else
                {
                    if (isHeating)
                    {
                        recoveryEndTime_min = runTime_min;
                    }
                    else
                    {
                        if ((!isInFirstDrawCluster) || isDrawPatternComplete)
                        {
                            if ((runTime_min > prevDrawEndTime_min + 5) &&
                                (runTime_min > recoveryEndTime_min + 5))
                            {
                                hasStandbyPeriodStarted = true;
                                standbyStartTime_min = runTime_min;
                                standbyStartTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
                                standbyStartT_C = tankT_C;                             // Tsu,0

                                if (isDrawPatternComplete &&
                                    (runTime_min + 8 * min_per_hr > endTime_min))
                                {
                                    endTime_min = runTime_min + 8 * static_cast<int>(min_per_hr);
                                }
                            }
                        }
                    }
                }
            }

            if (!nextDraw)
            {
                ++iDraw;
                if (iDraw < drawPattern.size())
                {
                    nextDraw = true;
                    isInFirstDrawCluster = (iDraw < firstDrawClusterSize);
                }
                else
                {
                    isDrawPatternComplete = true;
                }
            }
        }
    }

    double finalTankT_C = tankT_C;

    if (!hasStandbyPeriodEnded)
    {
        hasStandbyPeriodEnded = true;
        standbyEndTime_min = endTime_min;
        standbyEndTankEnergy_kJ = testSummary.usedEnergy_kJ; // Qsu,0
        standbyEndT_C = tankT_C;                             // Tsu,0
    }

    if (testOptions.saveOutput)
    {
        for (auto& outputData : outputDataSet)
        {
            writeRowAsCSV(testOptions.outputFile, outputData);
        }
    }

    testSummary.avgOutletT_C = sumOutletVolumeT_LC / testSummary.removedVolume_L;
    testSummary.avgInletT_C = sumInletVolumeT_LC / testSummary.removedVolume_L;

    constexpr double standardSetpointT_C = 51.7;
    constexpr double standardInletT_C = 14.4;
    constexpr double standardAmbientT_C = 19.7;

    double tankContentMass_kg = DENSITYWATER_kgperL * tankVolume_L;
    double tankHeatCapacity_kJperC = CPWATER_kJperkgC * tankContentMass_kg;

    double removedMass_kg = DENSITYWATER_kgperL * testSummary.removedVolume_L;
    double removedHeatCapacity_kJperC = CPWATER_kJperkgC * removedMass_kg;

    // require heating during 24-hr test for unit to qualify as consumer water heater
    if (hasHeated && !isDrawing)
    {
        testSummary.qualifies = true;
    }

    // find the "Recovery Efficiency" (6.3.3)
    testSummary.recoveryEfficiency = 0.;
    if (testSummary.recoveryUsedEnergy_kJ > 0.)
    {
        testSummary.recoveryEfficiency =
            (testSummary.recoveryStoredEnergy_kJ + testSummary.recoveryDeliveredEnergy_kJ) /
            testSummary.recoveryUsedEnergy_kJ;
    }

    // find the energy consumed during the standby-loss test, Qstdby
    testSummary.standbyUsedEnergy_kJ = standbyEndTankEnergy_kJ - standbyStartTankEnergy_kJ;

    int standbyPeriodTime_min = standbyEndTime_min - standbyStartTime_min - 1;
    testSummary.standbyPeriodTime_h = standbyPeriodTime_min / min_per_hr; // tau_stby,1
    if ((testSummary.standbyPeriodTime_h > 0) && (testSummary.recoveryEfficiency > 0.))
    {
        double standardTankEnergy_kJ = tankHeatCapacity_kJperC * (standbyEndT_C - standbyStartT_C) /
                                       testSummary.recoveryEfficiency;
        testSummary.standbyHourlyLossEnergy_kJperh =
            (testSummary.standbyUsedEnergy_kJ - standardTankEnergy_kJ) /
            testSummary.standbyPeriodTime_h;

        double standbyAverageTankT_C = standbySumTimeTankT_minC / standbyPeriodTime_min;
        double standbyAverageAmbientT_C = standbySumTimeAmbientT_minC / standbyPeriodTime_min;

        double dT_C = standbyAverageTankT_C - standbyAverageAmbientT_C;
        if (dT_C > 0.)
        {
            testSummary.standbyLossCoefficient_kJperhC =
                testSummary.standbyHourlyLossEnergy_kJperh / dT_C; // UA
        }
    }

    //
    testSummary.noDrawTotalTime_h = noDrawTotalTime_min / min_per_hr; // tau_stby,2
    if (noDrawTotalTime_min > 0)
    {
        testSummary.noDrawAverageAmbientT_C =
            noDrawSumTimeAmbientT_minC / noDrawTotalTime_min; // <Ta,stby,2>
    }

    // find the standard delivered daily energy
    double standardDeliveredEnergy_kJ =
        removedHeatCapacity_kJperC * (standardSetpointT_C - standardInletT_C);

    // find the "Daily Water Heating Energy Consumption (Q_d)" (6.3.5)
    testSummary.consumedHeatingEnergy_kJ = testSummary.usedEnergy_kJ;
    if (testSummary.recoveryEfficiency > 0.)
    {
        testSummary.consumedHeatingEnergy_kJ -= tankHeatCapacity_kJperC *
                                                (finalTankT_C - initialTankT_C) /
                                                testSummary.recoveryEfficiency;
    }

    // find the "Adjusted Daily Water Heating Energy Consumption (Q_da)" (6.3.6)
    testSummary.adjustedConsumedWaterHeatingEnergy_kJ =
        testSummary.consumedHeatingEnergy_kJ -
        (standardAmbientT_C - testSummary.noDrawAverageAmbientT_C) *
            testSummary.standbyLossCoefficient_kJperhC * testSummary.noDrawTotalTime_h;

    // find the "Energy Used to Heat Water (Q_HW)" (6.3.6)
    testSummary.waterHeatingEnergy_kJ = 0.;
    if (testSummary.recoveryEfficiency > 0.)
    {
        testSummary.waterHeatingEnergy_kJ = deliveredEnergy_kJ / testSummary.recoveryEfficiency;
    }

    // find the "Standard Energy Used to Heat Water (Q_HW,T)" (6.3.6)
    testSummary.standardWaterHeatingEnergy_kJ = 0.;
    if (testSummary.recoveryEfficiency > 0.)
    {
        double standardRemovedEnergy_kJ =
            removedHeatCapacity_kJperC * (standardSetpointT_C - standardInletT_C);
        testSummary.standardWaterHeatingEnergy_kJ =
            standardRemovedEnergy_kJ / testSummary.recoveryEfficiency;
    }

    // find the "Modified Daily Water Heating Energy Consumption (Q_dm = Q_da - Q_HWD) (p.
    // 40487) note: same as Q_HW,T
    double waterHeatingDifferenceEnergy_kJ =
        testSummary.standardWaterHeatingEnergy_kJ - testSummary.waterHeatingEnergy_kJ; // Q_HWD
    testSummary.modifiedConsumedWaterHeatingEnergy_kJ =
        testSummary.adjustedConsumedWaterHeatingEnergy_kJ + waterHeatingDifferenceEnergy_kJ;

    // find the "Uniform Energy Factor" (6.4.4)
    testSummary.EF = 0.;
    if (testSummary.modifiedConsumedWaterHeatingEnergy_kJ > 0.)
    {
        testSummary.EF =
            standardDeliveredEnergy_kJ / testSummary.modifiedConsumedWaterHeatingEnergy_kJ;
    }

    // find the "Annual Energy Consumption" (6.4.5)
    testSummary.annualConsumedEnergy_kJ = 0.;
    if (testSummary.EF > 0.)
    {
        constexpr double days_per_year = 365.;
        const double nominalDifferenceT_C = F_TO_C(67.);
        testSummary.annualConsumedEnergy_kJ =
            days_per_year * removedHeatCapacity_kJperC * nominalDifferenceT_C / testSummary.EF;
    }

    // find the "Annual Electrical Energy Consumption" (6.4.6)
    testSummary.annualConsumedElectricalEnergy_kJ = 0.;
    if (testSummary.usedEnergy_kJ > 0.)
    {
        testSummary.annualConsumedElectricalEnergy_kJ =
            (testSummary.usedElectricalEnergy_kJ / testSummary.usedEnergy_kJ) *
            testSummary.annualConsumedEnergy_kJ;
    }
}

//-----------------------------------------------------------------------------
///	@brief	Modify *this hpwh to meet target metrics
//-----------------------------------------------------------------------------
void HPWH::makeGenericE50_UEF_E95(double targetE50,
                                  double targetUEF,
                                  double targetE95,
                                  TestOptions& testOptions)
{
    testOptions.testConfiguration = testConfiguration_E50;
    makeGenericEF(targetE50, testOptions);

    testOptions.testConfiguration = testConfiguration_UEF;
    makeGenericEF(targetUEF, testOptions);

    testOptions.testConfiguration = testConfiguration_E95;
    makeGenericEF(targetE95, testOptions);
}

//-----------------------------------------------------------------------------
///	@brief	first-hour rating as a verbose string
//-----------------------------------------------------------------------------
std::string HPWH::FirstHourRating::report()
{
    std::string results = "";
    results.append("\tFirst-Hour Rating:\n");
    results.append(fmt::format("\t\tVolume Drawn (L): {:g}\n", drawVolume_L));
    results.append(fmt::format("\t\tDesignation: {}\n", sDesigMap[desig]));
    return results;
}

//-----------------------------------------------------------------------------
///	@brief	24-hr test summary as a verbose string
//-----------------------------------------------------------------------------
std::string HPWH::TestSummary::report()
{
    std::string results = "";
    results.append("\t24-Hour Test Results:\n");
    if (!qualifies)
    {
        results.append("\t\tDoes not qualify as consumer water heater.\n");
    }

    results.append(fmt::format("\t\tRecovery Efficiency: {:g}\n", recoveryEfficiency));

    results.append(fmt::format("\t\tStandby Loss Coefficient (kJ/h degC): {:g}\n",
                               standbyLossCoefficient_kJperhC));

    results.append(fmt::format("\t\tEF: {:g}\n", EF));

    results.append(fmt::format("\t\tAverage Inlet Temperature (degC): {:g}\n", avgInletT_C));

    results.append(fmt::format("\t\tAverage Outlet Temperature (degC): {:g}\n", avgOutletT_C));

    results.append(fmt::format("\t\tTotal Volume Drawn (L): {:g}\n", removedVolume_L));

    results.append(fmt::format("\t\tDaily Water-Heating Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(waterHeatingEnergy_kJ)));

    results.append(fmt::format("\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(adjustedConsumedWaterHeatingEnergy_kJ)));

    results.append(fmt::format("\t\tModified Daily Water-Heating Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(modifiedConsumedWaterHeatingEnergy_kJ)));

    results.append("\tAnnual Values:\n");
    results.append(fmt::format("\t\tAnnual Electrical Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(annualConsumedElectricalEnergy_kJ)));

    results.append(fmt::format("\t\tAnnual Energy Consumption (kWh): {:g}\n",
                               KJ_TO_KWH(annualConsumedEnergy_kJ)));

    return results;
}

//-----------------------------------------------------------------------------
///	@brief	Measure the EF and other results of standard 24-hr tests
//-----------------------------------------------------------------------------
void HPWH::measureMetrics(TestOptions& testOptions, TestSummary& testSummary)
{
    if (testOptions.saveOutput)
    {
        std::string sFullOutputFilename =
            testOptions.sOutputDirectory + "/" + testOptions.sOutputFilename;
        testOptions.outputFile.open(sFullOutputFilename.c_str(), std::ifstream::out);
        if (!testOptions.outputFile.is_open())
        {
            send_error(fmt::format("Could not open output file {}", sFullOutputFilename));
        }

        std::string strPreamble;
        std::string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";

        int csvOptions = HPWH::CSVOPT_NONE;
        WriteCSVHeading(
            testOptions.outputFile, sHeader.c_str(), testOptions.nTestTCouples, csvOptions);
    }

    run24hrTest(testOptions, testSummary);

    if (testOptions.saveOutput)
    {
        testOptions.outputFile.close();
    }
}

//-----------------------------------------------------------------------------
///	@brief	Make a generic model with target EF from the current model
/// by varying COP coef's
//-----------------------------------------------------------------------------
void HPWH::makeGenericEF(double targetEF, HPWH::TestOptions& testOptions)
{
    // set up metrics
    std::vector<std::shared_ptr<Fitter::Metric>> pMetrics = {};
    auto ef_metric =
        std::make_shared<HPWH::Fitter::EF_Metric>(targetEF, &testOptions, get_courier(), this);
    pMetrics.push_back(ef_metric);

    auto& compressor = heatSources[compressorIndex];

    // pick the nearest temperature index
    int nPerfPts = static_cast<int>(compressor.perfMap.size());
    int i0 = 0, i1 = 0;
    for (auto& perfPoint : compressor.perfMap)
    {
        if (testOptions.testConfiguration.ambientT_C < F_TO_C(perfPoint.T_F))
            break;
        i0 = i1;
        ++i1;
    }
    double ratio = 0.;
    if ((i1 > i0) && (i1 < nPerfPts))
    {
        ratio = (testOptions.testConfiguration.ambientT_C - compressor.perfMap[i0].T_F) /
                (compressor.perfMap[i1].T_F - compressor.perfMap[i0].T_F);
    }
    int i_ambientT = (ratio < 0.5) ? i0 : i1;

    // set up parameters
    std::vector<std::shared_ptr<Fitter::Param>> pParams;
    auto copCoeff0 = std::make_shared<HPWH::Fitter::COP_Coef>(i_ambientT, 0, get_courier(), this);
    pParams.push_back(copCoeff0);

    // auto copCoeff1 = std::make_shared<HPWH::Fitter::COP_Coef>(i_ambientT, 1, get_courier(),
    // this); pParams.push_back(copCoeff1);

    Fitter fitter(pMetrics, pParams, get_courier());
    fitter.fit();

    double input_BTUperHr, cap_BTUperHr, cop1, cop;
    compressor.getCapacity(testOptions.testConfiguration.ambientT_C,
                           compressor.maxSetpoint_C,
                           testOptions.setpointT_C,
                           input_BTUperHr,
                           cap_BTUperHr,
                           cop1);
    if (cop1 < 0.)
        send_error("COP is negative at maximum condenser temperature.");

    compressor.getCapacity(testOptions.testConfiguration.ambientT_C,
                           0., /// low condenserT_C
                           testOptions.setpointT_C,
                           input_BTUperHr,
                           cap_BTUperHr,
                           cop);
    if (cop < cop1)
        send_error("COP slope is positive.");
}
