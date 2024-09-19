/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

#include <unity/units.hpp>
#include "unit-test.hh"

using namespace Units;

/// units-values full specializations
typedef TimeVal<Time::s> Time_s;
typedef TimeVal<Time::min> Time_min;
typedef TempVal<Temp::C> Temp_C;
typedef TempVal<Temp::F> Temp_F;
typedef TempVal<Temp::K> Temp_K;
typedef Temp_dVal<Temp_d::C> Temp_dC;
typedef Temp_dVal<Temp_d::F> Temp_dF;
typedef EnergyVal<Energy::kJ> Energy_kJ;
typedef PowerVal<Power::W> Power_W;
typedef PowerVal<Power::kW> Power_kW;
typedef LengthVal<Length::m> Length_m;
typedef LengthVal<Length::ft> Length_ft;
typedef AreaVal<Area::ft2> Area_ft2;
typedef AreaVal<Area::m2> Area_m2;
typedef VolumeVal<Volume::L> Volume_L;
typedef FlowRateVal<FlowRate::L_per_s> FlowRate_L_per_s;
typedef UAVal<UA::kJ_per_hC> UA_kJ_per_hC;

/// units-pair full specialization
typedef TimePair<Time::h, Time::min> Time_h_min;

/// units-vectors full specialization
typedef TimeVect<Time::s> TimeVect_s, TimeV_s;
typedef TimeVect<Time::min> TimeVect_min, TimeV_min;
typedef TempVect<Temp::C> TempVect_C, TempV_C;
typedef TempVect_d<Temp_d::C> TempVect_dC, TempV_dC;
typedef EnergyVect<Energy::kJ> EnergyVect_kJ;
typedef PowerVect<Power::kW> PowerVect_kW, PowerV_kW;

/*
 * unitsConversion tests
 */
