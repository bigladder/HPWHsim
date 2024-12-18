/*
File of the heating logics available in HPWHsim
*/

#include "HPWH.hh"

/* State of Charge Based Logic*/
bool HPWH::SoCBasedHeatingLogic::isValid()
{
    bool isValid = true;
    if (decisionPoint < 0)
    {
        isValid = false;
    }
    return isValid;
}

double HPWH::SoCBasedHeatingLogic::getComparisonValue()
{
    return decisionPoint + hysteresisFraction;
}

double HPWH::SoCBasedHeatingLogic::getTankValue()
{
    if ((!hpwh->haveInletT) && (!useCostantMains))
    {
        hpwh->send_error("SoC-based heating logic used without constant mains.");
    }
    return hpwh->getSoCFraction();
}

double HPWH::SoCBasedHeatingLogic::getMainsT_C()
{
    if (useCostantMains)
    {
        return constantMains_C;
    }
    else
    {
        return hpwh->member_inletT_C;
    }
}

double HPWH::SoCBasedHeatingLogic::getTempMinUseful_C() { return tempMinUseful_C; }

void HPWH::SoCBasedHeatingLogic::setDecisionPoint(double value) { decisionPoint = value; }

void HPWH::SoCBasedHeatingLogic::setConstantMainsTemperature(double mains_C)
{
    constantMains_C = mains_C;
    useCostantMains = true;
}

double HPWH::SoCBasedHeatingLogic::nodeWeightAvgFract() { return getComparisonValue(); }

double HPWH::SoCBasedHeatingLogic::getFractToMeetComparisonExternal()
{
    double deltaSoCFraction = (getComparisonValue() + HPWH::TOL_MINVALUE) - getTankValue();

    // Check how much of a change in the SoC fraction occurs if one full node at set point is added.
    // If this is less than the change needed move on.
    double fullNodeSoC = 1. / hpwh->getNumNodes();
    if (deltaSoCFraction >= fullNodeSoC)
    {
        return 1.;
    }

    // Find the last node greater the min use temp
    int calcNode = 0;
    for (int i = hpwh->getNumNodes() - 1; i >= 0; i--)
    {
        if (hpwh->tankTemps_C[i] < tempMinUseful_C)
        {
            calcNode = i + 1;
            break;
        }
    }
    if (calcNode == hpwh->getNumNodes())
    { // if the whole tank is cold
        return 1.;
    }

    // Find the fraction to heat the calc node to meet the target SoC fraction without heating the
    // node below up to tempMinUseful.
    double maxSoC = hpwh->getNumNodes() *
                    hpwh->getChargePerNode(getMainsT_C(), tempMinUseful_C, hpwh->setpoint_C);
    double targetTemp = deltaSoCFraction * maxSoC + (hpwh->tankTemps_C[calcNode] - getMainsT_C()) /
                                                        (tempMinUseful_C - getMainsT_C());
    targetTemp = targetTemp * (tempMinUseful_C - getMainsT_C()) + getMainsT_C();

    // Catch case where node temperature == setpoint
    double fractCalcNode;
    if (hpwh->tankTemps_C[calcNode] >= hpwh->setpoint_C)
    {
        fractCalcNode = 1;
    }
    else
    {
        fractCalcNode = (targetTemp - hpwh->tankTemps_C[calcNode]) /
                        (hpwh->setpoint_C - hpwh->tankTemps_C[calcNode]);
    }

    // If we're at the bottom node there's not another node to heat so case 2 doesn't apply.
    if (calcNode == 0)
    {
        return fractCalcNode;
    }

    // Fraction to heat next node, where the step change occurs
    double fractNextNode = (tempMinUseful_C - hpwh->tankTemps_C[calcNode - 1]) /
                           (hpwh->tankTemps_C[calcNode] - hpwh->tankTemps_C[calcNode - 1]);
    fractNextNode += HPWH::TOL_MINVALUE;

    // if the fraction is enough to heat up the next node, do that minimum and handle the heating of
    // the next node next iteration.
    return std::min(fractCalcNode, fractNextNode);
}

/* Temperature Based Heating Logic*/
bool HPWH::TempBasedHeatingLogic::isValid()
{
    bool isValid = true;
    if (!areNodeWeightsValid())
    {
        isValid = false;
    }
    return isValid;
}

