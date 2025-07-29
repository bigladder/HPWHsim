# Calls the hpwh command 'run' to perform a test simulation HPWH model.

import os
import sys
import subprocess
from pathlib import Path

#
def simulate(data):

	model_spec = data['model_spec']
	model_name = data['model_name']
	build_dir = data['build_dir']
	test_dir = data['test_dir']

	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	os.chdir("../../../test")

	app_cmd = os.path.join(abs_build_dir, 'src', 'hpwh', 'hpwh')

	output_dir = os.path.join(abs_build_dir, 'test', 'output')

	if not os.path.exists(output_dir):
		os.mkdir(output_dir)
      
	if model_spec == 'JSON':
		gui_dir = os.path.join(output_dir, "gui")
		filepath = os.path.join(gui_dir, model_name)
		if not os.path.exists(filepath + ".json"):
			print("opening original")
			filepath = os.path.join("models_json", model_name)
		run_list = [app_cmd, 'run', '-s', model_spec, '-f', filepath, '-t', test_dir, '-d', output_dir]
	else:	
		run_list = [app_cmd, 'run', '-s', model_spec, '-m', model_name, '-t', test_dir, '-d', output_dir]
	result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
	os.chdir(orig_dir)

# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1
	if n_args == 4:
		data['model_spec'] = sys.argv[1]
		data['model_name'] = sys.argv[2]
		data['build_dir'] = sys.argv[4]
		data['test_dir'] = sys.argv[3]

		simulate(data)

	else:
		print('run_simulation arguments:')
		print('1. model specification (Preset or File)')
		print('2. model name')
		print('3. build directory')
		print('4. test directory')
		