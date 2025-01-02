# Functions to perform composite calls to hpwh

import os
import sys
import subprocess
import multiprocessing as mp
from pathlib import Path
from simulate import simulate
from plot import plot
from measure import measure
from dash import Dash, dcc, html, Input, Output, State, callback
import threading
import plotly
import time
import psutil	
import dash_proc

# Runs a simulation and generates plot
def dash_plot(test_dir, build_dir, show_types, measured_filename, simulated_filename):

	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	output_dir = os.path.join(abs_build_dir, "test", "output") 
	if not os.path.exists(output_dir):
		os.mkdir(output_dir)
	 
	measured_path = ""
	simulated_path = ""

	if show_types & 1:
		if test_dir != "none":
			print("show measured")
			measured_path = os.path.join(abs_repo_test_dir, test_dir, measured_filename)  
			 
	if show_types & 2:
		print("show simulated")
		simulated_path = os.path.join(output_dir, simulated_filename)		 

	print("creating plot...")
	print(f"measured_path: {measured_path}")
	print(f"simulated__path: {simulated_path}")
	plotter = plot(measured_path, simulated_path)
	time.sleep(1)
	
	if dash_plot.proc != -1:
		print("killing current dash...")
		dash_plot.proc.kill()
		time.sleep(1)
		
	dash_plot.proc = mp.Process(target=dash_proc.dash_proc, args=(plotter.plot.figure, ), name='dash-proc')
	time.sleep(1)
	print("launching dash...")
	dash_plot.proc.start()
	time.sleep(1)
	   
	test_results = {}
	test_results["energy_data"] = plotter.energy_data
	test_results["port_num"] = 8050
	return test_results

dash_plot.proc = -1
