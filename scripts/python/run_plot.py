
import os
import sys
import subprocess
from pathlib import Path
from run_simulation import run
from add_plots import add

#
repo_path = "/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim"
model_spec = 'File'
model_name = 'AeroTherm2023'
test_name = 'RE2H50_UEF67'
dest_name = 'aplot.html'

dest_path = os.path.join(repo_path, "build", "test", "output")
run(repo_path, model_spec, model_name, test_name, dest_path)

#
test_path = os.path.join(repo_path, "test")
measured_path = os.path.join(test_path, test_name, "measurements.csv")

build_path = os.path.join(repo_path, "build", "test", "output")
simulated_path=os.path.join(build_path, test_name + '_' + model_spec + '_' + model_name + ".csv")

output_path = os.path.join(dest_path, dest_name)

print(measured_path)
print(simulated_path)
print(dest_path)
add(measured_path, simulated_path, output_path)
