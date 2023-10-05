/*
 * Implementation of class HPWH::HeatSource
 */

#include <algorithm>
#include <regex>

 // vendor
#include "btwxt.h"

#include "HPWH.hh"

//public HPWH::HeatSource functions
HPWH::HeatSource::HeatSource(HPWH *parentInput)
	:hpwh(parentInput),isOn(false),lockedOut(false),doDefrost(false),backupHeatSource(NULL),companionHeatSource(NULL),
	followedByHeatSource(NULL),minT(-273.15),maxT(100),hysteresis_dC(0),airflowFreedom(1.0),maxSetpoint_C(100.),
	typeOfHeatSource(TYPE_none),extrapolationMethod(EXTRAP_LINEAR),maxOut_at_LowT{100,-273.15},standbyLogic(NULL),
	isMultipass(true),mpFlowRate_LPS(0.),externalInletHeight(-1),externalOutletHeight(-1),useBtwxtGrid(false),
	secondaryHeatExchanger{0.,0.,0.}
{}

HPWH::HeatSource::HeatSource(const HeatSource &hSource) {
	hpwh = hSource.hpwh;
	isOn = hSource.isOn;
	lockedOut = hSource.lockedOut;
	doDefrost = hSource.doDefrost;

	runtime_min = hSource.runtime_min;
	energyInput_kWh = hSource.energyInput_kWh;
	energyOutput_kWh = hSource.energyOutput_kWh;

	isVIP = hSource.isVIP;

	if(hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL || hSource.followedByHeatSource != NULL) {
		hpwh->simHasFailed = true;
		if(hpwh->hpwhVerbosity >= VRB_reluctant) {
			hpwh->msg("HeatSources cannot be copied if they contain pointers to other HeatSources\n");
		}
	}

	for(int i = 0; i < CONDENSITY_SIZE; i++) {
		condensity[i] = hSource.condensity[i];
	}
	shrinkage = hSource.shrinkage;

	perfMap = hSource.perfMap;

	perfGrid = hSource.perfGrid;
	perfGridValues = hSource.perfGridValues;
	perfRGI = hSource.perfRGI;
	useBtwxtGrid = hSource.useBtwxtGrid;

	defrostMap = hSource.defrostMap;
	resDefrost = hSource.resDefrost;

	//i think vector assignment works correctly here
	turnOnLogicSet = hSource.turnOnLogicSet;
	shutOffLogicSet = hSource.shutOffLogicSet;
	standbyLogic = hSource.standbyLogic;

	minT = hSource.minT;
	maxT = hSource.maxT;
	maxOut_at_LowT = hSource.maxOut_at_LowT;
	hysteresis_dC = hSource.hysteresis_dC;
	maxSetpoint_C = hSource.maxSetpoint_C;

	depressesTemperature = hSource.depressesTemperature;
	airflowFreedom = hSource.airflowFreedom;

	configuration = hSource.configuration;
	typeOfHeatSource = hSource.typeOfHeatSource;
	isMultipass = hSource.isMultipass;
	mpFlowRate_LPS = hSource.mpFlowRate_LPS;

	externalInletHeight = hSource.externalInletHeight;
	externalOutletHeight = hSource.externalOutletHeight;

	lowestNode = hSource.lowestNode;

	extrapolationMethod = hSource.extrapolationMethod;
	secondaryHeatExchanger = hSource.secondaryHeatExchanger;
}

HPWH::HeatSource& HPWH::HeatSource::operator=(const HeatSource &hSource) {
	if(this == &hSource) {
		return *this;
	}

	hpwh = hSource.hpwh;
	isOn = hSource.isOn;
	lockedOut = hSource.lockedOut;
	doDefrost = hSource.doDefrost;

	runtime_min = hSource.runtime_min;
	energyInput_kWh = hSource.energyInput_kWh;
	energyOutput_kWh = hSource.energyOutput_kWh;

	isVIP = hSource.isVIP;

	if(hSource.backupHeatSource != NULL || hSource.companionHeatSource != NULL || hSource.followedByHeatSource != NULL) {
		hpwh->simHasFailed = true;
		if(hpwh->hpwhVerbosity >= VRB_reluctant) {
			hpwh->msg("HeatSources cannot be copied if they contain pointers to other HeatSources\n");
		}
	} else {
		companionHeatSource = NULL;
		backupHeatSource = NULL;
		followedByHeatSource = NULL;
	}

	for(int i = 0; i < CONDENSITY_SIZE; i++) {
		condensity[i] = hSource.condensity[i];
	}
	shrinkage = hSource.shrinkage;

	perfMap = hSource.perfMap;

	perfGrid = hSource.perfGrid;
	perfGridValues = hSource.perfGridValues;
	perfRGI = hSource.perfRGI;
	useBtwxtGrid = hSource.useBtwxtGrid;

	defrostMap = hSource.defrostMap;
	resDefrost = hSource.resDefrost;

	//i think vector assignment works correctly here
	turnOnLogicSet = hSource.turnOnLogicSet;
	shutOffLogicSet = hSource.shutOffLogicSet;
	standbyLogic = hSource.standbyLogic;

	minT = hSource.minT;
	maxT = hSource.maxT;
	maxOut_at_LowT = hSource.maxOut_at_LowT;
	hysteresis_dC = hSource.hysteresis_dC;
	maxSetpoint_C = hSource.maxSetpoint_C;

	depressesTemperature = hSource.depressesTemperature;
	airflowFreedom = hSource.airflowFreedom;

	configuration = hSource.configuration;
	typeOfHeatSource = hSource.typeOfHeatSource;
	isMultipass = hSource.isMultipass;
	mpFlowRate_LPS = hSource.mpFlowRate_LPS;

	externalInletHeight = hSource.externalInletHeight;
	externalOutletHeight = hSource.externalOutletHeight;

	lowestNode = hSource.lowestNode;
	extrapolationMethod = hSource.extrapolationMethod;
	secondaryHeatExchanger = hSource.secondaryHeatExchanger;

	return *this;
}



