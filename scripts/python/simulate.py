import os
import sys
import subprocess
from pathlib import Path


#
def simulate(model_spec, model_name, test_name, build_dir):
    orig_dir = str(Path.cwd())
    
    test_dir = "../../test"
    os.chdir(test_dir)

    app_cmd = os.path.join(build_dir, 'src', 'hpwh' 'hpwh')
    output_dir = os.path.join(build_dir, 'test', 'output')
    run_list = [app_cmd, 'run', '-s', model_spec, '-m', model_name, '-t', test_name, '-d', output_dir]

    result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
    print("result: " + result.stdout)

    os.chdir(orig_dir)

# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 6:
        model_spec = sys.argv[1]
        model_name = sys.argv[2]
        test_name = sys.argv[3]
        build_dir = sys.argv[5]

        simulate(model_spec, model_name, test_name, build_dir)


    else:
        print('run_simulation arguments:')
        print('1. model specification (Preset or File)')
        print('2. model name')
        print('3. test name')
        print('4. build directory')
