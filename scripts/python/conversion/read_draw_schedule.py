import os

test_name = 'RE2H80_UEF50'

in_data_folder = os.path.join('test/', test_name)
out_data_folder = os.path.join('test/', test_name)

in_file_name = test_name + '.csv'
out_file_name = 'drawschedule.csv'

in_file_path = os.path.join(in_data_folder, in_file_name)
out_file_path = os.path.join(out_data_folder, out_file_name)

# load data
in_file = open(in_file_path, 'r')
Lines = in_file.readlines()
in_file.close()

out_file = open(out_file_path,"w+")

out_file.writelines("default 0\n")
out_file.writelines("minutes,flow\n")

i = 1
j = 0
k = 0
fr_sum = 0
for line in Lines:
	if k > 0:
		columns = line.split(',')
		flowrate = float(columns[13].strip('\n'))
		j = j + 1
		if k == 1:
			if flowrate != 0:
				out_file.writelines(f"{i},{flowrate}\n")
			i = i + 1
			j = 0
		else:	
			fr_sum = fr_sum + flowrate
			if j > 5:
				flowrate = fr_sum / 6
				if flowrate != 0:
					out_file.writelines(f"{i},{flowrate}\n")
				fr_sum = 0
				i = i + 1
				j = 0	
	k = k + 1
out_file.close()
   
