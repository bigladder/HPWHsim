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

const int HPWH::HPWH_ABORT = -274000;

template <typename T>
HPWH::Units::ConversionMap<T> HPWH::Units::conversionMap;

template <>
HPWH::Units::ConversionMap<HPWH::Units::Temp> HPWH::Units::conversionMap<HPWH::Units::Temp> = {
    {std::make_pair(HPWH::Units::Temp::F, HPWH::Units::Temp::F), &ident},
    {std::make_pair(HPWH::Units::Temp::C, HPWH::Units::Temp::C), &ident},
    {std::make_pair(HPWH::Units::Temp::C, HPWH::Units::Temp::F), &C_TO_F},
    {std::make_pair(HPWH::Units::Temp::F, HPWH::Units::Temp::C), &F_TO_C}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::TempDiff>
    HPWH::Units::conversionMap<HPWH::Units::TempDiff> = {
        {std::make_pair(HPWH::Units::TempDiff::F, HPWH::Units::TempDiff::F), ident},
        {std::make_pair(HPWH::Units::TempDiff::C, HPWH::Units::TempDiff::C), ident},
        {std::make_pair(HPWH::Units::TempDiff::C, HPWH::Units::TempDiff::F), dC_TO_dF},
        {std::make_pair(HPWH::Units::TempDiff::F, HPWH::Units::TempDiff::C), dF_TO_dC}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::Energy> HPWH::Units::conversionMap<HPWH::Units::Energy> = {
    {std::make_pair(HPWH::Units::Energy::kJ, HPWH::Units::Energy::kJ), ident},
    {std::make_pair(HPWH::Units::Energy::kWh, HPWH::Units::Energy::kWh), ident},
    {std::make_pair(HPWH::Units::Energy::Btu, HPWH::Units::Energy::Btu), ident},
    {std::make_pair(HPWH::Units::Energy::kJ, HPWH::Units::Energy::kWh), KJ_TO_KWH},
    {std::make_pair(HPWH::Units::Energy::kJ, HPWH::Units::Energy::Btu), KJ_TO_BTU},
    {std::make_pair(HPWH::Units::Energy::kWh, HPWH::Units::Energy::kJ), KWH_TO_KJ},
    {std::make_pair(HPWH::Units::Energy::kWh, HPWH::Units::Energy::Btu), KWH_TO_BTU},
    {std::make_pair(HPWH::Units::Energy::Btu, HPWH::Units::Energy::kJ), BTU_TO_KJ},
    {std::make_pair(HPWH::Units::Energy::Btu, HPWH::Units::Energy::kWh), BTU_TO_KWH}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::Power> HPWH::Units::conversionMap<HPWH::Units::Power> = {
    {std::make_pair(HPWH::Units::Power::kW, HPWH::Units::Power::kW), ident},
    {std::make_pair(HPWH::Units::Power::Btu_per_h, HPWH::Units::Power::Btu_per_h), ident},
    {std::make_pair(HPWH::Units::Power::W, HPWH::Units::Power::W), ident},
    {std::make_pair(HPWH::Units::Power::kJ_per_h, HPWH::Units::Power::kJ_per_h), ident},
    {std::make_pair(HPWH::Units::Power::kW, HPWH::Units::Power::Btu_per_h), KW_TO_BTUperH},
    {std::make_pair(HPWH::Units::Power::kW, HPWH::Units::Power::W), KW_TO_W},
    {std::make_pair(HPWH::Units::Power::kW, HPWH::Units::Power::kJ_per_h), KW_TO_KJperH},
    {std::make_pair(HPWH::Units::Power::Btu_per_h, HPWH::Units::Power::kW), BTUperH_TO_KW},
    {std::make_pair(HPWH::Units::Power::Btu_per_h, HPWH::Units::Power::W), BTUperH_TO_W},
    {std::make_pair(HPWH::Units::Power::Btu_per_h, HPWH::Units::Power::kJ_per_h),
     BTUperH_TO_KJperH},
    {std::make_pair(HPWH::Units::Power::W, HPWH::Units::Power::kW), W_TO_KW},
    {std::make_pair(HPWH::Units::Power::W, HPWH::Units::Power::Btu_per_h), W_TO_BTUperH},
    {std::make_pair(HPWH::Units::Power::W, HPWH::Units::Power::kJ_per_h), W_TO_KJperH},
    {std::make_pair(HPWH::Units::Power::kJ_per_h, HPWH::Units::Power::kW), KJperH_TO_KW},
    {std::make_pair(HPWH::Units::Power::kJ_per_h, HPWH::Units::Power::Btu_per_h),
     KJperH_TO_BTUperH},
    {std::make_pair(HPWH::Units::Power::kJ_per_h, HPWH::Units::Power::W), KJperH_TO_W}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::Length> HPWH::Units::conversionMap<HPWH::Units::Length> = {
    {std::make_pair(HPWH::Units::Length::m, HPWH::Units::Length::m), &ident},
    {std::make_pair(HPWH::Units::Length::ft, HPWH::Units::Length::ft), &ident},
    {std::make_pair(HPWH::Units::Length::m, HPWH::Units::Length::ft), &M_TO_FT},
    {std::make_pair(HPWH::Units::Length::ft, HPWH::Units::Length::m), &FT_TO_M}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::Area> HPWH::Units::conversionMap<HPWH::Units::Area> = {
    {std::make_pair(HPWH::Units::Area::m2, HPWH::Units::Area::m2), &ident},
    {std::make_pair(HPWH::Units::Area::ft2, HPWH::Units::Area::ft2), &ident},
    {std::make_pair(HPWH::Units::Area::m2, HPWH::Units::Area::ft2), &M2_TO_FT2},
    {std::make_pair(HPWH::Units::Area::ft2, HPWH::Units::Area::m2), &FT2_TO_M2}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::Volume> HPWH::Units::conversionMap<HPWH::Units::Volume> = {
    {std::make_pair(HPWH::Units::Volume::L, HPWH::Units::Volume::L), ident},
    {std::make_pair(HPWH::Units::Volume::gal, HPWH::Units::Volume::gal), ident},
    {std::make_pair(HPWH::Units::Volume::ft3, HPWH::Units::Volume::ft3), ident},
    {std::make_pair(HPWH::Units::Volume::L, HPWH::Units::Volume::gal), L_TO_GAL},
    {std::make_pair(HPWH::Units::Volume::L, HPWH::Units::Volume::ft3), L_TO_FT3},
    {std::make_pair(HPWH::Units::Volume::gal, HPWH::Units::Volume::L), GAL_TO_L},
    {std::make_pair(HPWH::Units::Volume::gal, HPWH::Units::Volume::ft3), GAL_TO_FT3},
    {std::make_pair(HPWH::Units::Volume::ft3, HPWH::Units::Volume::L), FT3_TO_L},
    {std::make_pair(HPWH::Units::Volume::ft3, HPWH::Units::Volume::gal), FT3_TO_GAL}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::FlowRate>
    HPWH::Units::conversionMap<HPWH::Units::FlowRate> = {
        {std::make_pair(HPWH::Units::FlowRate::L_per_s, HPWH::Units::FlowRate::L_per_s), &ident},
        {std::make_pair(HPWH::Units::FlowRate::gal_per_min, HPWH::Units::FlowRate::gal_per_min),
         &ident},
        {std::make_pair(HPWH::Units::FlowRate::L_per_s, HPWH::Units::FlowRate::gal_per_min),
         &LPS_TO_GPM},
        {std::make_pair(HPWH::Units::FlowRate::gal_per_min, HPWH::Units::FlowRate::L_per_s),
         &GPM_TO_LPS}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::Time> HPWH::Units::conversionMap<HPWH::Units::Time> = {
    {std::make_pair(HPWH::Units::Time::h, HPWH::Units::Time::h), &ident},
    {std::make_pair(HPWH::Units::Time::min, HPWH::Units::Time::min), &ident},
    {std::make_pair(HPWH::Units::Time::s, HPWH::Units::Time::s), &ident},
    {std::make_pair(HPWH::Units::Time::h, HPWH::Units::Time::min), H_TO_MIN},
    {std::make_pair(HPWH::Units::Time::h, HPWH::Units::Time::s), &H_TO_S},
    {std::make_pair(HPWH::Units::Time::min, HPWH::Units::Time::h), MIN_TO_H},
    {std::make_pair(HPWH::Units::Time::min, HPWH::Units::Time::s), &MIN_TO_S},
    {std::make_pair(HPWH::Units::Time::s, HPWH::Units::Time::h), &S_TO_H},
    {std::make_pair(HPWH::Units::Time::s, HPWH::Units::Time::min), &S_TO_MIN}};

template <>
HPWH::Units::ConversionMap<HPWH::Units::UA> HPWH::Units::conversionMap<HPWH::Units::UA> = {
    {std::make_pair(HPWH::Units::UA::kJ_per_hC, HPWH::Units::UA::kJ_per_hC), &ident},
    {std::make_pair(HPWH::Units::UA::Btu_per_hF, HPWH::Units::UA::Btu_per_hF), &ident},
    {std::make_pair(HPWH::Units::UA::kJ_per_hC, HPWH::Units::UA::Btu_per_hF), &KJperHC_TO_BTUperHF},
    {std::make_pair(HPWH::Units::UA::Btu_per_hF, HPWH::Units::UA::kJ_per_hC),
     &BTUperHF_TO_KJperHC}};

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

void linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1)
{
    ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

double expandSeries(const std::vector<double>& coeffs, const double x)
{

    double y = 0.;
    for (auto coeff : coeffs)
    {
        y = coeff + y * x;
    }
    return y;
}

template <typename T>
std::vector<double>
changeSeriesUnits(const std::vector<double>& coeffs, const T fromUnits, const T toUnits)
{
    std::vector<double> newCoeffs = coeffs;
    for (std::size_t j = 0; j < coeffs.size(); ++j)
    {
        for (std::size_t i = j + 1; i < coeffs.size(); ++i)
        {
            newCoeffs[i] = HPWH::Units::convert(coeffs[i], fromUnits, toUnits);
        }
    }
    return newCoeffs;
}

void regressedMethod(
    double& ynew, std::vector<double>& coefficents, double x1, double x2, double x3)
{
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x3 +
           coefficents[4] * x1 * x1 + coefficents[5] * x2 * x2 + coefficents[6] * x3 * x3 +
           coefficents[7] * x1 * x2 + coefficents[8] * x1 * x3 + coefficents[9] * x2 * x3 +
           coefficents[10] * x1 * x2 * x3;
}

void regressedMethodMP(double& ynew, std::vector<double>& coefficents, double x1, double x2)
{
    // Const Tair Tin Tair2 Tin2 TairTin
    ynew = coefficents[0] + coefficents[1] * x1 + coefficents[2] * x2 + coefficents[3] * x1 * x1 +
           coefficents[4] * x2 * x2 + coefficents[5] * x1 * x2;
}

HPWH::PerfPoint::PerfPoint(const double T_in /* 0.*/,
                           const std::vector<double>& inputPower_coeffs_in /*{}*/,
                           std::vector<double> COP_coeffs_in /*{}*/,
                           const Units::Temp unitsTemp_in /*C*/,
                           const Units::Power unitsPower_in /*kW*/)
{
    T_C = Units::convert(T_in, unitsTemp_in, Units::Temp::C);
    Units::TempDiff unitsTempDiff_in =
        (unitsTemp_in == Units::Temp::C) ? Units::TempDiff::C : Units::TempDiff::F;

    inputPower_coeffs_kW = Units::convert(inputPower_coeffs_in, unitsPower_in, Units::Power::kW);
    COP_coeffs = COP_coeffs_in;

    if (inputPower_coeffs_in.size() == 3) // use expandSeries
    {
        inputPower_coeffs_kW = changeSeriesUnits<Units::TempDiff>(
            inputPower_coeffs_kW, unitsTempDiff_in, Units::TempDiff::C);
        COP_coeffs =
            changeSeriesUnits<Units::TempDiff>(COP_coeffs, unitsTempDiff_in, Units::TempDiff::C);
        return;
    }

    if (inputPower_coeffs_in.size() == 11) // use regressMethod
    {
        for (std::size_t j : {1, 4, 10})
        {
            for (std::size_t i = j; i < 11; ++i)
            {
                inputPower_coeffs_kW[i] =
                    Units::invert(inputPower_coeffs_kW[i], unitsTempDiff_in, Units::TempDiff::C);
                COP_coeffs[i] = Units::invert(COP_coeffs[i], unitsTempDiff_in, Units::TempDiff::C);
            }
        }
        return;
    }

    if (inputPower_coeffs_in.size() == 6) // use regressMethodMP
    {
        for (std::size_t j : {1, 3})
        {
            for (std::size_t i = j; i < 6; ++i)
            {
                inputPower_coeffs_kW[i] =
                    Units::invert(inputPower_coeffs_kW[i], unitsTempDiff_in, Units::TempDiff::C);
                COP_coeffs[i] = Units::invert(COP_coeffs[i], unitsTempDiff_in, Units::TempDiff::C);
            }
        }
        return;
    }
}

void HPWH::setMinutesPerStep(const double minutesPerStep_in)
{
    minutesPerStep = minutesPerStep_in;
    secondsPerStep = s_per_min * minutesPerStep;
    hoursPerStep = minutesPerStep / min_per_h;
}

// public HPWH functions
HPWH::HPWH() : hpwhVerbosity(VRB_silent), messageCallback(NULL), messageCallbackContextPtr(NULL)
{
    setAllDefaults();
}

void HPWH::setAllDefaults()
{
    tankTs_C.clear();
    nextTankTs_C.clear();
    heatSources.clear();

    simHasFailed = true;
    isHeating = false;
    setpointFixed = false;
    tankSizeFixed = true;
    canScale = false;
    inletT_C = HPWH_ABORT;
    currentSoCFraction = 1.;
    doTempDepression = false;
    locationT_C = UNINITIALIZED_LOCATIONTEMP;
    mixBelowFractionOnDraw = 1. / 3.;
    doInversionMixing = true;
    doConduction = true;
    inletNodeIndex = 0;
    inlet2NodeIndex = 0;
    fittingsUA_kJperhC = 0.;
    prevDRstatus = DR_ALLOW;
    timerLimitTOT = 60.;
    timerTOT = 0.;
    usesSoCLogic = false;
    setMinutesPerStep(1.);
    hpwhVerbosity = VRB_minuteOut;
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
    }

    tankVolume_L = hpwh.tankVolume_L;
    tankUA_kJperhC = hpwh.tankUA_kJperhC;
    fittingsUA_kJperhC = hpwh.fittingsUA_kJperhC;

    setpointT_C = hpwh.setpointT_C;

    tankTs_C = hpwh.tankTs_C;
    nextTankTs_C = hpwh.nextTankTs_C;

    inletNodeIndex = hpwh.inletNodeIndex;
    inlet2NodeIndex = hpwh.inlet2NodeIndex;

    outletT_C = hpwh.outletT_C;
    condenserInletT_C = hpwh.condenserInletT_C;
    condenserOutletT_C = hpwh.condenserOutletT_C;
    externalVolumeHeated_L = hpwh.externalVolumeHeated_L;
    standbyLosses_kJ = hpwh.standbyLosses_kJ;

    tankMixesOnDraw = hpwh.tankMixesOnDraw;
    mixBelowFractionOnDraw = hpwh.mixBelowFractionOnDraw;

    doTempDepression = hpwh.doTempDepression;

    doInversionMixing = hpwh.doInversionMixing;
    doConduction = hpwh.doConduction;

    locationT_C = hpwh.locationT_C;

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
            msg("DR_TOO | DR_TOT use conflicting logic sets. The logic will follow a DR_TOT "
                "scheme "
                " \n");
        }
    }

    if (hpwhVerbosity >= VRB_typical)
    {
        msg("Beginning runOneStep.  \nTank Temps: ");
        printTankTs_C();
        msg("Step Inputs: InletT_C:  %.2lf, drawVolume_L:  %.2lf, tankAmbientT_C:  %.2lf, "
            "heatSourceAmbientT_C:  %.2lf, DRstatus:  %d, minutesPerStep:  %.2lf \n",
            inletT_C,
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
    outletT_C = 0.;
    condenserInletT_C = 0.;
    condenserOutletT_C = 0.;
    externalVolumeHeated_L = 0.;
    standbyLosses_kJ = 0.;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = 0;
        heatSources[i].energyInput_kJ = 0.;
        heatSources[i].energyOutput_kJ = 0.;
        heatSources[i].energyRemovedFromEnvironment_kJ = 0.;
    }
    extraEnergyInput_kJ = 0.;

    // if you are doing temp. depression, set tank and heatSource ambient temps
    // to the tracked locationTemperature
    double temperatureGoal = tankAmbientT_C;
    if (doTempDepression)
    {
        if (locationT_C == UNINITIALIZED_LOCATIONTEMP)
        {
            locationT_C = tankAmbientT_C;
        }
        tankAmbientT_C = locationT_C;
        heatSourceAmbientT_C = locationT_C;
    }

    // process draws and standby losses
    updateTankTemps(drawVolume_L, inletT_C, tankAmbientT_C, inletVol2_L, inletT2_C);

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
                msg("TURNED ON DR_TOO engaged compressor and lowest resistance element, "
                    "DRstatus = "
                    "%i \n",
                    DRstatus);
            }
        }

        // do HeatSource choice
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (hpwhVerbosity >= VRB_emetic)
            {
                msg("Heat source choice:\theatsource %d can choose from %lu turn on logics and "
                    "%lu "
                    "shut off logics\n",
                    i,
                    heatSources[i].turnOnLogicSet.size(),
                    heatSources[i].shutOffLogicSet.size());
            }
            if (isHeating == true)
            {
                // check if anything that is on needs to turn off (generally for lowT cutoffs)
                // things that just turn on later this step are checked for this in shouldHeat
                if (heatSources[i].isEngaged() && heatSources[i].shouldShutOff())
                {
                    heatSources[i].disengageHeatSource();
                    // check if the backup heat source would have to shut off too
                    if (heatSources[i].backupHeatSource != NULL &&
                        heatSources[i].backupHeatSource->shouldShutOff() != true)
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
                    // engaging a heat source sets isHeating to true, so this will only trigger
                    // once
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

                    // Check that the backup isn't locked out too or already engaged then it
                    // will heat on its own.
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
                            msg("Locked out back up heat source AND the engaged heat source "
                                "%i, "
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

                // if it finished early. i.e. shuts off early like if the heatsource met
                // setpoint or maxed out
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
                        heatSources[i].followedByHeatSource->shouldShutOff() == false)
                    {
                        // turn it on
                        heatSources[i].followedByHeatSource->engageHeatSource(DRstatus);
                    }
                    // or if there heat source can't produce hotter water (i.e. it's maxed out)
                    // and the tank still isn't at setpoint. the compressor should get locked
                    // out when the maxedOut is true but have to run the resistance first during
                    // this timestep to make sure tank is above the max temperature for the
                    // compressor.
                    else if (heatSources[i].maxedOut() && heatSources[i].backupHeatSource != NULL)
                    {

                        // Check that the backup isn't locked out or already engaged then it
                        // will heat or already heated on it's own.
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
            temperatureGoal -= maxDepressionT_C; // hardcoded 4.5 degree total drop - from
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
        locationT_C -= (locationT_C - temperatureGoal) * (1 - 0.9289);
    }

    // settle outputs

    // outletTemp_C and standbyLosses_kWh are taken care of in updateTankTemps

    // cursory check for inverted temperature profile
    if (tankTs_C[getNumNodes() - 1] < tankTs_C[0])
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
                    double* inletTs_C,
                    double* drawVolumes_L,
                    double* tankAmbientTs_C,
                    double* heatSourceAmbientTs_C,
                    DRMODES* DRstatus)
{
    // these are all the accumulating variables we'll need
    double standbyLosses_kJ_SUM = 0;
    double outletTemp_C_AVG = 0;
    double totalDrawVolume_L = 0;
    std::vector<double> heatSources_runTimes_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyInputs_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyOutputs_SUM(getNumHeatSources());
    std::vector<double> heatSources_energyRemovedFromEnvironment_SUM(getNumHeatSources());

    if (hpwhVerbosity >= VRB_typical)
    {
        msg("Begin runNSteps.  \n");
    }
    // run the sim one step at a time, accumulating the outputs as you go
    for (int i = 0; i < N; i++)
    {
        runOneStep(inletTs_C[i],
                   drawVolumes_L[i],
                   tankAmbientTs_C[i],
                   heatSourceAmbientTs_C[i],
                   DRstatus[i]);

        if (simHasFailed)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("RunNSteps has encountered an error on step %d of N and has ceased "
                    "running.  "
                    "\n",
                    i + 1);
            }
            return HPWH_ABORT;
        }

        standbyLosses_kJ_SUM += standbyLosses_kJ;

        outletTemp_C_AVG += outletT_C * drawVolumes_L[i];
        totalDrawVolume_L += drawVolumes_L[i];

        for (int j = 0; j < getNumHeatSources(); j++)
        {
            heatSources_runTimes_SUM[j] += getNthHeatSourceRunTime(j);
            heatSources_energyInputs_SUM[j] += getNthHeatSourceEnergyInput(j);
            heatSources_energyOutputs_SUM[j] += getNthHeatSourceEnergyOutput(j);
            heatSources_energyRemovedFromEnvironment_SUM[j] +=
                getNthHeatSourceEnergyRemovedFromEnvironment(j);
        }

        // print minutely output
        if (hpwhVerbosity == VRB_minuteOut)
        {
            msg("%f,%f,%f,", tankAmbientTs_C[i], drawVolumes_L[i], inletTs_C[i]);
            for (int j = 0; j < getNumHeatSources(); j++)
            {
                msg("%f,%f,", getNthHeatSourceEnergyInput(j), getNthHeatSourceEnergyOutput(j));
            }

            std::vector<double> displayTemps_C(10);
            resampleIntensive(displayTemps_C, tankTs_C);
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

                msg("%f", getNthThermocoupleT(k, 6));
            }
            msg("\n");
        }
    }
    // finish weighted avg. of outlet temp by dividing by the total drawn volume
    outletTemp_C_AVG /= totalDrawVolume_L;

    // now, reassign all of the accumulated values to their original spots
    standbyLosses_kJ = standbyLosses_kJ_SUM;
    outletT_C = outletTemp_C_AVG;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        heatSources[i].runtime_min = heatSources_runTimes_SUM[i];
        heatSources[i].energyInput_kJ = heatSources_energyInputs_SUM[i];
        heatSources[i].energyOutput_kJ = heatSources_energyOutputs_SUM[i];
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
            setpointT_C >= heatSourcePtr->maxOut_at_LowT.outT_C)
        {
            tempSetpoint_C = setpointT_C; // Store setpoint
            setSetpointT(heatSourcePtr->maxOut_at_LowT
                             .outT_C); // Reset to new setpoint as this is used in the add heat calc
        }
    }
    // and add heat if it is
    heatSourcePtr->addHeat(heatSourceAmbientT_C, minutesToRun);

    // Change the setpoint back to what it was pre-compressor depression
    if (tempSetpoint_C > -273.15)
    {
        setSetpointT(tempSetpoint_C);
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
        ss << "input energy kWh: " << std::setw(7) << getNthHeatSourceEnergyInput(i) << "\t";
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
        ss << "input power kW: " << std::setw(7) << outputVar << "\t\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        ss << "output energy kWh: " << std::setw(7) << getNthHeatSourceEnergyOutput(i) << "\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        runtime = getNthHeatSourceRunTime(i);
        if (runtime != 0)
        {
            outputVar = getNthHeatSourceEnergyOutput(i) / (runtime / 60.0);
        }
        else
        {
            outputVar = 0;
        }
        ss << "output power kW: " << std::setw(7) << outputVar << "\t";
    }
    ss << endl;

    for (int i = 0; i < getNumHeatSources(); i++)
    {
        ss << "run time min: " << std::setw(7) << getNthHeatSourceRunTime(i) << "\t\t";
    }
    ss << endl << endl << endl;

    msg(ss.str().c_str());
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
                               getNthHeatSourceEnergyInput(iHS) * 1000.,
                               getNthHeatSourceEnergyOutput(iHS) * 1000.);
    }

    for (int iTC = 0; iTC < nTCouples; iTC++)
    {
        outFILE << fmt::format(
            ",{:0.2f}",
            getNthThermocoupleT(iTC + 1, nTCouples, doIP ? Units::Temp::F : Units::Temp::C));
    }

    if (options & HPWH::CSVOPT_IS_DRAWING)
    {
        outFILE << fmt::format(",{:0.2f}", doIP ? C_TO_F(outletT_C) : outletT_C);
    }
    else
    {
        outFILE << ",";
    }

    outFILE << std::endl;

    return 0;
}

