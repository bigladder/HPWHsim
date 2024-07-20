
import os
import sys
import subprocess
from pathlib import Path

#
def run(repo_path, model_spec, model_name, test_name, dest_path):

	os.chdir(os.path.join(repo_path, "test"))

	app_path = os.path.join(repo_path, "build", "src", "hpwh", "hpwh")

	run_list = [app_path, 'run', '-s', model_spec, '-m', model_name, '-t', test_name, '-d', dest_path ]
	print(run_list)

	result = subprocess.run(run_list, stdout = subprocess.PIPE, text = True)
	print(result.stdout)

# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1

	if n_args == 5:
		repo_path = sys.argv[1]
		model_spec = sys.argv[2]
		model_name = sys.argv[3]
		test_name = sys.argv[4]
		dest_path = sys.argv[5]

		run(repo_path, model_spec, model_name, test_name, dest_path)

	else:
		print('run_simulation arguments:')
		print('1. path to root of repo')
		print('2. model specification (Preset or File)')
		print('3. model name')
		print('4. test name')
		print('5. destination path')
