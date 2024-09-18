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

    HPWH::Temp_t num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({99., Units::C}, num, why));   // Can go to near boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({100., Units::C}, num, why));  // Can go to boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({10., Units::C}, num, why));   // Can go low, albiet dumb
    EXPECT_EQ(expectedRE_maxT_C, num());

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpointT({101., Units::C})); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpointT({99., Units::C}));
}

/*
 * scalableCompressor tests
 */
TEST(MaxSetpointTest, scalableCompressor)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "TamScalable_SP";
    hpwh.initPreset(sModelName);

    HPWH::Temp_t num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({99., Units::C}, num, why));   // Can go to near boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({60., Units::C}, num, why));   // Can go to normal
    EXPECT_TRUE(hpwh.isNewSetpointPossible({100, Units::C}, num, why));   // Can go to programed max

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpointT({101., Units::C})); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpointT({50., Units::C}));
}

/*
 * NyleC90A_SP tests
 */
TEST(MaxSetpointTest, NyleC90A_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "NyleC90A_SP";
    hpwh.initPreset(sModelName);

    HPWH::Temp_t num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling
    EXPECT_FALSE(hpwh.isNewSetpointPossible({99., Units::C}, num, why)); // Can't go to near boiling
    EXPECT_EQ(HPWH::MAXOUTLET_R134A(), num); // Assert we're getting the right number
    EXPECT_TRUE(hpwh.isNewSetpointPossible({60., Units::C}, num, why)); // Can go to normal
    EXPECT_TRUE(
        hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R134A(), num, why)); // Can go to programed max

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpointT({101., Units::C})); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpointT({50., Units::C}));
}

/*
 * ColmacCxV_5_SP tests
 */
TEST(MaxSetpointTest, ColmacCxV_5_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_SP";
    hpwh.initPreset(sModelName);

    HPWH::Temp_t num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling
    EXPECT_FALSE(hpwh.isNewSetpointPossible({99., Units::C}, num, why)); // Can't go to near boiling
    EXPECT_TRUE(HPWH::MAXOUTLET_R410A() == num); // Assert we're getting the right number
    EXPECT_TRUE(hpwh.isNewSetpointPossible({50., Units::C}, num, why)); // Can go to normal
    EXPECT_TRUE(
        hpwh.isNewSetpointPossible(HPWH::MAXOUTLET_R410A(), num, why)); // Can go to programed max

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpointT({101., Units::C})); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpointT({50., Units::C}));
}

/*
 * QAHV_N136TAU_HPB_SP tests
 */
TEST(MaxSetpointTest, QAHV_N136TAU_HPB_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    hpwh.initPreset(sModelName);

    HPWH::Temp_t num;
    std::string why;

    const HPWH::Temp_t maxQAHVSetpoint = {176.1, Units::F};
    const HPWH::Temp_d_t qAHVHotSideTemepratureOffset = {15., Units::dF};

    // isNewSetpointPossible should be fine, we aren't changing the setpoint of the Sanden.
    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling
    EXPECT_FALSE(hpwh.isNewSetpointPossible({99., Units::C}, num, why)); // Can't go to near boiling

    EXPECT_TRUE(hpwh.isNewSetpointPossible({60., Units::C}, num, why)); // Can go to normal

    EXPECT_FALSE(hpwh.isNewSetpointPossible(maxQAHVSetpoint, num, why));
    EXPECT_TRUE(
        hpwh.isNewSetpointPossible(maxQAHVSetpoint - qAHVHotSideTemepratureOffset, num, why));

    // Check this carries over into setting the setpoint.
    EXPECT_ANY_THROW(hpwh.setSetpointT({101, Units::C}));
    EXPECT_ANY_THROW(hpwh.setSetpointT(maxQAHVSetpoint));
    EXPECT_NO_THROW(hpwh.setSetpointT(
        {maxQAHVSetpoint(Units::C) - qAHVHotSideTemepratureOffset(Units::dC), Units::C}));
}

/*
 * AOSmithCAHP120 tests
 */
TEST(MaxSetpointTest, AOSmithCAHP120)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "AOSmithCAHP120"; // Hybrid unit with a compressor with R134A
    hpwh.initPreset(sModelName);

    HPWH::Temp_t num;
    std::string why;

    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({99., Units::C}, num, why));   // Can go to near boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({100., Units::C}, num, why));  // Can go to boiling
    EXPECT_TRUE(hpwh.isNewSetpointPossible({10., Units::C}, num, why));   // Can go low, albiet dumb
    EXPECT_EQ(expectedRE_maxT_C, num());                                  // Max is boiling

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpointT({101., Units::C})); // Can't go above boiling
    EXPECT_NO_THROW(hpwh.setSetpointT({99., Units::C}));   // Can go lower than boiling though
}

/*
 * StorageTank tests
 */
TEST(MaxSetpointTest, StorageTank)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "StorageTank"; // Hybrid unit with a compressor with R134A
    hpwh.initPreset(sModelName);
    ;

    HPWH::Temp_t num;
    std::string why;

    // Storage tanks have free reign!
    EXPECT_TRUE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can go above boiling!
    EXPECT_TRUE(hpwh.isNewSetpointPossible({99., Units::C}, num, why));  // Can go to near boiling!
    EXPECT_TRUE(hpwh.isNewSetpointPossible({10., Units::C}, num, why));  // Can go low, albiet dumb

    // Check this carries over into setting the setpoint
    EXPECT_NO_THROW(hpwh.setSetpointT({101., Units::C})); // Can go above boiling
    EXPECT_NO_THROW(hpwh.setSetpointT({99., Units::C}));  // Can go lower than boiling though
}

