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

void resample(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    if (sampleValues.empty())
        return;
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
}

//-----------------------------------------------------------------------------
///	@brief	Resample an extensive property (e.g., heat)
///	@note	See definition of int resample.
//-----------------------------------------------------------------------------
void resampleExtensive(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    resample(values, sampleValues);
    if (!values.empty())
    {
        double scale =
            static_cast<double>(sampleValues.size()) / static_cast<double>(values.size());
        for (auto& value : values)
            value *= scale;
    }
}

double expitFunc(double x, double offset)
{
    double val;
    val = 1 / (1 + exp(x - offset));
    return val;
}

std::vector<double> normalize(std::vector<double> distribution)
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
    return distribution;
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
HPWH::Temp_d_t findShrinkage_dT(const std::vector<double>& nodeDist)
{
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

    HPWH::Temp_d_t alpha_dT(1., Units::dC), beta_dT(2., Units::dC);
    return alpha_dT + standard_condentropy * beta_dT;
}

//-----------------------------------------------------------------------------
///	@brief	Calculates a thermal distribution for heat distribution.
/// @note	Fails if all nodeTemp_C values exceed setpointT
/// @param[out]	thermalDist		resulting thermal distribution; does not require pre-allocation
/// @param[in]	shrinkageT_C	width of distribution
/// @param[in]	lowestNode		index of lowest non-zero contribution
/// @param[in]	nodeTemp_C		node temperatures
/// @param[in]	setpointT_C		distribution parameter
//-----------------------------------------------------------------------------
std::vector<double> calcThermalDist(HPWH::Temp_d_t shrinkage_dT,
                                    int lowestNode,
                                    const HPWH::TempVect_t& nodeTs,
                                    const HPWH::Temp_t setpointT)
{
    std::vector<double> thermalDist(nodeTs.size());

    // Populate the vector of heat distribution
    double totDist = 0.;
    for (int i = 0; i < static_cast<int>(nodeTs.size()); i++)
    {
        double dist = 0.;
        if (i >= lowestNode)
        {
            HPWH::Temp_d_t standardOffset_dT(1.0, Units::dC);
            HPWH::Temp_d_t offset_dT(5.0, Units::dF);
            double offset = offset_dT / standardOffset_dT; // dimensionless
            dist = expitFunc((nodeTs[i] - nodeTs[lowestNode]) / shrinkage_dT, offset);
            dist *= (setpointT - nodeTs[i])();
            if (dist < 0.)
                dist = 0.;
        }
        thermalDist[i] = dist;
        totDist += dist;
    }

    if (totDist > 0.)
    {
        thermalDist = normalize(thermalDist);
    }
    else
    {
        thermalDist.assign(thermalDist.size(), 1. / static_cast<double>(thermalDist.size()));
    }
    return thermalDist;
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

double linearInterp(double xnew, double x0, double x1, double y0, double y1)
{
    return y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

double factorial(const int n)
{
    double f = 1.;
    for (int i = 1; i <= n; ++i)
        f *= i;
    return f;
}

double choose(const int n, const int i)
{
    double f = 0.;
    if (i <= n)
    {
        f = factorial(n) / factorial(i) / factorial(n - i);
    }
    return f;
}

double regressedMethod(const std::vector<double>& coeffs, double x1, double x2, double x3)
{
    return coeffs[0] + coeffs[1] * x1 + coeffs[2] * x2 + coeffs[3] * x3 + coeffs[4] * x1 * x1 +
           coeffs[5] * x2 * x2 + coeffs[6] * x3 * x3 + coeffs[7] * x1 * x2 + coeffs[8] * x1 * x3 +
           coeffs[9] * x2 * x3 + coeffs[10] * x1 * x2 * x3;
}

double regressedMethodMP(const std::vector<double>& coeffs, double x1, double x2)
{
    // Const Tair Tin Tair2 Tin2 TairTin
    return coeffs[0] + coeffs[1] * x1 + coeffs[2] * x2 + coeffs[3] * x1 * x1 + coeffs[4] * x2 * x2 +
           coeffs[5] * x1 * x2;
}

const std::vector<int> HPWH::powers3 = {0, 1, 2};

const std::vector<std::pair<int, int>> HPWH::powers6 = {
    {0, 0}, {1, 0}, {0, 1}, {2, 0}, {0, 2}, {1, 1}};

const std::vector<std::tuple<int, int, int>> HPWH::powers11 = {{0, 0, 0},
                                                               {1, 0, 0},
                                                               {0, 1, 0},
                                                               {0, 0, 1},
                                                               {2, 0, 0},
                                                               {0, 2, 0},
                                                               {0, 0, 2},
                                                               {1, 1, 0},
                                                               {1, 0, 1},
                                                               {0, 1, 1},
                                                               {1, 1, 1}};

std::vector<double> changeSeriesUnitsTemp3(const std::vector<double> coeffs,
                                           const Units::Temp fromUnits,
                                           const Units::Temp toUnits)
{
    auto newCoeffs = coeffs;
    if (fromUnits == toUnits)
    {
        return newCoeffs;
    }

    auto t = Units::scaleShift(toUnits, fromUnits);
    double alpha = t.shift()(), beta = t.scale()();

    auto& powers = HPWH::powers3;
    for (std::size_t i = 0; i < coeffs.size(); ++i)
    {
        newCoeffs[i] = 0.;
        int power_i = powers[i];
        for (std::size_t j = 0; j < coeffs.size(); ++j)
        {
            int power_j = powers[j];
            double fac = choose(power_j, power_i) * std::pow(alpha, power_j - power_i) *
                         std::pow(beta, power_i);

            newCoeffs[i] += fac * coeffs[j];
        }
    }
    return newCoeffs;
}

std::vector<double> changeSeriesUnitsTemp6(const std::vector<double> coeffs,
                                           const Units::Temp fromUnits,
                                           const Units::Temp toUnits)
{
    auto newCoeffs = coeffs;
    if (fromUnits == toUnits)
    {
        return newCoeffs;
    }

    auto t = Units::scaleShift(toUnits, fromUnits);
    double alpha = t.shift()(), beta = t.scale()();

    auto& powers = HPWH::powers6;
    for (std::size_t i = 0; i < coeffs.size(); ++i)
    {
        newCoeffs[i] = 0.;
        int power_i0 = std::get<0>(powers[i]);
        int power_i1 = std::get<1>(powers[i]);
        for (std::size_t j = 0; j < coeffs.size(); ++j)
        {
            int power_j0 = std::get<0>(powers[j]);
            int power_j1 = std::get<1>(powers[j]);

            double fac0 = choose(power_j0, power_i0) * std::pow(alpha, power_j0 - power_i0) *
                          std::pow(beta, power_i0);

            double fac1 = choose(power_j1, power_i1) * std::pow(alpha, power_j1 - power_i1) *
                          std::pow(beta, power_i1);

            newCoeffs[i] += fac0 * fac1 * coeffs[j];
        }
    }
    return newCoeffs;
}

std::vector<double> changeSeriesUnitsTemp11(const std::vector<double> coeffs,
                                            const Units::Temp fromUnits,
                                            const Units::Temp toUnits)
{
    auto newCoeffs = coeffs;
    if (fromUnits == toUnits)
    {
        return newCoeffs;
    }

    auto t = Units::scaleShift(toUnits, fromUnits);
    double alpha = t.shift()(), beta = t.scale()();

    auto& powers = HPWH::powers11;
    for (std::size_t i = 0; i < coeffs.size(); ++i)
    {
        newCoeffs[i] = 0.;
        int power_i0 = std::get<0>(powers[i]);
        int power_i1 = std::get<1>(powers[i]);
        int power_i2 = std::get<2>(powers[i]);
        for (std::size_t j = 0; j < coeffs.size(); ++j)
        {
            int power_j0 = std::get<0>(powers[j]);
            int power_j1 = std::get<1>(powers[j]);
            int power_j2 = std::get<2>(powers[j]);

            double fac0 = choose(power_j0, power_i0) * std::pow(alpha, power_j0 - power_i0) *
                          std::pow(beta, power_i0);

            double fac1 = choose(power_j1, power_i1) * std::pow(alpha, power_j1 - power_i1) *
                          std::pow(beta, power_i1);

            double fac2 = choose(power_j2, power_i2) * std::pow(alpha, power_j2 - power_i2) *
                          std::pow(beta, power_i2);

            newCoeffs[i] += fac0 * fac1 * fac2 * coeffs[j];
        }
    }
    return newCoeffs;
}

HPWH::PerfPoint::PerfPoint(std::pair<double, Units::Temp> T_in,
                           PowerVect_t inputPower_coeffs_in, // with appropriate temperature factors
                           std::vector<double> COP_coeffs_in)
    : T(T_in.first, T_in.second), inputPower_coeffs(inputPower_coeffs_in), COP_coeffs(COP_coeffs_in)
{
    auto unitsTemp_in = T_in.second;
    if (inputPower_coeffs.size() == 3) // use expandSeries
    {
        inputPower_coeffs() =
            changeSeriesUnitsTemp3(inputPower_coeffs_in(), unitsTemp_in, UnitsTemp);
        COP_coeffs = changeSeriesUnitsTemp3(COP_coeffs_in, unitsTemp_in, UnitsTemp);
        return;
    }

    if (inputPower_coeffs.size() == 11) // use regressMethod
    {
        inputPower_coeffs() = changeSeriesUnitsTemp11(inputPower_coeffs(), unitsTemp_in, UnitsTemp);
        COP_coeffs = changeSeriesUnitsTemp11(COP_coeffs, unitsTemp_in, UnitsTemp);
        return;
    }

    if (inputPower_coeffs.size() == 6) // use regressMethodMP
    {
        inputPower_coeffs() = changeSeriesUnitsTemp6(inputPower_coeffs(), unitsTemp_in, UnitsTemp);
        COP_coeffs = changeSeriesUnitsTemp6(COP_coeffs, unitsTemp_in, UnitsTemp);
        return;
    }
}

void HPWH::setStepTime(const Time_t stepTime_in) { stepTime = stepTime_in; }

// public HPWH functions
HPWH::HPWH(const std::shared_ptr<Courier::Courier>& courier, const std::string& name_in /*"hpwh"*/)
    : Sender("HPWH", name_in, courier)
{
    setAllDefaults();
}

void HPWH::setAllDefaults()
{
    tankTs.clear();
    nextTankTs.clear();
    heatSources.clear();

    isHeating = false;
    setpointFixed = false;
    tankSizeFixed = true;
    canScale = false;
    member_inletT = Temp_t(-1., Units::F); // invalid unit setInletT called
    haveInletT = false;
    currentSoCFraction = 1.;
    doTempDepression = false;
    locationT = UNINITIALIZED_LOCATIONTEMP();
    mixBelowFractionOnDraw = 1. / 3.;
    doInversionMixing = true;
    doConduction = true;
    inletHeight = 0;
    inlet2Height = 0;
    fittingsUA() = 0.;
    prevDRstatus = DR_ALLOW;
    timerLimitTOT = 60.;
    timerTOT = 0.;
    usesSoCLogic = false;
    stepTime = Time_t(1.0, Units::min);
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

    tankVolume = hpwh.tankVolume;
    tankUA = hpwh.tankUA;
    fittingsUA = hpwh.fittingsUA;

    setpointT = hpwh.setpointT;

    tankTs = hpwh.tankTs;
    nextTankTs = hpwh.nextTankTs;

    inletHeight = hpwh.inletHeight;
    inlet2Height = hpwh.inlet2Height;

    outletT = hpwh.outletT;
    condenserInletT = hpwh.condenserInletT;
    condenserOutletT = hpwh.condenserOutletT;
    externalVolumeHeated = hpwh.externalVolumeHeated;
    energyRemovedFromEnvironment = hpwh.energyRemovedFromEnvironment;
    standbyLosses = hpwh.standbyLosses;

    tankMixesOnDraw = hpwh.tankMixesOnDraw;
    mixBelowFractionOnDraw = hpwh.mixBelowFractionOnDraw;

    doTempDepression = hpwh.doTempDepression;

    doInversionMixing = hpwh.doInversionMixing;
    doConduction = hpwh.doConduction;

    locationT = hpwh.locationT;

    prevDRstatus = hpwh.prevDRstatus;
    timerLimitTOT = hpwh.timerLimitTOT;

    usesSoCLogic = hpwh.usesSoCLogic;

    nodeVolume = hpwh.nodeVolume;
    nodeHeight = hpwh.nodeHeight;
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

void HPWH::runOneStep(Volume_t drawVolume,
                      Temp_t tankAmbientT,
                      Temp_t heatSourceAmbientT,
                      DRMODES DRstatus,
                      Volume_t inlet2Vol,
                      Temp_t inlet2T,
                      PowerVect_t* extraHeatDist)
{

    // check for errors
    if (doTempDepression && (stepTime(Units::min) != 1))
    {
        send_error("minutesPerStep must equal one for temperature depression to work.");
    }

    // reset the output variables
    outletT = Temp_t(0., Units::C);
    condenserInletT = Temp_t(0., Units::C);
    condenserOutletT = Temp_t(0., Units::C);
    externalVolumeHeated() = 0.;
    energyRemovedFromEnvironment() = 0.;
    standbyLosses() = 0.;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime() = 0;
        heatSources[i].energyInput() = 0.;
        heatSources[i].energyOutput() = 0.;
    }
    extraEnergyInput() = 0.;

    // if you are doing temp. depression, set tank and heatSource ambient temps
    // to the tracked locationTemperature
    Temp_t temperatureGoal = tankAmbientT;
    if (doTempDepression)
    {
        if (locationT == UNINITIALIZED_LOCATIONTEMP())
        {
            locationT = tankAmbientT;
        }
        tankAmbientT = locationT;
        heatSourceAmbientT = locationT;
    }

    // process draws and standby losses
    updateTankTemps(drawVolume, member_inletT, tankAmbientT, inlet2Vol, inlet2T);

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
        Time_t availableTime = stepTime;
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
                heatSources[i].toLockOrUnlock(heatSourceAmbientT);
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
                    if (heatSources[i].backupHeatSource->toLockOrUnlock(heatSourceAmbientT) ||
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

                addHeatParent(heatSourcePtr, heatSourceAmbientT, availableTime);

                // if it finished early. i.e. shuts off early like if the heatsource met setpoint or
                // maxed out
                // static const Time_t minRuntime = 0.;
                if (availableTime > heatSourcePtr->runtime)
                {
                    // subtract time it ran and turn it off
                    availableTime -= heatSourcePtr->runtime;
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
                                heatSourceAmbientT) && // If not locked out
                            !shouldDRLockOut(heatSources[i].backupHeatSource->typeOfHeatSource,
                                             DRstatus) && // and not DR locked out
                            !heatSources[i].backupHeatSource->isEngaged())
                        { // and not already engaged

                            HeatSource* backupHeatSourcePtr = heatSources[i].backupHeatSource;
                            // turn it on
                            backupHeatSourcePtr->engageHeatSource(DRstatus);
                            // add heat if it hasn't heated up this whole minute already
                            if (backupHeatSourcePtr->runtime >= 0.)
                            {
                                addHeatParent(backupHeatSourcePtr,
                                              heatSourceAmbientT,
                                              availableTime - backupHeatSourcePtr->runtime);
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
    if ((extraHeatDist != NULL) && (extraHeatDist->size() != 0))
    {
        addExtraHeat(*extraHeatDist);
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
            temperatureGoal -= maxDepression_dT; // hardcoded 4.5 degree total drop - from
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
        locationT -= Temp_d_t((1 - 0.9289) * (locationT - temperatureGoal));
    }

    // sum energyRemovedFromEnvironment_kWh for each heat source;
    for (int i = 0; i < getNumHeatSources(); i++)
    {
        energyRemovedFromEnvironment += (heatSources[i].energyOutput - heatSources[i].energyInput);
    }

#if !NDEBUG
    // cursory check for inverted temperature profile
    if (tankTs[getNumNodes() - 1] < tankTs[0])
    {
        send_debug("The top of the tank is cooler than the bottom.");
    }
#endif

    // Handle DR timer
    prevDRstatus = DRstatus;
    // DR check for TOT to increase timer.
    timerTOT += stepTime(Units::min);
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
                     Temp_t* inletT,
                     Volume_t* drawVolume,
                     Temp_t* tankAmbientT,
                     Temp_t* heatSourceAmbientT,
                     DRMODES* DRstatus)
{
    // these are all the accumulating variables we'll need
    Energy_t energyRemovedFromEnvironment_SUM(0.);
    Energy_t standbyLosses_SUM(0.);
    double outletT_AVG = 0;
    Volume_t totalDrawVolume(0.);
    std::vector<Time_t> heatSources_runTimes_SUM(getNumHeatSources());
    std::vector<Energy_t> heatSources_energyInputs_SUM(getNumHeatSources());
    std::vector<Energy_t> heatSources_energyOutputs_SUM(getNumHeatSources());

    // run the sim one step at a time, accumulating the outputs as you go
    for (int i = 0; i < N; i++)
    {
        runOneStep(inletT[i], drawVolume[i], tankAmbientT[i], heatSourceAmbientT[i], DRstatus[i]);

        energyRemovedFromEnvironment_SUM += energyRemovedFromEnvironment;
        standbyLosses_SUM += standbyLosses;

        Volume_t vol_i = drawVolume[i];
        outletT_AVG += outletT() * vol_i();
        totalDrawVolume += vol_i;

        for (int j = 0; j < getNumHeatSources(); j++)
        {
            heatSources_runTimes_SUM[j] += getNthHeatSourceRunTime(j);
            heatSources_energyInputs_SUM[j] += getNthHeatSourceEnergyInput(j);
            heatSources_energyOutputs_SUM[j] += getNthHeatSourceEnergyOutput(j);
        }
    }
    // finish weighted avg. of outlet temp by dividing by the total drawn volume
    outletT_AVG /= totalDrawVolume();

    // now, reassign all accumulated values to their original spots
    energyRemovedFromEnvironment = energyRemovedFromEnvironment_SUM;
    standbyLosses = standbyLosses_SUM;
    outletT() = outletT_AVG;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime = heatSources_runTimes_SUM[i];
        heatSources[i].energyInput = heatSources_energyInputs_SUM[i];
        heatSources[i].energyOutput = heatSources_energyOutputs_SUM[i];
    }
}

void HPWH::addHeatParent(HeatSource* heatSourcePtr, Temp_t heatSourceAmbientT, Time_t availableTime)
{
    Temp_t tempSetpointT(-273.15, Units::C);

    // Check the air temprature and setpoint against maxOut_at_LowT
    if (heatSourcePtr->isACompressor())
    {
        if (heatSourceAmbientT <= heatSourcePtr->maxOut_at_LowT.airT &&
            setpointT >= heatSourcePtr->maxOut_at_LowT.outT)
        {
            tempSetpointT = setpointT; // Store setpoint
            setSetpointT(heatSourcePtr->maxOut_at_LowT
                             .outT); // Reset to new setpoint as this is used in the add heat calc
        }
    }
    // and add heat if it is
    heatSourcePtr->addHeat(heatSourceAmbientT, availableTime);

    // Change the setpoint back to what it was pre-compressor depression
    if (tempSetpointT(Units::C) > -273.15)
    {
        setSetpointT(tempSetpointT);
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
                               getNthHeatSourceEnergyInput(iHS)(Units::kWh) * 1000.,
                               getNthHeatSourceEnergyOutput(iHS)(Units::kWh) * 1000.);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        outFILE << fmt::format(",{:0.2f}",
                               getNthSimTcouple(iTC + 1, nTCouples)(doIP ? Units::F : Units::C));
    }

    if (options & HPWH::CSVOPT_IS_DRAWING)
    {
        outFILE << fmt::format(",{:0.2f}", outletT(doIP ? Units::F : Units::C));
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
    outFILE << fmt::format("{:d}", outputData.time(Units::min));
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? outputData.ambientT(Units::F) : outputData.ambientT(Units::C));
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? outputData.setpointT(Units::C) : outputData.setpointT(Units::C));
    outFILE << fmt::format(",{:0.2f}",
                           doIP ? outputData.inletT(Units::F) : outputData.inletT(Units::C));
    outFILE << fmt::format(
        ",{:0.2f}", doIP ? outputData.drawVolume(Units::gal) : outputData.drawVolume(Units::L));
    outFILE << fmt::format(",{}", static_cast<int>(outputData.drMode));

    //
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        outFILE << fmt::format(",{:0.2f},{:0.2f}",
                               outputData.h_srcIn[iHS](Units::kWh) * 1000.,
                               outputData.h_srcOut[iHS](Units::kWh) * 1000.);
    }

    //
    for (auto thermocoupleT : outputData.thermocoupleT)
    {
        outFILE << fmt::format(",{:0.2f}",
                               doIP ? thermocoupleT(Units::F) : thermocoupleT(Units::C));
    }

    //
    if (outputData.drawVolume > 0.)
    {
        outFILE << fmt::format(",{:0.2f}", outputData.outletT(doIP ? Units::F : Units::C));
    }
    else
    {
        outFILE << ",";
    }

    outFILE << std::endl;
    return 0;
}