double HPWH::calcSoCFraction(double mainsT_C, double minUsefulT_C, double maxT_C) const
{
    // Note that volume is ignored in here since with even nodes it cancels out of the SoC
    // fractional equation
    if (mainsT_C >= minUsefulT_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("mainsT_C is greater than or equal minUsefulT_C. \n");
        }
        return HPWH_ABORT;
    }
    if (minUsefulT_C > maxT_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("minUsefulT_C is greater maxT_C. \n");
        }
        return HPWH_ABORT;
    }

    double chargeEquivalent = 0.;
    for (auto& T : tankTs_C)
    {
        chargeEquivalent += getChargePerNode(mainsT_C, minUsefulT_C, T);
    }
    double maxSoC = getNumNodes() * getChargePerNode(mainsT_C, minUsefulT_C, maxT_C);
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

double HPWH::getChargePerNode(double coldT, double mixT, double hotT) const
{
    if (hotT < mixT)
    {
        return 0.;
    }
    return (hotT - coldT) / (mixT - coldT);
}

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
    doTempDepression = doTempDepress;
    return 0;
}

// Uses the UA before the function is called and adjusts the A part of the UA to match
// the input volume given getTankSurfaceArea().
int HPWH::setTankSizeWithSameU(double tankSize,
                               Units::Volume units /*L*/,
                               bool forceChange /*=false*/)
{
    double tankSize_L = Units::convert(tankSize, units, Units::Volume::L);
    double oldTankU_kJperHCm2 = tankUA_kJperhC / getTankSurfaceArea(Units::Area::m2);

    setTankSize(tankSize_L, Units::Volume::L, forceChange);
    setUA(oldTankU_kJperHCm2 * getTankSurfaceArea(Units::Area::m2), Units::UA::kJ_per_hC);
    return 0;
}

