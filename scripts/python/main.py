
import os
import sys 
import subprocess
from pathlib import Path
from simulate import simulate
from plot import plot

def main():
	#
	orig_dir = str(Path.cwd())
	os.chdir('../..')
	repo_path = str(Path.cwd())
	print("path is " + repo_path)

	model_spec = 'File'
	model_name = 'AeroTherm2023'
	test_name = 'RE2H50_UEF67'
	plot_name = 'aplot.html'
	measurements_name = 'measurements.csv'
	output_name = test_name + '_' + model_spec + '_' + model_name + ".csv"

	test_dir = os.path.join(repo_path, "test")
	output_dir = os.path.join(repo_path, "build", "test", "output")
	simulate(repo_path, model_spec, model_name, test_name, output_dir)

	#
	measured_path = os.path.join(test_dir, test_name, measurements_name)
	simulated_path = os.path.join(output_dir, output_name)
	plot_path = os.path.join(output_dir, plot_name)

	print(measured_path)
	print(simulated_path)
	print(plot_path)

	plot(measured_path, simulated_path, plot_path)
	
	os.chdir(orig_dir)
