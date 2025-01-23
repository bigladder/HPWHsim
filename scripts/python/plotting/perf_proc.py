import os
import sys

import pandas as pd  # type: ignore
import json
from dash import Dash, dcc, html, Input, Output, State, callback, set_props
import plotly.express as px
import plotly.graph_objects as go

import plotly.io as io
from scipy.optimize import least_squares
import numpy as np
import math
import time
from perf_plot import plot
import multiprocessing as mp
from pathlib import Path

def perf_proc(fig):
	
	
	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}
	fig.update_layout(clickmode='event+select')
		
	app.layout = [

			dcc.Graph(id='test-graph', figure=fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'} ),
			
	]
	
	app.run(debug=True, use_reloader=False, port = perf_proc.port_num)

perf_proc.port_num = 8051

# Runs a simulation and generates plot
def launch_perf_plot(model_name):

	orig_dir = str(Path.cwd())

	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	model_path = os.path.join(abs_repo_test_dir, "models_json", model_name + ".json") 
	print(model_path)
	
	print("creating plot...")
	plotter = plot(model_path)
	time.sleep(1)
	
	if launch_perf_plot.proc != -1:
		print("killing current dash for plotting performance...")
		launch_perf_plot.proc.kill()
		time.sleep(1)
		
	launch_perf_plot.proc = mp.Process(target=perf_proc, args=(plotter.fig, ), name='perf-proc')
	time.sleep(1)
	print("launching dash for plotting performance...")
	launch_perf_plot.proc.start()
	time.sleep(2)
	   
	results = {}
	results["port_num"] = perf_proc.port_num
	return results

launch_perf_plot.proc = -1
