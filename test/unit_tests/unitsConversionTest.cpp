/* Copyright (c) 2024 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

/*
 * unitsConversion tests
 */
TEST(UnitsConversionTest, conversions)
{
    /* time conversions */
    {
        // test min<->s conversion
        Units::Time_s t_s(15.);
        Units::Time_min t_min(0.25);

        EXPECT_EQ(t_s, t_min(Units::s));
        EXPECT_EQ(t_s(Units::min), t_min);
        EXPECT_EQ(t_s, t_min(Units::s));
        EXPECT_NE(t_s, 0.25);
        EXPECT_NE(t_min, 15.);

        // test min<->h conversion
        t_min = 45.;
        typedef Units::TimeVal<Units::Time::h> Time_h;
        Time_h t_h = t_min;
        EXPECT_EQ(t_h, 0.75);

        // test vector min<->s conversion
        Units::TimeVect_min tV_min({10., 20., 30., 60., 120., 360., 12.});
        Units::TimeVect_s tV_s = tV_min;
        EXPECT_EQ(tV_s[3], 3600.);
    }

    /* temperature conversions */
    {
        Units::Temp_C T_C(0.);
        Units::Temp_F T_F = T_C;
        EXPECT_EQ(T_F, 32.);

        // test F<->C conversion
        T_F = 212.;
        T_C = T_F;
        EXPECT_EQ(T_C, F_TO_C(T_F));

    }

    /* energy conversions */
    {
        Units::Energy_kJ E_kJ = 100.;
        Units::EnergyVal<Units::Energy::Btu> E_Btu = E_kJ;
        EXPECT_EQ(E_Btu, E_kJ(Units::Btu));
        EXPECT_EQ(E_Btu, KJ_TO_BTU(E_kJ));

        Units::EnergyVect_kJ EV_kJ({100., 200., 300.});
        std::vector<double> xV_kJ = EV_kJ;
        EXPECT_EQ(xV_kJ[1], 200.);
    }

    /* power conversion */
    {
        Units::Power_kW P_kW = 60.;
        Units::PowerVal<Units::Power::Btu_per_h> P_Btuperh = P_kW;
        EXPECT_EQ(P_Btuperh, KW_TO_BTUperH(P_kW));
        EXPECT_NEAR_REL(P_kW, BTUperH_TO_KW(P_Btuperh));
    }

    /* length conversion */
    {
        Units::Length_m x_m = 60.;
        Units::LengthVal<Units::Length::ft> x_ft = x_m;
        EXPECT_NEAR_REL(x_ft, M_TO_FT(x_m));
    }

    /* area conversion */
    {
        Units::Area_m2 a_m2 = 60.;
        Units::AreaVal<Units::Area::ft2> a_ft2 = a_m2;
        EXPECT_NEAR_REL(a_ft2, M2_TO_FT2(a_m2));
    }

    /* volume conversion */
    {
        Units::Volume_L volume_L = 60.;
        Units::VolumeVal<Units::Volume::gal> volume_gal = volume_L;
        EXPECT_NEAR_REL(volume_gal, L_TO_GAL(volume_L));

        Units::VolumeVal<Units::Volume::m3> volume_m3 = volume_L;
        EXPECT_NEAR_REL(volume_m3, L_TO_M3(volume_L));

        Units::VolumeVal<Units::Volume::ft3> volume_ft3 = volume_L;
        EXPECT_NEAR_REL(volume_ft3, L_TO_FT3(volume_L));
    }

    /* UA conversion */
    {
        Units::UA_kJ_per_hC ua_kJperhC = 60.;
        Units::UAVal<Units::UA::Btu_per_hF> ua_BtuperhF = ua_kJperhC;
        EXPECT_NEAR_REL(ua_BtuperhF, KJperHC_TO_BTUperHF(ua_kJperhC));
    }
}