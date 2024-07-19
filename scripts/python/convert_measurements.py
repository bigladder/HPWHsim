import pandas as pd  # type: ignore
in_data_folder = '../../test/RE2H50_UEF67/'
out_data_folder = '../../test/RE2H50_UEF67/'

in_file_name = in_data_folder + 'RE2H50_UEF67.csv'
out_file_name = out_data_folder + 'measurements.csv'

# load data
in_file = open(in_file_name, 'r')
Lines = in_file.readlines()
in_file.close()

out_file = open(out_file_name,"w+")
imin = 0
j = 5
k = 0
for line in Lines:
	line = line.strip('\n')
	line_out = ""
	j = j + 1
	if k==0:
		line_out = line + ",minutes"
		out_file.writelines(line_out+"\n")
		imin = imin + 1
	else:			
			columns = line.split(',')
			first = True
			for column in columns:
				if first:
					line_out = column
				else:
					line_out = line_out + "," + column
				first = False	
			line_out = f"{line_out},{imin}"
			if j > 5:
				out_file.writelines(line_out+"\n")
				imin = imin + 1
				j = 0
	k = k + 1
		
out_file.close()
   
