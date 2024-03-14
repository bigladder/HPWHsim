/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct PerformanceMapTest : public testing::Test
{
    struct performancePointMP
    {
        double airT;
        double inT;
    };

    double getCapacityMP(HPWH& hpwh,
                         performancePointMP& point,
                         Units::Power unitsPower = Units::Power::kW,
                         Units::Temp unitsTemp = Units::Temp::F)
    {
        return hpwh.getCompressorCapacity(point.airT, point.inT, point.inT, unitsPower, unitsTemp);
    }

    struct performancePointSP
    {
        double airT;
        double outT;
        double inT;
    };

    double getCapacitySP(HPWH& hpwh,
                         performancePointSP& point,
                         Units::Power unitsPower = Units::Power::Btu_per_h,
                         Units::Temp unitsTemp = Units::Temp::F)
    {
        return hpwh.getCompressorCapacity(point.airT, point.inT, point.outT, unitsPower, unitsTemp);
    }

    double getQAHV_capacitySP(HPWH& hpwh,
                              performancePointSP& point,
                              Units::Power unitsPower = Units::Power::Btu_per_h,
                              Units::Temp unitsTemp = Units::Temp::F)
    {
        double inOffsetT = Units::convert(10., Units::Temp::F, unitsTemp);
        double outOffsetT = Units::convert(15., Units::Temp::F, unitsTemp);
        return hpwh.getCompressorCapacity(
            point.airT, point.inT - inOffsetT, point.outT - outOffsetT, unitsPower, unitsTemp);
    }
};

/*
 * ColmacCxA_15_SP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_15_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_15_SP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    double capacity_kW, capacity_BTUperH;

    // test hot //////////////////////////
    double airT_F = 100., airT_C = F_TO_C(airT_F);
    double waterT_F = 125., waterT_C = F_TO_C(waterT_F);
    double setpointT_F = 150., setpointT_C = F_TO_C(setpointT_F);
    double capacityData_kW = 52.779317, capacityData_BTUperH = KW_TO_BTUperH(capacityData_kW);

    capacity_kW =
        hpwh.getCompressorCapacity(airT_C, waterT_C, setpointT_C, Units::Power::kW, Units::Temp::C);
    capacity_BTUperH = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, Units::Power::Btu_per_h, Units::Temp::F);

    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
    EXPECT_NEAR_REL(capacityData_BTUperH, capacity_BTUperH);

    // test middle ////////////////
    airT_F = 80.;
    waterT_F = 40.;
    setpointT_F = 150.;
    capacityData_kW = 44.962957379;

    airT_C = F_TO_C(airT_F);
    waterT_C = F_TO_C(waterT_F);
    setpointT_C = F_TO_C(setpointT_F);
    capacityData_BTUperH = KW_TO_BTUperH(capacityData_kW);

    capacity_kW =
        hpwh.getCompressorCapacity(airT_C, waterT_C, setpointT_C, Units::Power::kW, Units::Temp::C);
    capacity_BTUperH = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, Units::Power::Btu_per_h, Units::Temp::F);

    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
    EXPECT_NEAR_REL(capacityData_BTUperH, capacity_BTUperH);

    // test cold ////////////////
    airT_F = 60.;
    waterT_F = 40.;
    setpointT_F = 125.;
    capacityData_kW = 37.5978306881;

    airT_C = F_TO_C(airT_F);
    waterT_C = F_TO_C(waterT_F);
    setpointT_C = F_TO_C(setpointT_F);
    capacityData_BTUperH = KW_TO_BTUperH(capacityData_kW);

    capacity_kW =
        hpwh.getCompressorCapacity(airT_C, waterT_C, setpointT_C, Units::Power::kW, Units::Temp::C);
    capacity_BTUperH = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, Units::Power::Btu_per_h, Units::Temp::F);

    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
    EXPECT_NEAR_REL(capacityData_BTUperH, capacity_BTUperH);
}

/*
 * ColmacCxA_30_SP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_30_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_30_SP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test hot //////////////////////////
    double airT_F = 100., airT_C = F_TO_C(airT_F);
    double waterT_F = 125., waterT_C = F_TO_C(waterT_F);
    double setpointT_F = 150., setpointT_C = F_TO_C(setpointT_F);
    double capacityData_kW = 105.12836804, capacityData_BTUperH = KW_TO_BTUperH(capacityData_kW);

    double capacity_kW =
        hpwh.getCompressorCapacity(airT_C, waterT_C, setpointT_C, Units::Power::kW, Units::Temp::C);
    double capacity_BTUperH = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, Units::Power::Btu_per_h, Units::Temp::F);

    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
    EXPECT_NEAR_REL(capacityData_BTUperH, capacity_BTUperH);

    // test middle ////////////////
    airT_F = 80.;
    waterT_F = 40.;
    setpointT_F = 150.;
    capacityData_kW = 89.186101453;

    airT_C = F_TO_C(airT_F);
    waterT_C = F_TO_C(waterT_F);
    setpointT_C = F_TO_C(setpointT_F);
    capacityData_BTUperH = KW_TO_BTUperH(capacityData_kW);

    capacity_kW =
        hpwh.getCompressorCapacity(airT_C, waterT_C, setpointT_C, Units::Power::kW, Units::Temp::C);
    capacity_BTUperH = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, Units::Power::Btu_per_h, Units::Temp::F);

    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
    EXPECT_NEAR_REL(capacityData_BTUperH, capacity_BTUperH);

    // test cold ////////////////
    airT_F = 60.;
    waterT_F = 40.;
    setpointT_F = 125.;
    capacityData_kW = 74.2437689948;

    airT_C = F_TO_C(airT_F);
    waterT_C = F_TO_C(waterT_F);
    setpointT_C = F_TO_C(setpointT_F);
    capacityData_BTUperH = KW_TO_BTUperH(capacityData_kW);

    capacity_kW =
        hpwh.getCompressorCapacity(airT_C, waterT_C, setpointT_C, Units::Power::kW, Units::Temp::C);
    capacity_BTUperH = hpwh.getCompressorCapacity(
        airT_F, waterT_F, setpointT_F, Units::Power::Btu_per_h, Units::Temp::F);

    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
    EXPECT_NEAR_REL(capacityData_BTUperH, capacity_BTUperH);
}

/*
 * ColmacCxV_5_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxV_5_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {10.0, 60.0};
    double capData = 8.7756391;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {10.0, 102.0};
    capData = 9.945545;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {70.0, 74.0};
    capData = 19.09682784;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {70.0, 116.0};
    capData = 18.97090763;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 116.0};
    capData = 24.48703111;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * ColmacCxA_10_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_10_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_10_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 66.0};
    double capData = 29.36581754;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {60.0, 114.0};
    capData = 27.7407144;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 66.0};
    capData = 37.4860496;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 114.0};
    capData = 35.03416199;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 66.0};
    capData = 46.63144;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 114.0};
    capData = 43.4308219;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * ColmacCxA_15_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_15_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_15_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 66.0};
    double capData = 37.94515042;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {60.0, 114.0};
    capData = 35.229539;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 66.0};
    capData = 50.360549115;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 114.0};
    capData = 43.528417017;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 66.0};
    capData = 66.2675493;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 114.0};
    capData = 55.941855;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * ColmacCxA_20_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_20_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_20_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 66.0};
    double capData = 57.0994624;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {60.0, 114.0};
    capData = 53.554293;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 66.0};
    capData = 73.11242842;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 114.0};
    capData = 68.034677;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 66.0};
    capData = 90.289474295;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 114.0};
    capData = 84.69323;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * ColmacCxA_25_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_25_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_25_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 66.0};
    double capData = 67.28116620;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {60.0, 114.0};
    capData = 63.5665037;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 66.0};
    capData = 84.9221285742;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 114.0};
    capData = 79.6237088;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 66.0};
    capData = 103.43268186;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 114.0};
    capData = 97.71458413;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * ColmacCxA_30_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_30_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_30_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 66.0};
    double capData = 76.741462845, cap;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {60.0, 114.0};
    capData = 73.66879620;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 66.0};
    capData = 94.863116775;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 114.0};
    capData = 90.998904269;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 66.0};
    capData = 112.864628;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {100.0, 114.0};
    capData = 109.444451;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * RheemHPHD60 tests
 */
