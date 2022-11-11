/*
File of the presets heating logics available HPWHsim
*/

/* State of Charge Based Logic*/
const bool HPWH::SoCBasedHeatingLogic::isValid() {
	bool isValid = true;
	if (decisionPoint < 0) {
		isValid = false;
	}
	return isValid;
}

const double HPWH::SoCBasedHeatingLogic::getComparisonValue() {
	return decisionPoint + hysteresisFraction;
}

const double HPWH::SoCBasedHeatingLogic::getTankValue() {
	double socFraction;
	if (useCostantMains) {
		socFraction = parentHPWH->getSoCFraction(constantMains_C, tempMinUseful_C);
	}
	else {
		socFraction = parentHPWH->getSoCFraction(parentHPWH->member_inletT_C, tempMinUseful_C);
	}
	return socFraction;
}

int HPWH::SoCBasedHeatingLogic::setDecisionPoint(double value) {
	decisionPoint = value;
	return 0;
}

int HPWH::SoCBasedHeatingLogic::setConstantMainsTemperature(double mains_C) {
	constantMains_C = mains_C;
	useCostantMains = true;
	return 0;
}

const double HPWH::SoCBasedHeatingLogic::nodeWeightAvgFract(int numberOfNodes, int condensity_size) {
	return decisionPoint * numberOfNodes / condensity_size;
}

const double HPWH::SoCBasedHeatingLogic::getFractToMeetComparisonExternal() {
	parentHPWH->numNodes; //TODO
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
	for (auto nodeWeight : nodeWeights) {
		if (nodeWeight.nodeNum > 13 || nodeWeight.nodeNum < 0) {
			return false;
		}
	}
	return true;
}

const double HPWH::TempBasedHeatingLogic::getComparisonValue() {
	double value = decisionPoint;
	if (isAbsolute) {
		return value;
	}
	else {
		return parentHPWH->getSetpoint() - value;
	}
}

const double HPWH::TempBasedHeatingLogic::getTankValue() {
	return parentHPWH->tankAvg_C(nodeWeights);
}

int HPWH::TempBasedHeatingLogic::setDecisionPoint(double value) {
	decisionPoint = value;
	return 0;
}
int HPWH::TempBasedHeatingLogic::setDecisionPoint(double value, bool absolute) {
	isAbsolute = absolute;
	return setDecisionPoint(value);
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

const double HPWH::TempBasedHeatingLogic::getFractToMeetComparisonExternal() {
	double fracTemp, diff;
	int calcNode = 0;
	int firstNode = -1;
	double sum = 0;
	double totWeight = 0;

	double comparison = getComparisonValue();
	comparison += HPWH::TOL_MINVALUE; // Make this possible so we do slightly over heat

	for (auto nodeWeight : nodeWeights) {

		// bottom calc node only
		if (nodeWeight.nodeNum == 0) { // simple equation
			calcNode = 0;
			firstNode = 0;
			sum = parentHPWH->tankTemps_C[firstNode] * nodeWeight.weight;
			totWeight = nodeWeight.weight;
		}
		// top calc node only
		else if (nodeWeight.nodeNum == 13) {
			calcNode = parentHPWH->getNumNodes() - 1;
			firstNode = parentHPWH->getNumNodes() - 1;
			sum = parentHPWH->tankTemps_C[firstNode] * nodeWeight.weight;
			totWeight = nodeWeight.weight;
		}
		else { // have to tally up the nodes
			// frac = ( nodesN*comparision - ( Sum Ti from i = 0 to N ) ) / ( TN+1 - T0 )
			firstNode = (nodeWeight.nodeNum - 1) * parentHPWH->nodeDensity;
			for (int n = 0; n < parentHPWH->nodeDensity; ++n) { // Loop on the nodes in the logics 
				calcNode = (nodeWeight.nodeNum - 1) * parentHPWH->nodeDensity + n;
				sum += parentHPWH->tankTemps_C[calcNode] * nodeWeight.weight;
				totWeight += nodeWeight.weight;
			}
		}
	}

	if (calcNode == parentHPWH->numNodes - 1) { // top node calc
		diff = parentHPWH->getSetpoint() - parentHPWH->tankTemps_C[firstNode];
	}
	else {
		diff = parentHPWH->tankTemps_C[calcNode + 1] - parentHPWH->tankTemps_C[firstNode];
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