void HPWH::HeatSource::setCondensity(double cnd1,double cnd2,double cnd3,double cnd4,
	double cnd5,double cnd6,double cnd7,double cnd8,
	double cnd9,double cnd10,double cnd11,double cnd12) {
	condensity[0] = cnd1;
	condensity[1] = cnd2;
	condensity[2] = cnd3;
	condensity[3] = cnd4;
	condensity[4] = cnd5;
	condensity[5] = cnd6;
	condensity[6] = cnd7;
	condensity[7] = cnd8;
	condensity[8] = cnd9;
	condensity[9] = cnd10;
	condensity[10] = cnd11;
	condensity[11] = cnd12;
}

int HPWH::HeatSource::findParent() const {
	for(int i = 0; i < hpwh->numHeatSources; ++i) {
		if(this == hpwh->setOfSources[i].backupHeatSource) {
			return i;
		}
	}
	return -1;
}

bool HPWH::HeatSource::isEngaged() const {
	return isOn;
}

bool HPWH::HeatSource::isLockedOut() const {
	return lockedOut;
}

void HPWH::HeatSource::lockOutHeatSource() {
	lockedOut = true;
}

void HPWH::HeatSource::unlockHeatSource() {
	lockedOut = false;
}

bool HPWH::HeatSource::shouldLockOut(double heatSourceAmbientT_C) const {

	// if it's already locked out, keep it locked out
	if(isLockedOut() == true) {
		return true;
	} else {
		//when the "external" temperature is too cold - typically used for compressor low temp. cutoffs
		//when running, use hysteresis
		bool lock = false;
		if(isEngaged() == true && heatSourceAmbientT_C < minT - hysteresis_dC) {
			lock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic) {
				hpwh->msg("\tlock-out: running below minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
			}
		}
		//when not running, don't use hysteresis
		else if(isEngaged() == false && heatSourceAmbientT_C < minT) {
			lock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic) {
				hpwh->msg("\tlock-out: already below minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
			}
		}

		//when the "external" temperature is too warm - typically used for resistance lockout
		//when running, use hysteresis
		if(isEngaged() == true && heatSourceAmbientT_C > maxT + hysteresis_dC) {
			lock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic) {
				hpwh->msg("\tlock-out: running above maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
			}
		}
		//when not running, don't use hysteresis
		else if(isEngaged() == false && heatSourceAmbientT_C > maxT) {
			lock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic) {
				hpwh->msg("\tlock-out: already above maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
			}
		}

		if(maxedOut()) {
			lock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic) {
				hpwh->msg("\tlock-out: condenser water temperature above max: %.2f",maxSetpoint_C);
			}
		}
		//	if (lock == true && backupHeatSource == NULL) {
		//		if (hpwh->hpwhVerbosity >= HPWH::VRB_emetic) {
		//			hpwh->msg("\nWARNING: lock-out triggered, but no backupHeatSource defined. Simulation will continue without lock-out");
		//		}
		//		lock = false;
		//	}
		if(hpwh->hpwhVerbosity >= VRB_typical) {
			hpwh->msg("\n");
		}
		return lock;
	}
}

bool HPWH::HeatSource::shouldUnlock(double heatSourceAmbientT_C) const {

	// if it's already unlocked, keep it unlocked
	if(isLockedOut() == false) {
		return true;
	}
	// if it the heat source is capped and can't produce hotter water
	else if(maxedOut()) {
		return false;
	} else {
		//when the "external" temperature is no longer too cold or too warm
		//when running, use hysteresis
		bool unlock = false;
		if(isEngaged() == true && heatSourceAmbientT_C > minT + hysteresis_dC && heatSourceAmbientT_C < maxT - hysteresis_dC) {
			unlock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C > minT + hysteresis_dC) {
				hpwh->msg("\tunlock: running above minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
			}
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C < maxT - hysteresis_dC) {
				hpwh->msg("\tunlock: running below maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
			}
		}
		//when not running, don't use hysteresis
		else if(isEngaged() == false && heatSourceAmbientT_C > minT && heatSourceAmbientT_C < maxT) {
			unlock = true;
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C > minT) {
				hpwh->msg("\tunlock: already above minT\tambient: %.2f\tminT: %.2f",heatSourceAmbientT_C,minT);
			}
			if(hpwh->hpwhVerbosity >= HPWH::VRB_emetic && heatSourceAmbientT_C < maxT) {
				hpwh->msg("\tunlock: already below maxT\tambient: %.2f\tmaxT: %.2f",heatSourceAmbientT_C,maxT);
			}
		}
		if(hpwh->hpwhVerbosity >= VRB_typical) {
			hpwh->msg("\n");
		}
		return unlock;
	}
}

bool HPWH::HeatSource::toLockOrUnlock(double heatSourceAmbientT_C) {

	if(shouldLockOut(heatSourceAmbientT_C)) {
		lockOutHeatSource();
	}
	if(shouldUnlock(heatSourceAmbientT_C)) {
		unlockHeatSource();
	}

	return isLockedOut();
}

bool HPWH::shouldDRLockOut(HEATSOURCE_TYPE hs,DRMODES DR_signal) const {

	if(hs == TYPE_compressor && (DR_signal & DR_LOC) != 0) {
		return true;
	} else if(hs == TYPE_resistance && (DR_signal & DR_LOR) != 0) {
		return true;
	}
	return false;
}

void HPWH::resetTopOffTimer() {
	timerTOT = 0.;
}

void HPWH::HeatSource::engageHeatSource(DRMODES DR_signal) {
	isOn = true;
	hpwh->isHeating = true;
	if(companionHeatSource != NULL &&
		companionHeatSource->shutsOff() != true &&
		companionHeatSource->isEngaged() == false &&
		hpwh->shouldDRLockOut(companionHeatSource->typeOfHeatSource,DR_signal) == false)
	{
		companionHeatSource->engageHeatSource(DR_signal);
	}
}

void HPWH::HeatSource::disengageHeatSource() {
	isOn = false;
}

