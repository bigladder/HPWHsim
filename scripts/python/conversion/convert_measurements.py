import pandas as pd  # type: ignore
in_data_folder = 'test/RE2H65_UEF50/'
out_data_folder = 'test/RE2H65_UEF50/'

in_file_name = in_data_folder + 'RE2H65_UEF50.csv'
out_file_name = out_data_folder + 'measurements_test.csv'

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
in_file = open(in_file_name, 'r')
Lines = in_file.readlines()
in_file.close()

powerSum = 0
drawSum = 0
iSum = 0

out_file = open(out_file_name,"w+")
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
			
			for iCol in range(numTankTs):
				new_columns.append(columns[iColTankT1 + iCol])	

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
   
