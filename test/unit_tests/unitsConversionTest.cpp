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
        Units::Time_s t_s(15.);
        Units::Time_min t_min(0.25);
        EXPECT_EQ(t_s, t_min);
        EXPECT_EQ(t_s(Units::Time::min), t_min);
        EXPECT_EQ(t_s, t_min(Units::Time::s));
        EXPECT_NE(t_s, 0.25);
        EXPECT_NE(t_min, 15.);

        t_min = 45.;
        typedef Units::UnitsVal<Units::Time, Units::Time::h> Time_h;
        Time_h t_h = t_min;
        EXPECT_EQ(t_h, 0.75);

        Units::TimeVect_min tV_min({10., 20., 30., 60., 120., 360., 12.});
        Units::TimeVect_s tV_s = tV_min;
        // EXPECT_EQ(tV_s, tV_min);
        EXPECT_EQ(tV_s[3], 1.);
    }

    /* temperature conversions */
    {
        Units::Temp_C T_C(0.);
        Units::TempVal<Units::Temp::F> T_F = T_C;
        EXPECT_EQ(T_F, 32.);

        T_F = 212.;
        T_C = T_F;
        EXPECT_EQ(T_C, F_TO_C(T_F));

        T_C = 72.;
        typedef Units::UnitsVal<Units::Temp, Units::Temp::F, Units::Mode::Diff> TempDiff_F;
        EXPECT_EQ(T_C + TempDiff_F(18.), T_C + dF_TO_dC(18.));
        EXPECT_EQ(T_C + TempDiff_F(10., Units::Temp::C), T_C + TempDiff_F(18.));
    }
}