bool HPWH::HeatSource::shouldHeat() const {
	//return true if the heat source logic tells it to come on, false if it doesn't,
	//or if an unsepcified selector was used
	bool shouldEngage = false;

	for(int i = 0; i < (int)turnOnLogicSet.size(); i++) {
		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("\tshouldHeat logic: %s ",turnOnLogicSet[i]->description.c_str());
		}

		double average = turnOnLogicSet[i]->getTankValue();
		double comparison = turnOnLogicSet[i]->getComparisonValue();

		if(turnOnLogicSet[i]->compare(average,comparison)) {
			if(turnOnLogicSet[i]->description == "standby" && standbyLogic != NULL) {
				double comparisonStandby = standbyLogic->getComparisonValue();
				double avgStandby = standbyLogic->getTankValue();

				if(turnOnLogicSet[i]->compare(avgStandby,comparisonStandby)) {
					shouldEngage = true;
				}
			} else{
				shouldEngage = true;
			}
		}

		//quit searching the logics if one of them turns it on
		if(shouldEngage) {
			//debugging message handling
			if(hpwh->hpwhVerbosity >= VRB_typical) {
				hpwh->msg("engages!\n");
			}
			if(hpwh->hpwhVerbosity >= VRB_emetic) {
				hpwh->msg("average: %.2lf \t setpoint: %.2lf \t decisionPoint: %.2lf \t comparison: %2.1f\n",average,
					hpwh->setpoint_C,turnOnLogicSet[i]->getDecisionPoint(),comparison);
			}
			break;
		}

		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("returns: %d \t",shouldEngage);
		}
	}  //end loop over set of logic conditions

	//if everything else wants it to come on, but if it would shut off anyways don't turn it on
	if(shouldEngage == true && shutsOff() == true) {
		shouldEngage = false;
		if(hpwh->hpwhVerbosity >= VRB_typical) {
			hpwh->msg("but is denied by shutsOff");
		}
	}

	if(hpwh->hpwhVerbosity >= VRB_typical) {
		hpwh->msg("\n");
	}
	return shouldEngage;
}

bool HPWH::HeatSource::shutsOff() const {
	bool shutOff = false;

	if(hpwh->tankTemps_C[0] >= hpwh->setpoint_C) {
		shutOff = true;
		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("shutsOff  bottom node hot: %.2d C  \n returns true",hpwh->tankTemps_C[0]);
		}
		return shutOff;
	}

	for(int i = 0; i < (int)shutOffLogicSet.size(); i++) {
		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("\tshutsOff logic: %s ",shutOffLogicSet[i]->description.c_str());
		}

		double average = shutOffLogicSet[i]->getTankValue();
		double comparison = shutOffLogicSet[i]->getComparisonValue();

		if(shutOffLogicSet[i]->compare(average,comparison)) {
			shutOff = true;

			//debugging message handling
			if(hpwh->hpwhVerbosity >= VRB_typical) {
				hpwh->msg("shuts down %s\n",shutOffLogicSet[i]->description.c_str());
			}
		}
	}

	if(hpwh->hpwhVerbosity >= VRB_emetic) {
		hpwh->msg("returns: %d \n",shutOff);
	}
	return shutOff;
}

bool HPWH::HeatSource::maxedOut() const {
	bool maxed = false;

	// If the heat source can't produce water at the setpoint and the control logics are saying to shut off
	if(hpwh->setpoint_C > maxSetpoint_C){
		if(hpwh->tankTemps_C[0] >= maxSetpoint_C || shutsOff()) {
			maxed = true;
		}
	}
	return maxed;
}

double HPWH::HeatSource::fractToMeetComparisonExternal() const {
	double fracTemp;
	double frac = 1.;

	for(int i = 0; i < (int)shutOffLogicSet.size(); i++) {
		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("\tshutsOff logic: %s ",shutOffLogicSet[i]->description.c_str());
		}

		fracTemp = shutOffLogicSet[i]->getFractToMeetComparisonExternal();

		frac = fracTemp < frac ? fracTemp : frac;
	}

	return frac;
}

void HPWH::HeatSource::addHeat(double externalT_C,double minutesToRun) {
	double input_BTUperHr = 0.,cap_BTUperHr = 0.,cop = 0.,captmp_kJ = 0.;
	double leftoverCap_kJ = 0.0;
	// set the leftover capacity of the Heat Source to 0, so the first round of
	// passing it on works

	switch(configuration) {
	case CONFIG_SUBMERGED:
	case CONFIG_WRAPPED:
	{
		static std::vector<double> heatDistribution(hpwh->numNodes);
		//clear the heatDistribution vector, since it's static it is still holding the
		//distribution from the last go around
		heatDistribution.clear();
		//calcHeatDist takes care of the swooping for wrapped configurations
		calcHeatDist(heatDistribution);

		if(isACompressor()) {
			hpwh->condenserInlet_C = getCondenserTemp();
		}
		// calculate capacity btu/hr, input btu/hr, and cop
		getCapacity(externalT_C,getCondenserTemp(),input_BTUperHr,cap_BTUperHr,cop);

		//some outputs for debugging
		if(hpwh->hpwhVerbosity >= VRB_typical) {
			hpwh->msg("capacity_kWh %.2lf \t\t cap_BTUperHr %.2lf \n",BTU_TO_KWH(cap_BTUperHr)*(minutesToRun) / 60.0,cap_BTUperHr);
		}
		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("heatDistribution: %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf %4.3lf \n",heatDistribution[0],heatDistribution[1],heatDistribution[2],heatDistribution[3],heatDistribution[4],heatDistribution[5],heatDistribution[6],heatDistribution[7],heatDistribution[8],heatDistribution[9],heatDistribution[10],heatDistribution[11]);
		}
		//the loop over nodes here is intentional - essentially each node that has
		//some amount of heatDistribution acts as a separate resistive element
		//maybe start from the top and go down?  test this with graphs
		for(int i = hpwh->numNodes - 1; i >= 0; i--) {
			//for(int i = 0; i < hpwh->numNodes; i++){
			captmp_kJ = BTU_TO_KJ(cap_BTUperHr * minutesToRun / 60.0 * heatDistribution[i]);
			if(captmp_kJ != 0) {
				//add leftoverCap to the next run, and keep passing it on
				leftoverCap_kJ = addHeatAboveNode(captmp_kJ + leftoverCap_kJ,i);
			}
		}

		if(isACompressor()) { // outlet temperature is the condenser temperature after heat has been added
			hpwh->condenserOutlet_C = getCondenserTemp();
		}

		//after you've done everything, any leftover capacity is time that didn't run
		this->runtime_min = (1.0 - (leftoverCap_kJ / BTU_TO_KJ(cap_BTUperHr * minutesToRun / 60.0))) * minutesToRun;
#if 1	// error check, 1-22-2017
		if(runtime_min < -0.001)
			if(hpwh->hpwhVerbosity >= VRB_reluctant)
				hpwh->msg("Internal error: Negative runtime = %0.3f min\n",runtime_min);
#endif
	}
	break;

	case CONFIG_EXTERNAL:
		//Else the heat source is external. SANCO2 system is only current example
		//capacity is calculated internal to this function, and cap/input_BTUperHr, cop are outputs
		this->runtime_min = addHeatExternal(externalT_C,minutesToRun,cap_BTUperHr,input_BTUperHr,cop);
		break;
	}

	// Write the input & output energy
	energyInput_kWh = BTU_TO_KWH(input_BTUperHr * runtime_min / 60.0);
	energyOutput_kWh = BTU_TO_KWH(cap_BTUperHr * runtime_min / 60.0);
}

