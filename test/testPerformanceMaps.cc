


/*
  unit test to check data input and performance maps are correct
*/

#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 

using std::cout;
using std::string;

struct performancePointMP {
	double tairF;
	double tinF;
	double outputBTUH;
};
double getCapacityMP_F_KW(HPWH& hpwh, performancePointMP& point) {
	return hpwh.getCompressorCapacity(point.tairF, point.tinF, point.tinF, HPWH::UNITS_KW, HPWH::UNITS_F);
}

struct performancePointSP {
	double tairF;
	double toutF;
	double tinF;
	double outputBTUH;
};
double getCapacitySP_F_BTUHR(HPWH& hpwh, performancePointSP& point) {
	return hpwh.getCompressorCapacity(point.tairF, point.tinF, point.toutF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
}

void testCXA15MatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_15_SP";
	double capacity_kW, capacity_BTUperHr;
	// get preset model 
	getHPWHObject(hpwh, input);

	// test hot //////////////////////////
	double airTempF = 100.;
	double waterTempF = 125.;
	double setpointF = 150.;
	double capacityData_kW = 52.779317;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test middle ////////////////
	airTempF = 80.;
	waterTempF = 40.;
	setpointF = 150.;
	capacityData_kW = 44.962957379;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test cold ////////////////
	airTempF = 60.;
	waterTempF = 40.;
	setpointF = 125.;
	capacityData_kW = 37.5978306881;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));
}

void testCXA30MatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_30_SP";
	double capacity_kW, capacity_BTUperHr;
	// get preset model 
	getHPWHObject(hpwh, input);

	// test hot //////////////////////////
	double airTempF = 100.;
	double waterTempF = 125.;
	double setpointF = 150.;
	double capacityData_kW = 105.12836804;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test middle ////////////////
	airTempF = 80.;
	waterTempF = 40.;
	setpointF = 150.;
	capacityData_kW = 89.186101453;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test cold ////////////////
	airTempF = 60.;
	waterTempF = 40.;
	setpointF = 125.;
	capacityData_kW = 74.2437689948;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));
}


