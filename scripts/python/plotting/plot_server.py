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

def dash_proc(fig):
	
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

	dash_proc.t_minV = []
	dash_proc.tank_TV = []
	dash_proc.ambient_TV = []

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

					html.P(id='ua-p', style = {'fontSize': 18}), html.Br()
				],
				id = 'ua-div',
				className='four columns',
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
		
		found_avgT = False
		for trace in fig["data"]:
			if "name" in trace:
				if trace["name"] == "Storage Tank Average Temperature - Measured":
					measured_tank_T = trace
					found_avgT = True
				
		found_ambientT = False		
		for trace in fig["data"]:				
			if trace["name"] == "Ambient Temperature - Measured":
				if "name" in trace:
					ambient_T = trace
					found_ambientT = True
				
		if not(found_avgT and found_ambientT):
			return True, True, "", fig
		
		new_fig = go.Figure(fig)
		for i, trace in enumerate(fig["data"]):
			if "name" in trace:	
				if trace["name"] == "temperature fit":
					new_data = list(new_fig.data)
					new_data.pop(i)
					new_fig.data = new_data	
		new_fig.update_layout()	
									
		dash_proc.t_minV = []
		dash_proc.tank_TV = []
		dash_proc.ambient_TV = []
		n = 0
		for t_min in measured_tank_T["x"]:
			if t_min_i <= t_min and t_min <= t_min_f:
				dash_proc.t_minV.append(t_min)
				dash_proc.tank_TV.append(measured_tank_T["y"][n])
				dash_proc.ambient_TV.append(ambient_T["y"][n])
			n += 1	
					
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
		
		t_min_i = dash_proc.t_minV[0]	
		t_min_f = dash_proc.t_minV[-1]
		
		tank_T_i = dash_proc.tank_TV[0]
		tank_T_f = dash_proc.tank_TV[-1]

		ambient_T_i = dash_proc.ambient_TV[0]
		ambient_T_f = dash_proc.ambient_TV[-1]
		
		rhoWater_kg_per_L = 0.995
		sWater_kJ_per_kgC = 4.180
		
		cTank_kJ_per_C = rhoWater_kg_per_L * float(tankVol_L) * sWater_kJ_per_kgC 
		ambientT_avg = (ambient_T_i + ambient_T_f) / 2
		
		temp_ratio = (tank_T_i - tank_T_f)  / (tank_T_i - ambientT_avg)
		dt_min = t_min_f - t_min_i
		dt_h = dt_min / 60	

		UA = cTank_kJ_per_C * temp_ratio / dt_h
		tau_min0 = dt_min / temp_ratio
		
		t_minV = np.array(dash_proc.t_minV)	
		tank_TV = np.array(dash_proc.tank_TV)

		def T_t(params, t_min):
			return ambientT_avg + (tank_T_i - ambientT_avg) * np.exp(-(t_min - t_min_i) / params[0])

		def diffT_t(params, t_min):
			return T_t(params, t_min) - tank_TV
		
		tau_min = tau_min0
		res = least_squares(diffT_t, t_minV, args=(tau_min, ))
		UA = cTank_kJ_per_C / (tau_min / 60)
		
		fit_tank_TV = tank_TV
		for i, t_min in enumerate(t_minV):
			fit_tank_TV[i] = T_t([tau_min], t_min)
		#print(fit_tank_TV)	
		
		trace = go.Scatter(name = "temperature fit", x=t_minV, y=fit_tank_TV, xaxis="x3", yaxis="y3", mode="lines", line={'width': 3})
		new_fig = go.Figure(fig)	
		new_fig.add_trace(trace)
		new_fig.update_layout()		
		#new_fig.show()
		
		return new_fig, "{:.4f} kJ/hC".format(UA)

	app.run(debug=True, use_reloader=False)