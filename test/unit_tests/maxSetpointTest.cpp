/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

const double expectedRE_maxT_C = 100.;

/*
 * resistanceTank tests
 */
TEST(MaxSetpointTest, resistanceTank)
{
    HPWH hpwh;
    EXPECT_NO_THROW(hpwh.initResistanceTank()) << "Could not initialize resistance tank.";

    double num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(99., num, why));   // Can go to near boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(100., num, why));  // Can go to boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(10., num, why));   // Can go low, albiet dumb
    EXPECT_EQ(expectedRE_maxT_C, num);

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpoint(101.)); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpoint(99.));
}

/*
 * scalableCompressor tests
 */
TEST(MaxSetpointTest, scalableCompressor)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    double num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(99., num, why));   // Can go to near boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(60., num, why));   // Can go to normal
    EXPECT_TRUE(hpwh.isNewSetpointPossible(100, num, why));   // Can go to programed max

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpoint(101.)); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpoint(50.));
}

/*
 * NyleC90A_SP tests
 */
TEST(MaxSetpointTest, NyleC90A_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "NyleC90A_SP";
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    double num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
    EXPECT_FALSE(hpwh.isNewSetpointPossible(99., num, why));  // Can't go to near boiling
    EXPECT_EQ(HPWH::MAXOUTLET_R134A, num);                  // Assert we're getting the right number
    EXPECT_TRUE(hpwh.isNewSetpointPossible(60., num, why)); // Can go to normal
    EXPECT_TRUE(
        hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R134A, num, why)); // Can go to programed max

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpoint(101.)); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpoint(50.));
}

/*
 * ColmacCxV_5_SP tests
 */
TEST(MaxSetpointTest, ColmacCxV_5_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_SP";
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    double num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
    EXPECT_FALSE(hpwh.isNewSetpointPossible(99., num, why));  // Can't go to near boiling
    EXPECT_TRUE(HPWH::MAXOUTLET_R410A == num);              // Assert we're getting the right number
    EXPECT_TRUE(hpwh.isNewSetpointPossible(50., num, why)); // Can go to normal
    EXPECT_TRUE(
        hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R410A, num, why)); // Can go to programed max

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpoint(101.)); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpoint(50.));
}

/*
 * QAHV_N136TAU_HPB_SP tests
 */
TEST(MaxSetpointTest, QAHV_N136TAU_HPB_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    double num;
    std::string why;

    const double maxQAHVSetpoint = F_TO_C(176.1);
    const double qAHVHotSideTemepratureOffset = dF_TO_dC(15.);

    // isNewSetpointPossible should be fine, we aren't changing the setpoint of the Sanden.
    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
    EXPECT_FALSE(hpwh.isNewSetpointPossible(99., num, why));  // Can't go to near boiling

    EXPECT_TRUE(hpwh.isNewSetpointPossible(60., num, why)); // Can go to normal

    EXPECT_FALSE(hpwh.isNewSetpointPossible(maxQAHVSetpoint, num, why));
    EXPECT_TRUE(
        hpwh.isNewSetpointPossible(maxQAHVSetpoint - qAHVHotSideTemepratureOffset, num, why));

    // Check this carries over into setting the setpoint.
    EXPECT_ANY_THROW(hpwh.setSetpoint(101));
    EXPECT_ANY_THROW(hpwh.setSetpoint(maxQAHVSetpoint));
    EXPECT_NO_THROW(hpwh.setSetpoint(maxQAHVSetpoint - qAHVHotSideTemepratureOffset));
}

/*
 * AOSmithCAHP120 tests
 */
TEST(MaxSetpointTest, AOSmithCAHP120)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AOSmithCAHP120"; // Hybrid unit with a compressor with R134A
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    double num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(99., num, why));   // Can go to near boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(100., num, why));  // Can go to boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible(10., num, why));   // Can go low, albiet dumb
    EXPECT_EQ(expectedRE_maxT_C, num);                        // Max is boiling

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpoint(101.)); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpoint(99.));   // Can go lower than boiling though
}

/*
 * StorageTank tests
 */
TEST(MaxSetpointTest, StorageTank)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "StorageTank"; // Hybrid unit with a compressor with R134A
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;
    ;

    double num;
    std::string why;

    // Storage tanks have free reign!
    EXPECT_TRUE(hpwh.isNewSetpointPossible(101., num, why)); // Can go above boiling!
    EXPECT_TRUE(hpwh.isNewSetpointPossible(99., num, why));  // Can go to near boiling!
    EXPECT_TRUE(hpwh.isNewSetpointPossible(10., num, why));  // Can go low, albiet dumb

    // Check this carries over into setting the setpoint
    EXPECT_NO_THROW(hpwh.setSetpoint(101.)); // Can go above boiling
    EXPECT_NO_THROW(hpwh.setSetpoint(99.));  // Can go lower than boiling though
}

/*
 * Sanden80 tests
 */
