﻿#include "HPWH.hh"
#include "HPWHUtils.hh"
#include "Tank.hh"

HPWH::Tank::Tank(const HPWH::Tank& tank_in) : Sender(tank_in) { *this = tank_in; }

HPWH::Tank& HPWH::Tank::operator=(const HPWH::Tank& tank_in)
{
    if (this == &tank_in)
    {
        return *this;
    }
    Sender::operator=(tank_in);
    hpwh = tank_in.hpwh;

    volume_L = tank_in.volume_L;
    volumeFixed = tank_in.volumeFixed;
    UA_kJperHrC = tank_in.UA_kJperHrC;
    fittingsUA_kJperHrC = tank_in.fittingsUA_kJperHrC;
    nodeTs_C = tank_in.nodeTs_C;
    nextNodeTs_C = tank_in.nextNodeTs_C;
    mixesOnDraw = tank_in.mixesOnDraw;
    mixBelowFractionOnDraw = tank_in.mixBelowFractionOnDraw;
    doInversionMixing = tank_in.doInversionMixing;
    hasHeatExchanger = tank_in.hasHeatExchanger;
    return *this;
}

void HPWH::Tank::from(hpwh_data_model::rstank::RSTANK& rstank)
{
    auto& perf = rstank.performance;

    checkFrom(volume_L, perf.volume_is_set, 1000. * perf.volume, 0.);
    checkFrom(UA_kJperHrC, perf.ua_is_set, 3600. * perf.ua / 1000., 0.);
    checkFrom(fittingsUA_kJperHrC, perf.fittings_ua_is_set, 3600. * perf.fittings_ua / 1000., 0.);
    checkFrom(mixBelowFractionOnDraw,
              perf.bottom_fraction_of_tank_mixing_on_draw_is_set,
              perf.bottom_fraction_of_tank_mixing_on_draw,
              0.);
    mixesOnDraw = (mixBelowFractionOnDraw > 0.);

    hasHeatExchanger = perf.heat_exchanger_effectiveness_is_set;
    if (hasHeatExchanger)
    {
        checkFrom(heatExchangerEffectiveness,
                  perf.heat_exchanger_effectiveness_is_set,
                  perf.heat_exchanger_effectiveness,
                  1.);
    }
}

void HPWH::Tank::to(hpwh_data_model::rstank::RSTANK& rstank) const
{
    generate_metadata<hpwh_data_model::rstank::Schema>(
        rstank,
        "RSTANK",
        "https://github.com/bigladder/hpwh-data-model/blob/main/schema/RSTANK.schema.yaml");

    auto& perf = rstank.performance;
    checkTo(volume_L / 1000., perf.volume_is_set, perf.volume);
    checkTo(1000. * UA_kJperHrC / 3600., perf.ua_is_set, perf.ua);
    checkTo(1000. * fittingsUA_kJperHrC / 3600., perf.fittings_ua_is_set, perf.fittings_ua);
    double out_mixBelowFractionOnDraw = mixesOnDraw ? mixBelowFractionOnDraw : 0.;
    checkTo(out_mixBelowFractionOnDraw,
            perf.bottom_fraction_of_tank_mixing_on_draw_is_set,
            perf.bottom_fraction_of_tank_mixing_on_draw);

    if (hasHeatExchanger)
        checkTo(heatExchangerEffectiveness,
                perf.heat_exchanger_effectiveness_is_set,
                perf.heat_exchanger_effectiveness);

    else
        perf.heat_exchanger_effectiveness_is_set = false;

    rstank.performance_is_set = true;
}

void HPWH::Tank::setAllDefaults()
{
    nodeTs_C.clear();
    nextNodeTs_C.clear();
    volumeFixed = true;
    inletHeight = 0;
    inlet2Height = 0;
    mixBelowFractionOnDraw = 1. / 3.;
    doInversionMixing = true;
    doConduction = true;
    hasHeatExchanger = false;
    heatExchangerEffectiveness = 0.9;
    fittingsUA_kJperHrC = 0.;
}