bool HPWH::isSetpointFixed() const { return setpointFixed; }

void HPWH::setSetpointT(Temp_t newSetpointT)
{
    Temp_t maxAllowedSetpointT;
    string why;
    if (isNewSetpointPossible(newSetpointT, maxAllowedSetpointT, why))
    {
        setpointT = newSetpointT;
    }
    else
    {
        send_error(fmt::format("Cannot set this setpoint for the currently selected model, "
                               "max setpoint is {:0.2f} C. {}",
                               maxAllowedSetpointT(Units::C),
                               why.c_str()));
    }
}

HPWH::Temp_t HPWH::getSetpointT() const { return setpointT; }

HPWH::Temp_t HPWH::getMaxCompressorSetpointT() const
{
    if (!hasACompressor())
    {
        send_error("Unit does not have a compressor.");
    }
    return heatSources[compressorIndex].maxSetpointT;
}

bool HPWH::isNewSetpointPossible(Temp_t newSetpointT,
                                 Temp_t& maxAllowedSetpointT,
                                 string& why) const
{
    maxAllowedSetpointT = {-273.15, Units::C};

    bool returnVal = false;

    if (isSetpointFixed())
    {
        returnVal = (newSetpointT == setpointT);
        maxAllowedSetpointT = setpointT;
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

            maxAllowedSetpointT =
                heatSources[compressorIndex].maxSetpointT -
                heatSources[compressorIndex].secondaryHeatExchanger.hotSideOffset_dT;

            if (newSetpointT > maxAllowedSetpointT && lowestElementIndex == -1)
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
            maxAllowedSetpointT = heatSources[lowestElementIndex].maxSetpointT;

            if (newSetpointT > maxAllowedSetpointT)
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
    return returnVal;
}

double HPWH::calcSoCFraction(Temp_t mainsT, Temp_t minUsefulT, Temp_t maxT) const
{
    // Note that volume is ignored in here since with even nodes it cancels out of the SoC
    // fractional equation
    if (mainsT >= minUsefulT)
    {
        send_warning("mainsT is greater than or equal to minUsefulT.");
    }
    if (minUsefulT > maxT)
    {
        send_warning("minUsefulT is greater maxT.");
    }

    double chargeEquivalent = 0.;
    for (auto& T : tankTs)
    {
        chargeEquivalent += getChargePerNode(mainsT, minUsefulT, T);
    }
    double maxSoC = getNumNodes() * getChargePerNode(mainsT, minUsefulT, maxT);
    return chargeEquivalent / maxSoC;
}

double HPWH::getSoCFraction() const { return currentSoCFraction; }

void HPWH::calcAndSetSoCFraction()
{
    double newSoCFraction = -1.;

    std::shared_ptr<SoCBasedHeatingLogic> logicSoC =
        std::dynamic_pointer_cast<SoCBasedHeatingLogic>(
            heatSources[compressorIndex].turnOnLogicSet[0]);
    newSoCFraction = calcSoCFraction(logicSoC->getMainsT(), logicSoC->getMinUsefulT());

    currentSoCFraction = newSoCFraction;
}

double HPWH::getChargePerNode(Temp_t coldT, Temp_t mixT, Temp_t hotT) const
{
    if (hotT < mixT)
    {
        return 0.;
    }
    return (hotT - coldT) / (mixT - coldT);
}

HPWH::Temp_t HPWH::getMinOperatingT() const
{
    if (!hasACompressor())
    {
        send_error("No compressor found in this HPWH.");
    }
    return heatSources[compressorIndex].minT;
}

void HPWH::resetTankToSetpoint() { setTankToT(setpointT); }

//-----------------------------------------------------------------------------
///	@brief	Assigns new temps provided from a std::vector to tankTs.
/// @param[in]	setTankTemps	new tank temps (arbitrary non-zero size)
///	@param[in]	units          temp units in setTankTemps (default = UNITS_C)
//-----------------------------------------------------------------------------
void HPWH::setTankTs(const TempVect_t& setTankTs)
{
    std::size_t numSetNodes = setTankTs.size();
    if (numSetNodes == 0)
    {
        send_error("No temperatures provided.");
    }

    // set node temps
    resampleIntensive(tankTs(), setTankTs());
}

void HPWH::getTankTs(TempVect_t& tankTemps) { tankTemps = tankTs; }

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

// Uses the UA before the function is called and adjusts the A part of the UA to match
// the input volume given getTankSurfaceArea().
int HPWH::setTankSizeAdjustUA(Volume_t vol, bool forceChange /*=false*/)
{
    double oldTankU_kJperhCm2 = tankUA(Units::kJ_per_hC) / getTankSurfaceArea()(Units::m2);

    setTankSize(vol, forceChange);
    tankUA = {oldTankU_kJperhCm2 * getTankSurfaceArea()(Units::m2), Units::kJ_per_hC};
    return 0;
}

/*static*/ HPWH::Area_t HPWH::getTankSurfaceArea(const Volume_t vol)
{
    // returns tank surface area, old defualt was in ft2
    // Based off 88 insulated storage tanks currently available on the market from Sanden,
    // AOSmith, HTP, Rheem, and Niles. Corresponds to the inner tank with volume
    // tankVolume_L with the assumption that the aspect ratio is the same as the outer
    // dimenisions of the whole unit.
    Length_t radius = getTankRadius(vol);
    return {2. * Pi * pow(radius(Units::m), 2) * (ASPECTRATIO + 1.), Units::m2};
}

HPWH::Area_t HPWH::getTankSurfaceArea() const { return getTankSurfaceArea(tankVolume); }

/*static*/ HPWH::Length_t HPWH::getTankRadius(const Volume_t vol)
{ // returns tank radius, ft for use in calculation of heat loss in the bottom and top of the tank.
    // Based off 88 insulated storage tanks currently available on the market from Sanden, AOSmith,
    // HTP, Rheem, and Niles, assumes the aspect ratio for the outer measurements is the same is the
    // actual tank.

    if (vol < 0.)
        return Length_t(0.);
    return {std::pow(vol(Units::m3) / Pi / ASPECTRATIO, 1. / 3.), Units::m};
}

HPWH::Length_t HPWH::getTankRadius() const
{
    // returns tank radius, ft for use in calculation of heat loss in the bottom and top of the
    // tank. Based off 88 insulated storage tanks currently available on the market from Sanden,
    // AOSmith, HTP, Rheem, and Niles, assumes the aspect ratio for the outer measurements is the
    // same is the actual tank.

    Length_t radius = getTankRadius(tankVolume);
    if (radius < 0.)
    {
        send_error("Negative value for getTankRadius.");
    }
    return radius;
}

bool HPWH::isTankSizeFixed() const { return tankSizeFixed; }

void HPWH::setTankSize(const Volume_t vol, bool forceChange /*=false*/)
{
    if (isTankSizeFixed() && !forceChange)
    {
        send_error("Can not change the tank size for your currently selected model.");
    }
    if (vol <= 0)
    {
        send_error("You have attempted to set the tank volume outside of bounds.");
    }
    else
    {
        tankVolume = vol;
    }
    calcSizeConstants();
}

void HPWH::setDoInversionMixing(bool doInversionMixing_in)
{
    doInversionMixing = doInversionMixing_in;
}

void HPWH::setDoConduction(bool doConduction_in) { doConduction = doConduction_in; }

void HPWH::setUA(const UA_t UA) { tankUA = UA; }

void HPWH::getUA(UA_t& UA) const { UA = tankUA; }

void HPWH::setFittingsUA(const UA_t UA) { fittingsUA = UA; }

void HPWH::getFittingsUA(UA_t& UA) const { UA = fittingsUA; }

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

void HPWH::setMaxDepression_dT(Temp_d_t maxDepression_dT_in)
{
    maxDepression_dT = maxDepression_dT_in;
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

void HPWH::setEnteringWaterHighTempShutOff(GenTemp_t highT, int heatSourceIndex)
{
    if (!hasEnteringWaterHighTempShutOff(heatSourceIndex))
    {
        send_error("You have attempted to access a heating logic that does not exist.");
    }

    bool highTempIsNotValid = false;
    if (highT.index() == 0)
    {
        // check difference with setpoint
        if (setpointT(Units::C) - std::get<Temp_t>(highT)(Units::C) <
            MINSINGLEPASSLIFT()(Units::dC))
        {
            highTempIsNotValid = true;
        }
    }
    else
    {
        auto dT = std::get<Temp_d_t>(highT);
        if (dT < MINSINGLEPASSLIFT())
        {
            highTempIsNotValid = true;
        }
    }

    if (highTempIsNotValid)
    {
        send_error(fmt::format("High temperature shut off is too close to the setpoint, expected "
                               "a minimum difference of {:g}",
                               MINSINGLEPASSLIFT()(Units::dC)));
    }

    for (const std::shared_ptr<HeatingLogic>& shutOffLogic :
         heatSources[heatSourceIndex].shutOffLogicSet)
    {
        if (shutOffLogic->getIsEnteringWaterHighTempShutoff())
        {
            std::dynamic_pointer_cast<TempBasedHeatingLogic>(shutOffLogic)->setDecisionPoint(highT);
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
        for (auto logic : heatSources[i].shutOffLogicSet)
        {
            if (!logic->getIsEnteringWaterHighTempShutoff())
            {
                std::dynamic_pointer_cast<SoCBasedHeatingLogic>(logic)->setDecisionPoint(target);
            }
        }
        for (auto& logic : heatSources[i].turnOnLogicSet)
        {
            std::dynamic_pointer_cast<SoCBasedHeatingLogic>(logic)->setDecisionPoint(target);
        }
    }
}

bool HPWH::isSoCControlled() const { return usesSoCLogic; }

bool HPWH::canUseSoCControls() const
{
    bool retVal = true;
    if (getCompressorCoilConfig() != HPWH::HeatSource::CONFIG_EXTERNAL)
    {
        retVal = false;
    }
    return retVal;
}

void HPWH::switchToSoCControls(double targetSoC,
                               double hysteresisFraction /*0.05*/,
                               Temp_t minUsefulT /*{43.333, Units::C}*/,
                               bool constantMainsT /*false*/,
                               Temp_t mainsT /*{18.333, Units::C});*/)
{
    if (!canUseSoCControls())
    {
        send_error("Cannot set up state of charge controls for integrated or wrapped HPWHs.");
    }

    if (mainsT >= minUsefulT)
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

        heatSources[i].shutOffLogicSet.push_back(shutOffSoC(
            "SoC Shut Off", targetSoC, hysteresisFraction, minUsefulT, constantMainsT, mainsT));
        heatSources[i].turnOnLogicSet.push_back(turnOnSoC(
            "SoC Turn On", targetSoC, hysteresisFraction, minUsefulT, constantMainsT, mainsT));
    }

    usesSoCLogic = true;
}

std::shared_ptr<HPWH::SoCBasedHeatingLogic> HPWH::turnOnSoC(std::string desc,
                                                            double targetSoC,
                                                            double hystFract,
                                                            Temp_t minUsefulT,
                                                            bool constantMainsT,
                                                            Temp_t mainsT)
{
    return std::make_shared<SoCBasedHeatingLogic>(
        desc, targetSoC, this, -hystFract, minUsefulT, constantMainsT, mainsT);
}

std::shared_ptr<HPWH::SoCBasedHeatingLogic> HPWH::shutOffSoC(string desc,
                                                             double targetSoC,
                                                             double hystFract,
                                                             Temp_t minUsefulT,
                                                             bool constantMainsT,
                                                             Temp_t mainsT)
{
    return std::make_shared<SoCBasedHeatingLogic>(desc,
                                                  targetSoC,
                                                  this,
                                                  hystFract,
                                                  minUsefulT,
                                                  constantMainsT,
                                                  mainsT,
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

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::wholeTank(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "whole tank", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThird(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("top third", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondThird(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(1. / 3., 2. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second third", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomThird(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom third", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::thirdSixth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(2. / 6., 3. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "third sixth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fourthSixth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(3. / 6., 4. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fourth sixth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "fifth sixth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>("top sixth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomHalf(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom half", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfth(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::standby(GenTemp_t decisionT)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>("standby", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topNodeMaxTemp(GenTemp_t decisionT)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(LOGIC_SIZE + 1); // uses very top computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top node", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic>
HPWH::bottomNodeMaxTemp(GenTemp_t decisionT, bool enteringHighTempShutoff /*=false*/)
{
    std::vector<NodeWeight> nodeWeights;
    nodeWeights.emplace_back(0); // uses very bottom computation node
    return std::make_shared<HPWH::TempBasedHeatingLogic>("bottom node",
                                                         nodeWeights,
                                                         decisionT,
                                                         this,
                                                         std::greater<double>(),
                                                         enteringHighTempShutoff);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomTwelfthMaxTemp(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 12.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom twelfth", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topThirdMaxTemp(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(2. / 3., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top third", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::bottomSixthMaxTemp(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "bottom sixth", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::secondSixthMaxTemp(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(1. / 6., 2. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second sixth", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::fifthSixthMaxTemp(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(4. / 6., 5. / 6.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::topSixthMaxTemp(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(5. / 6., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "top sixth", nodeWeights, decisionT, this, std::greater<double>());
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largeDraw(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 4.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "large draw", nodeWeights, decisionT, this);
}

std::shared_ptr<HPWH::TempBasedHeatingLogic> HPWH::largerDraw(GenTemp_t decisionT)
{
    auto nodeWeights = getNodeWeightRange(0., 1. / 2.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "larger draw", nodeWeights, decisionT, this);
}

void HPWH::setNumNodes(const std::size_t num_nodes)
{
    tankTs.resize(num_nodes);
    nextTankTs.resize(num_nodes);
}

int HPWH::getNumNodes() const { return static_cast<int>(tankTs.size()); }

int HPWH::getIndexTopNode() const { return getNumNodes() - 1; }

HPWH::Temp_t HPWH::getTankNodeT(int nodeNum) const
{
    if (tankTs.empty())
    {
        send_error(
            "You have attempted to access the temperature of a tank node that does not exist.");
    }

    return tankTs[nodeNum];
}

HPWH::Temp_t HPWH::getNthSimTcouple(int iTCouple, int nTCouple) const
{
    if (iTCouple > nTCouple || iTCouple < 1)
    {
        send_error("You have attempted to access a simulated thermocouple that does not exist.");
    }

    double beginFraction = static_cast<double>(iTCouple - 1.) / static_cast<double>(nTCouple);
    double endFraction = static_cast<double>(iTCouple) / static_cast<double>(nTCouple);
    auto simTcoupleT = Temp_t(getResampledValue(tankTs(), beginFraction, endFraction));
    return simTcoupleT;
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

HPWH::Power_t HPWH::getCompressorCapacity(Temp_t airT, Temp_t inletT, Temp_t outT)
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }

    // calculate capacity btu/hr, input btu/hr, and cop
    Power_t inputPower, outputPower;
    double cop;

    if (airT < heatSources[compressorIndex].minT || airT > heatSources[compressorIndex].maxT)
    {
        send_error("The compress does not operate at the specified air temperature.");
    }

    double maxAllowedSetpointT =
        heatSources[compressorIndex].maxSetpointT() -
        heatSources[compressorIndex].secondaryHeatExchanger.hotSideOffset_dT();

    if (outT > maxAllowedSetpointT)
    {
        send_error("Inputted outlet temperature of the compressor is higher than can be produced.");
    }

    if (heatSources[compressorIndex].isExternalMultipass())
    {
        Temp_t averageT((outT() + inletT()) / 2.);
        heatSources[compressorIndex].getCapacityMP(airT, averageT, inputPower, outputPower, cop);
    }
    else
    {
        heatSources[compressorIndex].getCapacity(airT, inletT, outT, inputPower, outputPower, cop);
    }

    return outputPower;
}

HPWH::Energy_t HPWH::getNthHeatSourceEnergyInput(int N) const
{
    // energy used by the heat source is positive - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the energy input of a heat source that does not exist.");
    }
    return heatSources[N].energyInput;
}

HPWH::Energy_t HPWH::getNthHeatSourceEnergyOutput(int N) const
{
    // returns energy from the heat source into the water - this should always be positive
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the energy output of a heat source that does not exist.");
    }
    return heatSources[N].energyOutput;
}

HPWH::Time_t HPWH::getNthHeatSourceRunTime(int N) const
{
    if (N >= getNumHeatSources() || N < 0)
    {
        send_error(
            "You have attempted to access the run time of a heat source that does not exist.");
    }
    return heatSources[N].runtime;
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

HPWH::Volume_t HPWH::getTankSize() const { return tankVolume; }

HPWH::Volume_t HPWH::getExternalVolumeHeated() const { return externalVolumeHeated; }

HPWH::Energy_t HPWH::getEnergyRemovedFromEnvironment() const
{
    return energyRemovedFromEnvironment;
}

HPWH::Energy_t HPWH::getStandbyLosses() const
{
    return standbyLosses;
    ;
}

HPWH::Temp_t HPWH::getOutletT() const { return outletT; }

HPWH::Temp_t HPWH::getCondenserWaterInletT() const { return condenserInletT; }

HPWH::Temp_t HPWH::getCondenserWaterOutletT() const { return condenserOutletT; }

HPWH::Temp_t HPWH::getLocationT() const { return locationT; }

//-----------------------------------------------------------------------------
///	@brief	Evaluates the tank temperature averaged uniformly
/// @param[in]	nodeWeights	Discrete set of weighted nodes
/// @return	Tank temperature (C)
//-----------------------------------------------------------------------------
HPWH::Temp_t HPWH::getAverageTankT() const
{
    double totalT(0.);

    for (auto& T : tankTs())
    {
        totalT += T;
    }
    return Temp_t(totalT / static_cast<double>(getNumNodes()));
}

//-----------------------------------------------------------------------------
///	@brief	Evaluates the average tank temperature based on distribution.
/// @note	Distribution must have positive size and be normalized.
/// @param[in]	dist	Discrete set of distribution values
//-----------------------------------------------------------------------------
HPWH::Temp_t HPWH::getAverageTankT(const std::vector<double>& dist) const
{
    std::vector<double> resampledTankTs(dist.size());
    resample(resampledTankTs, tankTs());

    double tankT(0);

    std::size_t j = 0;
    for (auto& nodeT : resampledTankTs)
    {
        tankT += dist[j] * nodeT;
        ++j;
    }
    return Temp_t(tankT);
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
HPWH::Temp_t HPWH::getAverageTankT(const std::vector<HPWH::NodeWeight>& nodeWeights) const
{
    double sum = 0;
    double totWeight = 0;

    std::vector<double> resampledTankTs_t(LOGIC_SIZE);
    resample(resampledTankTs_t, tankTs());

    for (auto& nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum == 0)
        { // bottom node only
            sum += tankTs.front()() * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
        else if (nodeWeight.nodeNum > LOGIC_SIZE)
        { // top node only
            sum += tankTs.back()() * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
        else
        { // general case; sum over all weighted nodes
            sum += resampledTankTs_t[static_cast<std::size_t>(nodeWeight.nodeNum - 1)] *
                   nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
    }
    return Temp_t(sum / totWeight);
}

void HPWH::setTankToT(Temp_t T) { setTankTs({{T(Units::C)}, Units::C}); }

///////////////////////////////////////////////////////////////////////////////////

HPWH::Energy_t HPWH::getTankHeatContent() const
{
    // returns tank heat content relative to 0 C using kJ
    const double nodeHeatCapacity =
        DENSITYWATER_kg_per_L * nodeVolume(Units::L) * CPWATER_kJ_per_kgC;

    Temp_t totalT(0.);
    for (auto& T : tankTs)
    {
        totalT += T();
    }
    return {nodeHeatCapacity * totalT(Units::C), Units::kJ};
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

HPWH::FlowRate_t HPWH::getExternalMPFlowRate() const
{
    if (!isCompressorExternalMultipass())
    {
        send_error("Does not have an external multipass heat source.");
    }
    return heatSources[compressorIndex].mpFlowRate;
}

HPWH::Time_t HPWH::getCompressorMinRuntime() const
{
    if (!hasACompressor())
    {
        send_error("Current model does not have a compressor.");
    }
    return {10., Units::min};
}

int HPWH::getSizingFractions(double& aquaFract, double& useableFract) const
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

    return 0;
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
        scaleVector(perfP.inputPower_coeffs(), scaleCapacity);
        scaleVector(perfP.COP_coeffs, scaleCOP);
    }
}

void HPWH::setCompressorOutputCapacity(Power_t newCapacity, Temp_t airT, Temp_t inletT, Temp_t outT)
{
    Power_t oldCapacity = getCompressorCapacity(airT, inletT, outT);
    double scale = newCapacity / oldCapacity;
    setScaleCapacityCOP(scale, 1.); // Scale the compressor capacity
}

void HPWH::setResistanceCapacity(const Power_t power, int which /*=-1*/)
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
        send_error("Cannot have a negative input power.");
    }

    // Whew so many checks...
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                heatSources[i].changeResistancePower(power);
            }
        }
    }
    else
    {
        heatSources[resistanceHeightMap[which].index].changeResistancePower(power);

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                heatSources[resistanceHeightMap[i].index].changeResistancePower(power);
            }
        }
    }
}

HPWH::Power_t HPWH::getResistanceCapacity(int which /*=-1*/)
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

    Power_t returnPower(0.);
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
void HPWH::updateTankTemps(
    Volume_t drawVolume, Temp_t inletT, Temp_t tankAmbientT, Volume_t inlet2Vol, Temp_t inlet2T)
{
    double nodeCp_kJ_per_C = nodeCp(Units::kJ_per_C);
    if (drawVolume > 0.)
    {
        if (inlet2Vol > drawVolume)
        {
            send_error("Volume in inlet 2 is greater than the draw volume.");
        }

        // sort the inlets by height
        int highInletNodeIndex;
        Temp_t highInletT;
        double highInletFraction; // fraction of draw from high inlet

        int lowInletNodeIndex;
        Temp_t lowInletT;
        double lowInletFraction; // fraction of draw from low inlet

        if (inletHeight > inlet2Height)
        {
            highInletNodeIndex = inletHeight;
            highInletFraction = 1. - inlet2Vol / drawVolume;
            highInletT = inletT;
            lowInletNodeIndex = inlet2Height;
            lowInletT = inlet2T;
            lowInletFraction = inlet2Vol / drawVolume;
        }
        else
        {
            highInletNodeIndex = inlet2Height;
            highInletFraction = inlet2Vol / drawVolume;
            highInletT = inlet2T;
            lowInletNodeIndex = inletHeight;
            lowInletT = inletT;
            lowInletFraction = 1. - inlet2Vol / drawVolume;
        }

        // calculate number of nodes to draw
        double drawVolume_N = drawVolume / nodeVolume;
        double drawCp_kJ_per_C = CPWATER_kJ_per_kgC * DENSITYWATER_kg_per_L * drawVolume(Units::L);
        // heat-exchange models
        if (hasHeatExchanger)
        {
            outletT = inletT;
            for (auto& nodeT : tankTs)
            {
                Energy_t maxHeatExchange(drawCp_kJ_per_C * (nodeT - outletT)(Units::dC), Units::kJ);
                Energy_t heatExchange = nodeHeatExchangerEffectiveness * maxHeatExchange;

                nodeT -= {heatExchange(Units::kJ) / nodeCp_kJ_per_C, Units::dC};
                outletT += Temp_d_t(heatExchange(Units::kJ) / drawCp_kJ_per_C, Units::dC);
            }
        }
        else
        {
            double remainingDrawVolume_N = drawVolume_N;
            if (drawVolume > tankVolume)
            {
                for (int i = 0; i < getNumNodes(); i++)
                {
                    outletT += Temp_d_t(tankTs[i]());
                    tankTs[i]() =
                        (inletT() * (drawVolume - inlet2Vol) + inlet2T() * inlet2Vol) / drawVolume;
                }
                outletT() = (outletT() / getNumNodes() * tankVolume +
                             tankTs[0]() * (drawVolume - tankVolume)) /
                            drawVolume * remainingDrawVolume_N;

                remainingDrawVolume_N = 0.;
            }

            Energy_t totalExpelledHeat(0.);
            while (remainingDrawVolume_N > 0.)
            {

                // draw no more than one node at a time
                double incrementalDrawVolume_N =
                    remainingDrawVolume_N > 1. ? 1. : remainingDrawVolume_N;

                Energy_t outputHeat = {
                    incrementalDrawVolume_N * nodeCp_kJ_per_C * tankTs.back()(Units::C), Units::kJ};
                totalExpelledHeat += outputHeat;
                tankTs.back() -= Temp_d_t(outputHeat(Units::kJ) / nodeCp_kJ_per_C, Units::dC);

                for (int i = getNumNodes() - 1; i >= 0; --i)
                {
                    // combine all inlet contributions at this node
                    double inletFraction = 0.;
                    if (i == highInletNodeIndex)
                    {
                        inletFraction += highInletFraction;
                        tankTs[i] += incrementalDrawVolume_N * highInletFraction * highInletT();
                    }
                    if (i == lowInletNodeIndex)
                    {
                        inletFraction += lowInletFraction;
                        tankTs[i] += incrementalDrawVolume_N * lowInletFraction * lowInletT();
                    }

                    if (i > 0)
                    {
                        Temp_d_t transfer_dT(incrementalDrawVolume_N * (1. - inletFraction) *
                                             tankTs[i - 1]());
                        tankTs[i] += transfer_dT;
                        tankTs[i - 1] -= transfer_dT;
                    }
                }

                remainingDrawVolume_N -= incrementalDrawVolume_N;
                mixTankInversions();
            }

            outletT = Temp_t(totalExpelledHeat(Units::kJ) / drawCp_kJ_per_C, Units::C);
        }

        // account for mixing at the bottom of the tank
        if (tankMixesOnDraw && drawVolume > 0.)
        {
            int mixedBelowNode = (int)(getNumNodes() * mixBelowFractionOnDraw);
            mixTankNodes(0, mixedBelowNode, 1. / 3.);
        }

    } // end if(draw_volume > 0)

    // Initialize newTankTemps_C
    nextTankTs = tankTs;

    Energy_t standbyLossesBottom(0.);
    Energy_t standbyLossesTop(0.);
    Energy_t standbyLossesSides(0.);

    // Standby losses from the top and bottom of the tank
    {
        UA_t standbyLossRate = fracAreaTop * tankUA;

        standbyLossesBottom = Energy_t(standbyLossRate(Units::kJ_per_hC) * stepTime(Units::h) *
                                           (tankTs[0] - tankAmbientT)(Units::dC),
                                       Units::kJ);
        standbyLossesTop = Energy_t(standbyLossRate(Units::kJ_per_hC) * stepTime(Units::h) *
                                        (tankTs[getNumNodes() - 1] - tankAmbientT)(Units::dC),
                                    Units::kJ);

        nextTankTs.front() -= standbyLossesBottom(Units::kJ) / nodeCp_kJ_per_C;
        nextTankTs.back() -= standbyLossesTop(Units::kJ) / nodeCp_kJ_per_C;
    }

    // Standby losses from the sides of the tank
    {
        UA_t standbyLossRate((fracAreaSide * tankUA + fittingsUA) / getNumNodes());
        for (int i = 0; i < getNumNodes(); i++)
        {
            Energy_t standbyLossesNode =
                Energy_t(standbyLossRate(Units::kJ_per_hC) * stepTime(Units::h) *
                             (tankTs[i] - tankAmbientT)(Units::dC),
                         Units::kJ);
            standbyLossesSides += standbyLossesNode;

            nextTankTs[i] -= standbyLossesNode(Units::kJ) / nodeCp_kJ_per_C;
        }
    }

    // Heat transfer between nodes
    if (doConduction)
    {
        // Get the "constant" tau for the stability condition and the conduction calculation
        const double tau = 2. * KWATER_W_per_mC /
                           (Units::scale(Units::kJ, Units::J) * CPWATER_kJ_per_kgC) /
                           (DENSITYWATER_kg_per_L / Units::scale(Units::L, Units::m3)) /
                           std::pow(nodeHeight(Units::m), 2.) * stepTime(Units::s);
        if (tau > 1.)
        {
            send_error(fmt::format("The stability condition for conduction has failed!"));
        }

        // End nodes
        if (getNumNodes() > 1)
        { // inner edges of top and bottom nodes
            nextTankTs.front() += tau * (tankTs[1] - tankTs.front());
            nextTankTs.back() += tau * (tankTs[getNumNodes() - 2] - tankTs.back());
        }

        // Internal nodes
        for (int i = 1; i < getNumNodes() - 1; i++)
        {
            nextTankTs[i] += tau * (tankTs[i + 1]() - 2. * tankTs[i]() + tankTs[i - 1]());
        }
    }

    // Update tankTs
    tankTs = nextTankTs;

    standbyLosses += standbyLossesBottom + standbyLossesTop + standbyLossesSides;

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
    // int numdos = 0;
    if (doInversionMixing)
    {
        do
        {
            hasInversion = false;
            // Start from the top and check downwards
            double nodeMass_kg = (nodeVolume(Units::L) * DENSITYWATER_kg_per_L);
            for (int i = getNumNodes() - 1; i > 0; i--)
            {
                if (tankTs[i] < tankTs[i - 1])
                {
                    // Temperature inversion!
                    hasInversion = true;

                    // Mix this inversion mixing temperature by averaging all inverted nodes
                    // together.
                    double Tmixed_sum = 0.;
                    double massMixed = 0.;
                    int j = i;
                    for (; j >= 0; --j)
                    {
                        Tmixed_sum += nodeMass_kg * tankTs[j]();
                        massMixed += nodeMass_kg;
                        if ((j == 0) || (Tmixed_sum > massMixed * tankTs[j - 1]()))
                        {
                            break;
                        }
                    }
                    Temp_t mixT(Tmixed_sum / massMixed);

                    // Assign the tank temps from i to k
                    for (int k = i; k >= j; k--)
                        tankTs[k] = mixT;
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
HPWH::Energy_t HPWH::addHeatAboveNode(Energy_t qAdd, int nodeNum, Temp_t maxT)
{

    // Do not exceed maxT_C or setpoint
    Temp_t maxHeatToT = std::min(maxT, setpointT);

    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (tankTs[i] != tankTs[i + 1])
        {
            break;
        }
        else
        {
            numNodesToHeat++;
        }
    }

    while ((qAdd > 0.) && (nodeNum + numNodesToHeat - 1 < getNumNodes()))
    {
        // assume there is another node above the equal-temp nodes
        int targetTempNodeNum = nodeNum + numNodesToHeat;

        Temp_t heatToT;
        if (targetTempNodeNum > (getNumNodes() - 1))
        {
            // no nodes above the equal-temp nodes; target temperature is the maximum
            heatToT = maxHeatToT;
        }
        else
        {
            heatToT = tankTs[targetTempNodeNum];
            if (heatToT > maxHeatToT)
            {
                // Ensure temperature does not exceed maximum
                heatToT = maxHeatToT;
            }
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        Energy_t qIncrement(numNodesToHeat * nodeCp(Units::kJ_per_C) *
                                (heatToT(Units::C) - tankTs[nodeNum](Units::C)),
                            Units::kJ);

        if (qIncrement > qAdd)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT = Temp_t(tankTs[nodeNum](Units::C) +
                                 qAdd(Units::kJ) / nodeCp(Units::kJ_per_C) / numNodesToHeat,
                             Units::C);
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                tankTs[nodeNum + j] = heatToT;
            }
            qAdd() = 0.;
        }
        else if (qIncrement > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                tankTs[nodeNum + j] = heatToT;
            qAdd -= qIncrement;
        }
        numNodesToHeat++;
    }

    // return any unused heat
    return qAdd;
}

//-----------------------------------------------------------------------------
///	@brief	Adds extra heat amount qAdd_kJ at and above the node with index nodeNum.
/// 		Does not limit final temperatures.
/// @param[in]	qAdd_kJ				Amount of heat to add
///	@param[in]	nodeNum				Lowest node at which to add heat
//-----------------------------------------------------------------------------
void HPWH::addExtraHeatAboveNode(Energy_t qAdd, const int nodeNum)
{
    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (tankTs[i] != tankTs[i + 1])
        {
            break;
        }
        else
        {
            numNodesToHeat++;
        }
    }

    while ((qAdd > 0.) && (nodeNum + numNodesToHeat - 1 < getNumNodes()))
    {

        // assume there is another node above the equal-temp nodes
        int targetTempNodeNum = nodeNum + numNodesToHeat;

        Temp_t heatToT;
        if (targetTempNodeNum > (getNumNodes() - 1))
        {
            // no nodes above the equal-temp nodes; target temperature limited by the heat available
            heatToT =
                tankTs[nodeNum] +
                Temp_d_t(qAdd(Units::kJ) / nodeCp(Units::kJ_per_C) / numNodesToHeat, Units::dC);
        }
        else
        {
            heatToT = tankTs[targetTempNodeNum];
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        Energy_t qIncrement = {nodeCp(Units::kJ_per_C) * numNodesToHeat *
                                   (heatToT(Units::C) - tankTs[nodeNum](Units::C)),
                               Units::kJ};

        if (qIncrement > qAdd)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT =
                tankTs[nodeNum] +
                Temp_d_t(qAdd(Units::kJ) / nodeCp(Units::kJ_per_C) / numNodesToHeat, Units::dC);
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                tankTs[nodeNum + j] = heatToT;
            }
            qAdd() = 0.;
        }
        else if (qIncrement > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                tankTs[nodeNum + j] = heatToT;
            qAdd -= qIncrement;
        }
        numNodesToHeat++;
    }
}

//-----------------------------------------------------------------------------
///	@brief	Modifies a heat distribution using a thermal distribution.
//-----------------------------------------------------------------------------
std::vector<double> HPWH::modifyHeatDistribution(const std::vector<double>& heatDistribution)
{
    double totalHeat = 0.;
    for (auto& heatDist : heatDistribution)
        totalHeat += heatDist;

    if (totalHeat == 0.)
        return heatDistribution;

    auto modHeatDistribution = heatDistribution;
    for (auto& modHeatDist : modHeatDistribution)
        modHeatDist /= totalHeat;

    Temp_d_t shrinkage_dT = findShrinkage_dT(heatDistribution);
    int lowestNode = findLowestNode(modHeatDistribution, getNumNodes());

    modHeatDistribution = calcThermalDist(shrinkage_dT, lowestNode, tankTs, setpointT);
    ;
    for (auto& modHeatDist : modHeatDistribution)
        modHeatDist *= totalHeat;

    return modHeatDistribution;
}

//-----------------------------------------------------------------------------
///	@brief	Adds extra heat to tank.
/// @param[in]	extraHeatDist_W		A distribution of extra heat to add
//-----------------------------------------------------------------------------
void HPWH::addExtraHeat(PowerVect_t& extraHeatDist)
{

    PowerVect_t heatDistribution(getNumNodes());
    resampleExtensive(heatDistribution(), modifyHeatDistribution(extraHeatDist()));

    // Unnecessary unit conversions used here to match former method
    Energy_t tot_qAdded(0.);
    for (int i = getNumNodes() - 1; i >= 0; i--)
    {
        if (heatDistribution[i] != 0)
        {
            Energy_t qAdd = {heatDistribution[i](Units::W) * stepTime(Units::s), Units::J};
            addExtraHeatAboveNode(qAdd, i);
            tot_qAdded += qAdd;
        }
    }
    // Write the input & output energy
    extraEnergyInput = tot_qAdded;
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
    Temp_t avgT(0.);
    double numAvgNodes = static_cast<double>(mixBelowNode - mixBottomNode);
    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        avgT += tankTs[i]();
    }
    avgT() = avgT() / numAvgNodes;

    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        tankTs[i] += mixFactor * (avgT - tankTs[i]);
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
    const Length_t tankRad = getTankRadius();
    const Length_t tankHeight = ASPECTRATIO * tankRad;

    nodeVolume = tankVolume / getNumNodes();
    nodeCp = {CPWATER_kJ_per_kgC * DENSITYWATER_kg_per_L * nodeVolume(Units::L), Units::kJ_per_C};
    nodeHeight = tankHeight / getNumNodes();

    // The fraction of UA that is on the top or the bottom of the tank. So 2 * fracAreaTop +
    // fracAreaSide is the total tank area.
    fracAreaTop = tankRad / (2.0 * (tankHeight + tankRad));

    // fracAreaSide is the faction of the area of the cylinder that's not the top or bottom.
    fracAreaSide = tankHeight / (tankHeight + tankRad);

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
        heatSources[i].shrinkage_dT = findShrinkage_dT(heatSources[i].condensity);
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
bool HPWH::isEnergyBalanced(const Volume_t drawVol,
                            const Energy_t prevHeatContent,
                            const double fracEnergyTolerance /* = 0.001 */)
{
    // Check energy balancing.
    Energy_t qInElectrical(0.);
    for (int iHS = 0; iHS < getNumHeatSources(); iHS++)
    {
        qInElectrical += getNthHeatSourceEnergyInput(iHS);
    }

    Energy_t qInHeatSourceEnviron = getEnergyRemovedFromEnvironment();
    Energy_t qInExtra = extraEnergyInput;
    Energy_t qOutTankEnviron = standbyLosses;

    Cp_t drawCp = {CPWATER_kJ_per_kgC * DENSITYWATER_kg_per_L * drawVol(Units::L),
                   Units::kJ_per_C}; // heat capacity of draw
    Energy_t qOutWater = {drawCp(Units::kJ_per_C) * (outletT - member_inletT)(Units::dC),
                          Units::kJ}; // assumes only one inlet

    Energy_t expectedTankHeatContent =
        prevHeatContent        // previous heat content
        + qInElectrical        // electrical energy delivered to heat sources
        + qInHeatSourceEnviron // heat extracted from environment by condenser
        + qInExtra             // extra energy delivered to heat sources
        - qOutTankEnviron      // heat released from tank to environment
        - qOutWater;           // heat expelled to outlet by water flow

    auto currTankHeatContent = getTankHeatContent();
    Energy_t qBal = currTankHeatContent - expectedTankHeatContent;
    double fracEnergyDiff = fabs(qBal(Units::kJ)) / std::max(prevHeatContent(Units::kJ), 1.);
    if (fracEnergyDiff > fracEnergyTolerance)
    {
        send_warning(fmt::format(
            "Energy-balance error: {:g} kJ, {:g} %", qBal(Units::kJ), 100. * fracEnergyDiff));
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
            if ((heatSources[i].secondaryHeatExchanger.extraPumpPower() != 0) ||
                heatSources[i].secondaryHeatExchanger.extraPumpPower())
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

    Temp_t maxT;
    string why;
    Temp_t tempSetpointT = setpointT;
    if (!isNewSetpointPossible(tempSetpointT, maxT, why))
    {
        error_msgs.push(fmt::format("Cannot set new setpoint. ({})", why.c_str()));
    }

    // Check if the UA is out of bounds
    if (tankUA < 0.0)
    {
        error_msgs.push(
            fmt::format("The tankUA_kJperHrC is less than 0 for a HPWH, it must be greater than 0, "
                        "tankUA_kJperHrC is: {:g}",
                        tankUA(Units::kJ_per_hC)));
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
    std::size_t heatsource, sourceNum, nTemps = 0, tempInt;
    std::size_t num_nodes = 0, numHeatSources = 0;
    bool hasInitialTankTemp = false;
    Temp_t initialTankT = {120., Units::F};

    string tempString, units;
    double tempDouble;
    bool convertPerfMap = false;
    std::vector<Units::Temp> perfT_unit;

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

            if (units == "L")
                tankVolume = Volume_t(tempDouble, Units::L);
            else if (units == "gal")
            {
                tankVolume = Volume_t(tempDouble, Units::gal);
            }
            else
                send_warning(fmt::format("Invalid units: {}", token));
        }
        else if (token == "UA")
        {
            line_ss >> tempDouble >> units;
            tankUA = UA_t(tempDouble, Units::kJ_per_hC);
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
                setpointT = Temp_t(tempDouble, Units::C);
            else if (units == "F")
                setpointT = Temp_t(tempDouble, Units::F);
            else
                send_warning(fmt::format("Invalid units: {}", token));
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
                initialTankT = Temp_t(tempDouble, Units::C);
            }
            else if (units == "F")
                initialTankT = Temp_t(tempDouble, Units::F);
            else
                send_warning(fmt::format("Invalid units: {}", token));

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
            perfT_unit.resize(numHeatSources);
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
                if (units == "C")
                    heatSources[heatsource].minT = {tempDouble, Units::C};
                else if (units == "F")
                    heatSources[heatsource].minT = {tempDouble, Units::F};
                else
                    send_warning(fmt::format("Invalid units: {}", token));
            }
            else if (token == "maxT")
            {
                line_ss >> tempDouble >> units;
                if (units == "C")
                    heatSources[heatsource].maxT = {tempDouble, Units::C};
                else if (units == "F")
                    heatSources[heatsource].maxT = {tempDouble, Units::F};
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

                    std::vector<NodeWeight> nodeWeights;
                    for (size_t i = 0; i < nodeNums.size(); i++)
                    {
                        nodeWeights.emplace_back(nodeNums[i], weights[i]);
                    }

                    GenTemp_t logicT;
                    if (absolute)
                    {
                        if (units == "C")
                        {
                            logicT = {tempDouble, Units::C};
                        }
                        else if (units == "F")
                        {
                            logicT = {tempDouble, Units::F};
                        }
                        else
                            send_warning(fmt::format("Invalid units: {}", token));
                    }
                    else
                    {
                        if (units == "C")
                        {
                            logicT = {tempDouble, Units::dF};
                        }
                        else if (units == "F")
                        {
                            logicT = {tempDouble, Units::dC};
                        }
                        else
                            send_warning(fmt::format("Invalid units: {}", token));
                    }
                    auto logic = std::make_shared<HPWH::TempBasedHeatingLogic>(
                        "custom", nodeWeights, logicT, this, compare);
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
                                "standby logic", nodeWeights, logicT, this, compare);
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
                    GenTemp_t logicT;
                    if (units == "C")
                    {
                        if (absolute)
                            logicT = {tempDouble, Units::C};
                        else
                            logicT = {tempDouble, Units::dC};
                    }
                    else if (units == "F")
                    {
                        if (absolute)
                            logicT = {tempDouble, Units::F};
                        else
                            logicT = {tempDouble, Units::dF};
                    }
                    else
                    {
                        send_warning(fmt::format("Invalid units: {}", token));
                    }

                    if (tempString == "wholeTank")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::wholeTank(logicT));
                    }
                    else if (tempString == "topThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::topThird(logicT));
                    }
                    else if (tempString == "bottomThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomThird(logicT));
                    }
                    else if (tempString == "standby")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::standby(logicT));
                    }
                    else if (tempString == "bottomSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomSixth(logicT));
                    }
                    else if (tempString == "secondSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::secondSixth(logicT));
                    }
                    else if (tempString == "thirdSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::thirdSixth(logicT));
                    }
                    else if (tempString == "fourthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::fourthSixth(logicT));
                    }
                    else if (tempString == "fifthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::fifthSixth(logicT));
                    }
                    else if (tempString == "topSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::topSixth(logicT));
                    }
                    else if (tempString == "bottomHalf")
                    {
                        heatSources[heatsource].addTurnOnLogic(HPWH::bottomHalf(logicT));
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
                    GenTemp_t logicT = {0, Units::dC};
                    if (units == "C")
                    {
                        logicT = {tempDouble, Units::C};
                    }
                    else if (units == "F")
                    {
                        logicT = Temp_t(tempDouble, Units::F);
                    }
                    else
                        send_warning(fmt::format("Invalid units: {}", token));

                    if (tempString == "topNodeMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::topNodeMaxTemp(logicT));
                    }
                    else if (tempString == "bottomNodeMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::bottomNodeMaxTemp(logicT));
                    }
                    else if (tempString == "bottomTwelfthMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::bottomTwelfthMaxTemp(logicT));
                    }
                    else if (tempString == "bottomSixthMaxTemp")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::bottomSixthMaxTemp(logicT));
                    }
                    else if (tempString == "largeDraw")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::largeDraw(logicT));
                    }
                    else if (tempString == "largerDraw")
                    {
                        heatSources[heatsource].addShutOffLogic(HPWH::largerDraw(logicT));
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
                convertPerfMap = true;
            }
            else if (std::regex_match(token, std::regex("T\\d+")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("T(\\d+)"));
                std::size_t nTemp = std::stoi(match[1].str());

                if (nTemp > nTemps)
                {
                    if (nTemps == 0)
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
                            nTemps,
                            nTemp));
                    }
                }
                line_ss >> tempDouble >> units;
                auto& T = heatSources[heatsource].perfMap[nTemp - 1].T;
                if (units == "C")
                {
                    perfT_unit[heatsource] = Units::C;
                    T = {tempDouble, Units::C};
                }
                else if (units == "F")
                {
                    perfT_unit[heatsource] = Units::F;
                    T = {tempDouble, Units::F};
                }
                else
                    send_warning(fmt::format("Invalid units: {}", token));
            }
            else if (std::regex_match(token, std::regex("(?:inPow|cop)T\\d+(?:const|lin|quad)")))
            {
                std::smatch match;
                std::regex_match(token, match, std::regex("(inPow|cop)T(\\d+)(const|lin|quad)"));
                string var = match[1].str();
                nTemps = std::stoi(match[2].str());
                string coeff = match[3].str();

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
                        {tempDouble, Units::W});
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
                    heatSources[heatsource].hysteresis_dT = {tempDouble, Units::dC};
                else if (units == "F")
                {
                    heatSources[heatsource].hysteresis_dT = {tempDouble, Units::dF};
                }
                else
                    send_warning(fmt::format("Invalid units: {}", token));
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

    if (convertPerfMap)
    {
        std::size_t j = 0;
        for (auto& heatSource : heatSources)
        {
            auto uT = perfT_unit[j];
            auto& perfMap = heatSource.perfMap;
            std::size_t nPoints = perfMap.size();
            for (std::size_t i = 0; i < nPoints; ++i)
            {
                auto& p = perfMap[i];
                PerfPoint newPoint({p.T(uT), uT}, p.inputPower_coeffs, p.COP_coeffs);
                p = newPoint;
            }
            j++;
        }
    }

    // take care of the non-input processing
    model = MODELS_CustomFile;

    tankTs.resize(num_nodes);

    if (hasInitialTankTemp)
        setTankToT(initialTankT);
    else
        resetTankToSetpoint();

    nextTankTs.resize(num_nodes);

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
///	@brief	Performs a draw/heat cycle to prep for test
/// @return	true (success), false (failure).
//-----------------------------------------------------------------------------
void HPWH::prepForTest(StandardTestOptions& testOptions)
{
    FlowRate_t flowRate(3., Units::gal_per_min);
    if (tankVolume(Units::L) < 20.)
        flowRate = FlowRate_t(1.5, Units::gal_per_min);

    const Temp_t inletT(14.4, Units::C);   // EERE-2019-BT-TP-0032-0058, p. 40433
    const Temp_t ambientT(19.7, Units::C); // EERE-2019-BT-TP-0032-0058, p. 40435
    const Temp_t externalT(19.7, Units::C);

    if (testOptions.changeSetpoint)
    {
        setSetpointT(testOptions.setpointT);
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

        case 2: // wait for heat to turn on
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
        Volume_t maxFlowVolume = {flowRate(Units::L_per_s) * Time_t(1., Units::min)(Units::s),
                                  Units::L};
        Volume_t incrementalDrawVolume = isDrawing ? maxFlowVolume : Volume_t(0.);
        if (incrementalDrawVolume > tankVolume)
        {
            incrementalDrawVolume = tankVolume;
        }

        runOneStep(inletT,                // inlet water temperature (C)
                   incrementalDrawVolume, // draw volume (L)
                   ambientT,              // ambient Temp (C)
                   externalT,             // external Temp (C)
                   drMode,                // DDR Status
                   V0(),                  // inlet-2 volume (L)
                   inletT,                // inlet-2 Temp (C)
                   NULL);                 // no extra heat
    }
}

//-----------------------------------------------------------------------------
///	@brief	Find first-hour rating designation for 24-hr test
/// @param[out] firstHourRating	    contains first-hour rating designation
///	@param[in]	setpointT_C		    setpoint temperature (optional)
/// @return	true (success), false (failure).
//-----------------------------------------------------------------------------
void HPWH::findFirstHourRating(FirstHourRating& firstHourRating, StandardTestOptions& testOptions)
{
    FlowRate_t flowRate(3., Units::gal_per_min);
    if (tankVolume(Units::gal) < 20.)
        flowRate = {1.5, Units::gal_per_min};

    const Temp_t inletT = {14.4, Units::C};   // EERE-2019-BT-TP-0032-0058, p. 40433
    const Temp_t ambientT = {19.7, Units::C}; // EERE-2019-BT-TP-0032-0058, p. 40435
    const Temp_t externalT = {19.7, Units::C};

    if (testOptions.changeSetpoint)
    {
        setSetpointT(testOptions.setpointT);
    }

    Temp_t tankT = getAverageTankT();
    Temp_t maxTankT = tankT;
    Temp_t maxOutletT = {0., Units::C};

    DRMODES drMode = DR_ALLOW;
    Volume_t drawVolume(0.);

    firstHourRating.drawVolume() = 0.;

    double sumOutletVolumeT = 0.;
    double sumOutletVolume = 0.;

    Temp_t avgOutletT = {0., Units::C};
    Temp_t minOutletT = {0., Units::C};
    Temp_t prevAvgOutletT = {0., Units::C};
    Temp_t prevMinOutletT = {0., Units::C};

    bool isDrawing = false;
    bool done = false;
    int step = 0;

    prepForTest(testOptions);

    bool firstDraw = true;
    isDrawing = true;
    maxOutletT = {0., Units::C};
    drMode = DR_LOC;
    Time_t elapsedTime(0.);
    while (!done)
    {

        // limit draw-volume increment to tank volume
        Volume_t incrementalDrawVolume = {
            isDrawing ? flowRate(Units::L_per_s) * Time_t(1., Units::min)(Units::s) : 0., Units::L};
        if (incrementalDrawVolume > tankVolume)
        {
            incrementalDrawVolume = tankVolume;
        }

        runOneStep(inletT,                // inlet water temperature
                   incrementalDrawVolume, // draw volume
                   ambientT,              // ambient Temp
                   externalT,             // external Temp
                   drMode,                // DDR Status
                   V0(),                  // inlet-2 volume
                   inletT,                // inlet-2 Temp
                   NULL);                 // no extra heat

        tankT = getAverageTankT();

        switch (step)
        {
        case 0: // drawing
        {
            sumOutletVolume += incrementalDrawVolume();
            sumOutletVolumeT += incrementalDrawVolume() * outletT();

            maxOutletT = std::max(outletT, maxOutletT);
            if (outletT < (maxOutletT -
                           Temp_d_t(15., Units::dF))) // outletT has dropped by 15 degF below max T
            {
                avgOutletT() = sumOutletVolumeT / sumOutletVolume;
                minOutletT = outletT;
                if (elapsedTime(Units::h) >= 1.)
                {
                    double fac = 1;
                    if (!firstDraw)
                    {
                        fac = (avgOutletT - prevMinOutletT) / (prevAvgOutletT - prevMinOutletT);
                    }
                    firstHourRating.drawVolume += fac * drawVolume;
                    done = true;
                }
                else
                {
                    firstHourRating.drawVolume += drawVolume;
                    drawVolume() = 0.;
                    isDrawing = false;
                    drMode = DR_ALLOW;
                    maxTankT = tankT;     // initialize for next pass
                    maxOutletT = outletT; // initialize for next pass
                    prevAvgOutletT = avgOutletT;
                    prevMinOutletT = minOutletT;
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
            if ((tankT > maxTankT) && isHeating && (elapsedTime(Units::h) < 1.))
            { // has not reached maxTankT, heat is on, and less than 1 hr has elapsed
                maxTankT = std::max(tankT, maxTankT);
            }
            else // start another draw
            {
                firstDraw = false;
                isDrawing = true;
                drawVolume() = 0.;
                drMode = DR_LOC;
                step = 0; // repeat
            }
        }
        }

        drawVolume += incrementalDrawVolume;
        elapsedTime += Time_t(1., Units::min);
    }

    //
    if (firstHourRating.drawVolume(Units::gal) < 18.)
    {
        firstHourRating.desig = FirstHourRating::Desig::VerySmall;
    }
    else if (firstHourRating.drawVolume(Units::gal) < 51.)
    {
        firstHourRating.desig = FirstHourRating::Desig::Low;
    }
    else if (firstHourRating.drawVolume(Units::gal) < 75.)
    {
        firstHourRating.desig = FirstHourRating::Desig::Medium;
    }
    else
    {
        firstHourRating.desig = FirstHourRating::Desig::High;
    }

    const std::string sFirstHourRatingDesig =
        HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];

    std::cout << "\tFirst-Hour Rating:\n";
    std::cout << "\t\tVolume Drawn (L): " << firstHourRating.drawVolume(Units::L) << "\n";
    std::cout << "\t\tDesignation: " << sFirstHourRatingDesig << "\n";
}

//-----------------------------------------------------------------------------
///	@brief	Performs standard 24-hr test
/// @note	see https://www.regulations.gov/document/EERE-2019-BT-TP-0032-0058
/// @param[in] firstHourRating          specifies first-hour rating
/// @param[out] testSummary	            contains test metrics on output
///	@param[in]	setpointT_C		        setpoint temperature (optional)
/// @return	true (success), false (failure).
//-----------------------------------------------------------------------------
void HPWH::run24hrTest(const FirstHourRating firstHourRating,
                       StandardTestSummary& testSummary,
                       StandardTestOptions& testOptions)
{
    /// collection of standard draw patterns
    static std::unordered_map<HPWH::FirstHourRating::Desig, std::size_t> firstDrawClusterSizes = {
        {HPWH::FirstHourRating::Desig::VerySmall, 5},
        {HPWH::FirstHourRating::Desig::Low, 3},
        {HPWH::FirstHourRating::Desig::Medium, 3},
        {HPWH::FirstHourRating::Desig::High, 4}};

    static std::unordered_map<HPWH::FirstHourRating::Desig, HPWH::DrawPattern> drawPatterns = {
        {HPWH::FirstHourRating::Desig::VerySmall,
         {{hmin(0, 00), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 05), {1.9, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 10), {1.9, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 15), {1.9, Units::L}, {3.8, Units::L_per_s}},
          {hmin(8, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(8, 15), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(9, 00), {5.7, Units::L}, {3.8, Units::L_per_s}},
          {hmin(9, 15), {3.8, Units::L}, {3.8, Units::L_per_s}}}},

        {HPWH::FirstHourRating::Desig::Low,
         {{hmin(0, 00), {56.8, Units::L}, {6.4, Units::L_per_s}},
          {hmin(0, 30), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(10, 30), {22.7, Units::L}, {6.4, Units::L_per_s}},
          {hmin(11, 30), {15.1, Units::L}, {6.4, Units::L_per_s}},
          {hmin(12, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(12, 45), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(12, 50), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 15), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 45), {7.6, Units::L}, {6.4, Units::L_per_s}},
          {hmin(17, 00), {11.4, Units::L}, {6.4, Units::L_per_s}}}},

        {HPWH::FirstHourRating::Desig::Medium,
         {{hmin(0, 00), {56.8, Units::L}, {6.4, Units::L_per_s}},
          {hmin(0, 30), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 40), {34.1, Units::L}, {6.4, Units::L_per_s}},
          {hmin(10, 30), {34.1, Units::L}, {6.4, Units::L_per_s}},
          {hmin(11, 30), {18.9, Units::L}, {6.4, Units::L_per_s}},
          {hmin(12, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(12, 45), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(12, 50), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 15), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 45), {7.6, Units::L}, {6.4, Units::L_per_s}},
          {hmin(17, 00), {26.5, Units::L}, {6.4, Units::L_per_s}}}},

        {HPWH::FirstHourRating::Desig::High,
         {{hmin(0, 00), {102, Units::L}, {11.4, Units::L_per_s}},
          {hmin(0, 30), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(0, 40), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(1, 40), {34.1, Units::L}, {6.4, Units::L_per_s}},
          {hmin(10, 30), {56.8, Units::L}, {11.4, Units::L_per_s}},
          {hmin(11, 30), {18.9, Units::L}, {6.4, Units::L_per_s}},
          {hmin(12, 00), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(12, 45), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(12, 50), {3.8, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 00), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 15), {7.6, Units::L}, {3.8, Units::L_per_s}},
          {hmin(16, 30), {7.6, Units::L}, {6.4, Units::L_per_s}},
          {hmin(16, 45), {7.6, Units::L}, {6.4, Units::L_per_s}},
          {hmin(17, 00), {53.0, Units::L}, {11.4, Units::L_per_s}}}}};

    // select the first draw cluster size and pattern
    auto firstDrawClusterSize = firstDrawClusterSizes[firstHourRating.desig];
    DrawPattern& drawPattern = drawPatterns[firstHourRating.desig];

    const Temp_t inletT(14.4, Units::C);   // EERE-2019-BT-TP-0032-0058, p. 40433
    const Temp_t ambientT(19.7, Units::C); // EERE-2019-BT-TP-0032-0058, p. 40435
    const Temp_t externalT(19.7, Units::C);
    DRMODES drMode = DR_ALLOW;

    if (testOptions.changeSetpoint)
    {
        setSetpointT(testOptions.setpointT);
    }

    prepForTest(testOptions);

    std::vector<OutputData> outputDataSet;

    // idle for 1 hr
    Time_t preTime(0.);
    bool heatersAreOn = false;
    while ((preTime(Units::min) < 60.) || heatersAreOn)
    {
        runOneStep(inletT,    // inlet water temperature
                   V0(),      // draw volume
                   ambientT,  // ambient Temp
                   externalT, // external Temp
                   drMode,    // DDR Status
                   V0(),      // inlet-2 volume
                   inletT,    // inlet-2 Temp
                   NULL);     // no extra heat

        heatersAreOn = false;
        for (auto& heatSource : heatSources)
        {
            heatersAreOn |= heatSource.isEngaged();
        }

        {
            OutputData outputData;
            outputData.time = preTime;
            outputData.ambientT = ambientT;
            outputData.setpointT = getSetpointT();
            outputData.inletT = inletT;
            outputData.drawVolume() = 0.;
            outputData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                outputData.h_srcIn.push_back(getNthHeatSourceEnergyInput(iHS));
                outputData.h_srcOut.push_back(getNthHeatSourceEnergyOutput(iHS));
            }

            for (int iTC = 0; iTC < testOptions.nTestTCouples; ++iTC)
            {
                outputData.thermocoupleT.push_back(
                    getNthSimTcouple(iTC + 1, testOptions.nTestTCouples));
            }
            outputData.outletT = {0., Units::C};

            outputDataSet.push_back(outputData);
        }

        preTime += Time_t(1., Units::min);
    }

    // correct time to start test at 0 min
    for (auto& outputData : outputDataSet)
    {
        outputData.time -= preTime;
    }

    Temp_t tankT = getAverageTankT();
    Temp_t initialTankT = tankT;

    // used to find average draw temperatures
    double drawSumOutletVolumeT = 0.;
    double drawSumInletVolumeT = 0.;

    // used to find average 24-hr test temperatures
    double sumOutletVolumeT = 0.;
    double sumInletVolumeT = 0.;

    // first-recovery info
    testSummary.recoveryStoredEnergy() = 0.;
    testSummary.recoveryDeliveredEnergy() = 0.;
    testSummary.recoveryUsedEnergy() = 0.;

    Energy_t deliveredEnergy(0.); // total energy delivered to water
    testSummary.removedVolume() = 0.;
    testSummary.usedEnergy() = 0.;           // Q
    testSummary.usedFossilFuelEnergy() = 0.; // total fossil-fuel energy consumed, Qf
    testSummary.usedElectricalEnergy() = 0.; // total electrical energy consumed, Qe

    bool hasHeated = false;

    Time_t endTime(24., Units::h);
    std::size_t iDraw = 0;
    Volume_t remainingDrawVolume(0.);
    Volume_t drawVolume(0.);
    Time_t prevDrawEndTime(0.);

    bool isFirstRecoveryPeriod = true;
    bool isInFirstDrawCluster = true;
    bool hasStandbyPeriodStarted = false;
    bool hasStandbyPeriodEnded = false;
    bool nextDraw = true;
    bool isDrawing = false;
    bool isDrawPatternComplete = false;

    Time_t recoveryEndTime(0.);

    Time_t standbyStartTime(0.);
    Time_t standbyEndTime(0.);
    Temp_t standbyStartT(0.);
    Temp_t standbyEndT(0.);
    Energy_t standbyStartTankEnergy(0.);
    Energy_t standbyEndTankEnergy(0.);
    double standbySumTimeTankT(0.);
    double standbySumTimeAmbientT(0.);

    Time_t noDrawTotalTime(0.);
    double noDrawSumTimeAmbientT = 0.;

    bool inLastHour = false;
    Volume_t stepDrawVolume(0.);
    auto endStep = static_cast<int>(endTime(Units::min));
    for (int runStep = 0; runStep <= endStep; ++runStep)
    {
        Time_t runTime = {static_cast<double>(runStep), Units::min};
        if (inLastHour)
        {
            drMode = DR_LOC | DR_LOR;
        }

        if (nextDraw)
        {
            auto& draw = drawPattern[iDraw];
            if (runTime >= draw.startTime)
            {
                // limit draw-volume step to tank volume
                stepDrawVolume = {draw.flowRate(Units::L_per_s) * Time_t(1., Units::min)(Units::s),
                                  Units::L};
                if (stepDrawVolume > tankVolume)
                {
                    stepDrawVolume = tankVolume;
                }

                remainingDrawVolume = drawVolume = draw.volume;

                nextDraw = false;
                isDrawing = true;

                drawSumOutletVolumeT = 0.;
                drawSumInletVolumeT = 0.;

                if (hasStandbyPeriodStarted && (!hasStandbyPeriodEnded))
                {
                    hasStandbyPeriodEnded = true;
                    standbyEndTankEnergy = testSummary.usedEnergy; // Qsu,0
                    standbyEndT = tankT;                           // Tsu,0
                    standbyEndTime = runTime;
                }
            }
        }

        // iterate until 1) specified draw volume has been reached and 2) next draw has started
        // do not exceed specified draw volume
        if (isDrawing)
        {
            if (stepDrawVolume >= remainingDrawVolume)
            {
                stepDrawVolume = remainingDrawVolume;
                remainingDrawVolume() = 0.;
                isDrawing = false;
                prevDrawEndTime = runTime;
            }
            else
            {
                remainingDrawVolume -= stepDrawVolume;
            }
        }
        else
        {
            remainingDrawVolume() = stepDrawVolume() = 0.;
            noDrawSumTimeAmbientT += (1.) * ambientT();
            noDrawTotalTime += Time_t(1., Units::min);
        }

        // run a step
        runOneStep(inletT,         // inlet water temperature
                   stepDrawVolume, // draw volume
                   ambientT,       // ambient Temp
                   externalT,      // external Temp
                   drMode,         // DDR Status
                   V0(),           // inlet-2 volume
                   inletT,         // inlet-2 Temp
                   NULL);          // no extra heat

        {
            OutputData outputData;
            outputData.time = runTime;
            outputData.ambientT = ambientT;
            outputData.setpointT = getSetpointT();
            outputData.inletT = inletT;
            outputData.drawVolume = stepDrawVolume;
            outputData.drMode = drMode;

            for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
            {
                outputData.h_srcIn.push_back(getNthHeatSourceEnergyInput(iHS));
                outputData.h_srcOut.push_back(getNthHeatSourceEnergyOutput(iHS));
            }

            for (int iTC = 0; iTC < testOptions.nTestTCouples; ++iTC)
            {
                outputData.thermocoupleT.push_back(
                    getNthSimTcouple(iTC + 1, testOptions.nTestTCouples));
            }
            outputData.outletT = outletT;

            outputDataSet.push_back(outputData);
        }

        tankT = getAverageTankT();
        hasHeated |= isHeating;

        drawSumOutletVolumeT += stepDrawVolume() * outletT();
        drawSumInletVolumeT += stepDrawVolume() * inletT();

        sumOutletVolumeT += stepDrawVolume() * outletT();
        sumInletVolumeT += stepDrawVolume() * inletT();

        // collect energy added to water
        double stepDrawMass_kg = DENSITYWATER_kg_per_L * stepDrawVolume(Units::L);
        Cp_t stepDrawHeatCapacity = {CPWATER_kJ_per_kgC * stepDrawMass_kg, Units::kJ_per_C};
        deliveredEnergy +=
            Energy_t(stepDrawHeatCapacity(Units::kJ_per_C) * (outletT(Units::C) - inletT(Units::C)),
                     Units::kJ);

        // collect used-energy info
        Energy_t usedFossilFuelEnergy(0.);
        Energy_t usedElectricalEnergy(0.);
        for (int iHS = 0; iHS < getNumHeatSources(); ++iHS)
        {
            usedElectricalEnergy += getNthHeatSourceEnergyInput(iHS);
        }

        // collect 24-hr test info
        testSummary.removedVolume += stepDrawVolume;
        testSummary.usedFossilFuelEnergy += usedFossilFuelEnergy;
        testSummary.usedElectricalEnergy += usedElectricalEnergy;
        testSummary.usedEnergy += usedFossilFuelEnergy + usedElectricalEnergy;

        if (isFirstRecoveryPeriod)
        {
            testSummary.recoveryUsedEnergy += usedFossilFuelEnergy + usedElectricalEnergy;
        }

        if (!isDrawing)
        {
            if (isFirstRecoveryPeriod)
            {
                if (hasHeated && (!isHeating))
                {
                    // collect recovery info
                    isFirstRecoveryPeriod = false;

                    double tankContentMass_kg = DENSITYWATER_kg_per_L * tankVolume(Units::L);
                    Cp_t tankHeatCapacity = {CPWATER_kJ_per_kgC * tankContentMass_kg,
                                             Units::kJ_per_C};
                    testSummary.recoveryStoredEnergy = {
                        tankHeatCapacity(Units::kJ_per_C) *
                            (tankT(Units::C) - initialTankT(Units::C)),
                        Units::kJ};
                }

                if (!nextDraw)
                {
                    Temp_t meanDrawOutletT(drawSumOutletVolumeT / drawVolume());
                    Temp_t meanDrawInletT(drawSumInletVolumeT / drawVolume());

                    double drawMass_kg = DENSITYWATER_kg_per_L * drawVolume(Units::L);
                    Cp_t drawHeatCapacity = {CPWATER_kJ_per_kgC * drawMass_kg, Units::kJ_per_C};

                    testSummary.recoveryDeliveredEnergy +=
                        Energy_t(drawHeatCapacity(Units::kJ_per_C) *
                                     (meanDrawOutletT(Units::C) - meanDrawInletT(Units::C)),
                                 Units::kJ);
                }
            }

            if (!hasStandbyPeriodEnded)
            {
                if (hasStandbyPeriodStarted)
                {
                    standbySumTimeTankT += Time_t(1., Units::min)() * tankT();
                    standbySumTimeAmbientT += Time_t(1., Units::min)() * ambientT();

                    if (runTime >= standbyStartTime + Time_t(8, Units::h))
                    {
                        hasStandbyPeriodEnded = true;
                        standbyEndTankEnergy = testSummary.usedEnergy; // Qsu,0
                        standbyEndT = tankT;                           // Tsu,0
                        standbyEndTime = runTime;
                    }
                }
                else
                {
                    if (isHeating)
                    {
                        recoveryEndTime = runTime;
                    }
                    else
                    {
                        if ((!isInFirstDrawCluster) || isDrawPatternComplete)
                        {
                            if ((runTime > prevDrawEndTime + Time_t(5, Units::min)) &&
                                (runTime > recoveryEndTime + Time_t(5, Units::min)))
                            {
                                hasStandbyPeriodStarted = true;
                                standbyStartTime = runTime;
                                standbyStartTankEnergy = testSummary.usedEnergy; // Qsu,0
                                standbyStartT = tankT;                           // Tsu,0

                                if (isDrawPatternComplete &&
                                    (runTime + Time_t(8, Units::h) > endTime))
                                {
                                    endTime = runTime + Time_t(8, Units::h);
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

    Temp_t finalTankT = tankT;

    if (!hasStandbyPeriodEnded)
    {
        hasStandbyPeriodEnded = true;
        standbyEndTime = endTime;
        standbyEndTankEnergy = testSummary.usedEnergy; // Qsu,0
        standbyEndT = tankT;                           // Tsu,0
    }

    if (testOptions.saveOutput)
    {
        for (auto& outputData : outputDataSet)
        {
            writeRowAsCSV(testOptions.outputFile, outputData);
        }
    }

    testSummary.avgOutletT() = sumOutletVolumeT / testSummary.removedVolume();
    testSummary.avgInletT() = sumInletVolumeT / testSummary.removedVolume();

    const Temp_t standardSetpointT = {51.7, Units::C};
    const Temp_t standardInletT = {14.4, Units::C};
    const Temp_t standardAmbientT = {19.7, Units::C};

    double tankContentMass_kg = DENSITYWATER_kg_per_L * tankVolume(Units::L);
    Cp_t tankHeatCapacity = {CPWATER_kJ_per_kgC * tankContentMass_kg, Units::kJ_per_C};

    double removedMass_kg = DENSITYWATER_kg_per_L * testSummary.removedVolume(Units::L);
    Cp_t removedHeatCapacity = {CPWATER_kJ_per_kgC * removedMass_kg, Units::kJ_per_C};

    // require heating during 24-hr test for unit to qualify as consumer water heater
    if (hasHeated && !isDrawing)
    {
        testSummary.qualifies = true;
    }

    // find the "Recovery Efficiency" (6.3.3)
    testSummary.recoveryEfficiency = 0.;
    if (testSummary.recoveryUsedEnergy > 0.)
    {
        testSummary.recoveryEfficiency =
            (testSummary.recoveryStoredEnergy + testSummary.recoveryDeliveredEnergy) /
            testSummary.recoveryUsedEnergy;
    }

    // find the energy consumed during the standby-loss test, Qstdby
    testSummary.standbyUsedEnergy = standbyEndTankEnergy - standbyStartTankEnergy;

    Time_t standbyPeriodTime = standbyEndTime - standbyStartTime - Time_t(1, Units::min);
    testSummary.standbyPeriodTime = standbyPeriodTime; // tau_stby,1
    if ((testSummary.standbyPeriodTime > 0) && (testSummary.recoveryEfficiency > 0.))
    {
        Energy_t standardTankEnergy = {tankHeatCapacity(Units::kJ_per_C) *
                                           (standbyEndT(Units::C) - standbyStartT(Units::C)) /
                                           testSummary.recoveryEfficiency,
                                       Units::kJ};
        testSummary.standbyHourlyLossEnergy = {
            (testSummary.standbyUsedEnergy(Units::kJ) - standardTankEnergy(Units::kJ)) /
                testSummary.standbyPeriodTime(Units::s),
            Units::kW};

        Temp_t standbyAverageTankT(standbySumTimeTankT / standbyPeriodTime());
        Temp_t standbyAverageAmbientT(standbySumTimeAmbientT / standbyPeriodTime());

        Temp_d_t dT(standbyAverageTankT(Units::C) - standbyAverageAmbientT(Units::C), Units::dC);
        if (dT > 0.)
        {
            testSummary.standbyLossCoefficient() =
                testSummary.standbyHourlyLossEnergy() / dT(); // UA
        }
    }

    //
    testSummary.noDrawTotalTime = noDrawTotalTime; // tau_stby,2
    if (noDrawTotalTime > 0)
    {
        testSummary.noDrawAverageAmbientT() =
            noDrawSumTimeAmbientT / noDrawTotalTime(); // <Ta,stby,2>
    }

    // find the standard delivered daily energy
    Energy_t standardDeliveredEnergy(removedHeatCapacity(Units::kJ_per_C) *
                                         (standardSetpointT(Units::C) - standardInletT(Units::C)),
                                     Units::kJ);

    // find the "Daily Water Heating Energy Consumption (Q_d)" (6.3.5)
    testSummary.consumedHeatingEnergy = testSummary.usedEnergy;
    if (testSummary.recoveryEfficiency > 0.)
    {
        testSummary.consumedHeatingEnergy -= Energy_t(
            tankHeatCapacity(Units::kJ_per_C) * (finalTankT(Units::C) - initialTankT(Units::C)) /
                testSummary.recoveryEfficiency,
            Units::kJ);
    }

    // find the "Adjusted Daily Water Heating Energy Consumption (Q_da)" (6.3.6)
    testSummary.adjustedConsumedWaterHeatingEnergy =
        testSummary.consumedHeatingEnergy -
        Energy_t((standardAmbientT(Units::C) - testSummary.noDrawAverageAmbientT(Units::C)) *
                     testSummary.standbyLossCoefficient(Units::kJ_per_hC) *
                     testSummary.noDrawTotalTime(Units::h),
                 Units::kJ);

    // find the "Energy Used to Heat Water (Q_HW)" (6.3.6)
    testSummary.waterHeatingEnergy() = 0.;
    if (testSummary.recoveryEfficiency > 0.)
    {
        testSummary.waterHeatingEnergy = deliveredEnergy / testSummary.recoveryEfficiency;
    }

    // find the "Standard Energy Used to Heat Water (Q_HW,T)" (6.3.6)
    testSummary.standardWaterHeatingEnergy() = 0.;
    if (testSummary.recoveryEfficiency > 0.)
    {
        Energy_t standardRemovedEnergy(removedHeatCapacity(Units::kJ_per_C) *
                                           (standardSetpointT(Units::C) - standardInletT(Units::C)),
                                       Units::kJ);
        testSummary.standardWaterHeatingEnergy =
            standardRemovedEnergy / testSummary.recoveryEfficiency;
    }

    // find the "Modified Daily Water Heating Energy Consumption (Q_dm = Q_da - Q_HWD) (p.
    // 40487) note: same as Q_HW,T
    Energy_t waterHeatingDifferenceEnergy =
        testSummary.standardWaterHeatingEnergy - testSummary.waterHeatingEnergy; // Q_HWD
    testSummary.modifiedConsumedWaterHeatingEnergy =
        testSummary.adjustedConsumedWaterHeatingEnergy + waterHeatingDifferenceEnergy;

    // find the "Uniform Energy Factor" (6.4.4)
    testSummary.UEF = 0.;
    if (testSummary.modifiedConsumedWaterHeatingEnergy > 0.)
    {
        testSummary.UEF = standardDeliveredEnergy / testSummary.modifiedConsumedWaterHeatingEnergy;
    }

    // find the "Annual Energy Consumption" (6.4.5)
    testSummary.annualConsumedEnergy() = 0.;
    if (testSummary.UEF > 0.)
    {
        constexpr double days_per_year = 365.;
        const Temp_d_t nominalDifference_dT(67., Units::dF);
        testSummary.annualConsumedEnergy =
            Energy_t(days_per_year * removedHeatCapacity(Units::kJ_per_C) *
                         nominalDifference_dT(Units::dC) / testSummary.UEF,
                     Units::kJ);
    }

    // find the "Annual Electrical Energy Consumption" (6.4.6)
    testSummary.annualConsumedElectricalEnergy() = 0.;
    if (testSummary.usedEnergy > 0.)
    {
        testSummary.annualConsumedElectricalEnergy =
            (testSummary.usedElectricalEnergy / testSummary.usedEnergy) *
            testSummary.annualConsumedEnergy;
    }
}

void HPWH::measureMetrics(FirstHourRating& firstHourRating,
                          StandardTestOptions& standardTestOptions,
                          StandardTestSummary& standardTestSummary)
{
    if (standardTestOptions.saveOutput)
    {

        std::string sFullOutputFilename =
            standardTestOptions.sOutputDirectory + "/" + standardTestOptions.sOutputFilename;
        standardTestOptions.outputFile.open(sFullOutputFilename.c_str(), std::ifstream::out);
        if (!standardTestOptions.outputFile.is_open())
        {
            send_error(fmt::format("Could not open output file {}", sFullOutputFilename));
        }
        std::cout << "Output file: " << sFullOutputFilename << "\n";

        std::string strPreamble;
        std::string sHeader = "minutes,Ta,Tsetpoint,inletT,draw,";

        int csvOptions = HPWH::CSVOPT_NONE;
        WriteCSVHeading(standardTestOptions.outputFile,
                        sHeader.c_str(),
                        standardTestOptions.nTestTCouples,
                        csvOptions);
    }

    findFirstHourRating(firstHourRating, standardTestOptions);
    if (customTestOptions.overrideFirstHourRating)
    {
        firstHourRating.desig = customTestOptions.desig;
        const std::string sFirstHourRatingDesig =
            HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];
        std::cout << "\t\tUser-Specified Designation: " << sFirstHourRatingDesig << "\n";
    }

    run24hrTest(firstHourRating, standardTestSummary, standardTestOptions);

    std::cout << "\t24-Hour Test Results:\n";
    if (!standardTestSummary.qualifies)
    {
        std::cout << "\t\tDoes not qualify as consumer water heater.\n";
    }

    std::cout << "\t\tRecovery Efficiency: " << standardTestSummary.recoveryEfficiency << "\n";

    std::cout << "\t\tStandby Loss Coefficient (kJ/h degC): "
              << standardTestSummary.standbyLossCoefficient() << "\n";

    std::cout << "\t\tUEF: " << standardTestSummary.UEF << "\n";

    std::cout << "\t\tAverage Inlet Temperature (degC): " << standardTestSummary.avgInletT(Units::C)
              << "\n";

    std::cout << "\t\tAverage Outlet Temperature (degC): "
              << standardTestSummary.avgOutletT(Units::C) << "\n";

    std::cout << "\t\tTotal Volume Drawn (L): " << standardTestSummary.removedVolume(Units::L)
              << "\n";

    std::cout << "\t\tDaily Water-Heating Energy Consumption (kWh): "
              << standardTestSummary.waterHeatingEnergy(Units::kWh) << "\n";

    std::cout << "\t\tAdjusted Daily Water-Heating Energy Consumption (kWh): "
              << standardTestSummary.adjustedConsumedWaterHeatingEnergy(Units::kWh) << "\n";

    std::cout << "\t\tModified Daily Water-Heating Energy Consumption (kWh): "
              << standardTestSummary.modifiedConsumedWaterHeatingEnergy(Units::kWh) << "\n";

    std::cout << "\tAnnual Values:\n";
    std::cout << "\t\tAnnual Electrical Energy Consumption (kWh): "
              << standardTestSummary.annualConsumedElectricalEnergy(Units::kWh) << "\n";

    std::cout << "\t\tAnnual Energy Consumption (kWh): "
              << standardTestSummary.annualConsumedEnergy(Units::kWh) << "\n";

    if (standardTestOptions.saveOutput)
    {
        standardTestOptions.outputFile.close();
    }
}

void HPWH::makeGeneric(const double targetUEF)
{
    static HPWH::FirstHourRating firstHourRating;
    static HPWH::StandardTestOptions standardTestOptions;

    struct Inverter
    {
        static bool getLeftDampedInv(const double nu,
                                     const std::vector<double>& matV, // 1 x 2
                                     std::vector<double>& invMatV     // 2 x 1
        )
        {
            constexpr double thresh = 1.e-12;

            if (matV.size() != 2)
            {
                return false;
            }

            double a = matV[0];
            double b = matV[1];

            double A = (1. + nu) * a * a;
            double B = (1. + nu) * b * b;
            double C = a * b;
            double det = A * B - C * C;

            if (abs(det) < thresh)
            {
                return false;
            }

            invMatV.resize(2);
            invMatV[0] = (a * B - b * C) / det;
            invMatV[1] = (-a * C + b * A) / det;
            return true;
        }
    };

    struct ParamInfo
    {
        enum class Type
        {
            none,
            copCoef
        };

        virtual Type paramType() = 0;

        virtual void assign(HPWH& hpwh, double*& val) = 0;
        virtual void showInfo(std::ostream& os) = 0;
    };

    struct CopCoefInfo : public ParamInfo
    {
        unsigned heatSourceIndex;
        unsigned tempIndex;
        unsigned power;

        Type paramType() override { return Type::copCoef; }

        CopCoefInfo(const unsigned heatSourceIndex_in,
                    const unsigned tempIndex_in,
                    const unsigned power_in)
            : ParamInfo()
            , heatSourceIndex(heatSourceIndex_in)
            , tempIndex(tempIndex_in)
            , power(power_in)
        {
        }

        void assign(HPWH& hpwh, double*& val) override
        {
            val = nullptr;
            HPWH::HeatSource* heatSource;
            hpwh.getNthHeatSource(heatSourceIndex, heatSource);

            auto& perfMap = heatSource->perfMap;
            if (tempIndex >= perfMap.size())
            {
                hpwh.send_error("Invalid heat-source performance-map temperature index.");
            }

            auto& perfPoint = perfMap[tempIndex];
            auto& copCoeffs = perfPoint.COP_coeffs;
            if (power >= copCoeffs.size())
            {
                hpwh.send_error("Invalid heat-source performance-map cop-coefficient power.");
            }
            std::cout << "Valid parameter:";
            showInfo(std::cout);
            std::cout << "\n";
            val = &copCoeffs[power];
        };

        void showInfo(std::ostream& os) override
        {
            os << " heat-source index = " << heatSourceIndex;
            os << ", temperature index = " << tempIndex;
            os << ", power = " << power;
        }
    };

    struct Param
    {
        double* val;
        double dVal;

        Param() : val(nullptr), dVal(1.e3) {}

        virtual void assignVal(HPWH& hpwh) = 0;
        virtual void show(std::ostream& os) = 0;
    };

    struct CopCoef : public Param,
                     CopCoefInfo
    {
        CopCoef(CopCoefInfo& copCoefInfo) : Param(), CopCoefInfo(copCoefInfo) { dVal = 1.e-9; }

        void assignVal(HPWH& hpwh) override { assign(hpwh, val); }

        void show(std::ostream& os) override
        {
            showInfo(os);
            os << ": " << *val << "\n";
        }
    };

    struct Merit
    {
        double val;
        double targetVal;
        double tolVal;

        Merit() : val(0.), targetVal(0.), tolVal(1.e-6) {}

        virtual void eval(HPWH& hpwh) = 0;
        virtual void evalDiff(HPWH& hpwh, double& diff) = 0;
    };

    struct UEF_Merit : public Merit
    {
        UEF_Merit(double targetVal_in) : Merit() { targetVal = targetVal_in; }

        void eval(HPWH& hpwh) override
        {
            static HPWH::StandardTestSummary standardTestSummary;
            hpwh.run24hrTest(firstHourRating, standardTestSummary, standardTestOptions);
            val = standardTestSummary.UEF;
        }

        void evalDiff(HPWH& hpwh, double& diff) override
        {
            eval(hpwh);
            diff = (val - targetVal) / tolVal;
        }
    };

    standardTestOptions.saveOutput = false;
    standardTestOptions.changeSetpoint = true;
    standardTestOptions.nTestTCouples = 6;
    standardTestOptions.setpointT = {51.7, Units::C};

    findFirstHourRating(firstHourRating, standardTestOptions);
    if (customTestOptions.overrideFirstHourRating)
    {
        firstHourRating.desig = customTestOptions.desig;
        const std::string sFirstHourRatingDesig =
            HPWH::FirstHourRating::sDesigMap[firstHourRating.desig];
        std::cout << "\t\tUser-Specified Designation: " << sFirstHourRatingDesig << "\n";
    }

    // set up merit parameter
    Merit* pMerit;
    UEF_Merit uefMerit(targetUEF);
    pMerit = &uefMerit;

    // set up parameters
    std::vector<Param*> pParams;

    CopCoefInfo copT2constInfo = {2, 1, 0}; // heatSourceIndex, tempIndex, power
    CopCoefInfo copT2linInfo = {2, 1, 1};   // heatSourceIndex, tempIndex, power

    CopCoef copT2const(copT2constInfo);
    CopCoef copT2lin(copT2linInfo);

    pParams.push_back(&copT2const);
    pParams.push_back(&copT2lin);

    std::vector<double> dParams;

    //
    for (auto& pParam : pParams)
    {
        pParam->assignVal(*this);
        dParams.push_back(pParam->dVal);
    }

    auto nParams = pParams.size();

    double nu = 0.1;
    const int maxIters = 20;
    for (auto iter = 0; iter < maxIters; ++iter)
    {
        pMerit->eval(*this);
        std::cout << iter << ": ";
        std::cout << "UEF: " << pMerit->val << "; ";

        bool first = true;
        for (std::size_t j = 0; j < nParams; ++j)
        {
            if (!first)
                std::cout << ",";

            std::cout << " " << j << ": ";
            std::cout << *(pParams[j]->val);
            first = false;
        }

        double dMerit0 = 0.;
        pMerit->evalDiff(*this, dMerit0);
        double FOM0 = dMerit0 * dMerit0;
        double FOM1 = 0., FOM2 = 0.;
        std::cout << ", FOM: " << FOM0 << "\n";

        std::vector<double> paramV(nParams);
        for (std::size_t i = 0; i < nParams; ++i)
        {
            paramV[i] = *(pParams[i]->val);
        }

        std::vector<double> jacobiV(nParams); // 1 x 2
        for (std::size_t j = 0; j < nParams; ++j)
        {
            *(pParams[j]->val) = paramV[j] + (pParams[j]->dVal);
            double dMerit;
            pMerit->evalDiff(*this, dMerit);
            jacobiV[j] = (dMerit - dMerit0) / (pParams[j]->dVal);
            *(pParams[j]->val) = paramV[j];
        }

        // try nu
        std::vector<double> invJacobiV1;
        bool got1 = Inverter::getLeftDampedInv(nu, jacobiV, invJacobiV1);

        std::vector<double> inc1ParamV(2);
        std::vector<double> paramV1 = paramV;
        if (got1)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc1ParamV[j] = -invJacobiV1[j] * dMerit0;
                paramV1[j] += inc1ParamV[j];
                *(pParams[j]->val) = paramV1[j];
            }
            double dMerit;
            pMerit->evalDiff(*this, dMerit);
            FOM1 = dMerit * dMerit;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j];
            }
        }

        // try nu / 2
        std::vector<double> invJacobiV2;
        bool got2 = Inverter::getLeftDampedInv(nu / 2., jacobiV, invJacobiV2);

        std::vector<double> inc2ParamV(2);
        std::vector<double> paramV2 = paramV;
        if (got2)
        {
            for (std::size_t j = 0; j < nParams; ++j)
            {
                inc2ParamV[j] = -invJacobiV2[j] * dMerit0;
                paramV2[j] += inc2ParamV[j];
                *(pParams[j]->val) = paramV2[j];
            }
            double dMerit;
            pMerit->evalDiff(*this, dMerit);
            FOM2 = dMerit * dMerit;

            // restore
            for (std::size_t j = 0; j < nParams; ++j)
            {
                *(pParams[j]->val) = paramV[j];
            }
        }

        // check for improvement
        if (got1 && got2)
        {
            if ((FOM1 < FOM0) || (FOM2 < FOM0))
            { // at least one improved
                if (FOM1 < FOM2)
                { // pick 1
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        *(pParams[i]->val) = paramV1[i];
                        (pParams[i]->dVal) = inc1ParamV[i] / 1.e3;
                        FOM0 = FOM1;
                    }
                }
                else
                { // pick 2
                    for (std::size_t i = 0; i < nParams; ++i)
                    {
                        *(pParams[i]->val) = paramV2[i];
                        (pParams[i]->dVal) = inc2ParamV[i] / 1.e3;
                        FOM0 = FOM2;
                    }
                }
            }
            else
            { // no improvement
                nu *= 10.;
                if (nu > 1.e6)
                {
                    send_error("Failure in makeGenericModel");
                }
            }
        }
    }
}