TEST_F(PerformanceMapTest, RheemHPHD60)
{
    // MODELS_RHEEM_HPHD60HNU_201_MP
    // MODELS_RHEEM_HPHD60VNU_201_MP
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "RheemHPHD60";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {66.6666666, 60.0};
    double capData = 16.785535996;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {66.6666666, 120.0};
    capData = 16.76198953;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {88.3333333, 80.0};
    capData = 20.8571194294;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {110.0, 100.0};
    capData = 24.287512;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * RheemHPHD135 tests
 */
TEST_F(PerformanceMapTest, RheemHPHD135)
{
    // MODELS_RHEEM_HPHD135HNU_483_MP
    // MODELS_RHEEM_HPHD135VNU_483_MP
    HPWH hpwh;
    const std::string sModelName = "RheemHPHD135";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {66.6666666, 80.0};
    double capData = 38.560161199;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {66.6666666, 140.0};
    capData = 34.70681846;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {88.3333333, 140.0};
    capData = 42.40407101;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {110.0, 120.0};
    capData = 54.3580927;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * NyleC60A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC60A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC60A_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 60.0};
    double capData = 16.11;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 60.0};
    capData = 21.33;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {90.0, 130.0};
    capData = 19.52;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * NyleC60A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC90A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC90A_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 60.0};
    double capData = 28.19;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 60.0};
    capData = 37.69;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {90.0, 130.0};
    capData = 36.27;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * NyleC125A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC125A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC125A_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 60.0};
    double capData = 36.04;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 60.0};
    capData = 47.86;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {90.0, 130.0};
    capData = 43.80;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * NyleC185A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC185A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC185A_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 60.0};
    double capData = 55.00;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 60.0};
    capData = 74.65;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {90.0, 130.0};
    capData = 67.52;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * NyleC250A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC250A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC250A_MP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test some points outside of defrost ////////////////
    performancePointMP checkPoint = {60.0, 60.0};
    double capData = 75.70;
    double cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {80.0, 60.0};
    capData = 103.40;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {90.0, 130.0};
    capData = 77.89;
    cap = getCapacityMP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * QAHV_N136TAU_HPB_SP tests
 */