void HPWH::Tank::calcSizeConstants()
{
    const double tankRad_m = getRadius_m();
    const double tankHeight_m = ASPECTRATIO * tankRad_m;

    nodeVolume_L = volume_L / getNumNodes();
    nodeCp_kJperC = CPWATER_kJperkgC * DENSITYWATER_kgperL * nodeVolume_L;
    nodeHeight_m = tankHeight_m / getNumNodes();

    fracAreaTop = tankRad_m / (2.0 * (tankHeight_m + tankRad_m));
    fracAreaSide = tankHeight_m / (tankHeight_m + tankRad_m);

    /// Single-node heat-exchange effectiveness
    nodeHeatExchangerEffectiveness =
        1. - pow(1. - heatExchangerEffectiveness, 1. / static_cast<double>(getNumNodes()));
}

double HPWH::Tank::getVolume_L() const { return volume_L; }

void HPWH::Tank::setVolume_L(double volume_L_in, bool forceChange /*=false*/)
{
    if (volumeFixed && !forceChange)
    {
        send_error("Can not change the tank size for your currently selected model.");
    }
    if (volume_L_in <= 0)
    {
        send_error("You have attempted to set the tank volume outside of bounds.");
    }
    volume_L = volume_L_in;
    calcSizeConstants();
}

/// set the UA
void HPWH::Tank::setUA_kJperHrC(double UA_kJperHrC_in) { UA_kJperHrC = UA_kJperHrC_in; }

/// get the UA
double HPWH::Tank::getUA_kJperHrC() const { return UA_kJperHrC; }

/*static*/ double HPWH::Tank::getRadius_m(double volume_L)
{
    double value_m = -1.;
    if (volume_L >= 0.)
    {
        double vol_ft3 = L_TO_FT3(volume_L);
        double value_ft = pow(vol_ft3 / 3.14159 / ASPECTRATIO, 1. / 3.);
        value_m = FT_TO_M(value_ft);
    }
    return value_m;
}

double HPWH::Tank::getRadius_m() const { return getRadius_m(volume_L); }

void HPWH::Tank::setVolumeAndAdjustUA(double volume_L_in, bool forceChange)
{
    double newA = getSurfaceArea_m2(volume_L_in);
    double oldA = getSurfaceArea_m2();

    setVolume_L(volume_L_in, forceChange);
    setUA_kJperHrC(UA_kJperHrC * (newA / oldA));
    calcSizeConstants();
}

/*static*/ double HPWH::Tank::getSurfaceArea_m2(double volume_L)
{
    // retain legacy test values by unit conversion
    double vol_ft3 = L_TO_FT3(volume_L);
    double radius_ft = pow(vol_ft3 / 3.14159 / ASPECTRATIO, 1. / 3.);

    double SA_ft2 = 2. * 3.14159 * pow(radius_ft, 2) * (ASPECTRATIO + 1.);
    double SA_m2 = FT2_TO_M2(SA_ft2);
    return SA_m2;
}

double HPWH::Tank::getSurfaceArea_m2() const { return getSurfaceArea_m2(volume_L); }

void HPWH::Tank::setNumNodes(const std::size_t num_nodes)
{
    nodeTs_C.resize(num_nodes);
    nextNodeTs_C.resize(num_nodes);
}

void HPWH::Tank::setNodeTs_C(const std::vector<double>& nodeTs_C_in)
{
    std::size_t numSetNodes = nodeTs_C_in.size();
    if (numSetNodes == 0)
    {
        send_error("No temperatures provided.");
    }

    // set node temps
    resampleIntensive(nodeTs_C, nodeTs_C_in);
}

double HPWH::Tank::getNodeT_C(int nodeNum) const
{
    if (nodeTs_C.empty())
    {
        send_error(
            "You have attempted to access the temperature of a tank node that does not exist.");
    }
    return nodeTs_C[nodeNum];
}

