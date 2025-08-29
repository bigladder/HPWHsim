# Raw script for converting typical supplied data to HPWHsim schedules and measured data.
# uv run convert.py ../../../test/BradfordWhite/AeroThermRE2HP65/RE2HP65_UEF67 RE2HP65_UEF67
import os
import sys
from pathlib import Path

setpointT_C = 51.1
initialTankT_C = 51.1
initTime_min = 0
numRowsPerMin = 6
numTankTs = 6
tankTsOrder = 1

format = "BradfordWhite"

# for plotting
output_column_headers = ["Time(min)",  "AmbientT(C)", "PowerIn(W)",
						 "TankT1(C)", "TankT2(C)", "TankT3(C)", "TankT4(C)", "TankT5(C)", "TankT6(C)",
						 "InletT(C)", "OutletT(C)", "FlowRate(gal/min)"]	

if format == "Villara":
	orig_columns = dict([ ("Time", 0), ("AmbientT", 14), ("Power", 2), ("TankT1", 3), ("InletT", 12), ("OutletT", 13), ("Draw", 10) ])
	power_factor = 1000.
if format == "BradfordWhite":
	orig_columns = dict([ ("Time", 0), ("AmbientT", 1), ("Power", 4), ("TankT1", 5), ("InletT", 11), ("OutletT", 12), ("Draw", 13) ])
	power_factor = 1.		
if format == "LG":
	orig_columns = dict([ ("Time", 0), ("AmbientT", 14), ("Power", 2), ("TankT1", 8), ("InletT", 12), ("OutletT", 13), ("Draw", 10) ])
	power_factor = 1000.

#
def convert_draw_schedule(test_dir, data_filename):
	
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	out_filename = 'drawschedule.csv'
	out_file_path = os.path.join(test_dir, out_filename)
	out_file = open(out_file_path,"w+")

	out_file.writelines("default 0\n")
	out_file.writelines("time(min),flow(gal/min)\n")

	flowRate_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = 0
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			flowRate_sum = flowRate_sum + float(columns[orig_columns["Draw"]].strip('\n'))
			nLines = nLines + 1
			if nLines >= numRowsPerMin:
				if flowRate_sum > 0:
					if iMin >= initTime_min:
						out_file.writelines("{min}, {flowRate_avg:.3f}\n".format(min=iMin - initTime_min, flowRate_avg=flowRate_sum / nLines))
				nLines = 0
				flowRate_sum = 0
				iMin = iMin + 1
		first = False
	out_file.close()

#
def convert_ambientT_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	out_filename = 'ambientTschedule.csv'
	out_file_path = os.path.join(test_dir, out_filename)
	out_file = open(out_file_path,"w+")
	iMin = 0
	first = True
	nLines = 0
	ambientT_sum = 0
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[orig_columns["AmbientT"]].strip('\n'))
			nLines = nLines + 1
		first = False
	out_file.writelines("default {ambientT_avg:.3f}\n".format(ambientT_avg=ambientT_sum / nLines))	

	out_file.writelines("time(min),temperature(C)\n")
	
	ambientT_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = 0
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[orig_columns["AmbientT"]].strip('\n'))
			nLines = nLines + 1
			if nLines >= numRowsPerMin:
				if iMin >= initTime_min:
					out_file.writelines("{min}, {ambientT_avg:.3f}\n".format(min=iMin - initTime_min, ambientT_avg=ambientT_sum / nLines))	
				ambientT_sum = 0
				nLines = 0
				iMin = iMin + 1
		first = False

	out_file.close()

#
def convert_evaporatorT_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	out_filename = 'evaporatorTschedule.csv'
	out_file_path = os.path.join(test_dir, out_filename)

	# load data
	in_file.close()

	out_file = open(out_file_path,"w+")

	# get default (average)
	first = True
	nLines = 0
	ambientT_sum = 0
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[orig_columns["AmbientT"]].strip('\n'))
			nLines = nLines + 1
		first = False
	ambientT = ambientT_sum / nLines
	out_file.writelines("default {T:.3f}\n".format(T=ambientT))	

	# table header
	out_file.writelines("time(min),temperature(C)\n")
	
	ambientT_sum = 0
	nLines = 0
	jLine = numRowsPerMin - 1
	iMin = 0
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			ambientT_sum = ambientT_sum + float(columns[orig_columns["AmbientT"]].strip('\n'))
			nLines = nLines + 1
			if nLines >= numRowsPerMin:				
				if iMin >= initTime_min:
					out_file.writelines("{min}, {ambientT_avg:.3f}\n".format(min=iMin - initTime_min, ambientT_avg=ambientT_sum / nLines))
				ambientT_sum = 0
				nLines = 0
				iMin = iMin + 1
		first = False

	out_file.close()
	

