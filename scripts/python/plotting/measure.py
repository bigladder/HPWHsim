# Calls the hpwh command 'measure' to determine HPWH metrics from standard 24-hr test.
# Results are written to a text file in the output directory.

import os
import sys
import subprocess
from pathlib import Path

#
def measure(model_spec, model_name, build_dir, draw_profile):

	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	os.chdir("../../../test")
	    
	app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
	output_dir = os.path.join(abs_build_dir , "test", "output")
	if not os.path.exists(output_dir):
		os.mkdir(output_dir)
         
	results_file = os.path.join(output_dir, "results.txt")
	run_list = [app_cmd, 'measure', '-s', model_spec, '-m', model_name, '-d', output_dir, '-r', results_file]
	if draw_profile != "auto":
		run_list.append('-p')
		run_list.append(draw_profile)

	print(run_list)
	result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
	os.chdir(orig_dir)

# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args > 2:
        model_spec = sys.argv[1]
        model_name = sys.argv[2]
        build_dir = sys.argv[3]

    draw_profile = "auto"
    if n_args > 3:
        draw_profile  = sys.argv[4]

        measure(model_spec, model_name, build_dir, draw_profile)

    else:
        print('measure arguments:')
        print('1. model specification (Preset or File)')
        print('2. model name')
        print('3. build directory')
        print('4. draw profile')