// private HPWH::HeatSource functions
void HPWH::HeatSource::sortPerformanceMap() {
	std::sort(perfMap.begin(),perfMap.end(),
		[](const HPWH::HeatSource::perfPoint & a,const HPWH::HeatSource::perfPoint & b) -> bool {
			return a.T_F < b.T_F;
		});
}

double HPWH::HeatSource::expitFunc(double x,double offset) {
	double val;
	val = 1 / (1 + exp(x - offset));
	return val;
}

void HPWH::HeatSource::normalize(std::vector<double> &distribution) {
	double sum_tmp = 0.0;
	size_t N = distribution.size();

	for(size_t i = 0; i < N; i++) {
		sum_tmp += distribution[i];
	}
	for(size_t i = 0; i < N; i++) {
		if(sum_tmp > 0.0) {
			distribution[i] /= sum_tmp;
		} else {
			distribution[i] = 0.0;
		}
		//this gives a very slight speed improvement (milliseconds per simulated year)
		if(distribution[i] < TOL_MINVALUE) {
			distribution[i] = 0;
		}
	}
}

double HPWH::HeatSource::getCondenserTemp() const{
	double condenserTemp_C = 0.0;
	int tempNodesPerCondensityNode = hpwh->numNodes / CONDENSITY_SIZE;
	int j = 0;

	for(int i = 0; i < hpwh->numNodes; i++) {
		j = i / tempNodesPerCondensityNode;
		if(condensity[j] != 0) {
			condenserTemp_C += (condensity[j] / tempNodesPerCondensityNode) * hpwh->tankTemps_C[i];
			//the weights don't need to be added to divide out later because they should always sum to 1

			if(hpwh->hpwhVerbosity >= VRB_emetic) {
				hpwh->msg("condenserTemp_C:\t %.2lf \ti:\t %d \tj\t %d \tcondensity[j]:\t %.2lf \ttankTemps_C[i]:\t %.2lf\n",condenserTemp_C,i,j,condensity[j],hpwh->tankTemps_C[i]);
			}
		}
	}
	if(hpwh->hpwhVerbosity >= VRB_typical) {
		hpwh->msg("condenser temp %.2lf \n",condenserTemp_C);
	}
	return condenserTemp_C;
}