/*static*/ double HPWH::getTankSurfaceArea(double vol,
                                           const Units::Volume volUnits /*L*/,
                                           const Units::Area surfAUnits /*FT2*/)
{
    // returns tank surface area, old defualt was in ft2
    // Based off 88 insulated storage tanks currently available on the market from Sanden,
    // AOSmith, HTP, Rheem, and Niles. Corresponds to the inner tank with volume
    // tankVolume_L with the assumption that the aspect ratio is the same as the outer
    // dimenisions of the whole unit.
    double radius = getTankRadius(vol, volUnits, Units::Length::ft);
    return Units::convert(
        2. * Pi * pow(radius, 2) * (ASPECTRATIO + 1.), Units::Area::ft2, surfAUnits);
}

double HPWH::getTankSurfaceArea(const Units::Area units /*FT2*/) const
{
    return getTankSurfaceArea(tankVolume_L, Units::Volume::L, units);
}

/*static*/ double HPWH::getTankRadius(double vol,
                                      const Units::Volume volUnits /*L*/,
                                      const Units::Length radiusUnits /*FT*/)
{ // returns tank radius, ft for use in calculation of heat loss in the bottom and top of
  // the tank.
    // Based off 88 insulated storage tanks currently available on the market from Sanden,
    // AOSmith, HTP, Rheem, and Niles, assumes the aspect ratio for the outer measurements
    // is the same is the actual tank.

    double vol_ft3 = Units::convert(vol, volUnits, Units::Volume::ft3);

    double radius_ft = -1.;
    if (vol_ft3 >= 0.)
    {
        radius_ft = Units::convert(
            pow(vol_ft3 / Pi / ASPECTRATIO, 1. / 3.), Units::Length::ft, radiusUnits);
    }
    return radius_ft;
}

