/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct PerformanceMapTest : public testing::Test
{
    const double tInOffsetQAHV_dF = 10.;
    const double tOutOffsetQAHV_dF = 15.;

    struct performancePointMP
    {
        double tairF;
        double tinF;
        double outputBTUH;
    };

    double getCapacityMP_F_KW(HPWH& hpwh, performancePointMP& point)
    {
        return hpwh.getCompressorCapacity(
            point.tairF, point.tinF, point.tinF, HPWH::UNITS_KW, HPWH::UNITS_F);
    }

    struct performancePointSP
    {
        double tairF;
        double toutF;
        double tinF;
        double outputBTUH;
    };

    double getCapacitySP_F_BTUHR(HPWH& hpwh, performancePointSP& point)
    {
        return hpwh.getCompressorCapacity(
            point.tairF, point.tinF, point.toutF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    }

    double getCapacitySP_F_BTUHR(HPWH& hpwh,
                                 performancePointSP& point,
                                 double tInOffSet_dF,
                                 double tOutOffSet_dF)
    {
        return hpwh.getCompressorCapacity(point.tairF,
                                          point.tinF - tInOffSet_dF,
                                          point.toutF - tOutOffSet_dF,
                                          HPWH::UNITS_BTUperHr,
                                          HPWH::UNITS_F);
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
    hpwh.initPreset(sModelName);

    double capacity_kW, capacity_BTUperHr;

    // test hot //////////////////////////
    double airTempF = 100.;
    double waterTempF = 125.;
    double setpointF = 150.;
    double capacityData_kW = 52.779317;

    capacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    capacity_kW =
        hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

    EXPECT_NEAR_REL(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr);
    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);

    // test middle ////////////////
    airTempF = 80.;
    waterTempF = 40.;
    setpointF = 150.;
    capacityData_kW = 44.962957379;

    capacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    capacity_kW =
        hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

    EXPECT_NEAR_REL(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr);
    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);

    // test cold ////////////////
    airTempF = 60.;
    waterTempF = 40.;
    setpointF = 125.;
    capacityData_kW = 37.5978306881;

    capacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    capacity_kW =
        hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

    EXPECT_NEAR_REL(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr);
    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
}

/*
 * ColmacCxA_30_SP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_30_SP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_30_SP";
    hpwh.initPreset(sModelName);

    double capacity_kW, capacity_BTUperHr;

    // test hot //////////////////////////
    double airTempF = 100.;
    double waterTempF = 125.;
    double setpointF = 150.;
    double capacityData_kW = 105.12836804;

    capacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    capacity_kW =
        hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

    EXPECT_NEAR_REL(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr);
    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);

    // test middle ////////////////
    airTempF = 80.;
    waterTempF = 40.;
    setpointF = 150.;
    capacityData_kW = 89.186101453;

    capacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    capacity_kW =
        hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

    EXPECT_NEAR_REL(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr);
    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);

    // test cold ////////////////
    airTempF = 60.;
    waterTempF = 40.;
    setpointF = 125.;
    capacityData_kW = 74.2437689948;

    capacity_BTUperHr = hpwh.getCompressorCapacity(
        airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    capacity_kW =
        hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

    EXPECT_NEAR_REL(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr);
    EXPECT_NEAR_REL(capacityData_kW, capacity_kW);
}

/*
 * ColmacCxV_5_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxV_5_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxV_5_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {10.0, 60.0, 8.7756391};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {10.0, 102.0, 9.945545};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {70.0, 74.0, 19.09682784};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {70.0, 116.0, 18.97090763};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 116.0, 24.48703111};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * ColmacCxA_10_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_10_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_10_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 66.0, 29.36581754};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {60.0, 114.0, 27.7407144};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 66.0, 37.4860496};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 114.0, 35.03416199};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 66.0, 46.63144};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 114.0, 43.4308219};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * ColmacCxA_15_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_15_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_15_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 66.0, 37.94515042};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {60.0, 114.0, 35.2295393};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 66.0, 50.360549115};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 114.0, 43.528417017};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 66.0, 66.2675493};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 114.0, 55.941855};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * ColmacCxA_20_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_20_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_20_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 66.0, 57.0994624};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {60.0, 114.0, 53.554293};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 66.0, 73.11242842};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 114.0, 68.034677};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 66.0, 90.289474295};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 114.0, 84.69323};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * ColmacCxA_25_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_25_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_25_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 66.0, 67.28116620};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {60.0, 114.0, 63.5665037};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 66.0, 84.9221285742};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 114.0, 79.6237088};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 66.0, 103.43268186};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 114.0, 97.71458413};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * ColmacCxA_30_MP tests
 */