//-----------------------------------------------------------------------------
///	@brief	Evaluates the tank temperature averaged uniformly
/// @param[in]	nodeWeights	Discrete set of weighted nodes
/// @return	Tank temperature (C)
//-----------------------------------------------------------------------------
double HPWH::Tank::getAverageNodeT_C() const
{
    double totalT_C = 0.;
    for (auto& T_C : nodeTs_C)
    {
        totalT_C += T_C;
    }
    return totalT_C / static_cast<double>(getNumNodes());
}

double HPWH::Tank::getAverageNodeT_C(const std::vector<double>& dist) const
{
    std::vector<double> resampledTankTemps_C(dist.size());
    HPWH::resample(resampledTankTemps_C, nodeTs_C);

    double tankT_C = 0.;

    std::size_t j = 0;
    for (auto& nodeT_C : resampledTankTemps_C)
    {
        tankT_C += dist[j] * nodeT_C;
        ++j;
    }
    return tankT_C;
}

double HPWH::Tank::getAverageNodeT_C(const WeightedDistribution& wdist) const
{
    const int numNodes = hpwh->getNumNodes();
    double sum = 0;
    double totWeight = 0;
    auto distPoint = wdist.begin();
    for (int i = 0; i < numNodes; ++i)
    {
        double norm_node_height = static_cast<double>(i) / numNodes;
        double norm_dist_height = distPoint->height / wdist.maximumHeight();

        double next_norm_node_height = static_cast<double>(i + 1) / numNodes;
        double nodeT_C = hpwh->tank->nodeTs_C[i];
        double norm_height = norm_node_height;
        while (norm_dist_height < next_norm_node_height)
        {
            double weight = distPoint->weight * (norm_dist_height - norm_height);
            if (weight > 0.)
            {
                sum += weight * nodeT_C;
                totWeight += weight;
            }

            ++distPoint;
            if (distPoint == wdist.end())
                break;

            norm_height = norm_dist_height;
            norm_dist_height = distPoint->height / wdist.maximumHeight();
        }
        double weight = distPoint->weight * (next_norm_node_height - norm_height);
        if (weight > 0.)
        {
            sum += weight * nodeT_C;
            totWeight += weight;
        }
    }
    return sum / totWeight;
}

double HPWH::Tank::getAverageNodeT_C(const Distribution& dist) const
{
    switch (dist.distributionType)
    {
    case DistributionType::BottomOfTank:
    {
        return hpwh->tank->nodeTs_C.front();
    }

    case DistributionType::TopOfTank:
    {
        return hpwh->tank->nodeTs_C.back();
    }

    case DistributionType::Weighted:
    {
        return hpwh->tank->getAverageNodeT_C(dist.weightedDistribution);
    }
    }
    return 0;
}

// returns tank heat content relative to 0 C using kJ
double HPWH::Tank::getHeatContent_kJ() const
{
    return DENSITYWATER_kgperL * volume_L * CPWATER_kJperkgC * getAverageNodeT_C();
}

double HPWH::Tank::getNthSimTcouple(int iTCouple, int nTCouple) const
{
    if (iTCouple > nTCouple || iTCouple < 1)
    {
        send_error("You have attempted to access a simulated thermocouple that does not exist.");
    }
    double beginFraction = static_cast<double>(iTCouple - 1.) / static_cast<double>(nTCouple);
    double endFraction = static_cast<double>(iTCouple) / static_cast<double>(nTCouple);

    return getResampledValue(nodeTs_C, beginFraction, endFraction);
}

int HPWH::Tank::getInletHeight(int whichInlet) const
{
    if (whichInlet == 1)
    {
        return inletHeight;
    }
    else if (whichInlet != 2)
    {
        send_error("Invalid inlet chosen in getInletHeight.");
    }
    return inlet2Height;
}

