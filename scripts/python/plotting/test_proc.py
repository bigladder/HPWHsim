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

def test_proc(fig):
	
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
	test_proc.variable_type = "";

	fig.update_layout(clickmode='event+select')
		
	app.layout = [

			dcc.Graph(id='test-graph', figure=fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'} ),
			
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
		        )
		        ,

					html.Button('Get UA', id='get-ua-btn', n_clicks=0, disabled = True),

					html.P(id='ua-p', style = {'fontSize': 18, 'display': 'inline'}), html.Br()
				],
				id = 'ua-div',
				className='six columns',
				hidden = True
			)
	]
	
	@callback(
		Output('get-ua-btn', 'disabled'),
		Output('ua-div', 'hidden'),
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

	app.run(debug=True, use_reloader=False)