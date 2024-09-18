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
    if ((!hpwh->haveInletT) && (!useConstantMains))
    {
        hpwh->send_error("SoC-based heating logic used without constant mains.");
    }
    return hpwh->getSoCFraction();
}

HPWH::Temp_t HPWH::SoCBasedHeatingLogic::getMainsT()
{
    if (useConstantMains)
    {
        return constantMainsT;
    }
    else
    {
        return hpwh->member_inletT;
    }
}

HPWH::Temp_t HPWH::SoCBasedHeatingLogic::getMinUsefulT() { return minUsefulT; }

void HPWH::SoCBasedHeatingLogic::setConstantMainsT(Temp_t constantMainsT_in)
{
    constantMainsT = constantMainsT_in;
    useConstantMains = true;
}

double HPWH::SoCBasedHeatingLogic::nodeWeightAvgFract()
{
    return decisionPoint + hysteresisFraction;
}

double HPWH::SoCBasedHeatingLogic::getFractToMeetComparisonExternal()
{
    double deltaSoCFraction =
        (decisionPoint + hysteresisFraction + HPWH::TOL_MINVALUE) - getTankValue();

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
        if (hpwh->tankTs[i](UnitsTemp) < minUsefulT(UnitsTemp))
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
    double maxSoC =
        hpwh->getNumNodes() * hpwh->getChargePerNode(getMainsT(), minUsefulT, hpwh->setpointT);

    double fac = deltaSoCFraction * maxSoC +
                 (hpwh->tankTs[calcNode] - getMainsT()) / (minUsefulT - getMainsT());

    Temp_t targetT = getMainsT() + fac * (minUsefulT - getMainsT());

    // Catch case where node temperature == setpoint
    double fractCalcNode;
    if (hpwh->tankTs[calcNode](UnitsTemp) >= hpwh->setpointT(UnitsTemp))
    {
        fractCalcNode = 1;
    }
    else
    {
        fractCalcNode =
            (targetT - hpwh->tankTs[calcNode]) / (hpwh->setpointT - hpwh->tankTs[calcNode]);
    }

    // If we're at the bottom node there's not another node to heat so case 2 doesn't apply.
    if (calcNode == 0)
    {
        return fractCalcNode;
    }

    // Fraction to heat next node, where the step change occurs
    double fractNextNode = (minUsefulT - hpwh->tankTs[calcNode - 1]) /
                           (hpwh->tankTs[calcNode] - hpwh->tankTs[calcNode - 1]);

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
    if (decisionT.index() == 0)
    {
        return std::get<0>(decisionT)();
    }
    else
    {
        return (hpwh->getSetpointT() - std::get<1>(decisionT))();
    }
}

double HPWH::TempBasedHeatingLogic::getTankValue() { return hpwh->getAverageTankT(nodeWeights)(); }

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

    TempVect_t resampledTankTs(LOGIC_SIZE);
    resample(resampledTankTs(), hpwh->tankTs());

    double nodeDensity = static_cast<double>(hpwh->getNumNodes()) / LOGIC_SIZE;
    for (auto nodeWeight : nodeWeights)
    {
        if (nodeWeight.nodeNum == 0)
        { // bottom-most tank node only
            firstNode = calcNode = 0;
            double nodeT = hpwh->tankTs.front()();
            sum = nodeT * nodeWeight.weight;
            totWeight = nodeWeight.weight;
        }
        // top calc node only
        else if (nodeWeight.nodeNum == LOGIC_SIZE + 1)
        { // top-most tank node only
            firstNode = calcNode = hpwh->getNumNodes() - 1;
            double nodeT = hpwh->tankTs.back()();
            sum = nodeT * nodeWeight.weight;
            totWeight = nodeWeight.weight;
        }
        else
        { // logical nodes
            firstNode = static_cast<int>(nodeDensity *
                                         (nodeWeight.nodeNum - 1)); // first tank node in logic node
            calcNode = static_cast<int>(nodeDensity * (nodeWeight.nodeNum)) -
                       1; // last tank node in logical node
            auto logicNode = static_cast<std::size_t>(nodeWeight.nodeNum - 1);
            double logicNodeT = resampledTankTs[logicNode]();
            sum += logicNodeT * nodeWeight.weight;
            totWeight += nodeWeight.weight;
        }
    }

    Temp_t averageT(sum / totWeight);
    Temp_t targetT =
        (calcNode < hpwh->getNumNodes() - 1) ? hpwh->tankTs[calcNode + 1] : hpwh->getSetpointT();

    Temp_d_t node_dT = {targetT(Units::C) - hpwh->tankTs[firstNode](Units::C), Units::dC};

    Temp_t comparisonT = {getComparisonValue() + HPWH::TOL_MINVALUE, UnitsTemp};

    Temp_d_t logicNode_dT = {comparisonT(Units::C) - averageT(Units::C), Units::dC};
    // if averageT_C > comparison then the shutoff condition is already true and you
    // shouldn't be here. Will revaluate shut off condition at the end the do while loop of
    // addHeatExternal, in the meantime lets not shift anything around.
    // Then should shut off, 0 means shift no nodes

    // If the difference in denominator is <= 0 then we aren't adding heat to the nodes we care
    // about, so shift a whole node.
    // factor of nodeDensity converts logic-node fraction to tank-node fraction
    double nodeFrac = compare(averageT(), comparisonT())
                          ? 0.
                          : ((node_dT > 0.) ? nodeDensity * logicNode_dT / node_dT : 1.);

    return nodeFrac;
}
