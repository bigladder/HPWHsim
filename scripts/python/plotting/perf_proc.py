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
from dash_extensions import EventListener
import asyncio

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
				json.dump(json_data, json_file)			
	except:
			print(f"failed to write {filename}")
			return
		
def perf_proc():
	orig_dir = str(Path.cwd())
	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	perf_proc.prefs = read_file("prefs.json")
	perf_proc.model_index = read_file("model_index.json")
		
	model_path = os.path.join(abs_repo_test_dir, "models_json", perf_proc.prefs["model_id"] + ".json") 
	perf_proc.model_data = read_file(model_path)
	
	perf_proc.plotter = PerfPlotter()
	perf_proc.plotter.prepare(perf_proc.model_data)
	perf_proc.plotter.draw(perf_proc.prefs["contour_variable"])
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

	perf_proc.model_list = []
	i = 0
	for model in perf_proc.model_index["models"]:
		if "id" in model:			
			perf_proc.model_list.append({'label': model["id"], 'value': i})
			if model["id"] == perf_proc.prefs["model_id"]:
				perf_proc.imodel = i
			i = i + 1

	perf_proc.system_type = "integrated" if "integrated_system" in perf_proc.model_data else "central"
	
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
		
		html.Div(
			[
					html.Label("model name", htmlFor="model-name-dropdown", style = {'display': 'inline'}),
					dcc.Dropdown(options = perf_proc.model_list,
														value = perf_proc.imodel, 
														id='model-name-dropdown',
														style={'width': '50%', 'align': 'right'},
														clearable=False, className="column")
			]),

		html.Div(
			[
					html.Label("system type: ", htmlFor="system-type", style = {'display': 'inline'}),
					html.P(perf_proc.system_type, id='system-type', style = {'fontSize': 18, 'display': 'inline'})
			]),
						
		html.Div(
			[
					html.Label("display variable", htmlFor="display-dropdown", style = {'display': 'inline'}),
					dcc.Dropdown(options = [	{'label': 'Input Power (W)', 'value': 0}, 
												 		{'label': 'Heating Capacity (W)', 'value': 1},
														{'label': 'COP', 'value': 2}],
														value = perf_proc.prefs['contour_variable'], 
														id='display-dropdown',
														style={'width': '50%', 'align': 'right'},
														clearable=False, className="column")
			]),
	
		html.Div(
			[
				html.Label("condenser outlet temperature (\u00B0C)", htmlFor="outletT-dropdown", style = {'display': 'inline'}),
				dcc.Dropdown(options = perf_proc.outletTs,
																value = perf_proc.plotter.i3, 
																id='outletT-dropdown',
																style={'width': '50%', 'align': 'right'},
																clearable=False)
				
			], hidden = not(perf_proc.show_outletTs), id = "outletT-div"),
		html.Br(),
		html.Br(),
		html.Br(),
				dcc.Graph(id='perf-graph', figure=perf_proc.plotter.fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'},
					config={
            'modeBarButtonsToAdd': [
            "drawrect",
            "eraseshape"
            ]
        }, )	
	])
	@app.callback(
			Output("ws", "send"),
			[Input("input", "value")]
			)
	def send(value):
		print("sending")
		return json.dumps({"source": "dash"})

	@app.callback(
			Output("model-name-dropdown", "value"),
			[Input("ws", "message")],
			prevent_initial_call=True
			)
	def message(msg):
		print("receiving")
		print(msg)
		if 'data' in msg:
			data = json.loads(msg['data'])
			if 'model_name' in data:
				model_id = data['model_name']
				i = 0
				for model in perf_proc.model_index["models"]:
					if "id" in model:			
						if model["id"] == model_id:
							perf_proc.prefs["model_id"] = model_id
							perf_proc.imodel = i
							break
						i = i + 1
						
		return perf_proc.imodel

	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('outletT-div', 'hidden'),
			Output('outletT-dropdown', 'options'),
			Input('model-name-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_model(value):
		i = 0
		for model in perf_proc.model_index["models"]:
			if i == value:
				perf_proc.prefs["model_id"] = model["id"]
				model_path = os.path.join(abs_repo_test_dir, "models_json", model["id"] + ".json") 
				perf_proc.model_data = read_file(model_path)
				break
			i = i + 1
		perf_proc.plotter.prepare(perf_proc.model_data)
		perf_proc.system_type = "integrated" if "integrated_system" in perf_proc.model_data else "central"
		perf_proc.show_outletTs = perf_proc.plotter.is_central
		perf_proc.outletTs = []
		if perf_proc.show_outletTs:
			i = 0
			for outletT in perf_proc.plotter.T3s:
				perf_proc.outletTs.append({'label': f"{outletT:.2f} \u00B0C", 'value': i})
				i = i + 1
		else:
			perf_proc.outletTs = [{'label': "none", 'value': 0}]
		perf_proc.plotter.draw(perf_proc.prefs['contour_variable'])
		write_file("prefs.json", perf_proc.prefs)
		return perf_proc.plotter.fig, not(perf_proc.show_outletTs), perf_proc.outletTs
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('display-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_variable(value):	
		perf_proc.plotter.draw(value)
		perf_proc.prefs['contour_variable'] = value
		write_file("prefs.json", perf_proc.prefs)
		return perf_proc.plotter.fig
	
	@callback(
				Output('perf-graph', 'figure', allow_duplicate=True),
				Input('outletT-dropdown', 'value'),
				prevent_initial_call=True
			)
	def select_outletT(value):
		perf_proc.plotter.i3 = value
		perf_proc.plotter.get_slice()
		perf_proc.plotter.draw(perf_proc.prefs['contour_variable'])
		return perf_proc.plotter.fig
	
	@callback(
			Output('perf-graph', 'figure'),
			Input('perf-graph', 'relayoutData'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
		)
	def select_range(clickData, fig):
		if fig is None:
			return dash.no_update
		if 'shapes' in fig['layout']:
			shp = fig['layout']['shapes'][0]
			x0 = shp['x0']
			x1 = shp['x1']
			y0 = shp['y0']
			y1 = shp['y1']
			print(x0, y0, x1, y1)
			fig.update_layout({
				'modeBarButtonsToAdd': []})
		else:
			fig['layout']['modeBarButtonsToAdd'] = [
            "drawrect",
            "eraseshape"
						]
		return fig

	app.run(debug=True, use_reloader=False, port = perf_proc.port_num)
	
perf_proc.port_num = 8051

# Runs a simulation and generates plot
def launch_perf_plot():

	if launch_perf_plot.proc != -1:
		print("killing current dash for plotting performance...")
		launch_perf_plot.proc.kill()
		time.sleep(1)

	launch_perf_plot.proc = mp.Process(target=perf_proc, args=(), name='perf-proc')
	print("launching dash for plotting performance...")
	time.sleep(1)

	launch_perf_plot.proc.start()
	time.sleep(2)
	   
	results = {}
	results["port_num"] = perf_proc.port_num
	return results

launch_perf_plot.proc = -1

