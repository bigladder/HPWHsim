


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
	checkPoint = { 10.0, 60.0, 8.8669850 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 10.0, 102.0, 9.9042871 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 73.3333333, 88.0, 19.55298778 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 73.3333333, 130.0, 18.5934312 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 116.0, 25.079793448 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA10MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_10_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 61.66666667, 74.0, 29.38562077 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 61.66666667, 116.0, 27.6816012 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 74.0, 38.0960750 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 116.0, 35.5152451 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 88.0, 46.7514889 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 130.0, 43.159291 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}
void testCXA15MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_15_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 61.66666667, 74.0, 38.66064456 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 61.66666667, 116.0, 37.23973833 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 74.0, 48.6230256 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 116.0, 46.417962 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 74.0, 59.58111748 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 116.0, 56.66157 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA20MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_20_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 61.66666667, 74.0, 57.068742 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 61.66666667, 116.0, 53.5011483 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 74.0, 74.07692454 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 116.0, 69.0103440 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 105.0, 88.0, 90.8779510 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 105.0, 130.0, 83.8988727 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA25MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_25_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 61.66666667, 74.0, 67.43986302 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 61.66666667, 116.0, 63.79784501 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 74.0, 86.115911008 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 116.0, 81.122226086 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 88.0, 104.91458956 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 105.0, 130.0, 97.07718310 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
}

void testCXA30MPMatchesDataMap() {
	HPWH hpwh;
	string input = "ColmacCxA_30_MP";
	performancePointMP checkPoint;

	// get preset model 
	getHPWHObject(hpwh, input);

	// test some points outside of defrost ////////////////
	checkPoint = { 61.66666667, 74.0, 76.6435478 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));
	
	checkPoint = { 61.66666667, 116.0, 73.78295058 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 74.0, 97.12906673 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 83.3333333, 116.0, 91.9232593 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 74.0, 119.1488223395 };
	ASSERTTRUE(relcmpd(checkPoint.outputBTUH, getCapacityMP_F_KW(hpwh, checkPoint)));

	checkPoint = { 105.0, 116.0, 113.043855398 };
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

	//Made it through the gauntlet
	return 0;
}