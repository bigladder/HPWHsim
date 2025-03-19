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
	test_proc.prefs = read_file("prefs.json")
	test_proc.i_send = 0
	test_proc.prev_show = 0
	test_proc.uef_val = 0
	#test_proc.plotter = plot(data)
	
	def replot(data):
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
			
			hide_uef_fit = True
			hide_uef_input_val = True
			uef_out_text = "" 
			if 'is_standard_test' in data:
				if data['is_standard_test'] == 1:
					hide_uef_fit = False
					build_dir = data['build_dir']
					output_dir = os.path.join(build_dir, 'test', 'output')
					results_filename = os.path.join(output_dir, "results.json")
					results = read_file(results_filename)
					test_proc.uef_val = results['UEF']
					uef_out_text = "simulated UEF: {:.4f}".format(test_proc.uef_val)
					
					fit_list = read_file("fit_list.json")
					if 'metrics' in fit_list:
						metrics = fit_list['metrics']
						for index, metric in reversed(list(enumerate(metrics))):
							if 'type' not in metric or metric['type'] != 'UEF':
								continue
							if 'model' not in metric or (metric['model'] != test_proc.prefs['model_id']):
								continue
							hide_uef_input_val = False

			return test_proc.plotter.plot.figure, option_list, value_list, measured_msg, simulated_msg, True, hide_uef_fit, hide_uef_input_val, test_proc.uef_val, uef_out_text, False
		
		return no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update			
	
	app.layout = [
		html.Div(
			[
   		WebSocket(url="ws://localhost:8600", id="ws"),
			html.Div(html.Button("send", id='send-btn', n_clicks=0), hidden=True),
			
			dcc.Checklist(id="show-check", inline=True),
		
			dcc.Graph(id='test-graph', figure={}, style ={'width': '1200px', 'height': '800px', 'display': 'block'}),
			
			html.P("", id='energy_measured_p'),
			html.P("", id='energy_simulated_p'),
			
			html.Div(
				[
					dcc.Markdown("""
						**find UA**
									 
						Select an area on the temperature plot.
						"""),
					
					html.Label("tank volume (L)", htmlFor="tank-volume"),
					dcc.Input(
		          id = "tank-volume",
							type = "text",
		          value="189.271",
		        ),

						html.Button('Get UA', id='get-ua-btn', n_clicks=0, disabled = True),

						html.P(id='ua-p', style = {'fontSize': 18, 'display': 'inline'}), html.Br()
				],
				id = 'ua-div',
				className='six columns',
				hidden = True
			),
		
			html.Div([
				html.Div([
					html.P(
						id='UEF-output',
						children="simulated UEF: 0",
						style = {'fontSize': 16, 'display': 'inline-block'}),
					dcc.Checklist(
				    options = [{'label': 'fit', 'value': 'fit'}],
						value = [],
						id="fit-UEF-check",
						style = {'fontSize': 16, 'display': 'inline-block', 'margin-left': 8})]),
				html.Div(
					[html.Label("target:", htmlFor="target-UEF-input", style = {'fontSize': 16, 'display': 'inline-block'}),
					dcc.Input(id='target-UEF-input', type='number', value = 0)],
					id='UEF-input-div', 
					hidden=True)
				],
				id='fit-UEF-div',
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
				Output('ua-div', 'hidden', allow_duplicate=True),
				Output('fit-UEF-div', 'hidden'),
				Output('UEF-input-div', 'hidden', allow_duplicate=True),
				Output('target-UEF-input', 'value'),
				Output('UEF-output', 'children'),
				Output('main-div', 'hidden'),
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
		Output('UEF-input-div', 'hidden', allow_duplicate=True),
		Input('fit-UEF-check', 'value'),
		Input('target-UEF-input', 'value'),
		prevent_initial_call=True
	)
	def change_fit_UEF(value, uef_in):	
		fit_list = read_file("fit_list.json")
		if 'metrics' in fit_list:
			metrics = fit_list['metrics']
		else:
			metrics = []
		
		hide_input = True
		new_metrics = metrics
		for index, metric in reversed(list(enumerate(metrics))):
			if 'type' not in metric or metric['type'] != 'UEF':
				continue
			if 'model_id' not in metric or (metric['model_id'] != test_proc.prefs['model_id']):
					continue
			
			del new_data[index]	
		
		if 'fit' in value:
			new_metrics.append({'type': 'UEF', 'model_id': test_proc.prefs['model_id'], 'target': uef_in})
			hide_input = False

		fit_list['metrics'] = new_metrics
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
			test_proc.prefs["show_measured"] = True
			data['show'] |= 1
		if 'simulated' in value:
			test_proc.prefs["show_simulated"] = True
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
		Output('get-ua-btn', 'disabled'),
		Output('ua-div', 'hidden', allow_duplicate=True),
		Output('ua-p', 'children', allow_duplicate=True),
		Output('test-graph', 'figure', allow_duplicate=True),
		Input('test-graph', 'selectedData'),
		State('test-graph', 'figure'),
		State('tank-volume', 'value'),
		prevent_initial_call=True
	)
	def select_temperature_data(selectedData, fig, tank_vol_L):
		if float(tank_vol_L) <= 0:
			return True, True, "", fig
		
		if not selectedData:
			return True, True, "", fig
		
		if not "range" in selectedData:
			return True, True, "", fig
				
		range = selectedData["range"]
		if not "y3" in range:
			return True, True, "", fig
		
		t_min_i = range["x3"][0]
		t_min_f = range["x3"][1]

		have_measured = False
		have_simulated = False
		for trace in fig["data"]:
			if "name" in trace:
				if trace["name"] == "Storage Tank Average Temperature - Measured":
					have_measured = True
				elif trace["name"] == "Storage Tank Average Temperature - Simulated":
					have_simulated = True
					
		if have_measured:
			test_proc.variable_type = "Measured"
		elif have_simulated:
			test_proc.variable_type = "Simulated"
		else:
			return True, True, "", fig
			
		found_avgT = False
		for trace in fig["data"]:
			if "name" in trace:
				if trace["name"] == f"Storage Tank Average Temperature - {test_proc.variable_type}":
					measured_tank_T = trace
					found_avgT = True
		
		found_ambientT = False		
		for trace in fig["data"]:				
			if trace["name"] == f"Ambient Temperature - {test_proc.variable_type}":
				if "name" in trace:
					ambient_T = trace
					found_ambientT = True
				
		if not(found_avgT and found_ambientT):
			return True, True, "", fig
		
		new_fig = go.Figure(fig)
		for i, trace in enumerate(fig["data"]):
			if "name" in trace:	
				if trace["name"] == f"temperature fit - {test_proc.variable_type}":
					new_data = list(new_fig.data)
					new_data.pop(i)
					new_fig.data = new_data	
		new_fig.update_layout()	
				
		selected_t_minL = []
		selected_tank_TL = []
		selected_ambient_TL = []		

		n = 0
		for t_min in measured_tank_T["x"]:
			if t_min_i <= t_min and t_min <= t_min_f:
				selected_t_minL.append(t_min)
				selected_tank_TL.append(measured_tank_T["y"][n])
				selected_ambient_TL.append(ambient_T["y"][n])
			n += 1	
			
		test_proc.selected_t_minV = np.array(selected_t_minL)
		test_proc.selected_tank_TV = np.array(selected_tank_TL)
		test_proc.selected_ambient_TV = np.array(selected_ambient_TL)
					
		if n < 2:
			return True, True, "", fig

		return False, False, "", new_fig

	@callback(
		Output('test-graph', 'figure'),
		Output('ua-p', 'children'),
		Input('get-ua-btn', 'n_clicks'),
		State('test-graph', 'figure'),
		State('tank-volume', 'value'),
		prevent_initial_call=True
	)
	def calc_ua(n_clicks, fig, tankVol_L):
		for i, trace in enumerate(fig["data"]):	
			if "name" in trace:
				if trace["name"] == "temperature fit":
					new_data = list(new_fig.data)
					new_data.pop(i)
					new_fig.data = new_data	
		
		t_min_i = test_proc.selected_t_minV[0]	
		t_min_f = test_proc.selected_t_minV[-1]
		
		tank_T_i = test_proc.selected_tank_TV[0]
		tank_T_f = test_proc.selected_tank_TV[-1]

		ambient_T_i = test_proc.selected_ambient_TV[0]
		ambient_T_f = test_proc.selected_ambient_TV[-1]
		
		rhoWater_kg_per_L = 0.995
		sWater_kJ_per_kgC = 4.180
		
		cTank_kJ_per_C = rhoWater_kg_per_L * float(tankVol_L) * sWater_kJ_per_kgC 
		ambientT_avg = (ambient_T_i + ambient_T_f) / 2
		
		temp_ratio = (tank_T_i - tank_T_f)  / (tank_T_i - ambientT_avg)
		dt_min = t_min_f - t_min_i
		dt_h = dt_min / 60	

		UA = cTank_kJ_per_C * temp_ratio / dt_h
		tau_min0 = dt_min / temp_ratio

		def T_t(params, t_min):
			return ambientT_avg + (tank_T_i - ambientT_avg) * np.exp(-(t_min - t_min_i) / params[0])

		def diffT_t(params, t_min):
			return T_t(params, t_min) - test_proc.selected_tank_TV
	
		tau_min = tau_min0
		res = least_squares(diffT_t, test_proc.selected_t_minV, args=(tau_min, ))
		UA = cTank_kJ_per_C / (tau_min / 60)
		
		fit_tank_TV = test_proc.selected_tank_TV
		for i, t_min in enumerate(test_proc.selected_t_minV):
			fit_tank_TV[i] = T_t([tau_min], t_min)
		
		trace = go.Scatter(name = f"temperature fit - {test_proc.variable_type}", x=test_proc.selected_t_minV, y=fit_tank_TV, xaxis="x3", yaxis="y3", mode="lines", line={'width': 3})
		new_fig = go.Figure(fig)	
		new_fig.add_trace(trace)
		new_fig.update_layout()		
		
		return new_fig, " {:.4f} kJ/hC".format(UA)

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