TEST_F(PerformanceMapTest, ColmacCxA_30_MP)
{
    // get preset model
    HPWH hpwh;
    const std::string sModelName = "ColmacCxA_30_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 66.0, 76.741462845};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {60.0, 114.0, 73.66879620};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 66.0, 94.863116775};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {80.0, 114.0, 90.998904269};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 66.0, 112.864628};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));

    checkPoint = {100.0, 114.0, 109.444451};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
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
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {66.6666666, 60.0, 16.785535996};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {66.6666666, 120.0, 16.76198953};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {88.3333333, 80.0, 20.8571194294};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {110.0, 100.0, 24.287512};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
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
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {66.6666666, 80.0, 38.560161199};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {66.6666666, 140.0, 34.70681846};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {88.3333333, 140.0, 42.40407101};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {110.0, 120.0, 54.3580927};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * NyleC60A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC60A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC60A_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 60.0, 16.11};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {80.0, 60.0, 21.33};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {90.0, 130.0, 19.52};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * NyleC60A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC90A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC90A_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 60.0, 28.19};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {80.0, 60.0, 37.69};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {90.0, 130.0, 36.27};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * NyleC125A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC125A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC125A_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 60.0, 36.04};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {80.0, 60.0, 47.86};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {90.0, 130.0, 43.80};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * NyleC185A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC185A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC185A_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 60.0, 55.00};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {80.0, 60.0, 74.65};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {90.0, 130.0, 67.52};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * NyleC250A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC250A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC250A_MP";
    hpwh.initPreset(sModelName);

    performancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {60.0, 60.0, 75.70};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {80.0, 60.0, 103.40};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
    checkPoint = {90.0, 130.0, 77.89};
    EXPECT_NEAR_REL(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint));
}

/*
 * QAHV_N136TAU_HPB_SP tests
 */
TEST_F(PerformanceMapTest, QAHV_N136TAU_HPB_SP)
{
    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    hpwh.initPreset(sModelName);

    performancePointSP checkPoint; // tairF, toutF, tinF, outputW
    double outputBTUH;

    // test
    checkPoint = {-13.0, 140.0, 41.0, 66529.47273};
    outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF);
    EXPECT_NEAR_REL(checkPoint.outputBTUH, outputBTUH);

    // test
    checkPoint = {-13.0, 176.0, 41.0, 65872.57441};
    EXPECT_ANY_THROW(
        getCapacitySP_F_BTUHR(hpwh, checkPoint)); // max setpoint without adjustment forces error
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {-13.0, 176.0, 84.2, 55913.249232};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {14.0, 176.0, 84.2, 92933.01932};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {42.8, 140.0, 41.0, 136425.98804};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {50.0, 140.0, 41.0, 136425.98804};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {50.0, 176.0, 84.2, 136564.470884};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));
    // test
    checkPoint = {60.8, 158.0, 84.2, 136461.998288};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {71.6, 158.0, 48.2, 136498.001712};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test constant with setpoint between 140 and 158
    checkPoint = {71.6, 149.0, 48.2, 136498.001712};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {82.4, 176.0, 41.0, 136557.496756};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test
    checkPoint = {104.0, 140.0, 62.6, 136480};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));
    // test
    checkPoint = {104.0, 158.0, 62.6, 136480};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));
    // test
    checkPoint = {104.0, 176.0, 84.2, 136564.470884};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));
}

/*
 * QAHV_N136TAU_HPB_SP_extrapolation tests
 */
