
#include "HPWH.hh"
#include "HPWHTank.hh"

HPWH::Tank::Tank(const HPWH::Tank& tank)
{

    volume_L = tank.volume_L;
    UA_kJperHrC = tank.UA_kJperHrC;
    nodeTs_C = tank.nodeTs_C;
    nextNodeTs_C = tank.nextNodeTs_C;
    mixesOnDraw = tank.mixesOnDraw;
    mixBelowFractionOnDraw = tank.mixBelowFractionOnDraw;
    doInversionMixing = tank.doInversionMixing;

}

HPWH::Tank& HPWH::Tank::operator=(const HPWH::Tank& tank_in)
{
    if (this == &tank_in)
    {
        return *this;
    }

    return *this;
}

void HPWH::Tank::setAllDefaults()
{
    nodeTs_C.clear();
    nextNodeTs_C.clear();
    sizeFixed = true;
    inletHeight = 0;
    inlet2Height = 0;
    mixBelowFractionOnDraw = 1. / 3.;
    doInversionMixing = true;
    doConduction = true;
    hasHeatExchanger = false;
    heatExchangerEffectiveness = 0.9;
}

void HPWH::Tank::calcSizeConstants()
{
    const double tankRad_m = getTankRadius(UNITS_M);
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

double HPWH::Tank::getVolume_L() const
{
    return volume_L;
}

int HPWH::Tank::setVolume_L(double volume, bool forceChange /*=false*/)
{
    if (sizeFixed && !forceChange)
    {
        send_warning("Can not change the tank size for your currently selected model.");
        return HPWH_ABORT;
    }
    if (volume <= 0)
    {
        send_error("You have attempted to set the tank volume outside of bounds.");
        return HPWH_ABORT;
    }
    else
    {
        volume_L = volume;
    }
    return 0;
}

/// set the UA
void HPWH::Tank::setUA_kJperHrC(double UA_kJperHrC_in)
{
    UA_kJperHrC = UA_kJperHrC_in;
}

/// get the UA
double HPWH::Tank::getUA_kJperHrC() const
{
    return UA_kJperHrC;
}

int HPWH::Tank::setVolumeAndAdjustUA(double volume_L_in, bool forceChange)
{
    double oldA = getSurfaceArea_m2();
    setVolume_L(volume_L_in, forceChange);
    setUA_kJperHrC(UA_kJperHrC / oldA * getSurfaceArea_m2());
    return 0;
}

/*static*/ double
HPWH::Tank::getSurfaceArea_m2(double vol)
{
    double radius = getRadius_m(vol);
    double SA_m2 = 2. * 3.14159 * pow(radius, 2) * (ASPECTRATIO + 1.);
    return SA_m2;
}

/*static*/ double
HPWH::Tank::getRadius_m(double volume_L)
{
    double value_m = -1.;
    if (volume_L >= 0.)
    {
        value_m = 0.1 * pow(volume_L / 3.14159 / ASPECTRATIO, 1. / 3.);
    }
    return value_m;
}

int HPWH::Tank::setNodeTs_C(std::vector<double> nodeTs_C_in)
{
    std::size_t numSetNodes = nodeTs_C_in.size();
    if (numSetNodes == 0)
    {
        send_warning("No temperatures provided.");
        return HPWH_ABORT;
    }

    // set node temps
    if (!resampleIntensive(nodeTs_C, nodeTs_C_in))
        return HPWH_ABORT;

    return 0;
}

int HPWH::Tank::getInletHeight(int whichInlet) const
{
    if (whichInlet == 1)
    {
        return inletHeight;
    }
    else if (whichInlet == 2)
    {
        return inlet2Height;
    }

    send_warning("Invalid inlet chosen in getInletHeight.");
    return HPWH_ABORT;
}

int HPWH::Tank::setDoInversionMixing(bool doInvMix)
{
    doInversionMixing = doInvMix;
    return 0;
}

int HPWH::Tank::setDoConduction(bool doCondu)
{
    doConduction = doCondu;
    return 0;
}

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
                           double inletT2_C,
                              double stepTime_min)
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
            if (drawVolume_L > volume_L)
            {
                for (int i = 0; i < getNumNodes(); i++)
                {
                    outletT_C += nodeTs_C[i];
                    nodeTs_C[i] =
                        (inletT_C * (drawVolume_L - inletVol2_L) + inletT2_C * inletVol2_L) /
                        drawVolume_L;
                }
                outletT_C = (outletT_C / getNumNodes() * volume_L +
                                nodeTs_C[0] * (drawVolume_L - volume_L)) /
                               drawVolume_L * remainingDrawVolume_N;

                remainingDrawVolume_N = 0.;
            }

            double totalExpelledHeat_kJ = 0.;
            while (remainingDrawVolume_N > 0.)
            {

                // draw no more than one node at a time
                double incrementalDrawVolume_N =
                    remainingDrawVolume_N > 1. ? 1. : remainingDrawVolume_N;

                double outputHeat_kJ = nodeCp_kJperC * incrementalDrawVolume_N * nodeTs_C.back();
                totalExpelledHeat_kJ += outputHeat_kJ;
                nodeTs_C.back() -= outputHeat_kJ / nodeCp_kJperC;

                for (int i = getNumNodes() - 1; i >= 0; --i)
                {
                    // combine all inlet contributions at this node
                    double inletFraction = 0.;
                    if (i == highInletNodeIndex)
                    {
                        inletFraction += highInletFraction;
                        nodeTs_C[i] +=
                            incrementalDrawVolume_N * highInletFraction * highInletT_C;
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
        if (mixesOnDraw && drawVolume_L > 0.)
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
            double standbyLosses_kJ =
                standbyLossRate_kJperHrC * hpwh->hoursPerStep * (nodeTs_C[i] - tankAmbientT_C);
            standbyLossesSides_kJ += standbyLosses_kJ;

            nextNodeTs_C[i] -= standbyLosses_kJ / nodeCp_kJperC;
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
            int HPWH::Tank::setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum)
            {
                if (fractionalHeight > 1. || fractionalHeight < 0.)
                {
                    send_warning("Out of bounds fraction for setInletByFraction.");
                    return HPWH_ABORT;
                }

                int node = (int)std::floor(getNumNodes() * fractionalHeight);
                inletNum = (node == getNumNodes()) ? getIndexTopNode() : node;

                return 0;
            }

            int HPWH::Tank::setInletByFraction(double fractionalHeight)
            {
                return setNodeNumFromFractionalHeight(fractionalHeight, inletHeight);
            }
            int HPWH::Tank::setInlet2ByFraction(double fractionalHeight)
            {
                return setNodeNumFromFractionalHeight(fractionalHeight, inlet2Height);
            }
                tau * (nodeTs_C[i + 1] - 2. * nodeTs_C[i] + nodeTs_C[i - 1]);
        }
    }

    // Update nodeTs_C
    nodeTs_C = nextNodeTs_C;

    standbyLosses_kJ += standbyLossesBottom_kJ + standbyLossesTop_kJ + standbyLossesSides_kJ;

     mixInversions();

} // end updateNodes

int HPWH::Tank::setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum)
{
    if (fractionalHeight > 1. || fractionalHeight < 0.)
    {
        send_warning("Out of bounds fraction for setInletByFraction.");
        return HPWH_ABORT;
    }

    int node = (int)std::floor(getNumNodes() * fractionalHeight);
    inletNum = (node == getNumNodes()) ? getIndexTopNode() : node;

    return 0;
}

int HPWH::Tank::setInletByFraction(double fractionalHeight)
{
    return setNodeNumFromFractionalHeight(fractionalHeight, inletHeight);
}
int HPWH::Tank::setInlet2ByFraction(double fractionalHeight)
{
    return setNodeNumFromFractionalHeight(fractionalHeight, inlet2Height);
}
