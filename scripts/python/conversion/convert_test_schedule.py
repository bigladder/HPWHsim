# Raw script for converting typical supplied data to a HPWHsim draw schedule.

import os
import sys
from pathlib import Path

setpointT_C = 51.1
initialTankT_C = 51.1
initTime_min = -60
numRowsPerMin = 1

#
def convert_draw_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	out_filename = 'drawschedule.csv'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	out_file = open(out_file_path,"w+")

	out_file.writelines("default 0\n")
	out_file.writelines("time(min),flow(gal/min)\n")

	flowRate_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = initTime_min
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			flowRate_sum = flowRate_sum + float(columns[10].strip('\n'))
			nLines = nLines + 1
			jLine = jLine + 1
			if jLine >= numRowsPerMin:
				if flowRate_sum > 0:
					if iMin >= 0:
						out_file.writelines(f"{iMin}," + format(flowRate_sum / nLines, '.3f') + '\n')
				jLine = 0
				nLines = 0
				flowRate_sum = 0
				iMin = iMin + 1
		first = False
	out_file.close()

#
def convert_ambientT_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	out_filename = 'ambientTschedule.csv'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	out_file = open(out_file_path,"w+")

	first = True
	nLines = 0
	ambientT_sum = 0
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[14].strip('\n'))
			nLines = nLines + 1
		first = False
	ambientT = ambientT_sum / nLines
	out_file.writelines(f"default " + format(ambientT, '.3f') + '\n')	

	out_file.writelines("time(min),temperature(C)\n")
	
	ambientT_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = initTime_min
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[14].strip('\n'))
			nLines = nLines + 1
			jLine = jLine + 1
			if jLine >= numRowsPerMin:
				if iMin >= 0:
					out_file.writelines(f"{iMin}," + format(ambientT_sum / nLines, '.3f') + '\n')	
				ambientT_sum = 0
				nLines = 0
				jLine = 0
				iMin = iMin + 1
		first = False

	out_file.close()

#
def convert_evaporatorT_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	out_filename = 'evaporatorTschedule.csv'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	out_file = open(out_file_path,"w+")

	# get default (average)
	first = True
	nLines = 0
	ambientT_sum = 0
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[14].strip('\n'))
			nLines = nLines + 1
		first = False
	ambientT = ambientT_sum / nLines
	out_file.writelines(f"default " + format(ambientT, '.3f') + '\n')	

	# table header
	out_file.writelines("time(min),temperature(C)\n")
	
	ambientT_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = initTime_min
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[14].strip('\n'))
			nLines = nLines + 1
			jLine = jLine + 1
			if jLine >= numRowsPerMin:				
				if iMin >= 0:
					out_file.writelines(f"{iMin}," + format(ambientT_sum / nLines, '.3f') + '\n')	
				ambientT_sum = 0
				nLines = 0
				jLine = 0
				iMin = iMin + 1
		first = False

	out_file.close()
	

# inletT schedule
def convert_inletT_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	out_filename = 'inletTschedule.csv'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	out_file = open(out_file_path,"w+")

	# get default (average)
	nLines = 0
	first = True
	inletT_sum = 0
	for line in Lines:
		if not first:
			columns = line.split(',')
			flowRate = float(columns[10].strip('\n'))			
			if flowRate != 0:
				inletT_sum = inletT_sum  + float(columns[12].strip('\n'))
				nLines = nLines + 1
		first = False
	out_file.writelines("default " + format(inletT_sum / nLines, '.3f') + '\n')	

	# table header
	out_file.writelines("time(min),temperature(C)\n")
	
	inletT_sum = 0
	flowRate_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = initTime_min
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			flowRate = float(columns[10].strip('\n'))
			if flowRate != 0:
				inletT_sum = inletT_sum + float(columns[12].strip('\n'))
				flowRate_sum = flowRate_sum + flowRate		
			nLines = nLines + 1
			jLine = jLine + 1
			if jLine >= numRowsPerMin:
				if flowRate_sum != 0:
					if iMin >= 0:
						out_file.writelines(f"{iMin}," + format(inletT_sum / nLines, '.3f') + '\n')
				flowRate_sum = 0
				inletT_sum = 0
				nLines = 0
				jLine = 0
				iMin = iMin + 1
		first = False
		
# DR schedule
def create_DR_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	out_filename = 'DRschedule.csv'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	out_file = open(out_file_path,"w+")
	out_file.writelines(f"default 0\n")
	out_file.close()


# test info
def create_test_into(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	out_filename = 'testInfo.txt'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	# count minutes
	jLine = numRowsPerMin - 1
	iMin = initTime_min
	testTime_min = 0
	first = True
	for line in Lines:
		if not first:
			jLine = jLine + 1
			if jLine >= numRowsPerMin:
				jLine = 0
				iMin = iMin + 1
				if iMin >= 0:
					testTime_min = testTime_min + 1
		first = False
		
	out_file = open(out_file_path,"w+")
	out_file.writelines(f"setpoint {setpointT_C}\n")
	out_file.writelines(f"length_of_test {iMin}\n")
	out_file.writelines(f"initialTankT_C {initialTankT_C}\n")
	out_file.close()
	
#  main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1
    if n_args == 2:
      test_dir = Path(sys.argv[1])
      data_filename = Path(sys.argv[2])
      convert_draw_schedule(test_dir,data_filename)
      convert_ambientT_schedule(test_dir,data_filename)
      convert_evaporatorT_schedule(test_dir,data_filename)
      convert_inletT_schedule(test_dir,data_filename)
      create_DR_schedule(test_dir,data_filename)
      create_test_into(test_dir,data_filename)
    else:
      sys.exit(
        "Expected two arguments: test_dir data_filename"
        )
  
