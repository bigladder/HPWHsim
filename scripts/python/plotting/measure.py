# Calls the hpwh command 'measure' to determine HPWH metrics from standard 24-hr test.
# Results are written to a text file in the output directory.

import os
import sys
import subprocess
from pathlib import Path

#
def measure(data):

	model_spec = data['model_spec']
	model_id_or_filepath = data['model_id_or_filepath'] # model id (short name) or filepath
	build_dir = data['build_dir']
	draw_profile = data['draw_profile']
      
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	os.chdir("../../../test")
	    
	app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
	output_dir = os.path.join(abs_build_dir , "test", "output")
	if not os.path.exists(output_dir):
		os.mkdir(output_dir)
         
	results_filename = "results"
	if model_spec == 'JSON':
		run_list = [app_cmd, 'measure', '-s', model_spec, '-f', model_id_or_filepath, '-d', output_dir, '-r', results_filename]
	else:
		run_list = [app_cmd, 'measure', '-s', model_spec, '-m', model_id_or_filepath, '-d', output_dir, '-r', results_filename]
	if draw_profile != "auto":
		run_list.append('-p')
		run_list.append(draw_profile.replace(" ", ""))
	print(run_list)
	result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
	os.chdir(orig_dir)

# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1
	
	data = {}
	if n_args > 2:
		data['model_spec'] = sys.argv[1]
		data['model_id_or_filepath'] = sys.argv[2]
		data['build_dir'] = sys.argv[3]

		data['draw_profile'] = "auto"
		
		if n_args > 3:
			data['draw_profile'] = sys.argv[4]

		measure(data)

	else:
			print('measure arguments:')
			print('1. model specification (Preset, Legacy, JSON)')
			print('2. model id or JSON filepath')
			print('3. build directory')
			print('4. draw profile')
