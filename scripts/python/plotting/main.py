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
import plot_server

# Runs a simulation and generates plot
def call_test(model_spec, model_name, test_dir, build_dir):

	print("running simulation...")
	simulate(model_spec, model_name, test_dir, build_dir)

	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	os.chdir("../../../test")
	abs_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	output_dir = os.path.join(abs_build_dir, "test", "output") 
	if not os.path.exists(output_dir):
		os.mkdir(output_dir)
	    
	measured_path = os.path.join(abs_test_dir, test_dir, "measured.csv")    
	test_name = Path(test_dir).name

	simulated_path = os.path.join(output_dir, test_name + "_" + model_spec + "_" + model_name + ".csv")

	print("creating plot...")
	plotter = plot(measured_path, simulated_path)

	time.sleep(1)	
	
	if call_test.proc != -1:
		print("killing current dash...")
		call_test.proc.kill()
		time.sleep(1)
		
	call_test.proc = mp.Process(target=plot_server.dash_proc, args=(plotter.plot.figure, ), name='dash-proc')	
	print("launching dash...")
	call_test.proc.start()
	time.sleep(1)
	   
	test_results = {}
	test_results["energy_data"] = plotter.energy_data
	test_results["port_num"] = 8050
	return test_results

call_test.proc = -1

# Calls the 24-hr test function.
def call_measure(model_spec, model_name, build_dir, draw_profile):

    measure(model_spec, model_name, build_dir, draw_profile)
    return 'success'