headers = global.h HPWH.h
objects = main.o HPWH.o

#options = -g
options = -g -Wall
#options = -Wshadow
#options = -pg -g
#options = -pg -O3
#options = -O3




hpwhTestTool.x  : $(objects)
	  cc -o hpwhTestTool.x $(objects) $(options) -lm
	  
	  
main.o  : main.c $(headers)
	  cc -c main.c $(options) -I$(CURDIR)
	  
HPWH.o  : HPWH.c HPWH.h global.h
	  cc -c HPWH.c $(options) -I$(CURDIR)

clean :
	 rm hpwhTestTool.x $(objects)
