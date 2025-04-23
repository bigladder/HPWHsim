/*
File of the heating logics available in HPWHsim
*/

#include "HPWHUtils.hh"
#include "HPWHHeatingLogic.hh"
#include "Tank.hh"

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
        if (hpwh->tank->nodeTs_C[i] < tempMinUseful_C)
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
        hpwh->getNumNodes() * getChargePerNode(getMainsT_C(), tempMinUseful_C, hpwh->setpoint_C);
    double targetTemp =
        deltaSoCFraction * maxSoC +
        (hpwh->tank->nodeTs_C[calcNode] - getMainsT_C()) / (tempMinUseful_C - getMainsT_C());
    targetTemp = targetTemp * (tempMinUseful_C - getMainsT_C()) + getMainsT_C();

    // Catch case where node temperature == setpoint
    double fractCalcNode;
    if (hpwh->tank->nodeTs_C[calcNode] >= hpwh->setpoint_C)
    {
        fractCalcNode = 1;
    }
    else
    {
        fractCalcNode = (targetTemp - hpwh->tank->nodeTs_C[calcNode]) /
                        (hpwh->setpoint_C - hpwh->tank->nodeTs_C[calcNode]);
    }

    // If we're at the bottom node there's not another node to heat so case 2 doesn't apply.
    if (calcNode == 0)
    {
        return fractCalcNode;
    }

    // Fraction to heat next node, where the step change occurs
    double fractNextNode = (tempMinUseful_C - hpwh->tank->nodeTs_C[calcNode - 1]) /
                           (hpwh->tank->nodeTs_C[calcNode] - hpwh->tank->nodeTs_C[calcNode - 1]);
    fractNextNode += HPWH::TOL_MINVALUE;

    // if the fraction is enough to heat up the next node, do that minimum and handle the heating of
    // the next node next iteration.
    return std::min(fractCalcNode, fractNextNode);
}

/* Temperature Based Heating Logic*/
HPWH::TempBasedHeatingLogic::TempBasedHeatingLogic(std::string desc,
                                                   std::vector<NodeWeight> n,
                                                   double decisionPoint,
                                                   HPWH* hpwh,
                                                   bool a,
                                                   std::function<bool(double, double)> c,
                                                   bool isHTS,
                                                   bool checksStandby_in)
    : HeatingLogic(desc, decisionPoint, hpwh, c, isHTS, checksStandby_in), isAbsolute(a)
{
    dist = {{}, {}};
    auto prev_height = 0.;
    for (auto node = n.begin(); node != n.end(); ++node)
    {
        if (node->nodeNum == 0)
        {
            dist = DistributionType::BottomOfTank;
            return;
        }
        if (node->nodeNum == LOGIC_SIZE + 1)
        {
            dist = {{}, {}};
            return;
        }
        double height_front = node->nodeNum - 1;
        if (height_front > prev_height)
            dist.weightedDistribution.push_back({height_front, 0.});
        while (node + 1 != n.end())
        {
            if ((node + 1)->weight == node->weight)
                ++node;
            else
                break;
        }
        height_front = node->nodeNum - 1;
        dist.weightedDistribution.push_back({height_front + 1., node->weight});
        prev_height = height_front + 1.;
    }
    if (prev_height < LOGIC_SIZE)
        dist.weightedDistribution.push_back({LOGIC_SIZE, 0.});
}

