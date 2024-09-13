import os
import sys
import subprocess
from pathlib import Path


#
def measure(repo_path, model_spec, model_name, results_file, draw_profile):
    orig_dir = str(Path.cwd())
    os.chdir(os.path.join(repo_path, "test"))

    app_path = os.path.join(repo_path, "build", "src", "hpwh", "hpwh")
    output_path = os.path.join(repo_path, "build", "test", "output")

    run_list = [app_path, 'measure', '-s', model_spec, '-m', model_name, '-d', output_path, '-r', results_file, '-n']
    if draw_profile != "auto":
        run_list.append('-p')
        run_list.append(draw_profile)       
    print(run_list)

    result = subprocess.run(run_list, stdout=subprocess.PIPE, text=True)
    print("result: " + result.stdout)

    os.chdir(orig_dir)

# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args > 4:
        repo_path = sys.argv[1]
        model_spec = sys.argv[2]
        model_name = sys.argv[3]
        results_file = sys.argv[4]
        
        if n_args > 5:
            draw_profile = sys.argv[5]
        else:
            draw_profile = "auto"
            
        measure(repo_path, model_spec, model_name, results_file, draw_profile)

    else:
        print('measure arguments:')
        print('1. path to root of repo')
        print('2. model specification (Preset or File)')
        print('3. model name')
        print('4. results filename')
        print('5. draw profile (optional)')
