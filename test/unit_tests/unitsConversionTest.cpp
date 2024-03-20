/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

/*
 * unitsConversion tests
 */
TEST(UnitsConversionTest, convsersions)
{

    {
        Units::Time_s t_s(15.);
        Units::Time_min t_min(0.25);
        EXPECT_EQ(t_s, t_min);
        EXPECT_EQ(t_s(Units::Time::min), t_min);
        EXPECT_EQ(t_s, t_min(Units::Time::s));
        EXPECT_NE(t_s, 0.25);
        EXPECT_NE(t_min, 15.);
    }

    {
        Units::Temp_C T_C(0.);
        Units::TempVal<Units::Temp::F> T_F = T_C;
        EXPECT_EQ(T_F, 32.);

        T_F = 212.;
        T_C = T_F;
        EXPECT_EQ(T_C, 100.);

        T_C = 72.;
        Units::Temp_C Tp_C =
            T_C + Units::UnitsVal<Units::Temp, Units::Temp::F, Units::Mode::Diff>(18.);
        EXPECT_EQ(Tp_C, T_C + dF_TO_dC(18.));
    }
    /*
    std::cout << f_min << std::endl;
    std::cout << f_min(Units::Time::s) << std::endl;
    std::cout << f_min << std::endl;

    Units::Time_s f_s = f_min;
    std::cout << f_s << std::endl;

    auto x_s = Units::Time_min(10., Units::Time::s);
    auto xV_s = Units::TimeVect_min({1., 30., 12.}, Units::Time::s);

    Units::TimeVect_s fV_s(xV_s, Units::Time::min);
    Units::TimeVect_min fV_min = fV_s;

    for (auto y_s : fV_s)
    {
        std::cout << y_s << std::endl;
    }

    double x_m_s = 10.;
    double y_ft_s = Units::Length_m(x_m_s).to(Units::Length::ft);
    double y_m_min = Units::Time_s(x_m_s).inv(Units::Time::min);
    */
}
