# Raw script for converting typical supplied data to format used within HPWHsim for plotting.
# New format parameters definitions could be added.

import os
import sys
from pathlib import Path
import os

test_name = 'villara_24hr67'
format = "LG"

# for plotting
output_column_headers = ["Time(min)",  "AmbientT(C)", "PowerIn(W)",
						 "TankT1(C)", "TankT2(C)", "TankT3(C)", "TankT4(C)", "TankT5(C)", "TankT6(C)",
						 "InletT(C)", "OutletT(C)", "FlowRate(gal/min)"]	

in_data_folder = os.path.join('test/', test_name)
out_data_folder = os.path.join('test/', test_name)

if format == "Villara":
	orig_columns = dict([ ("Time", 0), ("AmbientT", 14), ("Power", 2), ("TankT1", 3), ("InletT", 12), ("OutletT", 13), ("Draw", 10) ])
	power_factor = 1000.
if format == "BradfordWhite":
	orig_columns = dict([ ("Time", 0), ("AmbientT", 1), ("Power", 4), ("TankT1", 5), ("InletT", 11), ("OutletT", 12), ("Draw", 13) ])
	power_factor = 1.		
if format == "LG":
	orig_columns = dict([ ("Time", 0), ("AmbientT", 14), ("Power", 2), ("TankT1", 8), ("InletT", 12), ("OutletT", 13), ("Draw", 10) ])
	power_factor = 1000.

initTime_min = -60					 
numRowsPerMin = 1
numTankTs = 6
tankTsOrder = -1

def convert_measurements(test_dir, data_filename):
	
	in_filename = str(data_filename) + '.csv'
	out_filename = 'measured.csv'
	
	in_file_path = os.path.join(test_dir, in_filename)
	out_file_path = os.path.join(test_dir, out_filename)
	
	# load data
	in_file = open(in_file_path, 'r')
	Lines = in_file.readlines()
	in_file.close()

	powerSum = 0
	drawSum = 0
	iSum = 0

	out_file = open(out_file_path,"w+")
	iMin = -63
	jRow = numRowsPerMin
	iLine = 0
	for line in Lines:
		line = line.strip('\n')
		line_out = ""

		columns = line.split(',')

		if iLine == 0:
			new_columns = output_column_headers
			firstCol = True
			for new_column in new_columns:
				if firstCol:			
					line_out = new_column
					firstCol = False
				else:
					line_out = line_out + "," + new_column
			out_file.writelines(line_out + "\n")
			
		else:			
			powerSum = powerSum + power_factor* float(columns[orig_columns["Power"]].strip('\n'))
			drawSum = drawSum + float(columns[orig_columns["Draw"]].strip('\n'))
			iSum = iSum + 1
			
			jRow = jRow + 1
			if jRow >= numRowsPerMin:
				new_columns = []		
				
				new_columns.append(str(iMin))
				new_columns.append(columns[orig_columns["AmbientT"]])
				new_columns.append(str(powerSum / iSum))
				
				tankT_sum = 0
				for iCol in range(numTankTs):
					new_columns.append(columns[orig_columns["TankT1"] + tankTsOrder * iCol])
					tankT_sum = tankT_sum + float(columns[orig_columns["TankT1"] + tankTsOrder * iCol].strip('\n'))

				#new_columns.append(str(tankT_sum / numTankTs))
							
				if drawSum > 0:
					new_columns.append(columns[orig_columns["InletT"]])
					new_columns.append(columns[orig_columns["OutletT"]])
					new_columns.append(str(drawSum / iSum))
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
						
				if iMin >= 0:
					out_file.writelines(line_out + "\n")
				iMin = iMin + 1
				jRow = 0
				
				powerSum = 0
				drawSum = 0
				iSum = 0
											
		iLine = iLine + 1
			
	out_file.close()
	
#  main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1
    if n_args == 2:
      test_dir = Path(sys.argv[1])
      data_filename = Path(sys.argv[2])
      convert_measurements(test_dir, data_filename)
    else:
      sys.exit(
        "Expected two arguments: test_dir data_filename"
        )
  
