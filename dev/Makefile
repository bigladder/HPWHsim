

options = -g -Wall -Wshadow
#options = -O3
#options = -pg


#testTool.x : main.cc HPWH.o
#	g++ main.cc -o testTool.x HPWH.o $(options) -I$(CURDIR)

test.x	: test.cc HPWH.o Makefile
	g++ test.cc -o test.x HPWH.o $(options) -I$(CURDIR)
	
HPWH.o	: HPWH.cc HPWH.hh Makefile
	g++ -c HPWH.cc $(options)

clean	:
	rm *.o *.x
