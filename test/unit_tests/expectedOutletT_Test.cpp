/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// standard
#include <vector>

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

/*
 * checkOutletT tests
 */
TEST(ExpectedOutletTTest, checkOutletT)
{
    const double Pi = 4. * atan(1.);

    {
        // get preset model
        HPWH hpwh;
        const std::string sModelName = "AquaThermAire";
        EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model.";

        const double maxDrawVol_L = 1.;
        const double ambientT_C = 20.;
        const double externalT_C = 20.;
        const double inletT_C = 5.;

        //
        hpwh.setTankToTemperature(20.);
        hpwh.setInletT(inletT_C);

        double expectedOutletT_C;

        constexpr double testDuration_min = 60.;
        bool result = true;
        int i_min = 0;
        do
        {
            double t_min = static_cast<double>(i_min);

            double flowFac = sin(Pi * t_min / testDuration_min) - 0.5;
            flowFac += fabs(flowFac); // semi-sinusoidal flow profile
            double drawVol_L = flowFac * maxDrawVol_L;

            if (drawVol_L > 0.)
            {
                expectedOutletT_C = hpwh.getExpectedOutletT_C(inletT_C, drawVol_L);
            }

            double prevHeatContent_kJ = hpwh.getTankHeatContent_kJ();
            EXPECT_EQ(hpwh.runOneStep(drawVol_L, ambientT_C, externalT_C, HPWH::DR_ALLOW), 0);

            if (drawVol_L > 0.)
            {
                double outletT_C = hpwh.getOutletTemp();
                EXPECT_NEAR(expectedOutletT_C, outletT_C, 1.e-4);
            }

            ++i_min;
        } while (result && (i_min < testDuration_min));

        EXPECT_TRUE(result) << "Energy balance failed for model " << sModelName;
    }
}