// Colmac MP perfmaps tests
void testCXV5MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxV_5_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 10.0, 60.0, 8.7756391 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 10.0, 102.0, 9.945545 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 70.0, 74.0, 19.09682784 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 70.0, 116.0, 18.97090763 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 116.0, 24.48703111 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA10MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_10_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 66.0, 29.36581754 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 60.0, 114.0, 27.7407144 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 66.0, 37.4860496 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 114.0, 35.03416199 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 66.0, 46.63144 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 114.0, 43.4308219 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testCXA15MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_15_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 66.0, 37.94515042 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 60.0, 114.0, 35.2295393 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 66.0, 50.360549115 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 114.0, 43.528417017 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 66.0, 66.2675493 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 114.0, 55.941855 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA20MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_20_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 66.0, 57.0994624 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 60.0, 114.0, 53.554293 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 66.0, 73.11242842 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 114.0, 68.034677 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 100.0, 66.0, 90.289474295 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 100.0, 114.0, 84.69323 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA25MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_25_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 66.0, 67.28116620 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 60.0, 114.0, 63.5665037 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 66.0, 84.9221285742 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 114.0, 79.6237088 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 66.0, 103.43268186 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 100.0, 114.0, 97.71458413 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA30MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_30_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 66.0, 76.741462845 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 60.0, 114.0, 73.66879620 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 66.0, 94.863116775 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 80.0, 114.0, 90.998904269 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 66.0, 112.864628 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 100.0, 114.0, 109.444451 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testRheemHPHD60() {
	//MODELS_RHEEM_HPHD60HNU_201_MP
	//MODELS_RHEEM_HPHD60VNU_201_MP
	HPWH hpwh;
	string input = "RheemHPHD60";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 66.6666666, 60.0, 16.785535996 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 66.6666666, 120.0, 16.76198953 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 88.3333333, 80.0, 20.8571194294 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 110.0, 100.0, 24.287512 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testRheemHPHD135() {
	//MODELS_RHEEM_HPHD135HNU_483_MP
	//MODELS_RHEEM_HPHD135VNU_483_MP
	HPWH hpwh;
	string input = "RheemHPHD135";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 66.6666666, 80.0, 38.560161199 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 66.6666666, 140.0, 34.70681846 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 88.3333333, 140.0, 42.40407101 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 110.0, 120.0, 54.3580927 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testNyleC60AMP() {
	HPWH hpwh;
	string input = "NyleC60A_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 60.0, 16.258724494530256 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 80.0, 60.0, 21.073600659726043 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 90.0, 130.0, 20.128785625310147 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testNyleC90AMP() {
	HPWH hpwh;
	string input = "NyleC90A_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 60.0, 28.391312547894692 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 80.0, 60.0, 37.76629798021198 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 90.0, 130.0, 36.588687630737226 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testNyleC125AMP() {
	HPWH hpwh;
	string input = "NyleC125A_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 60.0, 36.450663648640635 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 80.0, 60.0, 47.60348009159024 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 90.0, 130.0, 44.89242490188696 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testNyleC185AMP() {
	HPWH hpwh;
	string input = "NyleC185A_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 60.0, 56.42805061073486 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 80.0, 60.0, 72.02025074279416 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 90.0, 130.0, 69.83381084066781 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testNyleC250AMP() {
	HPWH hpwh;
	string input = "NyleC250A_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 60.0, 60.0, 80.22629566828897 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 80.0, 60.0, 104.4274354892963 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	checkPoint = { 90.0, 130.0, 82.3921175498482 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}


void testQAHVMatchesDataMap() {
	HPWH hpwh;
	string input = "QAHV_N136TAU_HPB_SP";
	performancePointSP checkPoint; // tairF, toutF, tinF, outputW
	double outputBTUH;
	// get preset model 
	getHPWHObject(hpwh, input);

	// test
	checkPoint = { -13.0, 140.0, 41.0, 66529.49616 };
	outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint);
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, outputBTUH));
	// test
	checkPoint = { -13.0, 176.0, 41.0, 65872.597448 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { -13.0, 176.0, 84.2, 55913.249232 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { -11.2, 140.0, 41.0, 69590.397948 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { -07.6, 176.0, 48.2, 72951.552324 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { -04.0, 140.0, 41.0, 81293.824084 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { -00.4, 176.0, 84.2, 73181.70196 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 06.8, 158.0, 84.2, 84174.67122 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 14.0, 176.0, 84.2, 92933.01932 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 50.0, 140.0, 41.0, 136425.98804 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 50.0, 176.0, 84.2, 136564.470884 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 64.4, 140.0, 62.6, 136480 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 71.6, 158.0, 48.2, 136498.001712 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 82.4, 176.0, 41.0, 136557.496756 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 104.0, 140.0, 62.6, 136480 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 104.0, 158.0, 62.6, 136480 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
	// test
	checkPoint = { 104.0, 176.0, 84.2, 136564.470884 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
}
// Also test QAHV constant extrpolation at high air temperatures AND linear extrapolation to hot water temperatures!
void testQAHVExtrapolates() {
	HPWH hpwh;
	string input = "QAHV_N136TAU_HPB_SP";
	performancePointSP checkPoint; // tairF, toutF, tinF, outputW
	double outputBTUH;
	// get preset model 
	getHPWHObject(hpwh, input);

	// test linear along Tin
	checkPoint = { -13.0, 140.0, 36.0, 66529.49616 };
	outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint);
	ASSERTTRUE(checkPoint.outputBTUH < outputBTUH); // Check output has increased

	// test linear along Tin
	checkPoint = { -13.0, 140.0, 100.0, 66529.49616 };
	outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint);
	ASSERTTRUE(checkPoint.outputBTUH > outputBTUH); // Check output has decreased

	// test linear along Tin
	checkPoint = { -13.0, 176.0, 36.0, 65872.597448 };
	outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint);
	ASSERTTRUE(checkPoint.outputBTUH < outputBTUH); // Check output has increased
	
	// test linear along Tin
	checkPoint = { -13.0, 176.0, 100.0, 55913.249232 };
	outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint);
	ASSERTTRUE(checkPoint.outputBTUH > outputBTUH); // Check output has decreased

	// test linear along Tin
	checkPoint = { 104.0, 176.0, 100., 136564.470884 };
	outputBTUH = getCapacitySP_F_BTUHR(hpwh, checkPoint);
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, outputBTUH));  // Check output is relativly constant at the high airtemp range.

	// test const along Tair
	checkPoint = { 110.0, 140.0, 62.6, 136480 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));

	// test const along Tair
	checkPoint = { 114.0, 176.0, 84.2, 136564.470884 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacitySP_F_BTUHR(hpwh, checkPoint)));
}


int main(int, char*)
{
	testQAHVMatchesDataMap(); // Test QAHV grid data input correctly
	testQAHVExtrapolates(); // Test QAHV grid data input correctly

	testCXA15MatchesDataMap();  //Test we can set the correct capacity for specific equipement that matches the data
	testCXA30MatchesDataMap();  //Test we can set the correct capacity for specific equipement that matches the data

	// MP tests
	testCXV5MPMatchesDataMap(); //Test we can set the correct capacity for specific equipement that matches the data
	testCXA10MPMatchesDataMap(); //Test we can set the correct capacity for specific equipement that matches the data
	testCXA15MPMatchesDataMap(); //Test we can set the correct capacity for specific equipement that matches the data
	testCXA20MPMatchesDataMap(); //Test we can set the correct capacity for specific equipement that matches the data
	testCXA25MPMatchesDataMap(); //Test we can set the correct capacity for specific equipement that matches the data
	testCXA30MPMatchesDataMap(); //Test we can set the correct capacity for specific equipement that matches the data

	testRheemHPHD135();
	testRheemHPHD60();

	testNyleC60AMP();
	testNyleC90AMP();
	testNyleC125AMP();
	testNyleC185AMP();
	testNyleC250AMP();

	//Made it through the gauntlet
	return 0;
}