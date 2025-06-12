/*
 * Implementation of HPWH utility functions
 */

#include "HPWH.hh"
#include "HPWHUtils.hh"
#include "HPWHHeatSource.hh"
#include "Condenser.hh"
#include <fmt/format.h>

#include <algorithm>
#include <regex>

/*static*/
double HPWH::getResampledValue(const std::vector<double>& sampleValues,
                               double beginFraction,
                               double endFraction)
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

/*static*/
void HPWH::resample(std::vector<double>& values, const std::vector<double>& sampleValues)
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

/*static*/
void HPWH::resampleExtensive(std::vector<double>& values, const std::vector<double>& sampleValues)
{
    resample(values, sampleValues);
    double scale = static_cast<double>(sampleValues.size()) / static_cast<double>(values.size());
    for (auto& value : values)
        value *= scale;
}

/*static*/
double HPWH::expitFunc(double x, double offset)
{
    double val;
    val = 1 / (1 + exp(x - offset));
    return val;
}

/*static*/
void HPWH::normalize(std::vector<double>& distribution)
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

/*static*/
int HPWH::findLowestNode(const WeightedDistribution& wdist, const int numTankNodes)
{
    for (auto j = 0; j < numTankNodes; ++j)
    {
        if (wdist.normalizedWeight(j / numTankNodes, (j + 1.) / numTankNodes) > 0.)
            return j;
    }

    return 0;
}

/*static*/
double HPWH::findShrinkageT_C(const WeightedDistribution& wdist, const int numNodes)
{
    double alphaT_C = 1., betaT_C = 2.;
    double condentropy = 0.;
    double fracBegin = 0.;
    for (int iNode = 0; iNode < numNodes; ++iNode)
    {
        double fracEnd = static_cast<double>(iNode + 1) / numNodes;
        double dist = wdist.normalizedWeight(fracBegin, fracEnd);
        if (dist > 0.)
        {
            condentropy -= dist * log(dist);
        }
        fracBegin = fracEnd;
    }
    // condentropy shifts as ln(# of condensity nodes)
    double size_factor = static_cast<double>(numNodes) / HPWH::HeatSource::CONDENSITY_SIZE;
    double standard_condentropy = condentropy - log(size_factor);

    return alphaT_C + standard_condentropy * betaT_C;
}

/*static*/
void HPWH::calcThermalDist(std::vector<double>& thermalDist,
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

/*static*/
void HPWH::scaleVector(std::vector<double>& coeffs, const double scaleFactor)
{
    if (scaleFactor != 1.)
    {
        std::transform(coeffs.begin(),
                       coeffs.end(),
                       coeffs.begin(),
                       std::bind(std::multiplies<double>(), std::placeholders::_1, scaleFactor));
    }
}

/*static*/
double HPWH::getChargePerNode(double tCold, double tMix, double tHot)
{
    if (tHot < tMix)
    {
        return 0.;
    }
    return (tHot - tCold) / (tMix - tCold);
}

/*static*/
std::function<HPWH::Performance(double, double)>
HPWH::makePerformanceBtwxt(Condenser* condenser,
                           const std::vector<std::vector<double>>& perfGrid,
                           const std::vector<std::vector<double>>& perfGridValues)
{
    auto grid_axes = condenser->setUpGridAxes(perfGrid);

    auto perfRGI = std::make_shared<Btwxt::RegularGridInterpolator>(
        grid_axes, perfGridValues, "RegularGridInterpolator", condenser->get_courier());

    condenser->perfRGI = perfRGI;

    /// internal function to form target vector
    std::function<std::vector<double>(double externalT_C, double condenserT_C)>
        getPerformanceTarget;

    if (perfGrid.size() > 2)
    {
        getPerformanceTarget = [condenser](double externalT_C, double heatSourceT_C)
        {
            return std::vector<double>(
                {externalT_C,
                 condenser->hpwh->getSetpoint() +
                     condenser->secondaryHeatExchanger.hotSideTemperatureOffset_dC,
                 heatSourceT_C});
        };
    }
    else
    {
        getPerformanceTarget = [](double externalT_C, double heatSourceT_C) {
            return std::vector<double>({externalT_C, heatSourceT_C});
        };
    }

    return [perfRGI, getPerformanceTarget](double externalT_C, double heatSourceT_C)
    {
        auto target = getPerformanceTarget(externalT_C, heatSourceT_C);
        std::vector<double> result = perfRGI->get_values_at_target(target);
        Performance performance({result[0], result[1] * result[0], result[1]});
        return performance;
    };
}

/*static*/
void HPWH::linearInterp(double& ynew, double xnew, double x0, double x1, double y0, double y1)
{
    ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}