bool HPWH::TempBasedHeatingLogic::isValid()
{
    bool isValid = true;
    if (!dist.isValid())
    {
        isValid = false;
    }
    return isValid;
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

double HPWH::TempBasedHeatingLogic::getTankValue() { return hpwh->getAverageTankTemp_C(dist); }

void HPWH::TempBasedHeatingLogic::setDecisionPoint(double value) { decisionPoint = value; }
void HPWH::TempBasedHeatingLogic::setDecisionPoint(double value, bool absolute)
{
    isAbsolute = absolute;
    setDecisionPoint(value);
}

double HPWH::TempBasedHeatingLogic::nodeWeightAvgFract()
{
    switch (dist.distributionType)
    {
    case DistributionType::BottomOfTank:
        return 1. / (double)hpwh->getNumNodes();

    case DistributionType::TopOfTank:
        return 1.;

    case DistributionType::Weighted:
    {
        double tot_weight = 0., tot_weight_avg = 0.;
        double prev_height = 0.;
        for (auto distPoint : dist.weightedDistribution)
        {
            double weight = distPoint.weight * (distPoint.height - prev_height);
            double node_avg = 0.5 * (distPoint.height + prev_height);
            tot_weight_avg += weight * node_avg;
            tot_weight += weight;
            prev_height = distPoint.height;
        }
        double frac = tot_weight_avg / tot_weight / prev_height;
        return frac + 0.5 / LOGIC_SIZE;
    }
    }
    return 0.;
}

double HPWH::TempBasedHeatingLogic::getFractToMeetComparisonExternal()
{
    int calcNode = 0;
    int firstNode = -1;
    double sum = 0;
    double totWeight = 0;

    std::vector<double> resampledTankTemps(LOGIC_SIZE);
    resample(resampledTankTemps, hpwh->tank->nodeTs_C);
    double comparisonT_C = getComparisonValue() + HPWH::TOL_MINVALUE; // slightly over heat

    double nodeDensity = static_cast<double>(hpwh->getNumNodes()) / LOGIC_SIZE;
    switch (dist.distributionType)
    {
    case DistributionType::BottomOfTank:
    {
        firstNode = calcNode = 0;
        sum = hpwh->tank->nodeTs_C.front();
        totWeight = 1.;
        break;
    }

    case DistributionType::TopOfTank:
    {
        firstNode = calcNode = hpwh->getNumNodes() - 1;
        sum = hpwh->tank->nodeTs_C.back();
        totWeight = 1.;
        break;
    }

    case DistributionType::Weighted:
    {
        for (auto& distPoint : dist.weightedDistribution)
        {
            if (distPoint.weight > 0.)
            {
                firstNode = 0;
                calcNode = static_cast<int>(nodeDensity) - 1; // last tank node in logic node
                break;
            }
            else
            {
                double norm_dist_height =
                    distPoint.height / dist.weightedDistribution.maximumHeight();
                firstNode = static_cast<int>(
                    norm_dist_height * hpwh->getNumNodes()); // first tank node with non-zero weight
                calcNode =
                    firstNode + static_cast<int>(nodeDensity); // last tank node in logic node
                break;
            }
        }
        for (int i = firstNode; i <= calcNode; ++i)
        {
            sum += hpwh->tank->nodeTs_C[i];
            totWeight += 1.;
        }
    }
    }

    double averageT_C = sum / totWeight;
    double targetT_C = (calcNode < hpwh->getNumNodes() - 1) ? hpwh->tank->nodeTs_C[calcNode + 1]
                                                            : hpwh->getSetpoint();

    double nodeDiffT_C = targetT_C - hpwh->tank->nodeTs_C[firstNode];
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

/*static*/
std::shared_ptr<HPWH::HeatingLogic>
HPWH::HeatingLogic::make(const hpwh_data_model::heat_source_configuration::HeatingLogic& logic,
                         HPWH* hpwh)
{
    std::shared_ptr<HPWH::HeatingLogic> heatingLogic = nullptr;

    std::function<bool(double, double)> comparison_type;
    if (logic.comparison_type_is_set)
    {
        switch (logic.comparison_type)
        {
        case hpwh_data_model::heat_source_configuration::ComparisonType::GREATER_THAN:
        {
            comparison_type = std::greater<>();
            break;
        }

        case hpwh_data_model::heat_source_configuration::ComparisonType::LESS_THAN:
        {
            comparison_type = std::less<>();
            break;
        }

        default:
        case hpwh_data_model::heat_source_configuration::ComparisonType::UNKNOWN:
        {
        }
        }
    }

    switch (logic.heating_logic_type)
    {
    case hpwh_data_model::heat_source_configuration::HeatingLogicType::STATE_OF_CHARGE_BASED:
    {
        auto soc_based_logic = reinterpret_cast<
            hpwh_data_model::heat_source_configuration::StateOfChargeBasedHeatingLogic*>(
            logic.heating_logic.get());

        heatingLogic = std::make_shared<HPWH::SoCBasedHeatingLogic>(
            "name", soc_based_logic->decision_point, hpwh);

        break;
    }

    case hpwh_data_model::heat_source_configuration::HeatingLogicType::TEMPERATURE_BASED:
    default:
    {
        std::string label = "name";
        auto temp_based_logic = reinterpret_cast<
            hpwh_data_model::heat_source_configuration::TemperatureBasedHeatingLogic*>(
            logic.heating_logic.get());

        double temp = 20.;
        if (temp_based_logic->absolute_temperature_is_set)
        {
            temp = K_TO_C(temp_based_logic->absolute_temperature);
        }
        else if (temp_based_logic->differential_temperature_is_set)
        {
            temp = temp_based_logic->differential_temperature;
        }
        else
            break;

        bool checksStandby_in = false;
        Distribution dist;
        if (temp_based_logic->standby_temperature_location_is_set)
        {
            switch (temp_based_logic->standby_temperature_location)
            {
            case hpwh_data_model::heat_source_configuration::StandbyTemperatureLocation::
                TOP_OF_TANK:
            {
                dist = DistributionType::TopOfTank;
                label = "top of tank";
                checksStandby_in = true;
                break;
            }
            case hpwh_data_model::heat_source_configuration::StandbyTemperatureLocation::
                BOTTOM_OF_TANK:
            {
                dist = DistributionType::BottomOfTank;
                label = "bottom of tank";
                break;
            }
            default:
            {
            }
            }
        }

        if (temp_based_logic->temperature_weight_distribution_is_set)
        {
            auto& weight_dist = temp_based_logic->temperature_weight_distribution;
            if (weight_dist.normalized_height_is_set && weight_dist.weight_is_set)
            {
                dist = {weight_dist.normalized_height, weight_dist.weight};
            }
        }

        heatingLogic = std::make_shared<HPWH::TempBasedHeatingLogic>(
            label,
            dist,
            temp,
            hpwh,
            temp_based_logic->absolute_temperature_is_set,
            comparison_type,
            checksStandby_in);
        break;
    }
    }
    return heatingLogic;
}

void HPWH::SoCBasedHeatingLogic::to(
    hpwh_data_model::heat_source_configuration::HeatingLogic& heating_logic) const
{
    checkTo(hpwh_data_model::heat_source_configuration::HeatingLogicType::STATE_OF_CHARGE_BASED,
            heating_logic.heating_logic_type_is_set,
            heating_logic.heating_logic_type);

    hpwh_data_model::heat_source_configuration::StateOfChargeBasedHeatingLogic logic;
    checkTo(decisionPoint, logic.decision_point_is_set, logic.decision_point);
    checkTo(C_TO_K(tempMinUseful_C),
            logic.minimum_useful_temperature_is_set,
            logic.minimum_useful_temperature);
    checkTo(hysteresisFraction, logic.hysteresis_fraction_is_set, logic.hysteresis_fraction);
    checkTo(useCostantMains, logic.uses_constant_mains_is_set, logic.uses_constant_mains);
    checkTo(C_TO_K(constantMains_C),
            logic.constant_mains_temperature_is_set,
            logic.constant_mains_temperature);

    if (compare(1., 2.))
    {
        checkTo(hpwh_data_model::heat_source_configuration::ComparisonType::LESS_THAN,
                heating_logic.comparison_type_is_set,
                heating_logic.comparison_type);
    }
    else
    {
        checkTo(hpwh_data_model::heat_source_configuration::ComparisonType::GREATER_THAN,
                heating_logic.comparison_type_is_set,
                heating_logic.comparison_type);
    }

    heating_logic.heating_logic = std::make_unique<
        hpwh_data_model::heat_source_configuration::StateOfChargeBasedHeatingLogic>();
    *heating_logic.heating_logic = logic;
    heating_logic.heating_logic_is_set = true;
}

void HPWH::TempBasedHeatingLogic::to(
    hpwh_data_model::heat_source_configuration::HeatingLogic& heating_logic) const
{
    checkTo(hpwh_data_model::heat_source_configuration::HeatingLogicType::TEMPERATURE_BASED,
            heating_logic.heating_logic_type_is_set,
            heating_logic.heating_logic_type);

    hpwh_data_model::heat_source_configuration::TemperatureBasedHeatingLogic logic;

    checkTo(C_TO_K(decisionPoint),
            logic.absolute_temperature_is_set,
            logic.absolute_temperature,
            isAbsolute);

    checkTo(decisionPoint,
            logic.differential_temperature_is_set,
            logic.differential_temperature,
            !isAbsolute);

    if (compare(1., 2.))
    {
        checkTo(hpwh_data_model::heat_source_configuration::ComparisonType::LESS_THAN,
                heating_logic.comparison_type_is_set,
                heating_logic.comparison_type);
    }
    else
    {
        checkTo(hpwh_data_model::heat_source_configuration::ComparisonType::GREATER_THAN,
                heating_logic.comparison_type_is_set,
                heating_logic.comparison_type);
    }

    switch (dist.distributionType)
    {
    case DistributionType::TopOfTank:
    {
        checkTo(hpwh_data_model::heat_source_configuration::StandbyTemperatureLocation::TOP_OF_TANK,
                logic.standby_temperature_location_is_set,
                logic.standby_temperature_location);

        logic.temperature_weight_distribution_is_set = false;
        break;
    }
    case DistributionType::BottomOfTank:
    {
        checkTo(
            hpwh_data_model::heat_source_configuration::StandbyTemperatureLocation::BOTTOM_OF_TANK,
            logic.standby_temperature_location_is_set,
            logic.standby_temperature_location);

        logic.temperature_weight_distribution_is_set = false;
        break;
    }
    case DistributionType::Weighted:
    {
        std::vector<double> heights = {}, weights = {};
        for (std::size_t i = 0; i < dist.weightedDistribution.size(); ++i)
        {
            heights.push_back(dist.weightedDistribution.unitaryHeight(i));
            weights.push_back(dist.weightedDistribution.unitaryWeight(i));
        }

        hpwh_data_model::heat_source_configuration::WeightedDistribution wd;
        checkTo(heights, wd.normalized_height_is_set, wd.normalized_height);
        checkTo(weights, wd.weight_is_set, wd.weight);
        checkTo(wd,
                logic.temperature_weight_distribution_is_set,
                logic.temperature_weight_distribution);
        break;
    }
    }
    heating_logic.heating_logic =
        std::make_unique<hpwh_data_model::heat_source_configuration::TemperatureBasedHeatingLogic>(
            logic);

    heating_logic.heating_logic_is_set = true;
}
