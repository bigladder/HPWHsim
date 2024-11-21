import os
import sys

import pandas as pd  # type: ignore
import json
from dash import Dash, dcc, html, Input, Output, State, callback, set_props
import plotly.express as px
import plotly.graph_objects as go

def dash_proc(fig):

	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}

	fig.update_layout(clickmode='event+select')

	fig.update_traces(marker_size=4)

	PARAMETERS = (
	    "tank volume (L)",
	)
	
	dash_proc.t_minV = []
	dash_proc.tank_TV = []
	dash_proc.ambient_TV = []
	
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

					html.P(id='ua', style = {'fontSize': 18}), html.Br()
				],
				id = 'ua-div',
				className='four columns',
				hidden = True
			)
	]

	@callback(
		Output('get-ua-btn', 'disabled'),
		Output('ua-div', 'hidden'),
		Input('test-graph', 'selectedData'),
		State('test-graph', 'figure'),
		State('tank-volume', 'value'),
		prevent_initial_call=True
	)
	def select_temperature_data(selectedData, fig, tank_vol_L):
		#print(json.dumps(selectedData, indent=2))
		if float(tank_vol_L) <= 0:
			return True, True
		
		if not selectedData:
			print("not selected.")
			return True, True
		
		if not "range" in selectedData:
				print("no range.")
				return True, True
				
		range = selectedData["range"]
		if not "y3" in range:
			return True, True
		
		t_min_i = range["x3"][0]
		t_min_f = range["x3"][1]
			
		for trace in fig["data"]:
			if trace["name"] == "Storage Tank Average Temperature - Measured":
				measured_tank_T = trace
			if trace["name"] == "Ambient Temperature - Measured":
				ambient_T = trace
			
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
			return True, True

		print("selected")
		return False, False

	@callback(
		Output('test-graph', 'figure'),
		Output('ua', 'children'),
		Input('get-ua-btn', 'n_clicks'),
		State('test-graph', 'figure'),
		State('tank-volume', 'value'),
		prevent_initial_call=True
	)
	def calc_ua(n_clicks, fig, tankVol_L):
		
		t_min_i = dash_proc.t_minV[0]	
		t_min_f = dash_proc.t_minV[-1]
		
		tank_T_i = dash_proc.tank_TV[0]
		tank_T_f = dash_proc.tank_TV[-1]

		ambient_T_i = dash_proc.ambient_TV[0]
		ambient_T_f = dash_proc.ambient_TV[-1]
		
		rhoWater_kg_per_L = 0.995
		sWater_kJ_per_kgC = 4.180
		
		cTank_kJ_per_C = rhoWater_kg_per_L * float(tankVol_L) * sWater_kJ_per_kgC 
		ambientT = (ambient_T_i + ambient_T_f) / 2
		
		temp_ratio = (tank_T_i - tank_T_f)  / (tank_T_i - ambientT)
		t_h = (t_min_f - t_min_i) / 60
		
		UA = cTank_kJ_per_C * temp_ratio / t_h
		
		return fig, "{:.4f} kJ/hC".format(UA)

	app.run(debug=True, use_reloader=False)