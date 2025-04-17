#ifndef HPWHTANK_hh
#define HPWHTANK_hh

#include "HPWH.hh"

class HPWH::Tank : public Sender
{
  public:
    static const inline double ASPECTRATIO = 4.75;
    /// tank height / radius aspect ratio, derived from the median value of 88
    /// insulated storage tanks currently available on the market.

    friend class HPWH;

    HPWH* hpwh;

    Tank(HPWH* hpwh_in = NULL,
         const std::shared_ptr<Courier::Courier> courier = std::make_shared<DefaultCourier>(),
         const std::string& name_in = "tank")
        : Sender("HeatSource", name_in, courier), hpwh(hpwh_in)
    {
    }

    /**< constructor assigns a pointer to the hpwh that owns this heat source  */
    Tank(const Tank& tank);            /// copy constructor
    Tank& operator=(const Tank& tank); /// assignment operator
    /**< the copy constructor and assignment operator basically just checks if there
        are backup/companion pointers - these can't be copied */

    MetadataDescription metadataDescription;
    ProductInformation productInformation;

    void from(hpwh_data_model::rstank::RSTANK& rstank);
    void to(hpwh_data_model::rstank::RSTANK& rstank) const;

    void setAllDefaults();

    void calcSizeConstants();

    bool isVolumeFixed() const { return volumeFixed; }

    /// set the volume
    void setVolume_L(double volume_L_in, bool forceChange = false);

    /// get the volume
    double getVolume_L() const;

    /// set the tank size and adjust the UA such that U is unchanged
    void setVolumeAndAdjustUA(double volume_L_in, bool forceChange = false);

    double getSurfaceArea_m2() const;

    /// get the surface area (based off of real storage tanks)
    static double getSurfaceArea_m2(double volume_L);

    double getRadius_m() const;

    /// get the tank surface radius based off of real storage tanks
    static double getRadius_m(double volume_L);

    /// set the UA
    void setUA_kJperHrC(double UA_kJperHrC);

    /// get the UA
    double getUA_kJperHrC() const;

    double addHeatAboveNode(double qAdd_kJ, int nodeNum, const double maxHeatToT_C);

    void addExtraHeatAboveNode(double qAdd_kJ, const int nodeNum);

    void modifyHeatDistribution(std::vector<double>& heatDistribution_W, double setpointT_C);

    /// volume (L)
    double volume_L;

    /// node temperatures - 0 is the bottom node
    std::vector<double> nodeTs_C;

    /// future node temperature of each node - 0 is the bottom
    std::vector<double> nextNodeTs_C;

    /// heat lost to standby
    double standbyLosses_kJ;

    ///  whether the bottom fraction (defined by mixBelowFraction)
    /// of the tank should mix during draws
    bool mixesOnDraw;

    /// whether to mix the tank below this fraction on draws iff mixesOnDraw
    double mixBelowFractionOnDraw;

    /// iff true will model temperature inversion mixing
    bool doInversionMixing;

    /// iff true will model conduction between internal nodes
    bool doConduction;

    /// whether size can be changed
    bool volumeFixed;

    /// volume (L) of a single node
    double nodeVolume_L;

    /// heat capacity (kJ/degC) of the fluid (water, except for heat-exchange models) in a single
    /// node
    double nodeCp_kJperC;

    /// height (m) of one node
    double nodeHeight_m;

    /// fraction of the UA on the top and bottom of the tank, assuming it's a cylinder
    double fracAreaTop;

    /// fraction of the UA on the sides of the tank, assuming it's a cylinder
    double fracAreaSide;

    /// UA
    double UA_kJperHrC;

    /// number of node at which the inlet water enters. must be between 0 and numNodes-1
    int inletHeight;

    /// number of node at which the 2nd inlet water enters, must be between 0 and numNodes-1
    int inlet2Height;

    /// get the water inlet height node number
    int getInletHeight(int whichInlet) const;

    /// resize the nodeTs_C and nextNodeTs_C
    void setNumNodes(const std::size_t num_nodes);

    /// get number of nodes
    int getNumNodes() const { return static_cast<int>(nodeTs_C.size()); }

    /// get index of the top node
    int getIndexTopNode() const { return getNumNodes() - 1; }

    void setNodeTs_C(const std::vector<double>& nodeTs_C_in);

    void setNodeT_C(double T_C) { setNodeTs_C({T_C}); }

    void getNodeTs_C(std::vector<double>& tankTemps) { tankTemps = nodeTs_C; }

    double getAverageNodeT_C() const;

    double getNodeT_C(int nodeNum) const;

    double getAverageNodeT_C(const std::vector<double>& dist) const;

    double getAverageNodeT_C(const WeightedDistribution& wdist) const;

    double getAverageNodeT_C(const Distribution& dist) const;

    double getHeatContent_kJ() const;

    double getNthSimTcouple(int iTCouple, int nTCouple) const;

    void setDoInversionMixing(bool doInversionMixing_in);

    void setDoConduction(bool doConduction_in);

    /// False: water is drawn from the tank itself; True: tank provides heat exchange only
    bool hasHeatExchanger;

    /// Coefficient (0-1) of effectiveness for heat exchange between tank and water line (used by
    /// heat-exchange models only).
    double heatExchangerEffectiveness;

    /// Coefficient (0-1) of effectiveness for heat exchange between a single tank node and water
    /// line (derived from heatExchangerEffectiveness).
    double nodeHeatExchangerEffectiveness;

    /// temperature of the outlet water - taken from top of tank, 0 if no flow  */
    double outletT_C;

    double getOutletT_C() const { return outletT_C; }

    void setOutletT_C(double outletT_C_in) { outletT_C = outletT_C_in; }

    void mixNodes(int mixBottomNode, int mixBelowNode, double mixFactor);

    void mixInversions();
    void checkForInversion();

    void updateNodes(double drawVolume_L,
                     double inletT_C,
                     double tankAmbientT_C,
                     double inletVol2_L,
                     double inletT2_C);

    void setNodeNumFromFractionalHeight(double fractionalHeight, int& inletNum);
    void setInletByFraction(double fractionalHeight);
    void setInlet2ByFraction(double fractionalHeight);

    double getStandbyLosses_kJ();

    double calcSoCFraction(double tMains_C, double tMinUseful_C, double tMax_C) const;

    /// UA of the fittings
    double fittingsUA_kJperHrC;

    double getFittingsUA_kJperHrC() { return fittingsUA_kJperHrC; }

    void setFittingsUA_kJperHrC(double fittingsUA_kJperHrC_in)
    {
        fittingsUA_kJperHrC = fittingsUA_kJperHrC_in;
    }

}; // end of HPWH::Tank class

#endif
