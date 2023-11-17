/*
* Heating logic
*/

#ifndef HPWHHeatingLogics_hh
#define HPWHHeatingLogics_hh

#include <string>
#include <sstream>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <functional>
#include <memory>

#include <cstdio>
#include <cstdlib>   //for exit
#include <vector>

#include "HPWH.hh"

struct HPWH::NodeWeight {
	int nodeNum;
	double weight;
	NodeWeight(int n,double w): nodeNum(n),weight(w) {};

	NodeWeight(int n): nodeNum(n),weight(1.0) {};
};
 
struct HPWH::HeatingLogic {
public:
	std::string description;
	std::function<bool(double,double)> compare;

	HeatingLogic(std::string desc,double decisionPoint_in,HPWH *hpwh_in,
		std::function<bool(double,double)> c,bool isHTS):
		description(desc),decisionPoint(decisionPoint_in),hpwh(hpwh_in),compare(c),
		isEnteringWaterHighTempShutoff(isHTS)
	{};

	/**< checks that the input is all valid. */
	virtual const bool isValid() = 0;
	/**< gets the value for comparing the tank value to, i.e. the target SoC */
	virtual const double getComparisonValue() = 0;
	/**< gets the calculated value from the tank, i.e. SoC or tank average of node weights*/
	virtual const double getTankValue() = 0;
	/**< function to calculate where the average node for a logic set is. */
	virtual const double nodeWeightAvgFract() = 0;
	/**< gets the fraction of a node that has to be heated up to met the turnoff condition*/
	virtual const double getFractToMeetComparisonExternal() = 0;

	virtual int setDecisionPoint(double value) = 0;
	double getDecisionPoint() { return decisionPoint; }
	bool getIsEnteringWaterHighTempShutoff() { return isEnteringWaterHighTempShutoff; }

protected:
	double decisionPoint;
	HPWH* hpwh;
	bool isEnteringWaterHighTempShutoff;
};

struct HPWH::SoCBasedHeatingLogic: HeatingLogic {
public:
	SoCBasedHeatingLogic(std::string desc,double decisionPoint,HPWH *hpwh,
		double hF = -0.05,double tM_C = 43.333,bool constMains = false,double mains_C = 18.333,
		std::function<bool(double,double)> c = std::less<double>()):
		HeatingLogic(desc,decisionPoint,hpwh,c,false),
		hysteresisFraction(hF),tempMinUseful_C(tM_C),
		useCostantMains(constMains),constantMains_C(mains_C)
	{};
	const bool isValid();

	const double getComparisonValue();
	const double getTankValue();
	const double nodeWeightAvgFract();
	const double getFractToMeetComparisonExternal();
	const double getMainsT_C();
	const double getTempMinUseful_C();
	int setDecisionPoint(double value);
	int setConstantMainsTemperature(double mains_C);

private:
	double tempMinUseful_C;
	double hysteresisFraction;
	bool useCostantMains;
	double constantMains_C;
};

struct HPWH::TempBasedHeatingLogic: HeatingLogic {
public:
	TempBasedHeatingLogic(std::string desc,std::vector<NodeWeight> n,
		double decisionPoint,HPWH *hpwh,bool a = false,
		std::function<bool(double,double)> c = std::less<double>(),
		bool isHTS = false):
		HeatingLogic(desc,decisionPoint,hpwh,c,isHTS),
		nodeWeights(n),isAbsolute(a)
	{};

	const bool isValid();

	const double getComparisonValue();
	const double getTankValue();
	const double nodeWeightAvgFract();
	const double getFractToMeetComparisonExternal();

	int setDecisionPoint(double value);
	int setDecisionPoint(double value,bool absolute);

private:
	const bool areNodeWeightsValid();

	bool isAbsolute;
	std::vector<NodeWeight> nodeWeights;
};

#endif // HPWHHeatingLogics_hh