double HPWH::getTankRadius(const Units::Length units /*FT*/) const
{
    // returns tank radius, ft for use in calculation of heat loss in the bottom and top of
    // the tank. Based off 88 insulated storage tanks currently available on the market from
    // Sanden, AOSmith, HTP, Rheem, and Niles, assumes the aspect ratio for the outer
    // measurements is the same is the actual tank.

    double value = getTankRadius(tankVolume_L, Units::Volume::L, units);
    if (value < 0.)
    {
        if (hpwhVerbosity >= VRB_reluctant)
            msg("Incorrect unit specification for getTankRadius.  \n");
        value = HPWH_ABORT;
    }
    return value;
}

bool HPWH::isTankSizeFixed() const { return tankSizeFixed; }

int HPWH::setTankSize(const double volume, Units::Volume units /*L*/, bool forceChange /*=false*/)
{
    if (isTankSizeFixed() && !forceChange)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not change the tank size for your currently selected model.  \n");
        }
        return HPWH_ABORT;
    }
    if (volume <= 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to set the tank volume outside of bounds.  \n");
        }
        simHasFailed = true;
        return HPWH_ABORT;
    }
    tankVolume_L = Units::convert(volume, units, Units::Volume::L);
    calcSizeConstants();
    return 0;
}

int HPWH::setDoInversionMixing(bool doInvMix)
{
    this->doInversionMixing = doInvMix;
    return 0;
}

int HPWH::setDoConduction(bool doCondu)
{
    this->doConduction = doCondu;
    return 0;
}

int HPWH::setUA(const double UA, const Units::UA units /*KJperHC*/)
{
    tankUA_kJperhC = Units::convert(UA, units, Units::UA::kJ_per_hC);
    return 0;
}

int HPWH::getUA(double& UA, Units::UA units /*KJperHC*/) const
{
    UA = Units::convert(tankUA_kJperhC, Units::UA::kJ_per_hC, units);
    return 0;
}

int HPWH::setFittingsUA(const double UA, const Units::UA units /*KJperHC*/)
{
    fittingsUA_kJperhC = Units::convert(UA, units, Units::UA::kJ_per_hC);
    return 0;
}

int HPWH::getFittingsUA(double& UA, const Units::UA units /*KJperHC*/) const
{
    UA = Units::convert(fittingsUA_kJperhC, Units::UA::kJ_per_hC, units);
    return 0;
}

int HPWH::setInletByFraction(double fractionalHeight)
{
    return getFractionalHeightNodeIndex(fractionalHeight, inletNodeIndex);
}

int HPWH::setInlet2ByFraction(double fractionalHeight)
{
    return getFractionalHeightNodeIndex(fractionalHeight, inlet2NodeIndex);
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
                returnVal = getFractionalHeightNodeIndex(fractionalHeight,
                                                         heatSources[i].externalInletNodeIndex);
            }
            else
            {
                returnVal = getFractionalHeightNodeIndex(fractionalHeight,
                                                         heatSources[i].externalOutletNodeIndex);
            }

            if (returnVal == HPWH_ABORT)
            {
                return returnVal;
            }
        }
    }
    return returnVal;
}

int HPWH::getFractionalHeightNodeIndex(double fractionalHeight, int& nodeIndex)
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
    nodeIndex = (node == getNumNodes()) ? getTopNodeIndex() : node;

    return 0;
}

int HPWH::getExternalInletNodeIndex() const
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
            return heatSources[i].externalInletNodeIndex; // Return the first one since all
                                                          // external sources have some ports
        }
    }
    return HPWH_ABORT;
}
int HPWH::getExternalOutletNodeIndex() const
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
            return heatSources[i].externalOutletNodeIndex; // Return the first one since all
                                                           // external sources have some ports
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

int HPWH::getInletNodeIndex(int whichInlet) const
{
    if (whichInlet == 1)
    {
        return inletNodeIndex;
    }
    else if (whichInlet == 2)
    {
        return inlet2NodeIndex;
    }
    else
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Invalid inlet chosen in getInletNodeIndex \n");
        }
        return HPWH_ABORT;
    }
}

int HPWH::setMaxDepressionT(double maxDepressionT, Units::Temp units /*C*/)
{
    maxDepressionT_C = Units::convert(maxDepressionT, units, Units::Temp::C);
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

int HPWH::setEnteringWaterHighTempShutOff(double highT,
                                          bool tempIsAbsolute,
                                          int heatSourceIndex,
                                          Units::Temp unit /*C*/)
{
    if (!hasEnteringWaterHighTempShutOff(heatSourceIndex))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to acess a heating logic that does not exist.  \n");
        }
        return HPWH_ABORT;
    }

    double highT_C = Units::convert(highT, unit, Units::Temp::C);

    bool highTempIsNotValid = false;
    if (tempIsAbsolute)
    {
        // check difference with setpoint
        if (setpointT_C - highT_C < MINSINGLEPASSLIFT)
        {
            highTempIsNotValid = true;
        }
    }
    else
    {
        if (highT_C < MINSINGLEPASSLIFT)
        {
            highTempIsNotValid = true;
        }
    }
    if (highTempIsNotValid)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("High temperature shut off is too close to the setpoint, excpected a "
                "minimum "
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
                ->setDecisionPoint(highT_C, tempIsAbsolute);
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
                              Units::Temp tempUnit /*C*/)
{
    if (!canUseSoCControls())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Cannot set up state of charge controls for integrated or wrapped "
                "HPWHs.\n");
        }
        return HPWH_ABORT;
    }

    double tempMinUseful_C = Units::convert(tempMinUseful, tempUnit, Units::Temp::C);
    double mainsT_C = Units::convert(mainsT, tempUnit, Units::Temp::C);

    if (mainsT_C >= tempMinUseful_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The mains temperature can't be equal to or greater than the minimum "
                "useful "
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
                                                             const bool absolute /* = false */)
{
    auto nodeWeights = getNodeWeightRange(0., 1.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "whole tank", nodeWeights, decisionPoint, this, absolute);
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
                                                               const bool absolute /* = false */)
{
    auto nodeWeights = getNodeWeightRange(1. / 3., 2. / 3.);
    return std::make_shared<HPWH::TempBasedHeatingLogic>(
        "second third", nodeWeights, decisionPoint, this, absolute);
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
    tankTs_C.resize(num_nodes);
    nextTankTs_C.resize(num_nodes);
}

int HPWH::getNumNodes() const { return static_cast<int>(tankTs_C.size()); }

int HPWH::getTopNodeIndex() const { return getNumNodes() - 1; }

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

double HPWH::getCompressorCapacity(double airTemp /*19.722*/,
                                   double inletTemp /*14.444*/,
                                   double outTemp /*57.222*/,
                                   Units::Power pwrUnit /*kW*/,
                                   Units::Temp tempUnit /*C*/)
{
    // calculate capacity, input power, and cop
    double tempCap_kW, tempInput_kW, temp_cop; // temporary variables
    double airTemp_C, inletTemp_C, outTemp_C;

    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model does not have a compressor.  \n");
        }
        return double(HPWH_ABORT);
    }

    airTemp_C = Units::convert(airTemp, tempUnit, Units::Temp::C);
    inletTemp_C = Units::convert(inletTemp, tempUnit, Units::Temp::C);
    outTemp_C = Units::convert(outTemp, tempUnit, Units::Temp::C);

    if (airTemp_C < heatSources[compressorIndex].minT_C ||
        airTemp_C > heatSources[compressorIndex].maxT_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The compress does not operate at the specified air temperature. \n");
        }
        return double(HPWH_ABORT);
    }

    double maxAllowedSetpoint_C =
        heatSources[compressorIndex].maxSetpointT_C -
        heatSources[compressorIndex].secondaryHeatExchanger.hotSideOffsetT_C;
    if (outTemp_C > maxAllowedSetpoint_C)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Inputted outlet temperature of the compressor is higher than can be "
                "produced.");
        }
        return double(HPWH_ABORT);
    }

    if (heatSources[compressorIndex].isExternalMultipass())
    {
        double averageTemp_C = (outTemp_C + inletTemp_C) / 2.;
        heatSources[compressorIndex].getCapacityMP(
            airTemp_C, averageTemp_C, tempInput_kW, tempCap_kW, temp_cop);
    }
    else
    {
        heatSources[compressorIndex].getCapacity(
            airTemp_C, inletTemp_C, outTemp_C, tempInput_kW, tempCap_kW, temp_cop);
    }

    return Units::convert(tempCap_kW, Units::Power::kW, pwrUnit);
}

bool HPWH::isHeatSourceIndexValid(const int n) const
{
    if ((n >= getNumHeatSources()) || n < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access a heat source that does not exist.\n");
        }
        return false;
    }
    return true;
}

double HPWH::getInputEnergy_kJ() const
{
    double energy_kJ = 0.;
    for (auto& heatSource : heatSources)
    {
        energy_kJ += heatSource.energyInput_kJ;
    }
    return energy_kJ;
}

double HPWH::getOutputEnergy_kJ() const
{
    double energy_kJ = 0.;
    for (auto& heatSource : heatSources)
    {
        energy_kJ += heatSource.energyInput_kJ;
    }
    return energy_kJ;
}

