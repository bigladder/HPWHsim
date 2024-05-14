#ifndef HPWHUTILS_hh
#define HPWHUTILS_hh

#include <string>

#include <cstdio>
#include <cstdlib>
#include <vector>
#include <unordered_map>

#include <courier/courier.h>

#include <nlohmann/json.hpp>

double getResampledValue(const std::vector<double>& sampleValues,
                         double beginFraction,
                         double endFraction);
bool resample(std::vector<double>& values, const std::vector<double>& sampleValues);
bool resampleExtensive(std::vector<double>& values, const std::vector<double>& sampleValues);
double expitFunc(double x, double offset);
void normalize(std::vector<double>& distribution);
int findLowestNode(const std::vector<double>& nodeDist, const int numTankNodes);
double findShrinkageT_C(const std::vector<double>& nodeDist);
void calcThermalDist(std::vector<double>& thermalDist,
                     const double shrinkageT_C,
                     const int lowestNode,
                     const std::vector<double>& nodeT_C,
                     const double setpointT_C);
void scaleVector(std::vector<double>& coeffs, const double scaleFactor);
double getChargePerNode(double tCold, double tMix, double tHot);

#endif
