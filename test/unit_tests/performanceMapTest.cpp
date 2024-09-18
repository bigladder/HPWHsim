/* Copyright (c) 2023 Big Ladder Software LLC. All rights reserved.
 * See the LICENSE file for additional terms and conditions. */

// HPWHsim
#include "HPWH.hh"
#include "unit-test.hh"

struct PerformanceMapTest : public testing::Test
{
    const HPWH::Temp_d_t in_dT_QAHV = {10., Units::dF};
    const HPWH::Temp_d_t out_dT_QAHV = {15., Units::dF};

    struct PerformancePointMP
    {
        HPWH::Temp_t airT;
        HPWH::Temp_t inT;
        HPWH::Power_t outputPower;

        PerformancePointMP(const HPWH::TempVect_t& tempVect = {},
                           HPWH::Power_t outputPower_in = {0., Units::W})
        {
            if (tempVect.size() > 0)
            {
                airT = tempVect[0];
                if (tempVect.size() > 1)
                {
                    inT = tempVect[1];
                }
            }
            outputPower = outputPower_in;
        }
    };

    HPWH::Power_t getCapacityMP(HPWH& hpwh, PerformancePointMP& point)
    {
        return hpwh.getCompressorCapacity(point.airT, point.inT, point.inT);
    }

    struct PerformancePointSP
    {
        HPWH::Temp_t airT = {0, Units::C};
        HPWH::Temp_t outT = {0, Units::C};
        HPWH::Temp_t inT = {0, Units::C};
        HPWH::Power_t outputPower = {0., Units::W};
        PerformancePointSP(const HPWH::TempVect_t& tempVect = {},
                           HPWH::Power_t outputPower_in = {0., Units::W})
        {
            if (tempVect.size() > 0)
            {
                airT = tempVect[0];
                if (tempVect.size() > 1)
                {
                    outT = tempVect[1];
                    if (tempVect.size() > 2)
                    {
                        inT = tempVect[2];
                    }
                }
            }
            outputPower = outputPower_in;
        }
    };

    HPWH::Power_t getCapacitySP(HPWH& hpwh, PerformancePointSP& point)
    {
        return hpwh.getCompressorCapacity(point.airT, point.inT, point.outT);
    }