double HPWH::getEnergyRemovedFromEnvironment_kJ() const
{
    double energy_kJ = 0.;
    for (auto& heatSource : heatSources)
    {
        energy_kJ += heatSource.energyRemovedFromEnvironment_kJ;
    }
    return energy_kJ;
}

double HPWH::getTankHeatContent_kJ() const
{
    // returns tank heat content relative to 0 C using kJ

    // sum over nodes
    double totalT_C = 0.;
    for (int i = 0; i < getNumNodes(); i++)
    {
        totalT_C += tankTs_C[i];
    }
    return nodeCp_kJperC * totalT_C;
}

double HPWH::getHeatContent_kJ() const { return getTankHeatContent_kJ(); }

double HPWH::getInputEnergy(const Units::Energy units /*kWh*/) const
{
    return Units::convert(getInputEnergy_kJ(), Units::Energy::kJ, units);
}

double HPWH::getOutputEnergy(const Units::Energy units /*kWh*/) const
{
    return Units::convert(getOutputEnergy_kJ(), Units::Energy::kJ, units);
}

double HPWH::getTankHeatContent(const Units::Energy units /*kWh*/) const
{
    return Units::convert(getTankHeatContent_kJ(), Units::Energy::kJ, units);
}

double HPWH::getHeatContent(const Units::Energy units /*kWh*/) const
{
    return Units::convert(getHeatContent_kJ(), Units::Energy::kJ, units);
}

double HPWH::getStandbyLosses(Units::Energy units /*kWh*/) const
{
    return Units::convert(getStandbyLosses_kJ(), Units::Energy::kJ, units);
}

double HPWH::getEnergyRemovedFromEnvironment(const Units::Energy units /*kWh*/) const
{
    return Units::convert(getEnergyRemovedFromEnvironment_kJ(), Units::Energy::kJ, units);
}

double HPWH::getNthHeatSourceEnergyInput(int N, Units::Energy units /*kWh*/) const
{
    return (isHeatSourceIndexValid(N))
               ? Units::convert(heatSources[N].energyInput_kJ, Units::Energy::kJ, units)
               : double(HPWH_ABORT);
}

double HPWH::getNthHeatSourceEnergyOutput(int N, Units::Energy units /*kWh*/) const
{
    return (isHeatSourceIndexValid(N))
               ? Units::convert(heatSources[N].energyOutput_kJ, Units::Energy::kJ, units)
               : double(HPWH_ABORT);
}

double HPWH::getNthHeatSourceEnergyRemovedFromEnvironment(int N, Units::Energy units /*kWh*/) const
{
    return (isHeatSourceIndexValid(N))
               ? Units::convert(
                     heatSources[N].energyRemovedFromEnvironment_kJ, Units::Energy::kJ, units)
               : double(HPWH_ABORT);
}

double HPWH::getNthHeatSourceRunTime(int N) const
{
    return isHeatSourceIndexValid(N) ? heatSources[N].runtime_min : double(HPWH_ABORT);
}

int HPWH::isNthHeatSourceRunning(int N) const
{
    return isHeatSourceIndexValid(N) ? (heatSources[N].isEngaged() ? 1 : 0) : HPWH_ABORT;
}

HPWH::HEATSOURCE_TYPE HPWH::getNthHeatSourceType(int N) const
{
    return isHeatSourceIndexValid(N) ? heatSources[N].typeOfHeatSource : HEATSOURCE_TYPE::TYPE_none;
}

double HPWH::getTankSize(Units::Volume units /*L*/) const
{
    return Units::convert(tankVolume_L, Units::Volume::L, units);
}

int HPWH::setSetpointT_C(const double setpointT_C_in)
{
    double maxSetpointT_C = -273.15;
    std::string why;
    if (!canApplySetpointT_C(setpointT_C_in, maxSetpointT_C, why))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Unwilling to set this setpoint, max setpoint is %f C. %s\n",
                maxSetpointT_C,
                why.c_str());
        }
        return HPWH_ABORT;
    }

    setpointT_C = setpointT_C_in;
    return 0;
}

int HPWH::setTankTs_C(std::vector<double> tankTs_C_in)
{
    if (tankTs_C_in.size() == 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("No temperatures provided.\n");
        }
        return HPWH_ABORT;
    }

    if (!resampleIntensive(tankTs_C, tankTs_C_in))
    {
        return HPWH_ABORT;
    }

    return 0;
}

int HPWH::setTankT_C(double tankT_C) { return setTankTs_C({tankT_C}); }

void HPWH::getTankTs_C(std::vector<double>& tankTemps) const { tankTemps = tankTs_C; }

double HPWH::tankAvg_C(const std::vector<HPWH::NodeWeight> nodeWeights) const
{
    double sum = 0;
    double totWeight = 0;

    std::vector<double> resampledTankTemps(LOGIC_SIZE);
    resample(resampledTankTemps, tankTs_C);

    for (auto& nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum == 0)
        { // bottom node only
            sum += tankTs_C.front() * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
        else if (nodeWeight.nodeNum > LOGIC_SIZE)
        { // top node only
            sum += tankTs_C.back() * nodeWeight.weight;
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

double HPWH::getTankNodeT_C(const int nodeNum) const
{
    if (tankTs_C.empty())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the temperature of a tank node that does not "
                "exist.\n");
        }
        return double(HPWH_ABORT);
    }
    return tankTs_C[nodeNum];
}

double HPWH::getMinOperatingT_C() const
{
    if (hasCompressor())
    {
        return heatSources[compressorIndex].minT_C;
    }
    return double(HPWH_ABORT);
}

double HPWH::getMaxCompressorSetpointT_C() const
{
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Unit does not have a compressor \n");
        }
        return HPWH_ABORT;
    }

    return heatSources[compressorIndex].maxSetpointT_C;
}

double HPWH::getNthThermocoupleT_C(const int iTCouple, const int nTCouple) const
{
    if (iTCouple > nTCouple || iTCouple < 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access a simulated thermocouple that does not "
                "exist.\n");
        }
        return double(HPWH_ABORT);
    }

    double beginFraction = static_cast<double>(iTCouple - 1.) / static_cast<double>(nTCouple);
    double endFraction = static_cast<double>(iTCouple) / static_cast<double>(nTCouple);

    return getResampledValue(tankTs_C, beginFraction, endFraction);
}

void HPWH::printTankTs_C()
{
    std::stringstream ss;

    ss << std::left;

    for (int i = 0; i < getNumNodes(); i++)
    {
        ss << std::setw(9) << getTankNodeT(i) << " ";
    }
    ss << endl;

    msg(ss.str().c_str());
}

double HPWH::getOutletT(const Units::Temp units /*C*/) const
{
    return Units::convert(getOutletT_C(), Units::Temp::C, units);
}

double HPWH::getCondenserInletT(const Units::Temp units /*C*/) const
{
    return Units::convert(getCondenserInletT_C(), Units::Temp::C, units);
}

double HPWH::getCondenserOutletT(const Units::Temp units /*C*/) const
{
    return Units::convert(getCondenserOutletT_C(), Units::Temp::C, units);
}

double HPWH::getSetpointT(const Units::Temp units /*C*/) const
{
    return Units::convert(getSetpointT_C(), Units::Temp::C, units);
}

double HPWH::getMaxCompressorSetpointT(const Units::Temp units /*C*/) const
{
    if (!hasACompressor())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Unit does not have a compressor \n");
        }
        return double(HPWH_ABORT);
    }
    return Units::convert(heatSources[compressorIndex].maxSetpointT_C, Units::Temp::C, units);
}

double HPWH::getTankNodeT(const int nodeNum, const Units::Temp units /*C*/) const
{
    if (tankTs_C.empty())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access the temperature of a tank node that does not "
                "exist.\n");
        }
        return double(HPWH_ABORT);
    }
    return Units::convert(getTankNodeT_C(nodeNum), Units::Temp::C, units);
}

double HPWH::getNthThermocoupleT(const int iTCouple,
                                 const int nTCouple,
                                 const Units::Temp units /*C*/) const
{
    if (iTCouple > nTCouple || iTCouple < 1)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("You have attempted to access a simulated thermocouple that does not "
                "exist.\n");
        }
        return double(HPWH_ABORT);
    }
    double beginFraction = static_cast<double>(iTCouple - 1.) / static_cast<double>(nTCouple);
    double endFraction = static_cast<double>(iTCouple) / static_cast<double>(nTCouple);
    double simTcoupleTemp_C = getResampledValue(tankTs_C, beginFraction, endFraction);

    return Units::convert(simTcoupleTemp_C, Units::Temp::C, units);
}

double HPWH::getMinOperatingT(const Units::Temp units /*C*/) const
{
    return hasCompressor()
               ? Units::convert(heatSources[compressorIndex].minT_C, Units::Temp::C, units)
               : double(HPWH_ABORT);
}

//-----------------------------------------------------------------------------
///	@brief	Assigns new temps provided from a std::vector to tankTs_C.
/// @param[in]	setTankTemps	new tank temps (arbitrary non-zero size)
///	@param[in]	units          temp units in setTankTemps (default = UNITS_C)
/// @return	Success: 0; Failure: HPWH_ABORT
//-----------------------------------------------------------------------------
int HPWH::setTankTs(std::vector<double> setTankTs, const Units::Temp units /*C*/)
{
    std::size_t numSetNodes = setTankTs.size();
    if (numSetNodes == 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("No temperatures provided.\n");
        }
        return HPWH_ABORT;
    }

    // Units::convert setTankTemps to degC, if necessary
    if (units == Units::Temp::F)
        for (auto& T : setTankTs)
            T = F_TO_C(T);

    // set node temps
    if (!resampleIntensive(tankTs_C, setTankTs))
        return HPWH_ABORT;

    return 0;
}

