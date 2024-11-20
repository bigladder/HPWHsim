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

class dash_thread(threading.Thread):
	def __init__(self, fig):
		super().__init__()
		self.fig = fig
		
	def run(self):
		app = Dash(__name__)

		app.layout = dcc.Graph(id='test-graph', figure=self.fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'} )
		app.run_server(debug=True, use_reloader=False)

		
# Runs a simulation and generates plot
def call_test(model_spec, model_name, test_dir, build_dir):

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
	plot_path =  os.path.join(output_dir, "plot.json") 

	plotter = plot(measured_path, simulated_path)

	thread = dash_thread(plotter.plot.figure)
	thread.start()
	#threading.Thread(target=dash_thread, args=(plot_path, ), name='dash_thread')
	time.sleep(1)

	#thread.join()
	   
	#plot_list = ['poetry','run', 'python', 'plot_server.py', plot_path]
	#result =  mp.Process(target=plot_list, stdout=subprocess.PIPE, text=True)
	return plotter.energy_data

# Calls the 24-hr test function.
def call_measure(model_spec, model_name, build_dir, draw_profile):

    measure(model_spec, model_name, build_dir, draw_profile)
    return 'success'