void HPWH::HeatSource::getCapacity(double externalT_C,double condenserTemp_C,double setpointTemp_C,double &input_BTUperHr,double &cap_BTUperHr,double &cop) {
	double externalT_F,condenserTemp_F;

	// Add an offset to the condenser temperature (or incoming coldwater temperature) to approximate a secondary heat exchange in line with the compressor
	condenserTemp_F = C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
	externalT_F = C_TO_F(externalT_C);

	// Get bounding performance map points for interpolation/extrapolation
	bool extrapolate = false;
	size_t i_prev = 0;
	size_t i_next = 1;
	double Tout_F = C_TO_F(setpointTemp_C + secondaryHeatExchanger.hotSideTemperatureOffset_dC);

	if(useBtwxtGrid) {
		std::vector<double> target{externalT_F,Tout_F,condenserTemp_F};
		btwxtInterp(input_BTUperHr,cop,target);
	} else {
		if(perfMap.size() > 1) {
			double COP_T1,COP_T2;    			   //cop at ambient temperatures T1 and T2
			double inputPower_T1_Watts,inputPower_T2_Watts; //input power at ambient temperatures T1 and T2

			for(size_t i = 0; i < perfMap.size(); ++i) {
				if(externalT_F < perfMap[i].T_F) {
					if(i == 0) {
						extrapolate = true;
						i_prev = 0;
						i_next = 1;
					} else {
						i_prev = i - 1;
						i_next = i;
					}
					break;
				} else {
					if(i == perfMap.size() - 1) {
						extrapolate = true;
						i_prev = i - 1;
						i_next = i;
						break;
					}
				}
			}

			// Calculate COP and Input Power at each of the two reference temepratures
			COP_T1 = perfMap[i_prev].COP_coeffs[0];
			COP_T1 += perfMap[i_prev].COP_coeffs[1] * condenserTemp_F;
			COP_T1 += perfMap[i_prev].COP_coeffs[2] * condenserTemp_F * condenserTemp_F;

			COP_T2 = perfMap[i_next].COP_coeffs[0];
			COP_T2 += perfMap[i_next].COP_coeffs[1] * condenserTemp_F;
			COP_T2 += perfMap[i_next].COP_coeffs[2] * condenserTemp_F * condenserTemp_F;

			inputPower_T1_Watts = perfMap[i_prev].inputPower_coeffs[0];
			inputPower_T1_Watts += perfMap[i_prev].inputPower_coeffs[1] * condenserTemp_F;
			inputPower_T1_Watts += perfMap[i_prev].inputPower_coeffs[2] * condenserTemp_F * condenserTemp_F;

			inputPower_T2_Watts = perfMap[i_next].inputPower_coeffs[0];
			inputPower_T2_Watts += perfMap[i_next].inputPower_coeffs[1] * condenserTemp_F;
			inputPower_T2_Watts += perfMap[i_next].inputPower_coeffs[2] * condenserTemp_F * condenserTemp_F;

			if(hpwh->hpwhVerbosity >= VRB_emetic) {
				hpwh->msg("inputPower_T1_constant_W   linear_WperF   quadratic_WperF2  \t%.2lf  %.2lf  %.2lf \n",perfMap[0].inputPower_coeffs[0],perfMap[0].inputPower_coeffs[1],perfMap[0].inputPower_coeffs[2]);
				hpwh->msg("inputPower_T2_constant_W   linear_WperF   quadratic_WperF2  \t%.2lf  %.2lf  %.2lf \n",perfMap[1].inputPower_coeffs[0],perfMap[1].inputPower_coeffs[1],perfMap[1].inputPower_coeffs[2]);
				hpwh->msg("inputPower_T1_Watts:  %.2lf \tinputPower_T2_Watts:  %.2lf \n",inputPower_T1_Watts,inputPower_T2_Watts);

				if(extrapolate) {
					hpwh->msg("Warning performance extrapolation\n\tExternal Temperature: %.2lf\tNearest temperatures:  %.2lf, %.2lf \n\n",externalT_F,perfMap[i_prev].T_F,perfMap[i_next].T_F);
				}
			}

			// Interpolate to get COP and input power at the current ambient temperature
			linearInterp(cop,externalT_F,perfMap[i_prev].T_F,perfMap[i_next].T_F,COP_T1,COP_T2);
			linearInterp(input_BTUperHr,externalT_F,perfMap[i_prev].T_F,perfMap[i_next].T_F,inputPower_T1_Watts,inputPower_T2_Watts);
			input_BTUperHr = KWH_TO_BTU(input_BTUperHr / 1000.0);//1000 converts w to kw);

		} else { //perfMap.size() == 1 or we've got an issue.
			if(externalT_F > perfMap[0].T_F) {
				extrapolate = true;
				if(extrapolationMethod == EXTRAP_NEAREST) {
					externalT_F = perfMap[0].T_F;
				}
			}

			regressedMethod(input_BTUperHr,perfMap[0].inputPower_coeffs,externalT_F,Tout_F,condenserTemp_F);
			input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

			regressedMethod(cop,perfMap[0].COP_coeffs,externalT_F,Tout_F,condenserTemp_F);
		}
	}

	if(doDefrost) {
		//adjust COP by the defrost factor
		defrostDerate(cop,externalT_F);
	}

	cap_BTUperHr = cop * input_BTUperHr;

	if(hpwh->hpwhVerbosity >= VRB_emetic) {
		hpwh->msg("externalT_F: %.2lf, Tout_F: %.2lf, condenserTemp_F: %.2lf\n",externalT_F,Tout_F,condenserTemp_F);
		hpwh->msg("input_BTUperHr: %.2lf , cop: %.2lf, cap_BTUperHr: %.2lf \n",input_BTUperHr,cop,cap_BTUperHr);
	}
	//here is where the scaling for flow restriction happens
	//the input power doesn't change, we just scale the cop by a small percentage
	//that is based on the flow rate.  The equation is a fit to three points,
	//measured experimentally - 12 percent reduction at 150 cfm, 10 percent at
	//200, and 0 at 375. Flow is expressed as fraction of full flow.
	if(airflowFreedom != 1) {
		double airflow = 375 * airflowFreedom;
		cop *= 0.00056*airflow + 0.79;
	}
	if(hpwh->hpwhVerbosity >= VRB_typical) {
		hpwh->msg("cop: %.2lf \tinput_BTUperHr: %.2lf \tcap_BTUperHr: %.2lf \n",cop,input_BTUperHr,cap_BTUperHr);
		if(cop < 0.) {
			hpwh->msg(" Warning: COP is Negative! \n");
		}
		if(cop < 1.) {
			hpwh->msg(" Warning: COP is Less than 1! \n");
		}
	}
}

void HPWH::HeatSource::getCapacityMP(double externalT_C,double condenserTemp_C,double &input_BTUperHr,double &cap_BTUperHr,double &cop) {
	double externalT_F,condenserTemp_F;
	bool resDefrostHeatingOn = false;
	// Convert Celsius to Fahrenheit for the curve fits
	condenserTemp_F = C_TO_F(condenserTemp_C + secondaryHeatExchanger.coldSideTemperatureOffest_dC);
	externalT_F = C_TO_F(externalT_C);

	// Check if we have resistance elements to turn on for defrost and add the constant lift.
	if(resDefrost.inputPwr_kW > 0) {
		if(externalT_F < resDefrost.onBelowT_F) {
			externalT_F += resDefrost.constTempLift_dF;
			resDefrostHeatingOn = true;
		}
	}

	if(useBtwxtGrid) {
		std::vector<double> target{externalT_F,condenserTemp_F};
		btwxtInterp(input_BTUperHr,cop,target);
	} else {
		// Get bounding performance map points for interpolation/extrapolation
		bool extrapolate = false;
		if(externalT_F > perfMap[0].T_F) {
			extrapolate = true;
			if(extrapolationMethod == EXTRAP_NEAREST) {
				externalT_F = perfMap[0].T_F;
			}
		}

		//Const Tair Tin Tair2 Tin2 TairTin
		regressedMethodMP(input_BTUperHr,perfMap[0].inputPower_coeffs,externalT_F,condenserTemp_F);
		regressedMethodMP(cop,perfMap[0].COP_coeffs,externalT_F,condenserTemp_F);

	}
	input_BTUperHr = KWH_TO_BTU(input_BTUperHr);

	if(doDefrost) {
		//adjust COP by the defrost factor
		defrostDerate(cop,externalT_F);
	}

	cap_BTUperHr = cop * input_BTUperHr;

	//For accounting add the resistance defrost to the input energy
	if(resDefrostHeatingOn){
		input_BTUperHr += KW_TO_BTUperH(resDefrost.inputPwr_kW);
	}
	if(hpwh->hpwhVerbosity >= VRB_emetic) {
		hpwh->msg("externalT_F: %.2lf, condenserTemp_F: %.2lf\n",externalT_F,condenserTemp_F);
		hpwh->msg("input_BTUperHr: %.2lf , cop: %.2lf, cap_BTUperHr: %.2lf \n",input_BTUperHr,cop,cap_BTUperHr);
	}
}

