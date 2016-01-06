

options = -g -Wall


testTool.x : main.cc HPWH.o
	g++ main.cc -o testTool.x HPWH.o $(options) -I$(CURDIR)

#test.x	: test.cc HPWH.o
#	g++ test.cc -o test.x HPWH.o $(options) -I$(CURDIR)
	
HPWH.o	: HPWH.cc HPWH.hh 
	g++ -c HPWH.cc $(options)

clean	:
	rm *.o *.x
