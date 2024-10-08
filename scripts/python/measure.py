import os
import sys
import subprocess
from pathlib import Path


#
def measure(model_spec, model_name, build_dir):

    app_cmd = os.path.join(build_dir, 'src', 'hpwh', 'hpwh')
    output_dir = os.path.join(build_dir, "test", "output")
    results_file = os.path.join(output_dir, "results.txt")
    run_list = [app_cmd, 'measure', '-s', model_spec, '-m', model_name, '-d', output_dir, '-r', results_file, '-n']

    result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
    print("result: " + result.stdout)


# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 7:
        model_spec = sys.argv[1]
        model_name = sys.argv[2]
        build_dir = sys.argv[3]

        measure(model_spec, model_name, build_dir)

    else:
        print('measure arguments:')
        print('1. model specification (Preset or File)')
        print('2. model name')
        print('3. build directory')

