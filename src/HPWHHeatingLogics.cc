/*
File of the presets heating logics available HPWHsim
*/


/*TODO test SOC
* 
* getComparisonValue -> make sure hystersis value works. 
*  test error on init with negative decision point.
* Test nodeWeightAvgFract is correct!
* Test getFractToMeetComparisonExternal when written. 
*
* Will use integration tests largely on a hpwh with soc. 
*/

/* State of Charge Based Logic*/
const bool HPWH::SOCBasedHeatingLogic::isValid() {
	bool isValid = true;
	if (!isValidSOCTarget(decisionPoint)) {
		isValid = false;
	}
	return isValid;
}

const bool HPWH::SOCBasedHeatingLogic::isValidSOCTarget(double target) {
	bool isValid = true;
	if (target < 0) {
		isValid = false;
	}
	return isValid;
}

const double HPWH::SOCBasedHeatingLogic::getComparisonValue(HPWH* phpwh) {
	return decisionPoint + hysteresisFraction;
}

const double HPWH::SOCBasedHeatingLogic::getTankValue(HPWH* phpwh) {
	double socFraction;
	if (useCostantMains) {
		socFraction = phpwh->getSoCFraction(constantMains_C, tempMinUseful_C);
	}
	else {
		socFraction = phpwh->getSoCFraction(phpwh->member_inletT_C, tempMinUseful_C);
	}
	return socFraction;
}

int HPWH::SOCBasedHeatingLogic::setTargetSOCFraction(double target) {
	if (isValidSOCTarget(target)) {
		decisionPoint = target;
		return 0;
	}
	else {
		return HPWH::HPWH_ABORT;
	}
}

int HPWH::SOCBasedHeatingLogic::setConstantMainsTemperature(double mains_C) {
	constantMains_C = mains_C;
	useCostantMains = true;
	return 0;
}

const double HPWH::SOCBasedHeatingLogic::nodeWeightAvgFract(int numberOfNodes, int condensity_size) {
	return decisionPoint * numberOfNodes / condensity_size;
}

const double HPWH::SOCBasedHeatingLogic::getFractToMeetComparisonExternal(HPWH* phpwh) {
	phpwh->numNodes; //TODO
	return 1.;
}



/* Temperature Based Heating Logic*/
const bool HPWH::TempBasedHeatingLogic::isValid() {
	bool isValid = true;
	if (!areNodeWeightsValid()) {
		isValid = false;
	}
	return isValid;
}

const bool HPWH::TempBasedHeatingLogic::areNodeWeightsValid() {
	for (auto nodeWeights : nodeWeights) {
		if (nodeWeights.nodeNum > 13 || nodeWeights.nodeNum < 0) {
			return false;
		}
	}
	return true;
}

const double HPWH::TempBasedHeatingLogic::getComparisonValue(HPWH* phpwh) {
	double value = decisionPoint;
	if (isAbsolute) {
		return value;
	}
	else {
		return phpwh->getSetpoint() - value;
	}
}

const double HPWH::TempBasedHeatingLogic::getTankValue(HPWH* phpwh) {
	return phpwh->tankAvg_C(nodeWeights);
}

const double HPWH::TempBasedHeatingLogic::nodeWeightAvgFract(int numberOfNodes, int condensity_size) {
	double logicNode;
	double calcNodes = 0, totWeight = 0;

	for (auto nodeWeight : nodeWeights) {
		// bottom calc node only
		if (nodeWeight.nodeNum == 0) { // simple equation
			return 1. / (double)numberOfNodes;
		}
		// top calc node only
		else if (nodeWeight.nodeNum == 13) {
			return 1.;
		}
		else { // have to tally up the nodes
			calcNodes += nodeWeight.nodeNum * nodeWeight.weight;
			totWeight += nodeWeight.weight;
		}
	}

	logicNode = calcNodes / totWeight;

	return logicNode / (double)condensity_size;
}

const double HPWH::TempBasedHeatingLogic::getFractToMeetComparisonExternal(HPWH* phpwh) {
	double fracTemp, diff;
	int calcNode = 0;
	int firstNode = -1;
	double sum = 0;
	double totWeight = 0;

	double comparison = getComparisonValue(phpwh);
	comparison += HPWH::TOL_MINVALUE; // Make this possible so we do slightly over heat

	for (auto nodeWeight : nodeWeights) {

		// bottom calc node only
		if (nodeWeight.nodeNum == 0) { // simple equation
			calcNode = 0;
			firstNode = 0;
			sum = phpwh->tankTemps_C[firstNode] * nodeWeight.weight;
			totWeight = nodeWeight.weight;
		}
		// top calc node only
		else if (nodeWeight.nodeNum == 13) {
			calcNode = phpwh->getNumNodes() - 1;
			firstNode = phpwh->getNumNodes() - 1;
			sum = phpwh->tankTemps_C[firstNode] * nodeWeight.weight;
			totWeight = nodeWeight.weight;
		}
		else { // have to tally up the nodes
			// frac = ( nodesN*comparision - ( Sum Ti from i = 0 to N ) ) / ( TN+1 - T0 )
			firstNode = (nodeWeight.nodeNum - 1) * phpwh->nodeDensity;
			for (int n = 0; n < phpwh->nodeDensity; ++n) { // Loop on the nodes in the logics 
				calcNode = (nodeWeight.nodeNum - 1) * phpwh->nodeDensity + n;
				sum += phpwh->tankTemps_C[calcNode] * nodeWeight.weight;
				totWeight += nodeWeight.weight;
			}
		}
	}

	if (calcNode == phpwh->numNodes - 1) { // top node calc
		diff = phpwh->getSetpoint() - phpwh->tankTemps_C[firstNode];
	}
	else {
		diff = phpwh->tankTemps_C[calcNode + 1] - phpwh->tankTemps_C[firstNode];
	}
	// if totWeight * comparison - sum < 0 then the shutoff condition is already true and you shouldn't
	// be here. Will revaluate shut off condition at the end the do while loop of addHeatExternal, in the
	// mean time lets not shift anything around. 
	if (compare(sum, totWeight * comparison)) { // Then should shut off
		fracTemp = 0.; // 0 means shift no nodes
	}
	else {
		// if the difference in denominator is <= 0 then we aren't adding heat to the nodes we care about, so 
		// shift a whole node.
		fracTemp = diff > 0. ? (totWeight * comparison - sum) / diff : 1.;
	}

	return fracTemp;
}