double HPWH::HeatSource::calcMPOutletTemperature(double heatingCapacity_KW) {
	return hpwh->tankTemps_C[externalOutletHeight] + heatingCapacity_KW / (mpFlowRate_LPS * DENSITYWATER_kgperL * CPWATER_kJperkgC);
}

void HPWH::HeatSource::setupDefrostMap(double derate35/*=0.8865*/) {
	doDefrost = true;
	defrostMap.reserve(3);
	defrostMap.push_back({17.,1.});
	defrostMap.push_back({35.,derate35});
	defrostMap.push_back({47.,1.});
}

void HPWH::HeatSource::defrostDerate(double &to_derate,double airT_F) {
	if(airT_F <= defrostMap[0].T_F || airT_F >= defrostMap[defrostMap.size() - 1].T_F) {
		return; // Air temperature outside bounds of the defrost map. There is no extrapolation here.
	}
	double derate_factor = 1.;
	size_t i_prev = 0;
	for(size_t i = 1; i < defrostMap.size(); ++i) {
		if(airT_F <= defrostMap[i].T_F) {
			i_prev = i - 1;
			break;
		}
	}
	linearInterp(derate_factor,airT_F,
		defrostMap[i_prev].T_F,defrostMap[i_prev + 1].T_F,
		defrostMap[i_prev].derate_fraction,defrostMap[i_prev + 1].derate_fraction);
	to_derate *= derate_factor;
}

void HPWH::HeatSource::linearInterp(double &ynew,double xnew,double x0,double x1,double y0,double y1) {
	ynew = y0 + (xnew - x0) * (y1 - y0) / (x1 - x0);
}

void HPWH::HeatSource::regressedMethod(double &ynew,std::vector<double> &coefficents,double x1,double x2,double x3) {
	ynew = coefficents[0] +
		coefficents[1] * x1 +
		coefficents[2] * x2 +
		coefficents[3] * x3 +
		coefficents[4] * x1 * x1 +
		coefficents[5] * x2 * x2 +
		coefficents[6] * x3 * x3 +
		coefficents[7] * x1 * x2 +
		coefficents[8] * x1 * x3 +
		coefficents[9] * x2 * x3 +
		coefficents[10] * x1 * x2 * x3;
}

void HPWH::HeatSource::regressedMethodMP(double &ynew,std::vector<double> &coefficents,double x1,double x2) {
	//Const Tair Tin Tair2 Tin2 TairTin
	ynew = coefficents[0] +
		coefficents[1] * x1 +
		coefficents[2] * x2 +
		coefficents[3] * x1 * x1 +
		coefficents[4] * x2 * x2 +
		coefficents[5] * x1 * x2;
}

void HPWH::HeatSource::btwxtInterp(double& input_BTUperHr,double& cop,std::vector<double> &target) {

	std::vector<double> result = perfRGI->get_values_at_target(target);

	input_BTUperHr = result[0];
	cop = result[1];
}

void HPWH::HeatSource::calcHeatDist(std::vector<double> &heatDistribution) {

	// Populate the vector of heat distribution
	for(int i = 0; i < hpwh->numNodes; i++) {
		if(i < lowestNode) {
			heatDistribution.push_back(0);
		} else {
			int k;
			if(configuration == CONFIG_SUBMERGED) { // Inside the tank, no swoopiness required
				//intentional integer division
				k = i / int(hpwh->numNodes / CONDENSITY_SIZE);
				heatDistribution.push_back(condensity[k]);
			} else if(configuration == CONFIG_WRAPPED) { // Wrapped around the tank, send through the logistic function
				double temp = 0;  //temp for temporary not temperature
				double offset = 5.0 / 1.8;
				temp = expitFunc((hpwh->tankTemps_C[i] - hpwh->tankTemps_C[lowestNode]) / this->shrinkage,offset);
				temp *= (hpwh->setpoint_C - hpwh->tankTemps_C[i]);
				if(temp < 0.) // SETPOINT_FIX
					temp = 0.;
				heatDistribution.push_back(temp);
			}
		}
	}
	normalize(heatDistribution);
}

double HPWH::HeatSource::addHeatAboveNode(double cap_kJ,int node) {
	double Q_kJ,deltaT_C,targetTemp_C;
	int setPointNodeNum;

	double volumePerNode_L = hpwh->tankVolume_L / hpwh->numNodes;
	double maxTargetTemp_C = std::min(maxSetpoint_C,hpwh->setpoint_C);

	if(hpwh->hpwhVerbosity >= VRB_emetic) {
		hpwh->msg("node %2d   cap_kwh %.4lf \n",node,KJ_TO_KWH(cap_kJ));
	}

	// find the first node (from the bottom) that does not have the same temperature as the one above it
	// if they all have the same temp., use the top node, hpwh->numNodes-1
	setPointNodeNum = node;
	for(int i = node; i < hpwh->numNodes - 1; i++) {
		if(hpwh->tankTemps_C[i] != hpwh->tankTemps_C[i + 1]) {
			break;
		} else {
			setPointNodeNum = i + 1;
		}
	}

	// maximum heat deliverable in this timestep
	while(cap_kJ > 0 && setPointNodeNum < hpwh->numNodes) {
		// if the whole tank is at the same temp, the target temp is the setpoint
		if(setPointNodeNum == (hpwh->numNodes - 1)) {
			targetTemp_C = maxTargetTemp_C;
		}
		//otherwise the target temp is the first non-equal-temp node
		else {
			targetTemp_C = hpwh->tankTemps_C[setPointNodeNum + 1];
		}
		// With DR tomfoolery make sure the target temperature doesn't exceed the setpoint.
		if(targetTemp_C > maxTargetTemp_C) {
			targetTemp_C = maxTargetTemp_C;
		}

		deltaT_C = targetTemp_C - hpwh->tankTemps_C[setPointNodeNum];

		//heat needed to bring all equal temp. nodes up to the temp of the next node. kJ
		Q_kJ = CPWATER_kJperkgC * volumePerNode_L * DENSITYWATER_kgperL * (setPointNodeNum + 1 - node) * deltaT_C;

		//Running the rest of the time won't recover
		if(Q_kJ > cap_kJ) {
			for(int j = node; j <= setPointNodeNum; j++) {
				hpwh->tankTemps_C[j] += cap_kJ / CPWATER_kJperkgC / volumePerNode_L / DENSITYWATER_kgperL / (setPointNodeNum + 1 - node);
			}
			cap_kJ = 0;
		}
		else if(Q_kJ > 0.) // SETPOINT_FIX
		{	// temp will recover by/before end of timestep
			for(int j = node; j <= setPointNodeNum; j++)
				hpwh->tankTemps_C[j] = targetTemp_C;
			cap_kJ -= Q_kJ;
		}
		setPointNodeNum++;
	}

	//return the unused capacity
	return cap_kJ;
}
bool HPWH::HeatSource::isACompressor() const {
	return this->typeOfHeatSource == TYPE_compressor;
}