void HPWH::Tank::setDoInversionMixing(bool doInversionMixing_in)
{
    doInversionMixing = doInversionMixing_in;
}

void HPWH::Tank::setDoConduction(bool doConduction_in) { doConduction = doConduction_in; }

void HPWH::Tank::mixNodes(int mixBottomNode, int mixBelowNode, double mixFactor)
{
    double avgT_C = 0.;
    double numAvgNodes = static_cast<double>(mixBelowNode - mixBottomNode);
    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        avgT_C += nodeTs_C[i];
    }
    avgT_C /= numAvgNodes;

    for (int i = mixBottomNode; i < mixBelowNode; i++)
    {
        nodeTs_C[i] += mixFactor * (avgT_C - nodeTs_C[i]);
    }
}

// Inversion mixing modeled after bigladder EnergyPlus code PK
void HPWH::Tank::mixInversions()
{
    bool hasInversion;
    const double volumePerNode_L = volume_L / getNumNodes();
    // int numdos = 0;
    if (doInversionMixing)
    {
        do
        {
            hasInversion = false;
            // Start from the top and check downwards
            for (int i = getNumNodes() - 1; i > 0; i--)
            {
                if (nodeTs_C[i] < nodeTs_C[i - 1])
                {
                    // Temperature inversion!
                    hasInversion = true;

                    // Mix this inversion mixing temperature by averaging all inverted nodes
                    // together.
                    double Tmixed = 0.0;
                    double massMixed = 0.0;
                    int m;
                    for (m = i; m >= 0; m--)
                    {
                        Tmixed += nodeTs_C[m] * (volumePerNode_L * DENSITYWATER_kgperL);
                        massMixed += (volumePerNode_L * DENSITYWATER_kgperL);
                        if ((m == 0) || (Tmixed / massMixed > nodeTs_C[m - 1]))
                        {
                            break;
                        }
                    }
                    Tmixed /= massMixed;

                    // Assign the tank temps from i to k
                    for (int k = i; k >= m; k--)
                        nodeTs_C[k] = Tmixed;
                }
            }

        } while (hasInversion);
    }
}

