import os
import sys
import subprocess
from pathlib import Path


#
def measure(repo_path, model_spec, model_name):
    orig_dir = str(Path.cwd())
    os.chdir(os.path.join(repo_path, "test"))

    app_path = os.path.join(repo_path, "build", "src", "hpwh", "hpwh")

    run_list = [app_path, 'measure', '-s', model_spec, '-m', model_name]
    print(run_list)

    result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
    print("result: " + result.stdout)

    os.chdir(orig_dir)

# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 5:
        repo_path = sys.argv[1]
        model_spec = sys.argv[2]
        model_name = sys.argv[3]

        measure(repo_path, model_spec, model_name)

    else:
        print('measure arguments:')
        print('1. path to root of repo')
        print('2. model specification (Preset or File)')
        print('3. model name')