TEST_F(PerformanceMapTest, QAHV_N136TAU_HPB_SP)
{
    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test
    performancePointSP checkPoint = {-13.0, 140.0, 41.0};
    double capData = 66529.49616;
    double cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {-13.0, 176.0, 41.0};
    capData = 65872.597448;
    cap = getCapacitySP(hpwh, checkPoint);
    EXPECT_EQ(cap, HPWH::HPWH_ABORT); // max setpoint without adjustment forces error

    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {-13.0, 176.0, 84.2};
    capData = 55913.249232;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {14.0, 176.0, 84.2};
    capData = 92933.01932;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {42.8, 140.0, 41.0};
    capData = 136425.98804;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {50.0, 140.0, 41.0};
    capData = 136425.98804;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {50.0, 176.0, 84.2};
    capData = 136564.470884;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {60.8, 158.0, 84.2};
    capData = 136461.998288;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {71.6, 158.0, 48.2};
    capData = 136498.001712;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test constant with setpoint between 140 and 158
    checkPoint = {71.6, 149.0, 48.2};
    capData = 136498.001712;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {82.4, 176.0, 41.0};
    capData = 136557.496756;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {104.0, 140.0, 62.6};
    capData = 136480;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {104.0, 158.0, 62.6};
    capData = 136480;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test
    checkPoint = {104.0, 176.0, 84.2};
    capData = 136564.470884;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);
}

/*
 * QAHV_N136TAU_HPB_SP_extrapolation tests
 */
TEST_F(PerformanceMapTest, QAHV_N136TAU_HPB_SP_extrapolation)
{
    // Also test QAHV constant extrpolation at high air temperatures AND linear extrapolation to
    // hot water temperatures!

    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // test linear along Tin
    performancePointSP checkPoint = {-13.0, 140.0, 36.0};
    double capData = 66529.49616;
    double cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData < cap); // Check output has increased

    // test linear along Tin
    checkPoint = {-13.0, 140.0, 100.0};
    capData = 66529.49616;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData > cap); // Check output has decreased

    // test linear along Tin
    checkPoint = {-13.0, 176.0, 36.0};
    capData = 65872.597448;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData < cap); // Check output has increased

    // test linear along Tin
    checkPoint = {-13.0, 176.0, 100.0};
    capData = 55913.249232;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData > cap); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {10.4, 140.0, 111.};
    capData = 89000.085396;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData > cap); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {64.4, 158.0, 100};
    capData = 136461.998288;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData > cap); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {86.0, 158.0, 100};
    capData = 136461.998288;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData > cap); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {104.0, 176.0, 100.};
    capData = 136564.470884;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_TRUE(capData > cap); // Check output has decreased at high Tin

    // test const along Tair
    checkPoint = {110.0, 140.0, 62.6};
    capData = 136480;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    // test const along Tair
    checkPoint = {114.0, 176.0, 84.2};
    capData = 136564.470884;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_NEAR_REL(capData, cap);

    checkPoint = {114.0, 200.0, 84.2};
    capData = 136564.470884;
    cap = getQAHV_capacitySP(hpwh, checkPoint);
    EXPECT_EQ(cap, HPWH::HPWH_ABORT);
}

/*
 * Sanden120 tests
 */
TEST_F(PerformanceMapTest, Sanden120)
{
    HPWH hpwh;
    const std::string sModelName = "Sanden120";
    EXPECT_EQ(hpwh.initPreset(sModelName), 0) << "Could not initialize model " << sModelName;

    // nominal
    performancePointSP checkPoint = {60, 149.0, 41.0};
    double capData = 15059.59167;
    double cap = hpwh.getCompressorCapacity(
        checkPoint.airT, checkPoint.inT, checkPoint.outT, Units::Power::Btu_per_h, Units::Temp::F);
    EXPECT_NEAR_REL(capData, cap);

    // Cold outlet temperature
    checkPoint = {60, 125.0, 41.0};
    capData = 15059.59167;
    cap = hpwh.getCompressorCapacity(
        checkPoint.airT, checkPoint.inT, checkPoint.outT, Units::Power::Btu_per_h, Units::Temp::F);
    EXPECT_NEAR_REL(capData, cap);

    // tests fails when output high
    checkPoint = {60, 200, 41.0};
    capData = 15059.59167;
    cap = hpwh.getCompressorCapacity(
        checkPoint.airT, checkPoint.inT, checkPoint.outT, Units::Power::Btu_per_h, Units::Temp::F);
    EXPECT_EQ(HPWH::HPWH_ABORT, cap);
}
