
import os
import sys 
import subprocess
from pathlib import Path
from simulate import simulate
from plot import plot

def main():
	#
	orig_path = str(Path.cwd())
	os.chdir('../..')
	repo_path = str(Path.cwd())
	print("path is " + repo_path)

	model_spec = 'File'
	model_name = 'AeroTherm2023'
	test_name = 'RE2H50_UEF67'
	dest_name = 'aplot.html'

	output_path = os.path.join(repo_path, "build", "test", "output")
	simulate(repo_path, model_spec, model_name, test_name, output_path)

	#
	test_path = os.path.join(repo_path, "test")
	measured_path = os.path.join(test_path, test_name, "measurements.csv")

	build_path = os.path.join(repo_path, "build", "test", "output")
	simulated_path=os.path.join(build_path, test_name + '_' + model_spec + '_' + model_name + ".csv")

	output_path = os.path.join(output_path, dest_name)

	print(measured_path)
	print(simulated_path)
	print(output_path)
	plot(measured_path, simulated_path, output_path)
	
	os.chdir(orig_path)