/*
 * Sanden80 tests
 */
TEST(MaxSetpointTest, Sanden80)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "Sanden80"; // Fixed setpoint model
    hpwh.initPreset(sModelName);

    HPWH::Temp_t num, num1;
    std::string why;

    // Storage tanks have free reign!
    EXPECT_FALSE(hpwh.isNewSetpointPossible({101., Units::C}, num, why)); // Can't go above boiling!
    EXPECT_FALSE(
        hpwh.isNewSetpointPossible({99., Units::C}, num, why)); // Can't go to near boiling!
    EXPECT_FALSE(hpwh.isNewSetpointPossible({60., Units::C}, num, why)); // Can't go to normalish
    EXPECT_FALSE(
        hpwh.isNewSetpointPossible({10., Units::C}, num, why)); // Can't go low, albiet dumb

    EXPECT_EQ(num, hpwh.getSetpointT()); // Make sure it thinks the max is the setpoint
    EXPECT_TRUE(hpwh.isNewSetpointPossible(
        num, num1, why)); // Check that the setpoint can be set to the setpoint.

    // Check this carries over into setting the setpoint
    EXPECT_ANY_THROW(hpwh.setSetpointT({101., Units::C})); // Can't go above boiling
    EXPECT_ANY_THROW(hpwh.setSetpointT({99., Units::C}));  //
    EXPECT_ANY_THROW(hpwh.setSetpointT({60., Units::C}));  // Can't go to normalish
    EXPECT_ANY_THROW(hpwh.setSetpointT({10., Units::C}));  // Can't go low, albiet dumb
    EXPECT_ANY_THROW(hpwh.setSetpointT({10., Units::C}));  // Can't go low, albiet dumb
}

/*
 * resample tests
 */
TEST(UtilityTest, resample)
{
    // test extensive resampling
    std::vector<double> sampleValues = {20., 40., 60., 40., 20.};
    std::vector<double> values(10);
    resampleExtensive(values, sampleValues);

    // Check some expected values.
    EXPECT_NEAR_REL(values[1], 10.); //
    EXPECT_NEAR_REL(values[5], 30.); //

    // test intensive resampling
    resampleIntensive(values, sampleValues);

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
    hpwh.initPreset(sModelName);

    // test 1
    {
        HPWH::TempVect_t setTs = {{10., 60.}, Units::C};
        hpwh.setTankTs(setTs);

        HPWH::TempVect_t newT;
        hpwh.getTankTs(newT);

        // Check some expected values.
        EXPECT_NEAR_REL(newT[0](Units::C), 10.);  //
        EXPECT_NEAR_REL(newT[11](Units::C), 60.); //
    }

    // test 2
    {
        HPWH::TempVect_t setTs = {{10., 20., 30., 40., 50., 60.}, Units::C};
        hpwh.setTankTs(setTs);

        HPWH::TempVect_t newTs;
        hpwh.getTankTs(newTs);

        // Check some expected values.
        EXPECT_NEAR_REL(newTs[0](Units::C), 10.);  //
        EXPECT_NEAR_REL(newTs[5](Units::C), 30.);  //
        EXPECT_NEAR_REL(newTs[6](Units::C), 40.);  //
        EXPECT_NEAR_REL(newTs[11](Units::C), 60.); //
    }

    // test 3
    {
        HPWH::TempVect_t setTs = {
            {10., 15., 20., 25., 30., 35., 40., 45., 50., 55., 60., 65., 70., 75., 80., 85., 90.},
            Units::C};
        hpwh.setTankTs(setTs);

        HPWH::TempVect_t newTs;
        hpwh.getTankTs(newTs);

        // Check some expected values.
        EXPECT_NEAR_REL(newTs[2](Units::C), 25.2941); //
        EXPECT_NEAR_REL(newTs[8](Units::C), 67.6471); //
    }

    // test 4
    {
        const std::size_t nSet = 24;
        HPWH::TempVect_t setTs;
        setTs.resize(nSet);
        HPWH::Temp_t initialT = {20., Units::C}, finalT = {66., Units::C};
        for (std::size_t i = 0; i < nSet; ++i)
            setTs[i] = initialT + i / static_cast<double>(nSet - 1) * (finalT - initialT);
        hpwh.setTankTs(setTs);

        HPWH::TempVect_t newTs;
        hpwh.getTankTs(newTs);

        // Check some expected values.
        EXPECT_NEAR_REL(newTs[4](Units::C), 37.);  //
        EXPECT_NEAR_REL(newTs[10](Units::C), 61.); //
    }

    // test 5
    {
        const std::size_t nSet = 12;
        HPWH::TempVect_t setTs;
        setTs.resize(nSet);
        HPWH::Temp_t initialT = {20., Units::C}, finalT = {64., Units::C};
        for (std::size_t i = 0; i < nSet; ++i)
            setTs[i] = initialT + i / static_cast<double>(nSet - 1) * (finalT - initialT);
        hpwh.setTankTs(setTs);

        HPWH::TempVect_t newTs;
        hpwh.getTankTs(newTs);

        // Check some expected values.
        EXPECT_NEAR_REL(newTs[3](Units::C), 32.);  //
        EXPECT_NEAR_REL(newTs[11](Units::C), 64.); //
    }
}