void HPWH::Tank::updateNodes(double drawVolume_L,
                             double inletT_C,
                             double tankAmbientT_C,
                             double inletVol2_L,
                             double inletT2_C)
{
    if (drawVolume_L > 0.)
    {
        if (inletVol2_L > drawVolume_L)
        {
            send_error("Volume in inlet 2 is greater than the draw volume.");
            return;
        }

        // sort the inlets by height
        int highInletNodeIndex;
        double highInletT_C;
        double highInletFraction; // fraction of draw from high inlet

        int lowInletNodeIndex;
        double lowInletT_C;
        double lowInletFraction; // fraction of draw from low inlet

        if (inletHeight > inlet2Height)
        {
            highInletNodeIndex = inletHeight;
            highInletFraction = 1. - inletVol2_L / drawVolume_L;
            highInletT_C = inletT_C;
            lowInletNodeIndex = inlet2Height;
            lowInletT_C = inletT2_C;
            lowInletFraction = inletVol2_L / drawVolume_L;
        }
        else
        {
            highInletNodeIndex = inlet2Height;
            highInletFraction = inletVol2_L / drawVolume_L;
            highInletT_C = inletT2_C;
            lowInletNodeIndex = inletHeight;
            lowInletT_C = inletT_C;
            lowInletFraction = 1. - inletVol2_L / drawVolume_L;
        }

        // calculate number of nodes to draw
        double drawVolume_N = drawVolume_L / nodeVolume_L;
        double drawCp_kJperC = CPWATER_kJperkgC * DENSITYWATER_kgperL * drawVolume_L;

        // heat-exchange models
        if (hasHeatExchanger)
        {
            outletT_C = inletT_C;
            for (auto& nodeT_C : nodeTs_C)
            {
                double maxHeatExchange_kJ = drawCp_kJperC * (nodeT_C - outletT_C);
                double heatExchange_kJ = nodeHeatExchangerEffectiveness * maxHeatExchange_kJ;

                nodeT_C -= heatExchange_kJ / nodeCp_kJperC;
                outletT_C += heatExchange_kJ / drawCp_kJperC;
            }
        }
        else
        {
            double remainingDrawVolume_N = drawVolume_N;
            double totalExpelledHeat_kJ = 0.;
            while (remainingDrawVolume_N > 0.)
            {

                // draw no more than one node at a time
                double incrementalDrawVolume_N =
                    remainingDrawVolume_N > 1. ? 1. : remainingDrawVolume_N;

                double outputHeat_kJ = nodeCp_kJperC * incrementalDrawVolume_N * nodeTs_C.back();
                totalExpelledHeat_kJ += outputHeat_kJ;
                nodeTs_C.back() -= outputHeat_kJ / nodeCp_kJperC;

                double inletFraction = 0.; // accumulate inlet contributions
                for (int i = getNumNodes() - 1; i >= 0; --i)
                {

                    if (i == highInletNodeIndex)
                    {
                        inletFraction += highInletFraction;
                        nodeTs_C[i] += incrementalDrawVolume_N * highInletFraction * highInletT_C;
                    }
                    if (i == lowInletNodeIndex)
                    {
                        inletFraction += lowInletFraction;
                        nodeTs_C[i] += incrementalDrawVolume_N * lowInletFraction * lowInletT_C;
                    }

                    if (i > 0)
                    {
                        double transferT_C =
                            incrementalDrawVolume_N * (1. - inletFraction) * nodeTs_C[i - 1];
                        nodeTs_C[i] += transferT_C;
                        nodeTs_C[i - 1] -= transferT_C;
                    }
                }

                remainingDrawVolume_N -= incrementalDrawVolume_N;
                mixInversions();
            }

            outletT_C = totalExpelledHeat_kJ / drawCp_kJperC;
        }

        // account for mixing at the bottom of the tank
        if (mixesOnDraw && (drawVolume_L > 0.))
        {
            int mixedBelowNode = (int)(getNumNodes() * mixBelowFractionOnDraw);
            mixNodes(0, mixedBelowNode, 1. / 3.);
        }

    } // end if(draw_volume_L > 0)

    // Initialize newnodeTs_C
    nextNodeTs_C = nodeTs_C;

    double standbyLossesBottom_kJ = 0.;
    double standbyLossesTop_kJ = 0.;
    double standbyLossesSides_kJ = 0.;

    // Standby losses from the top and bottom of the tank
    {
        auto standbyLossRate_kJperHrC = UA_kJperHrC * fracAreaTop;

        standbyLossesBottom_kJ =
            standbyLossRate_kJperHrC * hpwh->hoursPerStep * (nodeTs_C[0] - tankAmbientT_C);
        standbyLossesTop_kJ = standbyLossRate_kJperHrC * hpwh->hoursPerStep *
                              (nodeTs_C[getNumNodes() - 1] - tankAmbientT_C);

        nextNodeTs_C.front() -= standbyLossesBottom_kJ / nodeCp_kJperC;
        nextNodeTs_C.back() -= standbyLossesTop_kJ / nodeCp_kJperC;
    }

    // Standby losses from the sides of the tank
    {
        auto standbyLossRate_kJperHrC =
            (UA_kJperHrC * fracAreaSide + fittingsUA_kJperHrC) / getNumNodes();
        for (int i = 0; i < getNumNodes(); i++)
        {
            double losses_kJ =
                standbyLossRate_kJperHrC * hpwh->hoursPerStep * (nodeTs_C[i] - tankAmbientT_C);
            standbyLossesSides_kJ += losses_kJ;

            nextNodeTs_C[i] -= losses_kJ / nodeCp_kJperC;
        }
    }

    // Heat transfer between nodes
    if (doConduction)
    {

        // Get the "constant" tau for the stability condition and the conduction calculation
        const double tau = 2. * KWATER_WpermC /
                           ((CPWATER_kJperkgC * 1000.0) * (DENSITYWATER_kgperL * 1000.0) *
                            (nodeHeight_m * nodeHeight_m)) *
                           hpwh->secondsPerStep;
        if (tau > 1.)
        {
            send_error(fmt::format("The stability condition for conduction has failed!"));
            return;
        }

        // End nodes
        if (getNumNodes() > 1)
        { // inner edges of top and bottom nodes
            nextNodeTs_C.front() += tau * (nodeTs_C[1] - nodeTs_C.front());
            nextNodeTs_C.back() += tau * (nodeTs_C[getNumNodes() - 2] - nodeTs_C.back());
        }

        // Internal nodes
        for (int i = 1; i < getNumNodes() - 1; i++)
        {
            nextNodeTs_C[i] += tau * (nodeTs_C[i + 1] - 2. * nodeTs_C[i] + nodeTs_C[i - 1]);
        }
    }

    // Update nodeTs_C
    nodeTs_C = nextNodeTs_C;

    standbyLosses_kJ += standbyLossesBottom_kJ + standbyLossesTop_kJ + standbyLossesSides_kJ;

    mixInversions();

} // end updateNodes

