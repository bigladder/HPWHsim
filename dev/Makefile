

options = -g -Wall


test.x	: main.cc HPWH.o 
	g++ main.cc -o test.x HPWH.o $(options) -I$(CURDIR)
	
	
HPWH.o	: HPWH.cc HPWH.hh 
	g++ -c HPWH.cc $(options)


	
clean	:
	rm *.o
