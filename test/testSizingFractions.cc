

/*unit test for isTankSizeFixed() functionality
 *
 *
 *
 */
#include "HPWH.hh"
#include "testUtilityFcts.cc"

#include <iostream>
#include <string> 

#define SIZE (double)HPWH::CONDENSITY_SIZE

using std::cout;
using std::string;

void testScalableSizingFract();
void testSandenSizingFract();
void testColmacSizingFract();
void testHPTU50SizingFract();
void test220eSizingFract();

int main(int argc, char *argv[])
{
	testScalableSizingFract();
	testSandenSizingFract();
	testColmacSizingFract();
	testHPTU50SizingFract();
	test220eSizingFract();
	//Made it through the gauntlet
	return 0;
}


void testScalableSizingFract() {
	HPWH hpwh;
	double AF, pU;
	double AF_answer = 4. / SIZE;

	string input = "TamScalable_SP"; // Just a compressor with R134A
	getHPWHObject(hpwh, input); 	// get preset model 

	hpwh.getSizingFractions( AF, pU);
	ASSERTTRUE(cmpd(AF, AF_answer));
	ASSERTTRUE(cmpd(pU, 1 - 1. / SIZE));
}

void testSandenSizingFract() {
	HPWH hpwh;
	double AF, pU;
	double AF_answer = 8. / SIZE;

	string input = "Sanden80"; // Just a compressor with R134A
	getHPWHObject(hpwh, input); 	// get preset model 

	hpwh.getSizingFractions(AF, pU);
	ASSERTTRUE(cmpd(AF, AF_answer));
	ASSERTTRUE(cmpd(pU, 1 - 1. / SIZE));
}

void testColmacSizingFract() {
	HPWH hpwh;
	double AF, pU;
	double AF_answer = 4. / SIZE;

	string input = "ColmacCxV_5_SP"; // Just a compressor with R134A
	getHPWHObject(hpwh, input); 	// get preset model 

	hpwh.getSizingFractions(AF, pU);
	ASSERTTRUE(cmpd(AF, AF_answer));
	ASSERTTRUE(cmpd(pU, 1 - 1. / SIZE));
}

void testHPTU50SizingFract() {
	HPWH hpwh;
	double AF, pU;
	double AF_answer = (1. + 2. + 3. + 4.) / 4. / SIZE;

	string input = "AOSmithHPTU50"; // Just a compressor with R134A
	getHPWHObject(hpwh, input); 	// get preset model 

	hpwh.getSizingFractions(AF, pU);
	ASSERTTRUE(cmpd(AF, AF_answer));
	ASSERTTRUE(cmpd(pU, 1.));
}

void test220eSizingFract() {
	HPWH hpwh;
	double AF, pU;
	double AF_answer = (5. + 6.) / 2. / SIZE;

	string input = "Stiebel220e"; // Just a compressor with R134A
	getHPWHObject(hpwh, input); 	// get preset model 

	hpwh.getSizingFractions(AF, pU);
	ASSERTTRUE(cmpd(AF, AF_answer));
	ASSERTTRUE(cmpd(pU, 1 - 1./SIZE));
}