bool HPWH::TempBasedHeatingLogic::areNodeWeightsValid()
{
    for (auto nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum > 13 || nodeWeight.nodeNum < 0)
        {
            return false;
        }
    }
    return true;
}

double HPWH::TempBasedHeatingLogic::getComparisonValue()
{
    double value = decisionPoint;
    if (isAbsolute)
    {
        return value;
    }
    else
    {
        return hpwh->getSetpoint() - value;
    }
}

double HPWH::TempBasedHeatingLogic::getTankValue()
{
    return hpwh->getAverageTankTemp_C(nodeWeights);
}

void HPWH::TempBasedHeatingLogic::setDecisionPoint(double value) { decisionPoint = value; }
void HPWH::TempBasedHeatingLogic::setDecisionPoint(double value, bool absolute)
{
    isAbsolute = absolute;
    setDecisionPoint(value);
}

double HPWH::TempBasedHeatingLogic::nodeWeightAvgFract()
{
    double logicNode;
    double calcNodes = 0, totWeight = 0;

    for (auto nodeWeight : nodeWeights)
    {
        // bottom calc node only
        if (nodeWeight.nodeNum == 0)
        { // simple equation
            return 1. / (double)hpwh->getNumNodes();
        }
        // top calc node only
        else if (nodeWeight.nodeNum == LOGIC_SIZE + 1)
        {
            return 1.;
        }
        else
        { // have to tally up the nodes
            calcNodes += nodeWeight.nodeNum * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
    }

    logicNode = calcNodes / totWeight;

    return logicNode / static_cast<double>(LOGIC_SIZE);
}

double HPWH::TempBasedHeatingLogic::getFractToMeetComparisonExternal()
{
    int calcNode = 0;
    int firstNode = -1;
    double sum = 0;
    double totWeight = 0;

    std::vector<double> resampledTankTemps(LOGIC_SIZE);
    resample(resampledTankTemps, hpwh->tankTemps_C);
    double comparisonT_C = getComparisonValue() + HPWH::TOL_MINVALUE; // slightly over heat

    double nodeDensity = static_cast<double>(hpwh->getNumNodes()) / LOGIC_SIZE;
    for (auto nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum == 0)
        { // bottom-most tank node only
            firstNode = calcNode = 0;
            double nodeT_C = hpwh->tankTemps_C.front();
            sum = nodeT_C * nodeWeight.weight;
            totWeight = nodeWeight.weight;
        }
        // top calc node only
        else if (nodeWeight.nodeNum == LOGIC_SIZE + 1)
        { // top-most tank node only
            firstNode = calcNode = hpwh->getNumNodes() - 1;
            double nodeT_C = hpwh->tankTemps_C.back();
            sum = nodeT_C * nodeWeight.weight;
            totWeight = nodeWeight.weight;
        }
        else
        { // logical nodes
            firstNode = static_cast<int>(nodeDensity *
                                         (nodeWeight.nodeNum - 1)); // first tank node in logic node
            calcNode = static_cast<int>(nodeDensity * (nodeWeight.nodeNum)) -
                       1; // last tank node in logical node
            auto logicNode = static_cast<std::size_t>(nodeWeight.nodeNum - 1);
            double logicNodeT_C = resampledTankTemps[logicNode];
            sum += logicNodeT_C * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
    }

    double averageT_C = sum / totWeight;
    double targetT_C = (calcNode < hpwh->getNumNodes() - 1) ? hpwh->tankTemps_C[calcNode + 1]
                                                            : hpwh->getSetpoint();

    double nodeDiffT_C = targetT_C - hpwh->tankTemps_C[firstNode];
    double logicNodeDiffT_C = comparisonT_C - averageT_C;

    // if averageT_C > comparison then the shutoff condition is already true and you
    // shouldn't be here. Will revaluate shut off condition at the end the do while loop of
    // addHeatExternal, in the mean time lets not shift anything around.
    // Then should shut off, 0 means shift no nodes

    // If the difference in denominator is <= 0 then we aren't adding heat to the nodes we care
    // about, so shift a whole node.
    // factor of nodeDensity converts logic-node fraction to tank-node fraction
    double nodeFrac =
        compare(averageT_C, comparisonT_C)
            ? 0.
            : ((nodeDiffT_C > 0.) ? nodeDensity * logicNodeDiffT_C / nodeDiffT_C : 1.);

    return nodeFrac;
}