TEST(UnitsConversionTest, conversions)
{
    /* time conversions */
    {
        // test min<->s conversion
        Time_s t_s(15.);
        Time_min t_min(0.25);

        EXPECT_EQ(t_s(), t_min(s));
        EXPECT_EQ(t_s(min), t_min());
        EXPECT_EQ(t_s(), t_min(s));
        EXPECT_NE(t_s(), 0.25);
        EXPECT_NE(t_min(), 15.);

        // test min<->h conversion
        t_min() = 45.;
        typedef TimeVal<Time::h> Time_h;
        Time_h t_h(t_min);
        EXPECT_EQ(t_h(), 0.75);

        // test TimeVect_min -> std::vector<double>
        TimeVect_min tV_min({10., 20., 30., 60., 120., 360., 12.});
        TimeVect_s tV_s(tV_min);
        EXPECT_EQ(tV_s(min), tV_min(min));

        std::vector<double>& v = tV_min();
        v[2] = 15.;
        EXPECT_EQ(tV_min[2](min), 15.);

        std::vector<double> t_hV = tV_min(h);
        EXPECT_EQ(t_hV[2], 0.25);
    }

    /* temperature conversions */
    {
        Temp_C T_C(0.);
        Temp_F T_F(T_C);
        EXPECT_EQ(T_F, 32.);

        // test F<->C conversion
        T_F() = 212.;
        T_C = T_F;
        EXPECT_EQ(T_F, T_C(F));
    }

    /* temperature-difference conversions */
    {
        Temp_dC T_dC(10.);
        Temp_dF T_dF(T_dC);
        EXPECT_EQ(T_dF, 18.);

        Temp_C T_C(20);
        T_C += T_dF(dC);
        EXPECT_EQ(T_C, 30);
    }

    /* energy conversions */
    {
        auto E_kJ = Energy_kJ(100.);
        EnergyVal<Energy::Btu> E_Btu(E_kJ);
        EXPECT_EQ(E_Btu(kJ), E_kJ());
        EXPECT_EQ(E_Btu, E_kJ(Btu));

        EnergyVect_kJ EV_kJ({100., 200., 300.}, Btu);
        EXPECT_EQ(EV_kJ[1](Btu), 200.);

        auto x = EV_kJ[1];
    }

    /* power conversion */
    {
        auto P_kW = Power_kW(60.);
        PowerVal<Power::Btu_per_h> P_Btu_per_h(P_kW);
        EXPECT_EQ(P_Btu_per_h, P_kW(Btu_per_h));
        EXPECT_NEAR_REL(P_Btu_per_h(kW), P_kW());
    }

    /* length conversion */
    {
        auto x_m = Length_m(60.);
        LengthVal<Length::ft> x_ft(x_m);
        EXPECT_NEAR_REL(x_ft(), x_m(ft));
    }

    /* area conversion */
    {
        auto a_m2 = Area_m2(60.);
        AreaVal<Area::ft2> a_ft2(a_m2);
        EXPECT_NEAR_REL(a_ft2(), a_m2(ft2));
    }

    /* volume conversion */
    {
        auto volume_L = Volume_L(60.);
        VolumeVal<Volume::gal> volume_gal(volume_L);
        EXPECT_NEAR_REL(volume_gal(), volume_L(gal));

        VolumeVal<Volume::m3> volume_m3(volume_L);
        EXPECT_NEAR_REL(volume_m3(), volume_L(m3));

        VolumeVal<Volume::ft3> volume_ft3(volume_L);
        EXPECT_NEAR_REL(volume_ft3(), volume_L(ft3));
    }

    /* UA conversion */
    {
        auto ua_kJ_per_hC = UA_kJ_per_hC(60.);
        UAVal<UA::Btu_per_hF> ua_Btu_per_hF(ua_kJ_per_hC);
        EXPECT_NEAR_REL(ua_Btu_per_hF(), ua_kJ_per_hC(Btu_per_hF));
    }
    {
        Temp_F temp_F = {0., C};
        temp_F += Temp_dC(100)(dF);
        EXPECT_NEAR_REL(temp_F(), 212.);
    }
    {
        double sigma_W_per_m2K4 = 5.670374419e-8;
        Area_ft2 area_ft2(100.);

        Temp_F T1_F(300., K);
        auto power1_W = Power_W(sigma_W_per_m2K4 * area_ft2(m2) * std::pow(T1_F(K), 4.));

        Temp_F T2_F(600., K);
        auto power2_W = Power_W(sigma_W_per_m2K4 * area_ft2(m2) * std::pow(T2_F(K), 4.));

        EXPECT_NEAR_REL(power2_W(), 16. * power1_W());
    }
    {
        auto length_ft = Length_ft(3.);
        double& x = length_ft();
        x += 1.; // ft
        EXPECT_NEAR_REL(length_ft(), 4.);
    }
    {
        std::vector<EnergyVal<Energy::Btu>> E_Btu_V(3);
        E_Btu_V[0] = 10.;
        E_Btu_V[1] = 20.;
        E_Btu_V[2] = 30.;

        EnergyVect_kJ E_V_kJ(E_Btu_V);
        EXPECT_NEAR_REL(E_V_kJ[1](Btu), 20.);
    }
    { // vector construction
        TempVect_dC vT_dC({1.8, 3.6, 5.4}, dF);
        EXPECT_NEAR_REL(vT_dC[1](dF), 3.6);

        TempVect_C vT_C({32., 98.6, 212.}, F);
        EXPECT_NEAR_REL(vT_C[1](F), 98.6);
    }
    { // vector sizing
        EnergyVect_kJ EV_kJ(2);
        EV_kJ[0] = {0.2, kJ};
        EV_kJ[1] = 80.;
        EXPECT_NEAR_REL(EV_kJ[1](), 80.);

        TempVect_C TV_C(2);
        TV_C[0] = {32., Units::F};
        TV_C[1] = 20.;
        EXPECT_NEAR_REL(TV_C[1](), 20.);
    }
}
