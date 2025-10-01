# Calls the hpwh command 'run' to perform a test simulation HPWH model.

import os
import sys
import subprocess
from pathlib import Path

#
def simulate(data):

	model_spec = data['model_spec']
	model_id_or_filepath = data['model_id_or_filepath']
	build_dir = data['build_dir']
	test_dir = data['test_dir']
	
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	#os.chdir("../../../test")

	app_cmd = os.path.join(abs_build_dir, 'src', 'hpwh', 'hpwh')

	output_dir = os.path.join(abs_build_dir, 'test', 'output')

	if not os.path.exists(output_dir):
		os.mkdir(output_dir)
      
	if model_spec == 'JSON':
		run_list = [app_cmd, 'run', '-s', model_spec, '-f', model_id_or_filepath, '-t', test_dir, '-d', output_dir]
	else:	
		run_list = [app_cmd, 'run', '-s', model_spec, '-m', model_id_or_filepath, '-t', test_dir, '-d', output_dir]
	print(run_list)
	result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
	os.chdir(orig_dir)

# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1
	if n_args == 4:
		data['model_spec'] = sys.argv[1]
		data['model_id_or_filepath'] = sys.argv[2]
		data['build_dir'] = sys.argv[4]
		data['test_dir'] = sys.argv[3]

		simulate(data)

	else:
		print('run_simulation arguments:')
		print('1. model specification (Preset, Legacy, JSON)')
		print('2. model id or JSON filepath')
		print('3. build directory')
		print('4. test directory')
		