int HPWH::setSetpointT(const double setpointT, const Units::Temp units /*C*/)
{
    return setSetpointT_C(Units::convert(setpointT, units, Units::Temp::C));
}

bool HPWH::canApplySetpointT_C(double newSetpointT_C,
                               double& maxSetpointT_C,
                               std::string& why) const
{
    maxSetpointT_C = -273.15;
    if (isSetpointFixed())
    {
        maxSetpointT_C = setpointT_C;
        if (newSetpointT_C == maxSetpointT_C)
        {
            return true;
        }
        else
        {
            why = "The set point is fixed for the currently selected model.";
            return false;
        }
    }

    if (hasACompressor())
    { // check the new setpoint against the compressor's max setpoint
        maxSetpointT_C = heatSources[compressorIndex].maxSetpointT_C -
                         heatSources[compressorIndex].secondaryHeatExchanger.hotSideOffsetT_C;
    }

    if (lowestElementIndex >= 0)
    { // check the new setpoint against the resistor's max setpoint
        maxSetpointT_C = std::max(heatSources[lowestElementIndex].maxSetpointT_C, maxSetpointT_C);
    }

    if (newSetpointT_C > maxSetpointT_C)
    {
        if (model == MODELS_StorageTank)
        {
            return true; // no effect
        }
        why = "No heat sources can meet meet the setpoint temperature.";
        return false;
    }
    return true;
}

bool HPWH::canApplySetpointT(const double newSetpointT,
                             double& maxSetpointT,
                             std::string& why,
                             Units::Temp units /*C*/) const
{
    double newSetpointT_C = Units::convert(newSetpointT, units, Units::Temp::C);
    double maxSetpointT_C = -273.15;
    bool result = canApplySetpointT_C(newSetpointT_C, maxSetpointT_C, why);
    maxSetpointT = Units::convert(maxSetpointT_C, Units::Temp::C, units);
    return result;
}

bool HPWH::isSetpointFixed() const { return setpointFixed; }

int HPWH::resetTankToSetpoint() { return setTankT_C(setpointT_C); }

int HPWH::getCompressorCoilConfig() const
{
    if (!hasCompressor())
    {
        return HPWH_ABORT;
    }
    return heatSources[compressorIndex].configuration;
}

bool HPWH::isCompressorMultipass() const
{
    if (!hasCompressor())
    {
        return false;
    }
    return heatSources[compressorIndex].isMultipass;
}

bool HPWH::isCompressorExternalMultipass() const
{
    if (!hasCompressor())
    {
        return false;
    }
    return heatSources[compressorIndex].isExternalMultipass();
}

bool HPWH::hasCompressor() const
{
    if (compressorIndex < 0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("No compressor found in this HPWH. \n");
        }
        return false;
    }
    return true;
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

double HPWH::getExternalVolumeHeated(Units::Volume units /*L*/) const
{
    return Units::convert(externalVolumeHeated_L, Units::Volume::L, units);
}

double
HPWH::getExternalMPFlowRate(const Units::FlowRate units /*Units::FlowRate::gal_per_min*/) const
{
    if (!isCompressorExternalMultipass())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Does not have an external multipass heat source \n");
        }
        return (double)HPWH_ABORT;
    }

    return Units::convert(
        heatSources[compressorIndex].mpFlowRate_LPS, Units::FlowRate::L_per_s, units);
}

double HPWH::getCompressorMinimumRuntime(const Units::Time units /*Units::Time::min*/) const
{
    if (hasCompressor())
    {
        const double minimumRunTime_min = 10.;
        return Units::convert(minimumRunTime_min, Units::Time::min, units);
    }
    else
    {
        return (double)HPWH_ABORT;
    }
}

int HPWH::getSizingFractions(double& aquaFract, double& useableFract) const
{
    double aFract = 1.;
    double useFract = 1.;

    if (!hasCompressor())
    {
        return HPWH_ABORT;
    }
    else if (usesSoCLogic)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Current model uses SOC control logic and does not have a definition for "
                "sizing "
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
                tempUse = 1.; // These logics are just for checking if there's a big draw to
                              // switch to RE
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

    // Check if double's are approximately equally and adjust the relationship so it follows
    // the relationship we expect. The tolerance plays with 0.1 mm in position if the tank
    // is 1m tall...
    double temp = 1. - useableFract;
    if (aboutEqual(aquaFract, temp))
    {
        useableFract = 1. - aquaFract + TOL_MINVALUE;
    }

    return 0;
}

bool HPWH::isHPWHScalable() const { return canScale; }

int HPWH::setScaleCapacityCOP(double scaleCapacity /*=1.0*/, double scaleCOP /*=1.0*/)
{
    if (!isHPWHScalable())
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Can not scale the HPWH Capacity or COP  \n");
        }
        return HPWH_ABORT;
    }
    if (!hasCompressor())
    {
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
            scaleVector(perfP.inputPower_coeffs_kW, scaleCapacity);
        }
        if (scaleCOP != 1.)
        {
            scaleVector(perfP.COP_coeffs, scaleCOP);
        }
    }

    return 0;
}

int HPWH::setCompressorOutputCapacity(double newCapacity,
                                      double airTemp /*=19.722*/,
                                      double inletTemp /*=14.444*/,
                                      double outTemp /*=57.222*/,
                                      Units::Power pwrUnit /*kW*/,
                                      Units::Temp tempUnit /*C*/)
{
    double oldCapacity = getCompressorCapacity(airTemp, inletTemp, outTemp, pwrUnit, tempUnit);
    if (oldCapacity == double(HPWH_ABORT))
    {
        return HPWH_ABORT;
    }

    double scale = newCapacity / oldCapacity;
    return setScaleCapacityCOP(scale, 1.); // Scale the compressor capacity
}

int HPWH::setResistanceCapacity(double power, int which /*=-1*/, Units::Power pwrUnit /*KW*/)
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
    double power_kW = Units::convert(power, pwrUnit, Units::Power::kW);

    // Whew so many checks...
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                heatSources[i].changeResistancePower(power_kW, Units::Power::kW);
            }
        }
    }
    else
    {
        heatSources[resistanceHeightMap[which].index].changeResistancePower(power_kW);

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                heatSources[resistanceHeightMap[i].index].changeResistancePower(power_kW);
            }
        }
    }

    return 0;
}

