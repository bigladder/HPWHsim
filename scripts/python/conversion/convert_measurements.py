import pandas as pd  # type: ignore

import os

test_name = 'RE2H50_UEF67'

in_data_folder = os.path.join('test/', test_name)
out_data_folder = os.path.join('test/', test_name)

in_file_name = test_name + '.csv'
out_file_name = 'measurements_test.csv'

in_file_path = os.path.join(in_data_folder, in_file_name)
out_file_path = os.path.join(out_data_folder, out_file_name)

iColTime = 0
iColAmbientT = 1
iColPower = 4

iColTankT1 = 5

iColInletT = 11
iColOutletT = 12
iColDraw = 13

numRowsPerMin = 6
numTankTs = 6

# load data
in_file = open(in_file_path, 'r')
Lines = in_file.readlines()
in_file.close()

powerSum = 0
drawSum = 0
iSum = 0

out_file = open(out_file_path,"w+")
iMin = 0
jRow = numRowsPerMin
iLine = 0
for line in Lines:
	line = line.strip('\n')
	line_out = ""

	columns = line.split(',')

	if iLine == 0:
		new_columns = ["Time(min)",  "AmbientT(C)", "PowerIn(W)",
								 "TankT1(C)", "TankT2(C)", "TankT3(C)", "TankT4(C)", "TankT5(C)", "TankT6(C)",
								 "InletT(C)", "OutletT(C)", "FlowRate(gal/min)"]	
		firstCol = True
		for new_column in new_columns:
			if firstCol:			
				line_out = new_column
				firstCol = False
			else:
				line_out = line_out + "," + new_column
		out_file.writelines(line_out + "\n")
		
	else:			
		powerSum = powerSum + float(columns[iColPower].strip('\n'))
		drawSum = drawSum + float(columns[iColDraw].strip('\n'))
		iSum = iSum + 1
		
		jRow = jRow + 1
		if jRow >= numRowsPerMin:
			new_columns = []		
			
			new_columns.append(str(iMin))
			new_columns.append(columns[iColAmbientT])
			new_columns.append(str(powerSum / iSum))
			
			tankT_sum = 0
			for iCol in range(numTankTs):
				new_columns.append(columns[iColTankT1 + iCol])
				tankT_sum = tankT_sum + float(columns[iColTankT1 + iCol].strip('\n'))

			#new_columns.append(str(tankT_sum / numTankTs))
						
			if drawSum > 0:
				new_columns.append(columns[iColInletT])
				new_columns.append(columns[iColOutletT])
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
					
			out_file.writelines(line_out + "\n")
			iMin = iMin + 1
			jRow = 0
			
			powerSum = 0
			drawSum = 0
			iSum = 0
										
	iLine = iLine + 1
		
out_file.close()
   