bool HPWH::HeatSource::isAResistance() const {
	return this->typeOfHeatSource == TYPE_resistance;
}
bool  HPWH::HeatSource::isExternalMultipass() const {
	return isMultipass && configuration == HeatSource::CONFIG_EXTERNAL;
}

double HPWH::HeatSource::addHeatExternal(double externalT_C,double minutesToRun,double &cap_BTUperHr,double &input_BTUperHr,double &cop) {
	double heatingCapacity_kJ,heatingCapacityNeeded_kJ,deltaT_C,timeUsed_min,nodeHeat_kJperNode,nodeFrac,fractToShutOff;
	double inputTemp_BTUperHr = 0,capTemp_BTUperHr = 0,copTemp = 0;
	double volumePerNode_LperNode = hpwh->tankVolume_L / hpwh->numNodes;
	double timeRemaining_min = minutesToRun;
	double maxTargetTemp_C = std::min(maxSetpoint_C,hpwh->setpoint_C);
	double targetTemp_C = 0.;
	input_BTUperHr = 0;
	cap_BTUperHr = 0;
	cop = 0;

	do {
		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("bottom tank temp: %.2lf \n",hpwh->tankTemps_C[0]);
		}

		if(this->isMultipass) {
			// if multipass evenly mix the tank up
			hpwh->mixTankNodes(0,hpwh->numNodes,1.0); // 1.0 will give even mixing, so all temperatures mixed end at average temperature.

			//how much heat is added this timestep
			getCapacityMP(externalT_C,hpwh->tankTemps_C[externalOutletHeight],inputTemp_BTUperHr,capTemp_BTUperHr,copTemp);
			double heatingCapacity_KW = BTUperH_TO_KW(capTemp_BTUperHr);

			heatingCapacity_kJ = heatingCapacity_KW * (timeRemaining_min * 60.0);

			targetTemp_C = calcMPOutletTemperature(heatingCapacity_KW);
			deltaT_C = targetTemp_C - hpwh->tankTemps_C[externalOutletHeight];
		} else {
			//how much heat is available this timestep
			getCapacity(externalT_C,hpwh->tankTemps_C[externalOutletHeight],inputTemp_BTUperHr,capTemp_BTUperHr,copTemp);
			heatingCapacity_kJ = BTU_TO_KJ(capTemp_BTUperHr * (minutesToRun / 60.0));
			if(hpwh->hpwhVerbosity >= VRB_emetic) {
				hpwh->msg("\theatingCapacity_kJ stepwise: %.2lf \n",heatingCapacity_kJ);
			}

			//adjust capacity for how much time is left in this step
			heatingCapacity_kJ *= (timeRemaining_min / minutesToRun);
			if(hpwh->hpwhVerbosity >= VRB_emetic) {
				hpwh->msg("\theatingCapacity_kJ remaining this node: %.2lf \n",heatingCapacity_kJ);
			}

			//calculate what percentage of the bottom node can be heated to setpoint
			//with amount of heat available this timestep
			targetTemp_C = maxTargetTemp_C;
			deltaT_C = targetTemp_C - hpwh->tankTemps_C[externalOutletHeight];

		}

		nodeHeat_kJperNode = volumePerNode_LperNode * DENSITYWATER_kgperL * CPWATER_kJperkgC * deltaT_C;

		// Caclulate fraction of node to move
		if(nodeHeat_kJperNode <= 0.) { // protect against dividing by zero - if bottom node is at (or above) setpoint, add no heat
			nodeFrac = 0.;
		} else {
			nodeFrac = heatingCapacity_kJ / nodeHeat_kJperNode;
		}

		if(hpwh->hpwhVerbosity >= VRB_emetic) {
			hpwh->msg("nodeHeat_kJperNode: %.2lf nodeFrac: %.2lf \n\n",nodeHeat_kJperNode,nodeFrac);
		}

		fractToShutOff = fractToMeetComparisonExternal();
		if(fractToShutOff < 1. && fractToShutOff < nodeFrac && !this->isMultipass) { // circle back and check on this for multipass
			nodeFrac = fractToShutOff;
			heatingCapacityNeeded_kJ = nodeFrac * nodeHeat_kJperNode;

			timeUsed_min = (heatingCapacityNeeded_kJ / heatingCapacity_kJ) * timeRemaining_min;
			timeRemaining_min -= timeUsed_min;
		}
		//if more than one, round down to 1 and subtract the amount of time it would
		//take to heat that node from the timeRemaining
		else if(nodeFrac > 1.) {
			nodeFrac = 1.;
			timeUsed_min = (nodeHeat_kJperNode / heatingCapacity_kJ)*timeRemaining_min;
			timeRemaining_min -= timeUsed_min;
		}
		//otherwise just the fraction available
		//this should make heatingCapacity == 0 if nodeFrac < 1
		else {
			timeUsed_min = timeRemaining_min;
			timeRemaining_min = 0.;
		}

		// Track the condenser temperature if this is a compressor before moving the nodes
		if(isACompressor()) {
			hpwh->condenserInlet_C += hpwh->tankTemps_C[externalOutletHeight] * timeUsed_min;
			hpwh->condenserOutlet_C += targetTemp_C * timeUsed_min;
		}

		// Moving the nodes down
		// move all nodes down, mixing if less than a full node
		for(int n = externalOutletHeight; n < externalInletHeight; n++) {
			hpwh->tankTemps_C[n] = hpwh->tankTemps_C[n] * (1 - nodeFrac) + hpwh->tankTemps_C[n + 1] * nodeFrac;
		}
		//add water to top node, heated to setpoint
		hpwh->tankTemps_C[externalInletHeight] = hpwh->tankTemps_C[externalInletHeight] * (1. - nodeFrac) + targetTemp_C * nodeFrac;

		hpwh->mixTankInversions();
		hpwh->updateSoCIfNecessary();

		// track outputs - weight by the time ran
		// Add in pump power to approximate a secondary heat exchange in line with the compressor
		input_BTUperHr += (inputTemp_BTUperHr + W_TO_BTUperH(secondaryHeatExchanger.extraPumpPower_W)) * timeUsed_min;
		cap_BTUperHr += capTemp_BTUperHr * timeUsed_min;
		cop += copTemp * timeUsed_min;

		hpwh->externalVolumeHeated_L += nodeFrac * volumePerNode_LperNode;

		//if there's still time remaining and you haven't heated to the cutoff
		//specified in shutsOff logic, keep heating
	} while(timeRemaining_min > 0 && shutsOff() != true);

	// divide outputs by sum of weight - the total time ran
	// not timeRemaining_min == minutesToRun is possible
	//   must prevent divide by 0 (added 4-11-2023)
	double timeRun = minutesToRun - timeRemaining_min;
	if(timeRun > 0.)
	{
		input_BTUperHr /= timeRun;
		cap_BTUperHr /= timeRun;
		cop /= timeRun;
		hpwh->condenserInlet_C /= timeRun;
		hpwh->condenserOutlet_C /= timeRun;
	}

	if(hpwh->hpwhVerbosity >= VRB_emetic) {
		hpwh->msg("final remaining time: %.2lf \n",timeRemaining_min);
	}
	// return the time left
	return timeRun;
}

