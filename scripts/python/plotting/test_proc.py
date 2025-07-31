import os
import sys

import pandas as pd  # type: ignore
import json
from dash import Dash, dcc, html, Input, Output, State, callback, no_update
import plotly.express as px
import plotly.graph_objects as go

import plotly.io as io
from scipy.optimize import least_squares
import numpy as np
import math
import time
from test_plot import TestPlotter, plot
import multiprocessing as mp
from pathlib import Path
from dash_extensions import WebSocket

from common import read_file, write_file

def test_proc(data):
	
	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}

	PARAMETERS = (
	    "tank volume (L)",
	)
	
	test_proc.selected_t_minV = []
	test_proc.selected_tank_TV = []
	test_proc.selected_ambient_TV = []
	test_proc.variable_type = ""
	test_proc.i_send = 0
	test_proc.prev_show = 0
	test_proc.ef_val = 0
	test_proc.plotter = {}
	test_proc.prefs = {}
		
	def sync_prefs():
		prefs = read_file("prefs.json")
		if 'tests' in test_proc.prefs:
			if 'plots' in test_proc.prefs['tests']:
				prefs['tests']['plots'] = test_proc.prefs['tests']['plots']
		test_proc.prefs = prefs
		write_file("prefs.json", prefs)	
		
	def replot(data):
		print(data)
		sync_prefs()
		data['model_id'] = test_proc.prefs['model_id']
		data['test_id'] = test_proc.prefs['tests']['id']

		test_proc.plotter = plot(data)
		if test_proc.plotter.have_fig:
			test_proc.plotter.plot.figure.update_layout(clickmode='event+select')
			measured_msg = ""
			simulated_msg = ""
			if 'measuredE_Wh' in test_proc.plotter.energy_data:
				measured_msg = "Measured energy consumption (Wh): " + f"{test_proc.plotter.energy_data['measuredE_Wh']:.2f}"
			if 'simulatedE_Wh' in test_proc.plotter.energy_data:
				simulated_msg = "Simulated energy consumption (Wh): " + f"{test_proc.plotter.energy_data['simulatedE_Wh']:.2f}"
												
			option_list = []
			value_list = []
			if test_proc.plotter.measured.have_data:
				option_list.append({'label': 'measured', 'value': 'measured'})
				value_list.append('measured')
				test_proc.prev_show |= 1
			if test_proc.plotter.simulated.have_data:
				option_list.append({'label': 'simulated', 'value': 'simulated'})
				value_list.append('simulated')
				test_proc.prev_show |= 2
			
			hide_ef_fit = True
			hide_ef_input_val = True
			ef_out_text = "" 
			if 'is_standard_test' in data:
				if data['is_standard_test'] == 1:
					hide_ef_fit = False
					build_dir = test_proc.prefs['build_dir']
					output_dir = os.path.join(build_dir, 'test', 'output')
					results_filename = os.path.join(output_dir, "results.json")
					results = read_file(results_filename)
					summary = results["24_hr_test"]
					test_proc.ef_val = summary['EF']
					ef_out_text = "simulated EF: {:.4f}".format(test_proc.ef_val)
					
					fit_list = read_file("fit_list.json")
					if 'metrics' in fit_list:
						metrics = fit_list['metrics']
						for index, metric in reversed(list(enumerate(metrics))):
							if 'type' not in metric or metric['type'] != 'EF':
								continue
							if 'model' not in metric or (metric['model'] != test_proc.prefs['model_id']):
								continue
							hide_ef_input_val = False

			return test_proc.plotter.plot.figure, option_list, value_list, measured_msg, simulated_msg, hide_ef_fit, hide_ef_fit, test_proc.ef_val, ef_out_text, False, False
		
		return no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update
	
	app.layout = [
		html.Div(
			[
   		WebSocket(url="ws://localhost:8600", id="ws"),
			html.Div(html.Button("send", id='send-btn', n_clicks=0), hidden=True),
			
			dcc.Checklist(id="show-check", inline=True),

			html.Div(html.Button('Get UA', id='get-ua-btn', n_clicks=0), hidden=True),
		
			dcc.Graph(id='test-graph', figure={}, style ={'width': '1200px', 'height': '800px', 'display': 'block'}),
			
			html.P("", id='energy_measured_p'),
			html.P("", id='energy_simulated_p'),
	
			html.Div([
					html.P("match:", style={'fontSize': '12px', 'margin': '4px', 'display': 'inline-block'}),
					html.Button("+", id='match-selected', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),				
					html.Button("-", id='ignore-selected', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),				
					],
					id='select-div',
					hidden = True
				),
		
			html.Div([
				html.Div([
					html.P(
						id='EF-output',
						children="simulated EF: 0",
						style = {'fontSize': 16, 'display': 'inline-block'}),
					dcc.Checklist(
				    options = [{'label': 'fit', 'value': 'fit'}],
						value = [],
						id="fit-EF-check",
						style = {'fontSize': 16, 'display': 'inline-block', 'marginLeft': 8})]),
				html.Div(
					[html.Label("target:", htmlFor="target-EF-input", style = {'fontSize': 16, 'display': 'inline-block'}),
					dcc.Input(id='target-EF-input', type='number', value = 0)],
					id='EF-input-div', 
					hidden=True)
				],
				id='fit-EF-div',
				hidden = True)
		],
		id='main-div', hidden=True)
		
	]
	
	@app.callback(
		Output("ws", "send"),
		Input("send-btn", "n_clicks")
	)
	def send_message(n_clicks):
		test_proc.i_send = test_proc.i_send + 1
		message = json.dumps({"source": "test-proc", "dest": "index", "cmd": "init-test-proc", "index": test_proc.i_send})
		return message

	@app.callback(
				Output('test-graph', 'figure', allow_duplicate=True),
				Output('show-check', 'options'),
				Output('show-check', 'value'),
				Output('energy_measured_p', 'children'),
				Output('energy_simulated_p', 'children'),
				Output('fit-EF-div', 'hidden'),
				Output('EF-input-div', 'hidden', allow_duplicate=True),
				Output('target-EF-input', 'value'),
				Output('EF-output', 'children'),
				Output('main-div', 'hidden'),
				Output('select-div', 'hidden'),
				[Input("ws", "message")],
				prevent_initial_call=True
			)
	def message(msg):
		if 'data' in msg:
			data = json.loads(msg['data'])
			if 'dest' in data and data['dest'] == 'test-proc':
				if 'cmd' in data:
					if data['cmd'] == 'replot':						
						return replot(data)
						
		return no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update

	@app.callback( 
		Output('ws', 'send', allow_duplicate=True),
		Output('EF-input-div', 'hidden', allow_duplicate=True),
		Input('fit-EF-check', 'value'),
		Input('target-EF-input', 'value'),
		prevent_initial_call=True
	)
	def change_fit_EF(value, ef_in):	
		fit_list = read_file("fit_list.json")
		if 'metrics' in fit_list:
			metrics = fit_list['metrics']
		else:
			metrics = {}
		
		if 'energy_factors' in metrics:
			energy_factors = metrics['energy_factors']
		else:
			energy_factors = []
			
		hide_input = True
		new_energy_factors = energy_factors
		for index, energy_factor in reversed(list(enumerate(energy_factors))):
			if 'model_id' not in energy_factor or (energy_factor['model_id'] != test_proc.prefs['model_id']):
					continue			
			if 'draw_profile' not in energy_factor or energy_factor['draw_profile'] != test_proc.prefs['tests']['draw_profile']:
					continue			
			del new_energy_factors[index]	
		
		if 'fit' in value:
			new_energy_factors.append({'model_id': test_proc.prefs['model_id'], 'draw_profile': test_proc.prefs['tests']['draw_profile'], 'value': ef_in})
			hide_input = False

		metrics['energy_factors'] = new_energy_factors
		fit_list['metrics'] = metrics
		write_file("fit_list.json", fit_list)		
		test_proc.i_send = test_proc.i_send + 1
		msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": test_proc.i_send}
		return json.dumps(msg), hide_input

	@callback(
		Output('test-graph', 'figure', allow_duplicate=True),
		Output('show-check', 'value', allow_duplicate=True),
		Input('show-check', 'value'),
		prevent_initial_call=True
	)
	def change_show(value):	
		data = {'show': 0}
		if 'measured' in value:
			test_proc.prefs['tests']['plots']['show_measured'] = True
			data['show'] |= 1
		if 'simulated' in value:
			test_proc.prefs['tests']['plots']['show_simulated'] = True
			data['show'] |= 2
		
		if data['show'] == 0:
			data['show'] = test_proc.prev_show
		
		test_proc.prev_show = data['show']
		test_proc.plotter.draw(data)
		
		value_list = []
		if data['show'] & 1 == 1:
			value_list.append('measured')
		if data['show'] & 2 == 2:
			value_list.append('simulated')	
			
		return test_proc.plotter.plot.figure, value_list
			
	@callback(
		Output('test-graph', 'figure', allow_duplicate=True),
		Output('select-div', 'hidden', allow_duplicate=True),
		Input('test-graph', 'selectedData'),
		State('test-graph', 'figure'),
		prevent_initial_call=True
	)
	def select_data(selectedData, fig):
		prev_layout = fig['layout']
		if not selectedData:
			return no_update, True
		
		if not "range" in selectedData:
			return no_update, True
				
		test_proc.plotter.select_data(selectedData)	
		return fig, False
			
	@callback(
		Output('ws', 'send', allow_duplicate=True),
		Output('test-graph', 'figure', allow_duplicate=True),
		Input('test-graph', 'clickData'),
		State('test-graph', 'figure'),
		prevent_initial_call=True
	)
	def click_data(clickData, fig):
		prev_layout = fig['layout']
		if not clickData:
			return no_update, fig
		
		#print(clickData)
		if not "points" in clickData:
			return no_update, fig
				
		test_proc.plotter.click_data(clickData)	
		
		fit_list = read_file("fit_list.json")
		if 'metrics' in fit_list:
			metrics = fit_list['metrics']
		else:
			metrics = {}		

		if 'test_points' in metrics:
			test_points = metrics['test_points']
		else:
			test_points = []	
		
		for test_point in test_proc.plotter.test_points:
			test_points.append(test_point)
		
		if test_points:
			metrics['test_points'] = test_points
		
		fit_list['metrics'] = metrics			
		write_file("fit_list.json", fit_list)
		test_proc.i_send = test_proc.i_send + 1
		msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": test_proc.i_send}
		return json.dumps(msg), test_proc.plotter.plot.figure
	
	@app.callback(
			Output('ws', 'send', allow_duplicate=True),
			Input('match-selected', 'n_clicks'),
			prevent_initial_call=True
	)
	def match_selected(nclicks):
		fit_list = read_file("fit_list.json")
		if 'metrics' in fit_list:
			metrics = fit_list['metrics']
		else:
			metrics = {}		
		
		if 'test_points' in metrics:
			test_points = metrics['test_points']
		else:
			test_points = []
		for test_point in test_proc.plotter.test_points:
			test_points.append(test_point)
		
		if test_points:
			metrics['test_points'] = test_points
		
		if metrics:
			fit_list['metrics'] = metrics			
		write_file("fit_list.json", fit_list)
		test_proc.i_send = test_proc.i_send + 1
		msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": test_proc.i_send}
		return json.dumps(msg)

	@app.callback(
			Output('ws', 'send', allow_duplicate=True),
			Input('ignore-selected', 'n_clicks'),
			prevent_initial_call=True
	)
	def ignore_selected(nclicks):
		fit_list = read_file("fit_list.json")
		if 'metrics' in fit_list:
			fit_metrics = fit_list['metrics']
		else:
			fit_metrics = []		
	
		new_metrics = fit_metrics
		for metric in test_proc.plotter.metrics:
			for index, fit_metric in reversed(list(enumerate(fit_metrics))):
				if 'type' not in metric or metric['type'] != fit_metric['type']:
					continue
				if 'variable' not in metric or metric['variable'] != fit_metric['variable']:
					continue
				if 'model_id' not in metric or metric['model_id'] != fit_metric['model_id']:
					continue				
				if 't_min' not in metric or metric['t_min'] != fit_metric['t_min']:
					continue
								
				del new_metrics[index]
		
		fit_list['metrics'] = new_metrics			
		write_file("fit_list.json", fit_list)
		test_proc.i_send = test_proc.i_send + 1
		msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": test_proc.i_send}
		return json.dumps(msg)
	
	app.run(debug=True, use_reloader=False, port = test_proc.port_num)

test_proc.port_num = 8050

# Runs a simulation and generates plot
def launch_test_proc(data):

	print("creating plot...")
	
	if launch_test_proc.proc != -1:
		print("killing current dash for plotting tests...")
		launch_test_proc.proc.kill()
		time.sleep(1)
	
	launch_test_proc.proc = mp.Process(target=test_proc, args=(data, ), name='test-proc')
	time.sleep(1)
	print("launching dash for plotting tests...")
	launch_test_proc.proc.start()
	time.sleep(2)
	   
	test_results = {}
	test_results["port_num"] = test_proc.port_num
	return test_results

launch_test_proc.proc = -1