double HPWH::getResistanceCapacity(int which /*=-1*/, Units::Power pwrUnit /*KW*/)
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

    double returnPower_kW = 0;
    if (which == -1)
    {
        // Just get all the elements
        for (int i = 0; i < getNumHeatSources(); i++)
        {
            if (heatSources[i].isAResistance())
            {
                returnPower_kW += heatSources[i].perfMap[0].inputPower_coeffs_kW[0];
            }
        }
    }
    else
    {
        // get the power from "which" element by height
        returnPower_kW +=
            heatSources[resistanceHeightMap[which].index].perfMap[0].inputPower_coeffs_kW[0];

        // Then check for repeats in the position
        int pos = resistanceHeightMap[which].position;
        for (int i = 0; i < getNumResistanceElements(); i++)
        {
            if (which != i && resistanceHeightMap[i].position == pos)
            {
                returnPower_kW +=
                    heatSources[resistanceHeightMap[i].index].perfMap[0].inputPower_coeffs_kW[0];
            }
        }
    }

    // Unit conversion to kW
    return Units::convert(returnPower_kW, Units::Power::kW, pwrUnit);
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
                           double inletT_C_in,
                           double tankAmbientT_C,
                           double inletVol2_L,
                           double inletT2_C)
{

    inletT_C = inletT_C_in;
    if (drawVolume_L > 0.)
    {

        if (inletVol2_L > drawVolume_L)
        {
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("Volume in inlet 2 is greater than the draw volume.  \n");
            }
            simHasFailed = true;
            return;
        }

        // sort the inlets by height
        int highInletNodeIndex;
        double highInletT_C;
        double highInletFraction; // fraction of draw from high inlet

        int lowInletNodeIndex;
        double lowInletT_C;
        double lowInletFraction; // fraction of draw from low inlet

        if (inletNodeIndex > inlet2NodeIndex)
        {
            highInletNodeIndex = inletNodeIndex;
            highInletFraction = 1. - inletVol2_L / drawVolume_L;
            highInletT_C = inletT_C;
            lowInletNodeIndex = inlet2NodeIndex;
            lowInletT_C = inletT2_C;
            lowInletFraction = inletVol2_L / drawVolume_L;
        }
        else
        {
            highInletNodeIndex = inlet2NodeIndex;
            highInletFraction = inletVol2_L / drawVolume_L;
            highInletT_C = inletT2_C;
            lowInletNodeIndex = inletNodeIndex;
            lowInletT_C = inletT_C;
            lowInletFraction = 1. - inletVol2_L / drawVolume_L;
        }

        // calculate number of nodes to draw
        double drawVolume_N = drawVolume_L / nodeVolume_L;
        double drawCp_kJperC = CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVolume_L;

        // heat-exchange models
        if (hasHeatExchanger)
        {
            outletT_C = inletT_C;
            for (auto& nodeT_C : tankTs_C)
            {
                double maxHeatExchange_kJ = drawCp_kJperC * (nodeT_C - outletT_C);
                double heatExchange_kJ = nodeHeatExchangerEffectiveness * maxHeatExchange_kJ;

                nodeT_C -= heatExchange_kJ / nodeCp_kJperC;
                outletT_C += heatExchange_kJ / drawCp_kJperC;
            }
        }
        else
        {
            double remainingDrawVolume_N = drawVolume_N;
            if (drawVolume_L > tankVolume_L)
            {
                for (int i = 0; i < getNumNodes(); i++)
                {
                    outletT_C += tankTs_C[i];
                    tankTs_C[i] =
                        (inletT_C * (drawVolume_L - inletVol2_L) + inletT2_C * inletVol2_L) /
                        drawVolume_L;
                }
                outletT_C = (outletT_C / getNumNodes() * tankVolume_L +
                             tankTs_C[0] * (drawVolume_L - tankVolume_L)) /
                            drawVolume_L * remainingDrawVolume_N;

                remainingDrawVolume_N = 0.;
            }

            double totalExpelledHeat_kJ = 0.;
            while (remainingDrawVolume_N > 0.)
            {

                // draw no more than one node at a time
                double incrementalDrawVolume_N =
                    remainingDrawVolume_N > 1. ? 1. : remainingDrawVolume_N;

                double outputHeat_kJ = nodeCp_kJperC * incrementalDrawVolume_N * tankTs_C.back();
                totalExpelledHeat_kJ += outputHeat_kJ;
                tankTs_C.back() -= outputHeat_kJ / nodeCp_kJperC;

                for (int i = getNumNodes() - 1; i >= 0; --i)
                {
                    // combine all inlet contributions at this node
                    double inletFraction = 0.;
                    if (i == highInletNodeIndex)
                    {
                        inletFraction += highInletFraction;
                        tankTs_C[i] += incrementalDrawVolume_N * highInletFraction * highInletT_C;
                    }
                    if (i == lowInletNodeIndex)
                    {
                        inletFraction += lowInletFraction;
                        tankTs_C[i] += incrementalDrawVolume_N * lowInletFraction * lowInletT_C;
                    }

                    if (i > 0)
                    {
                        double transferT_C =
                            incrementalDrawVolume_N * (1. - inletFraction) * tankTs_C[i - 1];
                        tankTs_C[i] += transferT_C;
                        tankTs_C[i - 1] -= transferT_C;
                    }
                }

                remainingDrawVolume_N -= incrementalDrawVolume_N;
                mixTankInversions();
            }

            outletT_C = totalExpelledHeat_kJ / drawCp_kJperC;
        }

        // account for mixing at the bottom of the tank
        if (tankMixesOnDraw && drawVolume_L > 0.)
        {
            int mixedBelowNode = (int)(getNumNodes() * mixBelowFractionOnDraw);
            mixTankNodes(0, mixedBelowNode, 1. / 3.);
        }

    } // end if(draw_volume_L > 0)

    // Initialize newTankTemps_C
    nextTankTs_C = tankTs_C;

    double standbyLossesBottom_kJ = 0.;
    double standbyLossesTop_kJ = 0.;
    double standbyLossesSides_kJ = 0.;

    // Standby losses from the top and bottom of the tank
    {
        auto standbyLossRate_kJperHrC = tankUA_kJperhC * fracAreaTop;

        standbyLossesBottom_kJ =
            standbyLossRate_kJperHrC * hoursPerStep * (tankTs_C[0] - tankAmbientT_C);
        standbyLossesTop_kJ = standbyLossRate_kJperHrC * hoursPerStep *
                              (tankTs_C[getNumNodes() - 1] - tankAmbientT_C);

        nextTankTs_C.front() -= standbyLossesBottom_kJ / nodeCp_kJperC;
        nextTankTs_C.back() -= standbyLossesTop_kJ / nodeCp_kJperC;
    }

    // Standby losses from the sides of the tank
    {
        auto standbyLossRate_kJperHrC =
            (tankUA_kJperhC * fracAreaSide + fittingsUA_kJperhC) / getNumNodes();
        for (int i = 0; i < getNumNodes(); i++)
        {
            double standbyLossesNodeSides_kJ =
                standbyLossRate_kJperHrC * hoursPerStep * (tankTs_C[i] - tankAmbientT_C);
            standbyLossesSides_kJ += standbyLossesNodeSides_kJ;

            nextTankTs_C[i] -= standbyLossesNodeSides_kJ / nodeCp_kJperC;
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
            if (hpwhVerbosity >= VRB_reluctant)
            {
                msg("The stability condition for conduction has failed, these results are "
                    "going to "
                    "be interesting!\n");
            }
            simHasFailed = true;
            return;
        }

        // End nodes
        if (getNumNodes() > 1)
        { // inner edges of top and bottom nodes
            nextTankTs_C.front() += tau * (tankTs_C[1] - tankTs_C.front());
            nextTankTs_C.back() += tau * (tankTs_C[getNumNodes() - 2] - tankTs_C.back());
        }

        // Internal nodes
        for (int i = 1; i < getNumNodes() - 1; i++)
        {
            nextTankTs_C[i] += tau * (tankTs_C[i + 1] - 2. * tankTs_C[i] + tankTs_C[i - 1]);
        }
    }

    // Update tankTemps_C
    tankTs_C = nextTankTs_C;

    standbyLosses_kJ += standbyLossesBottom_kJ + standbyLossesTop_kJ + standbyLossesSides_kJ;

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
                if (tankTs_C[i] < tankTs_C[i - 1])
                {
                    // Temperature inversion!
                    hasInversion = true;

                    // Mix this inversion mixing temperature by averaging all of the inverted
                    // nodes together together.
                    double Tmixed = 0.0;
                    double massMixed = 0.0;
                    int m;
                    for (m = i; m >= 0; m--)
                    {
                        Tmixed += tankTs_C[m] * (volumePerNode_L * DENSITYWATER_kgperL);
                        massMixed += (volumePerNode_L * DENSITYWATER_kgperL);
                        if ((m == 0) || (Tmixed / massMixed > tankTs_C[m - 1]))
                        {
                            break;
                        }
                    }
                    Tmixed /= massMixed;

                    // Assign the tank temps from i to k
                    for (int k = i; k >= m; k--)
                        tankTs_C[k] = Tmixed;
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
    double maxHeatToT_C = std::min(maxT_C, setpointT_C);

    if (hpwhVerbosity >= VRB_emetic)
    {
        msg("node %2d   cap_kwh %.4lf \n", nodeNum, KJ_TO_KWH(qAdd_kJ));
    }

    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (tankTs_C[i] != tankTs_C[i + 1])
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
            heatToT_C = tankTs_C[targetTempNodeNum];
            if (heatToT_C > maxHeatToT_C)
            {
                // Ensure temperature does not exceed maximum
                heatToT_C = maxHeatToT_C;
            }
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        double qIncrement_kJ = numNodesToHeat * nodeCp_kJperC * (heatToT_C - tankTs_C[nodeNum]);

        if (qIncrement_kJ > qAdd_kJ)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT_C = tankTs_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                tankTs_C[nodeNum + j] = heatToT_C;
            }
            qAdd_kJ = 0.;
        }
        else if (qIncrement_kJ > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                tankTs_C[nodeNum + j] = heatToT_C;
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
        if (tankTs_C[i] != tankTs_C[i + 1])
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
            // no nodes above the equal-temp nodes; target temperature limited by the heat
            // available
            heatToT_C = tankTs_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
        }
        else
        {
            heatToT_C = tankTs_C[targetTempNodeNum];
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        double qIncrement_kJ = nodeCp_kJperC * numNodesToHeat * (heatToT_C - tankTs_C[nodeNum]);

        if (qIncrement_kJ > qAdd_kJ)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT_C = tankTs_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                tankTs_C[nodeNum + j] = heatToT_C;
            }
            qAdd_kJ = 0.;
        }
        else if (qIncrement_kJ > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                tankTs_C[nodeNum + j] = heatToT_C;
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
    calcThermalDist(modHeatDistribution_W, shrinkageT_C, lowestNode, tankTs_C, setpointT_C);

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
    double tot_qAdded_kJ = 0.;
    for (int i = getNumNodes() - 1; i >= 0; i--)
    {
        if (heatDistribution_W[i] != 0)
        {
            double powerAdd_kW = heatDistribution_W[i] / 1000.;
            double qAdd_kJ = KWH_TO_KJ(powerAdd_kW * minutesPerStep / min_per_h);
            addExtraHeatAboveNode(qAdd_kJ, i);
            tot_qAdded_kJ += qAdd_kJ;
        }
    }
    // Write the input & output energy
    extraEnergyInput_kJ = tot_qAdded_kJ;
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
        avgT_C += tankTs_C[i];
    }
    avgT_C /= numAvgNodes;

    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        tankTs_C[i] += mixFactor * (avgT_C - tankTs_C[i]);
    }
}

