/*
* General functions
*/

#include "HPWH.hh"
#include "btwxt.h"

#include <stdarg.h>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <regex>

using std::endl;
using std::cout;
using std::string;

//-----------------------------------------------------------------------------
///	@brief	Samples a std::vector to extract a single value spanning the fractional
///			coordinate range from frac_begin to frac_end. 
/// @note	Bounding fractions are clipped or swapped, if needed.
/// @param[in]	sampleValues	Contains values to be sampled
///	@param[in]	beginFraction		Lower (left) bounding fraction (0 to 1)
///	@param[in]	endFraction			Upper (right) bounding fraction (0 to 1) 	
/// @return	Resampled value; 0 if undefined.
//-----------------------------------------------------------------------------
double getResampledValue(const std::vector<double> &sampleValues,double beginFraction,double endFraction)
{
	if(beginFraction > endFraction)std::swap(beginFraction,endFraction);
	if(beginFraction < 0.) beginFraction = 0.;
	if(endFraction >1.) endFraction = 1.;

	double nNodes = static_cast<double>(sampleValues.size());
	auto beginIndex = static_cast<std::size_t>(beginFraction * nNodes);

	double previousFraction = beginFraction;
	double nextFraction = previousFraction;

	double totValueWeight = 0.;
	double totWeight = 0.;
	for(std::size_t index = beginIndex; nextFraction < endFraction; ++index)
	{
		nextFraction = static_cast<double>(index + 1) / nNodes;
		if(nextFraction > endFraction)
		{
			nextFraction = endFraction;
		}
		double weight = nextFraction - previousFraction;
		totValueWeight += weight *sampleValues[index];
		totWeight += weight;
		previousFraction = nextFraction;
	}
	double resampled_value = 0.;
	if(totWeight > 0.) resampled_value = totValueWeight / totWeight;
	return resampled_value;
}

//-----------------------------------------------------------------------------
///	@brief	Replaces the values in a std::vector by resampling another std::vector of
///			arbitrary size.
/// @param[in,out]	values			Contains values to be replaced
///	@param[in]		sampleValues	Contains values to replace with
/// @return	Success: true; Failure: false
//-----------------------------------------------------------------------------
bool resample(std::vector<double> &values,const std::vector<double> &sampleValues)
{
    if(sampleValues.empty()) return false;
    double actualSize = static_cast<double>(values.size());
    double sizeRatio = static_cast<double>(sampleValues.size()) / actualSize;
    auto binSize = static_cast<std::size_t>(1. / sizeRatio);
    double beginFraction = 0., endFraction;
    std::size_t index = 0;
    while(index < actualSize)
    {
        auto value = static_cast<double>(index);
        auto sampleIndex = static_cast<std::size_t>(floor(value * sizeRatio));
        if(sampleIndex + 1. < (value + 1.) * sizeRatio) { // General case: no binning possible
            endFraction = static_cast<double>(index + 1) / actualSize;
            values[index] = getResampledValue(sampleValues,beginFraction,endFraction);
            ++index;
        }
        else { // Special case: direct copy a single value to a bin
            std::size_t beginIndex = index;
            std::size_t adjustedBinSize = binSize;
            if(binSize > 1)
            { // Find beginning of bin and number to copy
                beginIndex = static_cast<std::size_t>(ceil(sampleIndex/sizeRatio));
                adjustedBinSize  = static_cast<std::size_t>(floor((sampleIndex + 1)/sizeRatio) - ceil(sampleIndex/sizeRatio));
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
bool resampleExtensive(std::vector<double> &values,const std::vector<double> &sampleValues)
{
	if(resample(values,sampleValues))
	{
		double scale = static_cast<double>(sampleValues.size()) / static_cast<double>(values.size());
		for(auto &value: values)
			value *= scale;
		return true;
	}
	return false;
}
