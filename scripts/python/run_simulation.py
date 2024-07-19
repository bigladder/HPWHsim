
import os
import sys
import subprocess
from pathlib import Path

repo_path = "/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim"

os.chdir(os.path.join(repo_path, "test"))

app_path = os.path.join(repo_path, "build", "src", "hpwh", "hpwh")
model_spec = 'File'
model_name = 'AeroTherm2023'
test_name = 'RE2H50_UEF67'
dest_path = "./"


run_list = [app_path, 'run', '-s', model_spec, '-m', model_name, '-t', test_name, '-d', './' ]
print(run_list)

result = subprocess.run(run_list, stdout = subprocess.PIPE, text = True)
print(result.stdout)