void HPWH::HeatSource::setupAsResistiveElement(int node,double Watts) {
	int i;

	isOn = false;
	isVIP = false;
	for(i = 0; i < CONDENSITY_SIZE; i++) {
		condensity[i] = 0;
	}
	condensity[node] = 1;

	perfMap.reserve(2);

	perfMap.push_back({
		50, // Temperature (T_F)
		{Watts,0.0,0.0}, // Input Power Coefficients (inputPower_coeffs)
		{1.0,0.0,0.0} // COP Coefficients (COP_coeffs)
		});

	perfMap.push_back({
		67, // Temperature (T_F)
		{Watts,0.0,0.0}, // Input Power Coefficients (inputPower_coeffs)
		{1.0,0.0,0.0} // COP Coefficients (COP_coeffs)
		});

	configuration = CONFIG_SUBMERGED; //immersed in tank

	typeOfHeatSource = TYPE_resistance;
}

void HPWH::HeatSource::setupExtraHeat(std::vector<double>* nodePowerExtra_W) {

	std::vector<double> tempCondensity(CONDENSITY_SIZE);
	double watts = 0.0;
	for(unsigned int i = 0; i < (*nodePowerExtra_W).size(); i++) {
		//get sum of vector
		watts += (*nodePowerExtra_W)[i];

		//put into vector for normalization
		tempCondensity[i] = (*nodePowerExtra_W)[i];
	}

	normalize(tempCondensity);

	if(hpwh->hpwhVerbosity >= VRB_emetic){
		hpwh->msg("extra heat condensity: ");
		for(unsigned int i = 0; i < tempCondensity.size(); i++) {
			hpwh->msg("C[%d]: %f",i,tempCondensity[i]);
		}
		hpwh->msg("\n ");
	}

	// set condensity based on normalized vector
	setCondensity(tempCondensity[0],tempCondensity[1],tempCondensity[2],tempCondensity[3],
		tempCondensity[4],tempCondensity[5],tempCondensity[6],tempCondensity[7],
		tempCondensity[8],tempCondensity[9],tempCondensity[10],tempCondensity[11]);

	perfMap.clear();
	perfMap.reserve(2);

	perfMap.push_back({
		50, // Temperature (T_F)
		{watts,0.0,0.0}, // Input Power Coefficients (inputPower_coeffs)
		{1.0,0.0,0.0} // COP Coefficients (COP_coeffs)
		});

	perfMap.push_back({
		67, // Temperature (T_F)
		{watts,0.0,0.0}, // Input Power Coefficients (inputPower_coeffs)
		{1.0,0.0,0.0} // COP Coefficients (COP_coeffs)
		});

}

void HPWH::HeatSource::addTurnOnLogic(std::shared_ptr<HeatingLogic> logic) {
	this->turnOnLogicSet.push_back(logic);
}

void HPWH::HeatSource::addShutOffLogic(std::shared_ptr<HeatingLogic> logic) {
	this->shutOffLogicSet.push_back(logic);
}

void HPWH::HeatSource::clearAllTurnOnLogic() {
	this->turnOnLogicSet.clear();
}

void HPWH::HeatSource::clearAllShutOffLogic() {
	this->shutOffLogicSet.clear();
}

void HPWH::HeatSource::clearAllLogic() {
	this->clearAllTurnOnLogic();
	this->clearAllShutOffLogic();
}

void HPWH::HeatSource::changeResistanceWatts(double watts) {
	for(auto &perfP : perfMap) {
		perfP.inputPower_coeffs[0] = watts;
	}
}