void HPWH::Tank::setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum)
{
    if (fractionalHeight > 1. || fractionalHeight < 0.)
    {
        send_error("Out of bounds fraction for setInletByFraction.");
    }

    auto node = static_cast<int>(std::floor(getNumNodes() * fractionalHeight));
    inletNum = (node == getNumNodes()) ? getIndexTopNode() : node;
}

void HPWH::Tank::setInletByFraction(double fractionalHeight)
{
    setNodeNumFromFractionalHeight(fractionalHeight, inletHeight);
}

void HPWH::Tank::setInlet2ByFraction(double fractionalHeight)
{
    setNodeNumFromFractionalHeight(fractionalHeight, inlet2Height);
}

void HPWH::Tank::checkForInversion()
{
    // cursory check for inverted temperature profile
    if (nodeTs_C[getNumNodes() - 1] < nodeTs_C[0])
    {
        send_warning("The top of the tank is cooler than the bottom.");
    }
}

double HPWH::Tank::calcSoCFraction(double tMains_C, double tMinUseful_C, double tMax_C) const
{
    // Note that volume is ignored in here since with even nodes it cancels out of the SoC
    // fractional equation
    if (tMains_C >= tMinUseful_C)
    {
        send_warning("tMains_C is greater than or equal tMinUseful_C.");
    }
    if (tMinUseful_C > tMax_C)
    {
        send_warning("tMinUseful_C is greater tMax_C.");
    }

    double chargeEquivalent = 0.;
    for (auto& T : nodeTs_C)
    {
        chargeEquivalent += getChargePerNode(tMains_C, tMinUseful_C, T);
    }
    double maxSoC = getNumNodes() * getChargePerNode(tMains_C, tMinUseful_C, tMax_C);
    return chargeEquivalent / maxSoC;
}

