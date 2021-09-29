


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
double getCapacityMP_F_KW(HPWH &hpwh, performancePointMP &point) {
	return hpwh.getCompressorCapacity(point.tairF, point.tinF, point.tinF, HPWH::UNITS_KW, HPWH::UNITS_F); 
}

void testCXA15MatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_15_SP";
	double capacity_kW, capacity_BTUperHr;
	// get preset model 
	getHPWHObject(hpwh, input);

	// test hot //////////////////////////
	double waterTempF = 100;
	double airTempF = 90;
	double setpointF = 135;
	double capacityData_kW = 49.14121289;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test middle ////////////////
	waterTempF = 70;
	airTempF = 75;
	setpointF = 140;
	capacityData_kW = 42.66070965;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test cold ////////////////
	waterTempF = 50;
	airTempF = 50;
	setpointF = 120;
	capacityData_kW = 33.28099803;

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
	double waterTempF = 100;
	double airTempF = 90;
	double setpointF = 135;
	double capacityData_kW = 97.81336856;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test middle ////////////////
	waterTempF = 70;
	airTempF = 75;
	setpointF = 140;
	capacityData_kW = 84.54702716;

	capacity_BTUperHr = hpwh.getCompressorCapacity(airTempF, waterTempF, setpointF, HPWH::UNITS_BTUperHr, HPWH::UNITS_F);
	capacity_kW = hpwh.getCompressorCapacity(F_TO_C(airTempF), F_TO_C(waterTempF), F_TO_C(setpointF));

	ASSERTTRUE(relcmpd(KWH_TO_BTU(capacityData_kW), capacity_BTUperHr));
	ASSERTTRUE(relcmpd(capacityData_kW, capacity_kW));

	// test cold ////////////////
	waterTempF = 50;
	airTempF = 50;
	setpointF = 120;
	capacityData_kW = 65.76051043;

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

int main(int argc, char *argv[])
{
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

	//Made it through the gauntlet
	return 0;
}