TEST(MaxSetpointTest, Sanden80)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Sanden80"; // Fixed setpoint model
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    double num, num1;
    std::string why;

    // Storage tanks have free reign!
    EXPECT_FALSE(hpwh.isNewSetpointPossible(101., num, why)); // Can't go above boiling!
    EXPECT_FALSE(hpwh.isNewSetpointPossible(99., num, why));  // Can't go to near boiling!
    EXPECT_FALSE(hpwh.isNewSetpointPossible(60., num, why));  // Can't go to normalish
    EXPECT_FALSE(hpwh.isNewSetpointPossible(10., num, why));  // Can't go low, albiet dumb

    EXPECT_EQ(num, hpwh.getSetpoint()); // Make sure it thinks the max is the setpoint
    EXPECT_TRUE(hpwh.isNewSetpointPossible(
        num, num1, why)); // Check that the setpoint can be set to the setpoint.

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpoint(101.)); // Can't go above boiling
    EXPECT_ANY_THROW(hpwh.setSetpoint(99.));  //
    EXPECT_ANY_THROW(hpwh.setSetpoint(60.));  // Can't go to normalish
    EXPECT_ANY_THROW(hpwh.setSetpoint(10.));  // Can't go low, albiet dumb
    EXPECT_ANY_THROW(hpwh.setSetpoint(10.));  // Can't go low, albiet dumb
}

/*
 * resample tests
 */
TEST(UtilityTest, resample)
{
    // test extensive resampling
    std::vector<double> values(10);
    std::vector<double> sampleValues {20., 40., 60., 40., 20.};
    EXPECT_TRUE(resampleExtensive(values, sampleValues));

    // Check some expected values.
    EXPECT_NEAR_REL(values[1], 10.); //
    EXPECT_NEAR_REL(values[5], 30.); //

    // test intensive resampling
    EXPECT_TRUE(resampleIntensive(values, sampleValues));

    // Check some expected values.
    EXPECT_NEAR_REL(values[1], 20.); //
    EXPECT_NEAR_REL(values[5], 60.); //
}

/*
 * setTemperatures tests
 */
TEST(UtilityTest, setTemperatures)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Rheem2020Prem50"; // 12-node model
    EXPECT_NO_THROW(hpwh.initPreset(sModelName)) << "Could not initialize model " << sModelName;

    // test 1
    {
        std::vector<double> setT_C {10., 60.};
        hpwh.setTankLayerTemperatures(setT_C);

        std::vector<double> newT_C;
        hpwh.getTankTemps(newT_C);

        // Check some expected values.
        EXPECT_NEAR_REL(newT_C[0], 10.);  //
        EXPECT_NEAR_REL(newT_C[11], 60.); //
    }

    // test 2
    {
        std::vector<double> setT_C = {10., 20., 30., 40., 50., 60.};
        hpwh.setTankLayerTemperatures(setT_C);

        std::vector<double> newT_C;
        hpwh.getTankTemps(newT_C);

        // Check some expected values.
        EXPECT_NEAR_REL(newT_C[0], 10.);  //
        EXPECT_NEAR_REL(newT_C[5], 30.);  //
        EXPECT_NEAR_REL(newT_C[6], 40.);  //
        EXPECT_NEAR_REL(newT_C[11], 60.); //
    }

    // test 3
    {
        std::vector<double> setT_C = {
            10., 15., 20., 25., 30., 35., 40., 45., 50., 55., 60., 65., 70., 75., 80., 85., 90.};
        hpwh.setTankLayerTemperatures(setT_C);

        std::vector<double> newT_C;
        hpwh.getTankTemps(newT_C);

        // Check some expected values.
        EXPECT_NEAR_REL(newT_C[2], 25.2941); //
        EXPECT_NEAR_REL(newT_C[8], 67.6471); //
    }

    // test 4
    {
        const std::size_t nSet = 24;
        std::vector<double> setT_C(nSet);
        double initialT_C = 20., finalT_C = 66.;
        for (std::size_t i = 0; i < nSet; ++i)
            setT_C[i] = initialT_C + (finalT_C - initialT_C) * i / static_cast<double>(nSet - 1);
        hpwh.setTankLayerTemperatures(setT_C);

        std::vector<double> newT_C;
        hpwh.getTankTemps(newT_C);

        // Check some expected values.
        EXPECT_NEAR_REL(newT_C[4], 37.);  //
        EXPECT_NEAR_REL(newT_C[10], 61.); //
    }

    // test 5
    {
        const std::size_t nSet = 12;
        std::vector<double> setT_C(nSet);
        double initialT_C = 20., finalT_C = 64.;
        for (std::size_t i = 0; i < nSet; ++i)
            setT_C[i] = initialT_C + (finalT_C - initialT_C) * i / static_cast<double>(nSet - 1);
        hpwh.setTankLayerTemperatures(setT_C);

        std::vector<double> newT_C;
        hpwh.getTankTemps(newT_C);

        // Check some expected values.
        EXPECT_NEAR_REL(newT_C[3], 32.);  //
        EXPECT_NEAR_REL(newT_C[11], 64.); //
    }
}