TEST_F(PerformanceMapTest, QAHV_N136TAU_HPB_SP_extrapolation)
{
    // Also test QAHV constant extrpolation at high air temperatures AND linear extrapolation to hot
    // water temperatures!

    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    hpwh.initPreset(sModelName);

    performancePointSP checkPoint; // tairF, toutF, tinF, outputW

    // test linear along Tin
    checkPoint = {-13.0, 140.0, 36.0, 66529.49616};
    EXPECT_TRUE(
        checkPoint.outputBTUH <
        getCapacitySP_F_BTUHR(
            hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF)); // Check output has increased

    // test linear along Tin
    checkPoint = {-13.0, 140.0, 100.0, 66529.49616};
    EXPECT_TRUE(
        checkPoint.outputBTUH >
        getCapacitySP_F_BTUHR(
            hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF)); // Check output has decreased

    // test linear along Tin
    checkPoint = {-13.0, 176.0, 36.0, 65872.597448};
    EXPECT_TRUE(
        checkPoint.outputBTUH <
        getCapacitySP_F_BTUHR(
            hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF)); // Check output has increased

    // test linear along Tin
    checkPoint = {-13.0, 176.0, 100.0, 55913.249232};
    EXPECT_TRUE(checkPoint.outputBTUH >
                getCapacitySP_F_BTUHR(hpwh,
                                      checkPoint,
                                      tInOffsetQAHV_dF,
                                      tOutOffsetQAHV_dF)); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {10.4, 140.0, 111., 89000.085396};
    EXPECT_TRUE(checkPoint.outputBTUH >
                getCapacitySP_F_BTUHR(hpwh,
                                      checkPoint,
                                      tInOffsetQAHV_dF,
                                      tOutOffsetQAHV_dF)); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {64.4, 158.0, 100, 136461.998288};
    EXPECT_TRUE(checkPoint.outputBTUH >
                getCapacitySP_F_BTUHR(hpwh,
                                      checkPoint,
                                      tInOffsetQAHV_dF,
                                      tOutOffsetQAHV_dF)); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {86.0, 158.0, 100, 136461.998288};
    EXPECT_TRUE(checkPoint.outputBTUH >
                getCapacitySP_F_BTUHR(hpwh,
                                      checkPoint,
                                      tInOffsetQAHV_dF,
                                      tOutOffsetQAHV_dF)); // Check output has decreased at high Tin

    // test linear along Tin
    checkPoint = {104.0, 176.0, 100., 136564.470884};
    EXPECT_TRUE(checkPoint.outputBTUH >
                getCapacitySP_F_BTUHR(hpwh,
                                      checkPoint,
                                      tInOffsetQAHV_dF,
                                      tOutOffsetQAHV_dF)); // Check output has decreased at high Tin

    // test const along Tair
    checkPoint = {110.0, 140.0, 62.6, 136480};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    // test const along Tair
    checkPoint = {114.0, 176.0, 84.2, 136564.470884};
    EXPECT_NEAR_REL(checkPoint.outputBTUH,
                    getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));

    checkPoint = {114.0, 200.0, 84.2, 136564.470884};
    EXPECT_ANY_THROW(getCapacitySP_F_BTUHR(hpwh, checkPoint, tInOffsetQAHV_dF, tOutOffsetQAHV_dF));
}

/*
 * Sanden120 tests
 */
TEST_F(PerformanceMapTest, Sanden120)
{
    HPWH hpwh;
    const std::string sModelName = "Sanden120";
    hpwh.initPreset(sModelName);

    performancePointSP checkPoint; // tairF, toutF, tinF, outputW
    double outputBTUH;

    // nominal
    checkPoint = {60, 149.0, 41.0, 15059.59167};
    outputBTUH = hpwh.getCompressorCapacity(
        checkPoint.tairF, checkPoint.tinF, checkPoint.toutF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    EXPECT_NEAR_REL(checkPoint.outputBTUH, outputBTUH);

    // Cold outlet temperature
    checkPoint = {60, 125.0, 41.0, 15059.59167};
    outputBTUH = hpwh.getCompressorCapacity(
        checkPoint.tairF, checkPoint.tinF, checkPoint.toutF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
    EXPECT_NEAR_REL(checkPoint.outputBTUH, outputBTUH);

    // tests fails when output high
    checkPoint = {60, 200, 41.0, 15059.59167};
    EXPECT_ANY_THROW(hpwh.getCompressorCapacity(
        checkPoint.tairF, checkPoint.tinF, checkPoint.toutF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F));
}
