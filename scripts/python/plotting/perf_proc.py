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
from perf_plot import PerfPlotter
import multiprocessing as mp
from pathlib import Path

def perf_proc(plotter, ivar):
	
	perf_proc.plotter = plotter
	perf_proc.plotter.fig.update_layout(clickmode='event+select')
	perf_proc.outletTs = []
	perf_proc.show_outletTs = perf_proc.plotter.is_central
	if perf_proc.show_outletTs:
		i = 0
		for outletT in perf_proc.plotter.T3s:
			perf_proc.outletTs.append({'label': f"{outletT:.2f} \u00B0C", 'value': i})
			i = i + 1
	else:
		perf_proc.outletTs = [{'label': "none", 'value': 0}]
		
	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}

	app.layout = [
		
		html.Div(
			[
					html.Label("display variable", htmlFor="display-dropdown"),
					dcc.Dropdown(options = [	{'label': 'Input Power (W)', 'value': 0}, 
												 		{'label': 'Heating Capacity (W)', 'value': 1},
														{'label': 'COP', 'value': 2}],
														value = ivar, 
														id='display-dropdown',
														style={'width': '50%'},
														clearable=False)
			]),
	
		html.Div(
			[
				html.Label("condenser outlet temperature (\u00B0C)", htmlFor="outletT-dropdown"),
				dcc.Dropdown(options = perf_proc.outletTs,
																value = perf_proc.plotter.i3, 
																id='outletT-dropdown',
																style={'width': '50%'},
																clearable=False,
																)
				
				],
				hidden = not(perf_proc.show_outletTs)
				),
														
		dcc.Graph(id='perf-graph', figure=perf_proc.plotter.fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'} )
			
	]

	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('display-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_value(value):
		
		perf_proc.plotter.draw(value)
		perf_proc.ivar = value
		try:
			with open("contour_prefs.json", 'w') as json_file:
				data = {'variable': value}
				json.dump(data, json_file)			
		except:
			print("failed to write")
			return
		return perf_proc.plotter.fig
	
	@callback(
				Output('perf-graph', 'figure', allow_duplicate=True),
				Input('outletT-dropdown', 'value'),
				prevent_initial_call=True
			)
	def select_outletT(value):
		perf_proc.plotter.i3 = value
		perf_proc.plotter.get_slice()
		perf_proc.plotter.draw(perf_proc.ivar)
		return perf_proc.plotter.fig

	
	app.run(debug=True, use_reloader=False, port = perf_proc.port_num)

perf_proc.port_num = 8051

# Runs a simulation and generates plot
def launch_perf_plot(model_name):

	orig_dir = str(Path.cwd())

	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	model_path = os.path.join(abs_repo_test_dir, "models_json", model_name + ".json") 
	
	print("creating plot...")
		
	try:
			with open("contour_prefs.json", 'r') as json_data:
				data = json.load(json_data)
				ivar = data["variable"]
	except:
			print("failed to load")
			return

	plotter = PerfPlotter()
	plotter.read_data(model_path)
	
	if launch_perf_plot.proc != -1:
		print("killing current dash for plotting performance...")
		launch_perf_plot.proc.kill()
		time.sleep(1)

	plotter.draw(ivar)
	time.sleep(1)
	
	launch_perf_plot.proc = mp.Process(target=perf_proc, args=(plotter, ivar), name='perf-proc')
	print("launching dash for plotting performance...")
	time.sleep(1)

	launch_perf_plot.proc.start()
	time.sleep(2)
	   
	results = {}
	results["port_num"] = perf_proc.port_num
	return results

launch_perf_plot.proc = -1