void HPWH::calcSizeConstants()
{
    // calculate conduction between the nodes AND heat loss by node with top and bottom
    // having greater surface area. model uses explicit finite difference to find conductive
    // heat exchange between the tank nodes with the boundary conditions on the top and
    // bottom node being the fraction of UA that corresponds to the top and bottom of the
    // tank. The assumption is that the aspect ratio is the same for all tanks and is the
    // same for the outside measurements of the unit and the inner water tank.
    const double tankRad_m = getTankRadius(Units::Length::m);
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
    static char outputString[MAXOUTSTRING]; // this is used for debugging outputs

    // find condentropy/shrinkage
    for (int i = 0; i < getNumHeatSources(); ++i)
    {
        heatSources[i].shrinkageT_C = findShrinkageT_C(heatSources[i].condensity);

        if (hpwhVerbosity >= VRB_emetic)
        {
            msg(outputString, "Heat Source %d \n", i);
            msg(outputString, "shrinkage %.2lf \n\n", heatSources[i].shrinkageT_C);
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

bool compressorIsRunning(HPWH& hpwh)
{
    return (bool)hpwh.isNthHeatSourceRunning(hpwh.getCompressorIndex());
}

//-----------------------------------------------------------------------------
///	@brief	Checks whether energy is balanced during a simulation step.
/// @note	Used in test/main.cc
/// @param[in]	drawVol_L				Water volume drawn during simulation step
///	@param[in]	prevHeatContent_kJ		Heat content prior to simulation step
///	@param[in]	fracEnergyTolerance		Fractional tolerance for energy imbalance
/// @return	true if balanced; false otherwise.
//-----------------------------------------------------------------------------
bool HPWH::isEnergyBalanced(const double drawVol_L,
                            const double prevHeatContent_kJ,
                            const double fracEnergyTolerance /* = 0.001 */)
{
    // Check energy balancing.
    double qInElectrical_kJ = getInputEnergy_kJ();
    double qInExtra_kJ = extraEnergyInput_kJ;
    double qInHeatSourceEnviron_kJ = getEnergyRemovedFromEnvironment_kJ();
    double qOutStandbyLosses_kJ = standbyLosses_kJ;
    double qOutWater_kJ = drawVol_L * (outletT_C - inletT_C) * DENSITYWATER_kgperL *
                          CPWATER_kJperkgC; // assumes only one inlet
    double expectedHeatContent_kJ =
        prevHeatContent_kJ        // previous heat content
        + qInElectrical_kJ        // electrical energy delivered to heat sources
        + qInExtra_kJ             // extra energy delivered to heat sources
        + qInHeatSourceEnviron_kJ // heat extracted from environment by condenser
        - qOutStandbyLosses_kJ    // heat released from tank to environment
        - qOutWater_kJ;           // heat expelled to outlet by water flow

    double currentHeatContent_kJ = getHeatContent_kJ();
    double qBal_kJ = currentHeatContent_kJ - expectedHeatContent_kJ;

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
    else if (modelName == "AquaThermAire")
    {
        model = HPWH::MODELS_AquaThermAire;
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
int HPWH::initPreset(const std::string& modelName)
{
    HPWH::MODELS targetModel;
    if (mapNameToPreset(modelName, targetModel))
    {
        return initPreset(targetModel);
    }
    return HPWH_ABORT;
}

// Used to check a few inputs after the initialization of a tank model from a preset or a file.
int HPWH::checkInputs()
{
    int returnVal = 0;
    // use a returnVal so that all checks are processed and error messages written

    if (getNumHeatSources() <= 0 && model != MODELS_StorageTank)
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
            if (0 > heatSources[i].externalOutletNodeIndex ||
                heatSources[i].externalOutletNodeIndex > getNumNodes() - 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("External heat sources need an external outlet height within the "
                        "bounds from from 0 to numNodes-1. \n");
                }
                returnVal = HPWH_ABORT;
            }
            if (0 > heatSources[i].externalInletNodeIndex ||
                heatSources[i].externalInletNodeIndex > getNumNodes() - 1)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("External heat sources need an external inlet height within the "
                        "bounds from from 0 to numNodes-1. \n");
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
        // perfGrid and perfGridValues, and the length of vectors in perfGridValues are
        // equal and that ;
        if (heatSources[i].useBtwxtGrid)
        {
            // If useBtwxtGrid is true that the perfMap is empty
            if (heatSources[i].perfMap.size() != 0)
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("Using the grid lookups but a regression based perforamnce map is "
                        "given "
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
                    msg("When using grid lookups for perfmance the vectors in "
                        "perfGridValues "
                        "must "
                        "be the same length. \n");
                }
                returnVal = HPWH_ABORT;
            }

            // Check perfGrid's vectors lengths multiplied together == the perfGridValues
            // vector lengths
            size_t expLength = 1;
            for (const auto& v : heatSources[i].perfGrid)
            {
                expLength *= v.size();
            }
            if (expLength != heatSources[i].perfGridValues[0].size())
            {
                if (hpwhVerbosity >= VRB_reluctant)
                {
                    msg("When using grid lookups for perfmance the vectors in "
                        "perfGridValues "
                        "must "
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
                    msg("External multipass heat sources must have a perfMap of only one "
                        "point with regression equations. \n");
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
                msg("The relationship between the on logic and off logic is not supported. "
                    "The off logic is beneath the on logic.");
            }
            returnVal = HPWH_ABORT;
        }
    }

    double maxTemp;
    string why;
    double tempSetpoint = setpointT_C;
    if (!canApplySetpointT(tempSetpoint, maxTemp, why))
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("Cannot set new setpoint. %s", why.c_str());
        }
        returnVal = HPWH_ABORT;
    }

    // Check if the UA is out of bounds
    if (tankUA_kJperhC < 0.0)
    {
        if (hpwhVerbosity >= VRB_reluctant)
        {
            msg("The tankUA_kJperhC is less than 0 for a HPWH, it must be greater than 0, "
                "tankUA_kJperhC is: %f  \n",
                tankUA_kJperhC);
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

#ifndef HPWH_ABRIDGED
int HPWH::initFromFile(string configFile)
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
    std::size_t heatsource, sourceNum, nTemps, tempInt;
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
            tankUA_kJperhC = tempDouble;
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
            setpointT_C = tempDouble;
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
            for (std::size_t i = 0; i < numHeatSources; i++)
            {
                heatSources.emplace_back(this);
            }
        }
        else if (token == "heatsource")
        {
            if (numHeatSources == 0)
            {
                msg("You must specify the number of heatsources before setting their "
                    "properties.  "
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
                heatSources[heatsource].minT_C = tempDouble;
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
                heatSources[heatsource].maxT_C = tempDouble;
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
                            if (hpwhVerbosity >= VRB_reluctant)
                            {
                                msg("Node number for heatsource %d %s must be between 0 and "
                                    "%d.  "
                                    "\n",
                                    heatsource,
                                    token.c_str(),
                                    LOGIC_SIZE + 1);
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
                        for ([[maybe_unused]] auto n : nodeNums)
                        {
                            weights.push_back(1.0);
                        }
                    }
                    if (nodeNums.size() != weights.size())
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Number of weights for heatsource %d %s (%d) does not match "
                                "number "
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
                                msg("Improper comparison, \"%s\", for heat source %d %s. "
                                    "Should be "
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
                        heatSources[heatsource].addTurnOnLogic(wholeTank(tempDouble, absolute));
                    }
                    else if (tempString == "topThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(topThird(tempDouble));
                    }
                    else if (tempString == "bottomThird")
                    {
                        heatSources[heatsource].addTurnOnLogic(bottomThird(tempDouble));
                    }
                    else if (tempString == "standby")
                    {
                        heatSources[heatsource].addTurnOnLogic(standby(tempDouble));
                    }
                    else if (tempString == "bottomSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(bottomSixth(tempDouble));
                    }
                    else if (tempString == "secondSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(secondSixth(tempDouble));
                    }
                    else if (tempString == "thirdSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(thirdSixth(tempDouble));
                    }
                    else if (tempString == "fourthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(fourthSixth(tempDouble));
                    }
                    else if (tempString == "fifthSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(fifthSixth(tempDouble));
                    }
                    else if (tempString == "topSixth")
                    {
                        heatSources[heatsource].addTurnOnLogic(topSixth(tempDouble));
                    }
                    else if (tempString == "bottomHalf")
                    {
                        heatSources[heatsource].addTurnOnLogic(bottomHalf(tempDouble));
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
                if (tempInt < num_nodes)
                {
                    heatSources[heatsource].externalInletNodeIndex = static_cast<int>(tempInt);
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
                if (tempInt < num_nodes)
                {
                    heatSources[heatsource].externalOutletNodeIndex = static_cast<int>(tempInt);
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
                std::size_t maxTemps = heatSources[heatsource].perfMap.size();

                if (maxTemps < nTemps)
                {
                    if (maxTemps == 0)
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("%s specified for heatsource %d before definition of nTemps.  "
                                "\n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect specification for %s from heatsource %d. nTemps, "
                                "%d, is "
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
                heatSources[heatsource].perfMap[nTemps - 1].T_C = tempDouble;
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
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("%s specified for heatsource %d before definition of nTemps.  "
                                "\n",
                                token.c_str(),
                                heatsource);
                        }
                        return HPWH_ABORT;
                    }
                    else
                    {
                        if (hpwhVerbosity >= VRB_reluctant)
                        {
                            msg("Incorrect specification for %s from heatsource %d. nTemps, "
                                "%d, is "
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
                    heatSources[heatsource].perfMap[nTemps - 1].inputPower_coeffs_kW.push_back(
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
                heatSources[heatsource].hysteresisOffsetT_C = tempDouble;
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
    model = MODELS_CustomFile;

    tankTs_C.resize(num_nodes);

    if (hasInitialTankTemp)
        setTankT_C(initalTankT_C);
    else
        resetTankToSetpoint();

    nextTankTs_C.resize(num_nodes);

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