    HPWH::Power_t getCapacitySP(HPWH& hpwh,
                                PerformancePointSP& point,
                                HPWH::Temp_d_t in_dT,
                                HPWH::Temp_d_t out_dT)
    {
        return hpwh.getCompressorCapacity(point.airT, point.inT - in_dT, point.outT - out_dT);
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

    // test hot //////////////////////////
    HPWH::Temp_t airT = {100., Units::F};
    HPWH::Temp_t waterT = {125., Units::F};
    HPWH::Temp_t setpointT = {150., Units::F};
    HPWH::Power_t capacityData = {52.779317, Units::kW};

    auto capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR_REL(capacityData(), capacity());

    // test middle ////////////////
    airT = {80., Units::F};
    waterT = {40., Units::F};
    setpointT = {150., Units::F};
    capacityData = {44.962957379, Units::kW};

    capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR_REL(capacityData(), capacity());

    // test cold ////////////////
    airT = {60., Units::F};
    waterT = {40., Units::F};
    setpointT = {125., Units::F};
    capacityData = {37.5978306881, Units::kW};

    capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR_REL(capacityData(), capacity());
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

    // test hot //////////////////////////
    HPWH::Temp_t airT = {100., Units::F};
    HPWH::Temp_t waterT = {125., Units::F};
    HPWH::Temp_t setpointT = {150., Units::F};
    HPWH::Power_t capacityData = {105.12836804, Units::kW};

    auto capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR_REL(capacityData(), capacity());

    // test middle ////////////////
    airT = {80., Units::F};
    waterT = {40., Units::F};
    setpointT = {150., Units::F};
    capacityData = {89.186101453, Units::kW};

    capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR_REL(capacityData(), capacity());

    // test cold ////////////////
    airT = {60., Units::F};
    waterT = {40., Units::F};
    setpointT = {125., Units::F};
    capacityData = {74.2437689948, Units::kW};

    capacity = hpwh.getCompressorCapacity(airT, waterT, setpointT);

    EXPECT_NEAR_REL(capacityData(), capacity());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{10.0, 60.0}, Units::F}, {8.7756391, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{10.0, 102.0}, Units::F}, {9.945545, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{70.0, 74.0}, Units::F}, {19.09682784, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{70.0, 116.0}, Units::F}, {18.97090763, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 116.0}, Units::F}, {24.48703111, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 66.0}, Units::F}, {29.36581754, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{60.0, 114.0}, Units::F}, {27.7407144, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 66.0}, Units::F}, {37.4860496, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 114.0}, Units::F}, {35.03416199, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 66.0}, Units::F}, {46.63144, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 114.0}, Units::F}, {43.4308219, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 66.0}, Units::F}, {37.94515042, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{60.0, 114.0}, Units::F}, {35.2295393, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 66.0}, Units::F}, {50.360549115, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 114.0}, Units::F}, {43.528417017, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 66.0}, Units::F}, {66.2675493, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 114.0}, Units::F}, {55.941855, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 66.0}, Units::F}, {57.0994624, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{60.0, 114.0}, Units::F}, {53.554293, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 66.0}, Units::F}, {73.11242842, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 114.0}, Units::F}, {68.034677, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 66.0}, Units::F}, {90.289474295, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 114.0}, Units::F}, {84.69323, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 66.0}, Units::F}, {67.28116620, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{60.0, 114.0}, Units::F}, {63.5665037, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 66.0}, Units::F}, {84.9221285742, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 114.0}, Units::F}, {79.6237088, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 66.0}, Units::F}, {103.43268186, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 114.0}, Units::F}, {97.71458413, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 66.0}, Units::F}, {76.741462845, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{60.0, 114.0}, Units::F}, {73.66879620, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 66.0}, Units::F}, {94.863116775, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{80.0, 114.0}, Units::F}, {90.998904269, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 66.0}, Units::F}, {112.864628, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());

    checkPoint = {{{100.0, 114.0}, Units::F}, {109.444451, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{66.6666666, 60.0}, Units::F}, {16.785535996, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{66.6666666, 120.0}, Units::F}, {16.76198953, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{88.3333333, 80.0}, Units::F}, {20.8571194294, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{110.0, 100.0}, Units::F}, {24.287512, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
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

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{66.6666666, 80.0}, Units::F}, {38.560161199, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{66.6666666, 140.0}, Units::F}, {34.70681846, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{88.3333333, 140.0}, Units::F}, {42.40407101, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{110.0, 120.0}, Units::F}, {54.3580927, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
}

/*
 * NyleC60A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC60A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC60A_MP";
    hpwh.initPreset(sModelName);

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 60.0}, Units::F}, {16.11, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{80.0, 60.0}, Units::F}, {21.33, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{90.0, 130.0}, Units::F}, {19.52, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
}

/*
 * NyleC60A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC90A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC90A_MP";
    hpwh.initPreset(sModelName);

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 60.0}, Units::F}, {28.19, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{80.0, 60.0}, Units::F}, {37.69, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{90.0, 130.0}, Units::F}, {36.27, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
}

/*
 * NyleC125A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC125A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC125A_MP";
    hpwh.initPreset(sModelName);

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 60.0}, Units::F}, {36.04, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{80.0, 60.0}, Units::F}, {47.86, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{90.0, 130.0}, Units::F}, {43.80, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
}

/*
 * NyleC185A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC185A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC185A_MP";
    hpwh.initPreset(sModelName);

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 60.0}, Units::F}, {55.00, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{80.0, 60.0}, Units::F}, {74.65, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{90.0, 130.0}, Units::F}, {67.52, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
}

/*
 * NyleC250A_MP tests
 */
TEST_F(PerformanceMapTest, NyleC250A_MP)
{
    HPWH hpwh;
    const std::string sModelName = "NyleC250A_MP";
    hpwh.initPreset(sModelName);

    PerformancePointMP checkPoint;

    // test some points outside of defrost ////////////////
    checkPoint = {{{60.0, 60.0}, Units::F}, {75.70, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{80.0, 60.0}, Units::F}, {103.40, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
    checkPoint = {{{90.0, 130.0}, Units::F}, {77.89, Units::kW}};
    EXPECT_NEAR_REL(checkPoint.outputPower(), getCapacityMP(hpwh, checkPoint)());
}

/*
 * QAHV_N136TAU_HPB_SP tests
 */
TEST_F(PerformanceMapTest, QAHV_N136TAU_HPB_SP)
{
    HPWH hpwh;
    const std::string sModelName = "QAHV_N136TAU_HPB_SP";
    hpwh.initPreset(sModelName);

    PerformancePointSP checkPoint; // airT, outT, inT, outputW
    double outputPower;

    // test
    checkPoint = {{{-13.0, 140.0, 41.0}, Units::F}, {66529.47273, Units::Btu_per_h}};
    outputPower = getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)();
    EXPECT_NEAR_REL(checkPoint.outputPower(), outputPower);

    // test
    checkPoint = {{{-13.0, 176.0, 41.0}, Units::F}, {65872.57441, Units::Btu_per_h}};
    EXPECT_ANY_THROW(
        getCapacitySP(hpwh, checkPoint)); // max setpoint without adjustment forces error
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{-13.0, 176.0, 84.2}, Units::F}, {55913.249232, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{14.0, 176.0, 84.2}, Units::F}, {92933.01932, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{42.8, 140.0, 41.0}, Units::F}, {136425.98804, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{50.0, 140.0, 41.0}, Units::F}, {136425.98804, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{50.0, 176.0, 84.2}, Units::F}, {136564.470884, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());
    // test
    checkPoint = {{{60.8, 158.0, 84.2}, Units::F}, {136461.998288, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{71.6, 158.0, 48.2}, Units::F}, {136498.001712, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test constant with setpoint between 140 and 158
    checkPoint = {{{71.6, 149.0, 48.2}, Units::F}, {136498.001712, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{82.4, 176.0, 41.0}, Units::F}, {136557.496756, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test
    checkPoint = {{{104.0, 140.0, 62.6}, Units::F}, {136480, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());
    // test
    checkPoint = {{{104.0, 158.0, 62.6}, Units::F}, {136480, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());
    // test
    checkPoint = {{{104.0, 176.0, 84.2}, Units::F}, {136564.470884, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());
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

    PerformancePointSP checkPoint; // airT, outT, inT, outputW

    // test linear along inT
    checkPoint = {{{-13.0, 140.0, 36.0}, Units::F}, {66529.49616, Units::Btu_per_h}};
    EXPECT_TRUE(
        checkPoint.outputPower <
        getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)); // Check output has increased

    // test linear along inT
    checkPoint = {{{-13.0, 140.0, 100.0}, Units::F}, {66529.49616, Units::Btu_per_h}};
    EXPECT_TRUE(
        checkPoint.outputPower >
        getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)); // Check output has decreased

    // test linear along inT
    checkPoint = {{{-13.0, 176.0, 36.0}, Units::F}, {65872.597448, Units::Btu_per_h}};
    EXPECT_TRUE(
        checkPoint.outputPower <
        getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)); // Check output has increased

    // test linear along inT
    checkPoint = {{{-13.0, 176.0, 100.0}, Units::F}, {55913.249232, Units::Btu_per_h}};
    EXPECT_TRUE(checkPoint.outputPower >
                getCapacitySP(hpwh,
                              checkPoint,
                              in_dT_QAHV,
                              out_dT_QAHV)); // Check output has decreased at high inT

    // test linear along inT
    checkPoint = {{{10.4, 140.0, 111.}, Units::F}, {89000.085396, Units::Btu_per_h}};
    EXPECT_TRUE(checkPoint.outputPower >
                getCapacitySP(hpwh,
                              checkPoint,
                              in_dT_QAHV,
                              out_dT_QAHV)); // Check output has decreased at high inT

    // test linear along inT
    checkPoint = {{{64.4, 158.0, 100}, Units::F}, {136461.998288, Units::Btu_per_h}};
    EXPECT_TRUE(checkPoint.outputPower >
                getCapacitySP(hpwh,
                              checkPoint,
                              in_dT_QAHV,
                              out_dT_QAHV)); // Check output has decreased at high inT

    // test linear along inT
    checkPoint = {{{86.0, 158.0, 100}, Units::F}, {136461.998288, Units::Btu_per_h}};
    EXPECT_TRUE(checkPoint.outputPower >
                getCapacitySP(hpwh,
                              checkPoint,
                              in_dT_QAHV,
                              out_dT_QAHV)); // Check output has decreased at high inT

    // test linear along inT
    checkPoint = {{{104.0, 176.0, 100.}, Units::F}, {136564.470884, Units::Btu_per_h}};
    EXPECT_TRUE(checkPoint.outputPower >
                getCapacitySP(hpwh,
                              checkPoint,
                              in_dT_QAHV,
                              out_dT_QAHV)); // Check output has decreased at high inT

    // test const along Tair
    checkPoint = {{{110.0, 140.0, 62.6}, Units::F}, {136480, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    // test const along Tair
    checkPoint = {{{114.0, 176.0, 84.2}, Units::F}, {136564.470884, Units::Btu_per_h}};
    EXPECT_NEAR_REL(checkPoint.outputPower(),
                    getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV)());

    checkPoint = {{{114.0, 200.0, 84.2}, Units::F}, {136564.470884, Units::Btu_per_h}};
    EXPECT_ANY_THROW(getCapacitySP(hpwh, checkPoint, in_dT_QAHV, out_dT_QAHV));
}

/*
 * Sanden120 tests
 */
TEST_F(PerformanceMapTest, Sanden120)
{
    HPWH hpwh;
    const std::string sModelName = "Sanden120";
    hpwh.initPreset(sModelName);

    PerformancePointSP checkPoint; // airT, outT, inT, outputW
    double outputPower;

    // nominal
    checkPoint = {{{60, 149.0, 41.0}, Units::F}, {15059.59167, Units::Btu_per_h}};
    outputPower = hpwh.getCompressorCapacity(checkPoint.airT, checkPoint.inT, checkPoint.outT)();
    EXPECT_NEAR_REL(checkPoint.outputPower(), outputPower);

    // Cold outlet temperature
    checkPoint = {{{60, 125.0, 41.0}, Units::F}, {15059.59167, Units::Btu_per_h}};
    outputPower = hpwh.getCompressorCapacity(checkPoint.airT, checkPoint.inT, checkPoint.outT)();
    EXPECT_NEAR_REL(checkPoint.outputPower(), outputPower);

    // tests fails when output high
    checkPoint = {{{60, 200, 41.0}, Units::F}, {15059.59167, Units::Btu_per_h}};
    EXPECT_ANY_THROW(hpwh.getCompressorCapacity(checkPoint.airT, checkPoint.inT, checkPoint.outT));
}
