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
from dash_extensions import WebSocket

def read_file(filename):
	try:
			with open(filename, 'r') as json_file:
				return json.load(json_file)
	except:
			print(f"failed to load {filename}")
			return
	
def write_file(filename, json_data):
	try:
			with open(filename, 'w') as json_file:
				json.dump(json_data, json_file, indent=2)			
	except:
			print(f"failed to write {filename}")
			return
		
def perf_proc(data):
	orig_dir = str(Path.cwd())
	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	perf_proc.fig = {}
	perf_proc.model_data = {}
	perf_proc.model_data_filepath = ""
	if "model_data_filepath" in data:
		perf_proc.model_data_filepath = data["model_data_filepath"]
		perf_proc.model_data = read_file(perf_proc.model_data_filepath)
		
	perf_proc.plotter = PerfPlotter()
	perf_proc.plotter.prepare(perf_proc.model_data)
	
	perf_proc.prefs = read_file("prefs.json")['performance_plots']
	if perf_proc.plotter.have_data:
		perf_proc.plotter.draw(perf_proc.prefs)
	
	perf_proc.show_outletTs = False
	perf_proc.outletTs = [{'label': "none", 'value': 0}]
	if perf_proc.plotter.have_data:
		perf_proc.show_outletTs = perf_proc.plotter.is_central	
		perf_proc.plotter.fig.update_layout(clickmode='event+select')

		if perf_proc.show_outletTs:
			i = 0
			for outletT in perf_proc.plotter.T3s:
				perf_proc.outletTs.append({'label': f"{outletT:.2f} \u00B0C", 'value': i})
				i = i + 1
			
	perf_proc.coloring_list = [{'label': 'heatmap', 'value': 0}, {'label': 'lines', 'value': 1}]
	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}

	app.layout = html.Div([
		dcc.Input(id="input", autoComplete="off"),
   	html.Div(id="message"),
   	WebSocket(url="ws://localhost:8600", id="ws"),
 
		html.Form(children=[
			
      html.P(
				children=["display variable: ",
											 dcc.Dropdown(options = [	{'label': 'Input Power (W)', 'value': 0}, 
													 		{'label': 'Heating Capacity (W)', 'value': 1},
															{'label': 'COP', 'value': 2}],
															value = perf_proc.prefs['contour_variable'], 
															id='display-dropdown',
															style={'width': '50%'},
															clearable=False)
				]),
		
			html.Div(
				[html.P(
					children=["condenser outlet temperature (\u00B0C)",							
						dcc.Dropdown(options = perf_proc.outletTs,
																		value = perf_proc.plotter.i3, 
																		id='outletT-dropdown',
																		style={'width': '50%'},
																		clearable=False)
						
					], hidden = not(perf_proc.show_outletTs), id = "outletT-div")]),
					
			html.P(
				children=["coloring",
						dcc.Dropdown(options = perf_proc.coloring_list,
															value = perf_proc.prefs['contour_coloring'], 
															id='coloring-dropdown',
															style={'width': '50%'},
															clearable=False)
				])
		], style={'width' : '100%', 'margin' : '0 auto'}, id="perf_form", hidden=False),
		

		html.Br(),
		html.Br(),
		html.Br(),
		html.Div(
			dcc.Graph(id='perf-graph', figure={}, style ={'width': '1200px', 'height': '800px', 'display': 'block'},
				config={
	        'modeBarButtonsToAdd': [
	        "drawrect",
	        "eraseshape"
	        ]
	    	}), id="graph-div", hidden = not(perf_proc.plotter.have_data))
	])
	@app.callback(
			Output("ws", "send"),
			[Input("input", "value")]
			)
	def send(value):
		print("sent by perf-proc")
		msg = {"source": "perf-proc", "dest": "perf-proc", "model_data_filepath": perf_proc.model_data_filepath}
		return json.dumps(msg)

	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('graph-div', 'hidden'),
			Output('outletT-div', 'hidden'),
			Output('outletT-dropdown', 'value'),
			Output('outletT-dropdown', 'options'),
			[Input("ws", "message")],
			prevent_initial_call=True
			)
	def message(msg):
		if 'data' in msg:
			data = json.loads(msg['data'])
			if 'dest' in data and data['dest'] == 'perf-proc':
				print("received by perf-proc")	
				perf_proc.model_data = {}
				if 'model_data_filepath' in data:
					perf_proc.model_data_filepath = data['model_data_filepath']			
					perf_proc.model_data = read_file(perf_proc.model_data_filepath)
					print(perf_proc.model_data)
					perf_proc.plotter.prepare(perf_proc.model_data)
					perf_proc.show_outletTs = False
					perf_proc.outletTs = []
					if perf_proc.plotter.have_data:
						perf_proc.show_outletTs = perf_proc.plotter.is_central
						if perf_proc.show_outletTs:
							i = 0
							for outletT in perf_proc.plotter.T3s:
								perf_proc.outletTs.append({'label': f"{outletT:.2f} \u00B0C", 'value': i})
								i = i + 1
						else:
							perf_proc.outletTs = [{'label': "none", 'value': 0}]
						perf_proc.plotter.draw(perf_proc.prefs)
						
						
		
		return perf_proc.plotter.fig, not(perf_proc.plotter.have_data), not(perf_proc.show_outletTs), perf_proc.plotter.i3, perf_proc.outletTs
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('display-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_variable(value):	
		perf_proc.prefs['contour_variable'] = value
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
			prefs = read_file("prefs.json")
			prefs["performance_plots"] = perf_proc.prefs
			write_file("prefs.json", prefs)
			return perf_proc.plotter.fig
		return {}
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('coloring-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_coloring(value):
		perf_proc.prefs['contour_coloring'] = value
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
			prefs = read_file("prefs.json")
			prefs["performance_plots"] = perf_proc.prefs
			write_file("prefs.json", prefs)
			return perf_proc.plotter.fig
		return {}
	
	@callback(
				Output('perf-graph', 'figure', allow_duplicate=True),
				Input('outletT-dropdown', 'value'),
				prevent_initial_call=True
			)
	def select_outletT(value):
		if perf_proc.plotter.have_data:
			if value == None:
				return perf_proc.plotter.fig
			if perf_proc.plotter.is_central:
				perf_proc.plotter.i3 = value
			else:
				perf_proc.plotter.i3 = 0
				perf_proc.plotter.get_slice()
				perf_proc.plotter.draw(perf_proc.prefs)	
				return perf_proc.plotter.fig
		return {}
	
	@callback(
			Output('perf-graph', 'figure'),
			Input('perf-graph', 'relayoutData'),
			prevent_initial_call=True
		)
	def select_range(clickData):
		if perf_proc.plotter.have_data:
			return perf_proc.plotter.fig
		return {}

	app.run(debug=True, use_reloader=False, port = perf_proc.port_num)
	
perf_proc.port_num = 8051

# Runs a simulation and generates plot
def launch_perf_proc(data):

	if launch_perf_proc.proc != -1:
		print("killing current dash for plotting performance...")
		launch_perf_proc.proc.kill()
		time.sleep(1)

	launch_perf_proc.proc = mp.Process(target=perf_proc, args=(data, ), name='perf-proc')
	print("launching dash for plotting performance...")
	time.sleep(1)

	launch_perf_proc.proc.start()
	time.sleep(2)
	   
	results = {}
	results["port_num"] = perf_proc.port_num
	return results

launch_perf_proc.proc = -1