double HPWH::Tank::addHeatAboveNode(double qAdd_kJ, int nodeNum, const double maxHeatToT_C)
{
    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (nodeTs_C[i] != nodeTs_C[i + 1])
        {
            break;
        }
        else
        {
            numNodesToHeat++;
        }
    }

    while ((qAdd_kJ > 0.) && (nodeNum + numNodesToHeat - 1 < getNumNodes()))
    {
        // assume there is another node above the equal-temp nodes
        int targetTempNodeNum = nodeNum + numNodesToHeat;

        double heatToT_C;
        if (targetTempNodeNum > (getNumNodes() - 1))
        {
            // no nodes above the equal-temp nodes; target temperature is the maximum
            heatToT_C = maxHeatToT_C;
        }
        else
        {
            heatToT_C = nodeTs_C[targetTempNodeNum];
            if (heatToT_C > maxHeatToT_C)
            {
                // Ensure temperature does not exceed maximum
                heatToT_C = maxHeatToT_C;
            }
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        double qIncrement_kJ = numNodesToHeat * nodeCp_kJperC * (heatToT_C - nodeTs_C[nodeNum]);

        if (qIncrement_kJ > qAdd_kJ)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT_C = nodeTs_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                nodeTs_C[nodeNum + j] = heatToT_C;
            }
            qAdd_kJ = 0.;
        }
        else if (qIncrement_kJ > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                nodeTs_C[nodeNum + j] = heatToT_C;
            qAdd_kJ -= qIncrement_kJ;
        }
        numNodesToHeat++;
    }

    // return any unused heat
    return qAdd_kJ;
}

void HPWH::Tank::addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum)
{
    // find number of nodes at or above nodeNum with the same temperature
    int numNodesToHeat = 1;
    for (int i = nodeNum; i < getNumNodes() - 1; i++)
    {
        if (nodeTs_C[i] != nodeTs_C[i + 1])
        {
            break;
        }
        else
        {
            numNodesToHeat++;
        }
    }

    while ((qAdd_kJ > 0.) && (nodeNum + numNodesToHeat - 1 < getNumNodes()))
    {

        // assume there is another node above the equal-temp nodes
        int targetTempNodeNum = nodeNum + numNodesToHeat;

        double heatToT_C;
        if (targetTempNodeNum > (getNumNodes() - 1))
        {
            // no nodes above the equal-temp nodes; target temperature limited by the heat available
            heatToT_C = nodeTs_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
        }
        else
        {
            heatToT_C = nodeTs_C[targetTempNodeNum];
        }

        // heat needed to bring all equal-temp nodes up to heatToT_C
        double qIncrement_kJ = nodeCp_kJperC * numNodesToHeat * (heatToT_C - nodeTs_C[nodeNum]);

        if (qIncrement_kJ > qAdd_kJ)
        {
            // insufficient heat to reach heatToT_C; use all available heat
            heatToT_C = nodeTs_C[nodeNum] + qAdd_kJ / nodeCp_kJperC / numNodesToHeat;
            for (int j = 0; j < numNodesToHeat; ++j)
            {
                nodeTs_C[nodeNum + j] = heatToT_C;
            }
            qAdd_kJ = 0.;
        }
        else if (qIncrement_kJ > 0.)
        { // add qIncrement_kJ to raise all equal-temp-nodes to heatToT_C
            for (int j = 0; j < numNodesToHeat; ++j)
                nodeTs_C[nodeNum + j] = heatToT_C;
            qAdd_kJ -= qIncrement_kJ;
        }
        numNodesToHeat++;
    }
}

void HPWH::Tank::modifyHeatDistribution(std::vector<double>& heatDistribution_W, double setpointT_C)
{
    double totalHeat_W = 0.;
    for (auto& heatDist_W : heatDistribution_W)
        totalHeat_W += heatDist_W;

    if (totalHeat_W == 0.)
        return;

    for (auto& heatDist_W : heatDistribution_W)
        heatDist_W /= totalHeat_W;

    WeightedDistribution wdist(heatDistribution_W);
    double shrinkageT_C = findShrinkageT_C(wdist, getNumNodes());
    int lowestNode = findLowestNode(wdist, getNumNodes());

    std::vector<double> modHeatDistribution_W;
    calcThermalDist(modHeatDistribution_W, shrinkageT_C, lowestNode, nodeTs_C, setpointT_C);

    heatDistribution_W = modHeatDistribution_W;
    for (auto& heatDist_W : heatDistribution_W)
        heatDist_W *= totalHeat_W;
}