# inletT schedule
def convert_inletT_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	# load data
	out_filename = 'inletTschedule.csv'
	out_file_path = os.path.join(test_dir, out_filename)
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
				inletT_sum = inletT_sum  + float(columns[orig_columns["InletT"]].strip('\n'))
				nLines = nLines + 1
		first = False
	out_file.writelines("default {inletT_avg:.3f}\n".format(inletT_avg=inletT_sum / nLines))	

	# table header
	out_file.writelines("time(min),temperature(C)\n")
	
	inletT_sum = 0
	flowRate_sum = 0
	nLines = 0
	iMin = 0
	first = True
	for line in Lines:
		if not first:
			columns = line.split(',')
			flowRate = float(columns[10].strip('\n'))
			if flowRate != 0:
				inletT_sum = inletT_sum + float(columns[orig_columns["InletT"]].strip('\n'))
				flowRate_sum = flowRate_sum + flowRate		
			nLines = nLines + 1
			if nLines >= numRowsPerMin:
				if flowRate_sum != 0:
					if iMin >= initTime_min:
						out_file.writelines("{min}, {inletT_avg:.3f}\n".format(min=iMin - initTime_min, inletT_avg=inletT_sum / nLines))
				flowRate_sum = 0
				inletT_sum = 0
				nLines = 0
				iMin = iMin + 1
		first = False
		
# DR schedule
def create_DR_schedule(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	out_filename = 'DRschedule.csv'
	out_file_path = os.path.join(test_dir, out_filename)
	out_file = open(out_file_path,"w+")
	out_file.writelines(f"default 0\n")
	out_file.close()


# test info
def create_test_into(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	out_filename = 'testInfo.txt'
	out_file_path = os.path.join(test_dir, out_filename)

	# count minutes
	jLine = numRowsPerMin - 1
	iMin = 0
	testTime_min = 0
	first = True
	for line in Lines:
		if not first:
			jLine = jLine + 1
			if jLine >= numRowsPerMin:
				jLine = 0
				iMin = iMin + 1
				if iMin >= initTime_min:
					testTime_min = testTime_min + 1
		first = False
		
	out_file = open(out_file_path,"w+")
	out_file.writelines(f"setpoint {setpointT_C}\n")
	out_file.writelines(f"length_of_test {iMin - initTime_min}\n")
	out_file.writelines(f"initialTankT_C {initialTankT_C}\n")
	out_file.close()

# measured data
def convert_measured(test_dir, data_filename):
	in_filename = str(data_filename) + '.csv'
	in_file_path = os.path.join(test_dir, in_filename)
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	
	out_filename = 'measured.csv'
	out_file_path = os.path.join(test_dir, out_filename)
	
	powerSum = 0
	drawSum = 0
	ambientT_sum = 0
	outletT_sum = 0
	inletT_sum = 0

	out_file = open(out_file_path,"w+")
	iMin = 0
	first = True
	nLines = 0
	for line in Lines:
		line = line.strip('\n')
		line_out = ""
		columns = line.split(',')

		if first:
			new_columns = output_column_headers
			firstCol = True
			for new_column in new_columns:
				if firstCol:			
					line_out = new_column
					firstCol = False
				else:
					line_out = line_out + "," + new_column
			out_file.writelines(line_out + "\n")
			first = False
			
		else:		
			powerSum = powerSum + power_factor * float(columns[orig_columns["Power"]].strip('\n'))
			drawSum = drawSum + float(columns[orig_columns["Draw"]].strip('\n'))
			ambientT_sum = ambientT_sum + float(columns[orig_columns["AmbientT"]])
			inletT_sum = inletT_sum + float(columns[orig_columns["InletT"]])
			outletT_sum = outletT_sum + float(columns[orig_columns["OutletT"]])
			if nLines  >= numRowsPerMin:
				new_columns = []		
				new_columns.append(str(iMin - initTime_min))
				new_columns.append(str(ambientT_sum / nLines))
				new_columns.append(str(powerSum / nLines))
				
				for iCol in range(numTankTs):
					new_columns.append(columns[orig_columns["TankT1"] + tankTsOrder * iCol])
							
				if drawSum > 0:
					new_columns.append(str(inletT_sum / nLines))
					new_columns.append(str(outletT_sum / nLines))
					new_columns.append(str(drawSum / nLines))
				else:
					new_columns.append("")		
					new_columns.append("")	
					new_columns.append("")	

				firstCol = True
				for new_column in new_columns:
					if firstCol:			
						line_out = new_column
						firstCol = False
					else:
						line_out = line_out + "," + new_column
					
				nLines = 0
						
				if iMin >= initTime_min:
					out_file.writelines(line_out + "\n")
				iMin = iMin + 1
				
				powerSum = 0
				drawSum = 0
				ambientT_sum = 0
				inletT_sum = 0
				outletT_sum = 0										
			nLines = nLines + 1
				
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
		convert_measured(test_dir,data_filename)
	else:
		sys.exit(
		  "Expected two arguments: test_dir data_filename"
		  )
  
