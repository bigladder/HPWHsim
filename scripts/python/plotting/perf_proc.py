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
from perf_plot import PerfPlotter
import multiprocessing as mp
from pathlib import Path
from dash_extensions import WebSocket
from common import read_file, write_file
	
def perf_proc(data):
	orig_dir = str(Path.cwd())
	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	def create_plot(data):
		fig = {}
		perf_proc.model_data = {}
		perf_proc.model_data_filepath = ""					
			
		if "model_data_filepath" in data:
			perf_proc.model_data_filepath = data["model_data_filepath"]
			perf_proc.model_data = read_file(perf_proc.model_data_filepath)
			data['model_data'] = perf_proc.model_data
	
		label = ""
		if 'label' in data:
			label = data['label']
			
		perf_proc.plotter = PerfPlotter(label)
		perf_proc.plotter.prepare(data)
		
		perf_proc.prefs = read_file("prefs.json")['performance_plots']
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
		
		perf_proc.show_outletTs = False
		perf_proc.outletTs = []
		if perf_proc.plotter.have_data:
			perf_proc.show_outletTs = perf_proc.plotter.is_central	
			perf_proc.plotter.fig.update_layout(clickmode='event+select')

			if perf_proc.show_outletTs:
				i = 0
				for outletT in perf_proc.plotter.T3s:
					perf_proc.outletTs.append({'label': f"{outletT:.2f} \u00B0C", 'value': i})
					i = i + 1
			
				fig = perf_proc.plotter.fig
			return fig
	
	fig = create_plot(data)
	perf_proc.coloring_list = [{'label': 'heatmap', 'value': 0}, {'label': 'lines', 'value': 1}]
	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']

	interp_list = []
	if perf_proc.prefs["interpolate"] == 1:
		interp_list= ['interpolate']

	show_list = []
	if perf_proc.prefs["show_points"] == 1:
		show_list= ['points']
				
	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}

	app.layout = [
		html.Div(dcc.Input(id="input", autoComplete="off"), hidden=True),
   	html.Div(id="message"),
   	WebSocket(url="ws://localhost:8600", id="ws"),
 
		html.Div([
			html.P("display variable:", style={'font-size': '12px', 'margin': '4px', 'display': 'inline-block'}),
			dcc.Dropdown(
						options = [
							{'label': 'Input Power (W)', 'value': 0}, 
					 		{'label': 'Heating Capacity (W)', 'value': 1},
							{'label': 'COP', 'value': 2}],
						value = perf_proc.prefs['contour_variable'], 
						id='display-dropdown',
						style={'width': '50%', 'display': 'inline-block', 'vertical-align': 'middle'},
						clearable=False)
			]),
						
		html.Div([
			html.P("condenser outlet temperature (\u00B0C)", style={'font-size': '12px', 'margin': '4px', 'display': 'inline-block'}),							
			dcc.Dropdown(
					options = perf_proc.outletTs,
					value = perf_proc.plotter.i3, 
					id='outletT-dropdown',
					style={'width': '50%', 'display': 'inline-block', 'vertical-align': 'middle'},
					clearable=False)
					], 
					id="outletT-p", hidden = not(perf_proc.show_outletTs), 
		),
					
		html.Div([
			html.P("coloring", style={'font-size': '12px', 'margin': '4px', 'display': 'inline-block'}),
			dcc.Dropdown(options = perf_proc.coloring_list,
				value = perf_proc.prefs['contour_coloring'], 
				id='coloring-dropdown',
				style= {'width': '50%', 'display': 'inline-block', 'vertical-align': 'middle'},
				clearable=False)
				]),
			
		html.P("show"),
		dcc.Checklist(
			    options = [{'label': 'points', 'value': 'points'}],
					value = show_list,
					id="show-check"
			),
			
		dcc.Checklist(
			    options = [{'label': 'interpolate', 'value': 'interpolate'}],
					value = interp_list,
					id="interp-check"
			),
		html.Div(	[		
				dcc.Input(id='Nx-input', type='number', value=perf_proc.prefs['Nx']),
				dcc.Input(id='Ny-input', type='number', value=perf_proc.prefs['Ny'])
			], id='interp-sizes', hidden = (perf_proc.prefs["interpolate"] == 0)
		),				
		
		html.Div([
					html.P("marked:", style={'font-size': '12px', 'margin': '4px', 'display': 'inline-block'}),
					html.Button("+", id='add-selected-to-marked', n_clicks=0, style={'font-size': '12px', 'margin': '1px', 'display': 'inline-block'}),
					html.Button("-", id='remove-selected-from-marked', n_clicks=0, style={'font-size': '12px', 'margin': '1px', 'display': 'inline-block'}),
					html.Button("clear", id='clear-marked', n_clicks=0, style={'font-size': '12px', 'margin': '1px', 'display': 'inline-block'}),					
					],
					id='select-div',
					hidden = True
				),
					
		html.Br(),
		html.Div(
			dcc.Graph(id='perf-graph', figure=fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'},
				config={
	        'modeBarButtonsToAdd': [
	        "drawrect",
	        "eraseshape"
	        ]
	    	}), id="graph-div", hidden = not(perf_proc.plotter.have_data))

	]
	
	@app.callback(
			Output("ws", "send"),
			[Input("input", "value")]
			)
	def send(value):
		print("sent by perf-proc")
		msg = {"source": "perf-proc", "dest": "perf-proc"}
		return json.dumps(msg)

	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('graph-div', 'hidden'),
			Output('outletT-p', 'hidden'),
			Output('outletT-dropdown', 'value'),
			Output('outletT-dropdown', 'options'),
			[Input("ws", "message")],
			prevent_initial_call=True
			)
	def message(msg):
		if 'data' in msg:
			data = json.loads(msg['data'])
			print(data)
			if 'dest' in data and data['dest'] == 'perf-proc':
				print("received by perf-proc")
				if 'cmd' in data:
					if data['cmd'] == 'replot':	
				
						prefs = read_file("prefs.json")
						prefs["performance_plots"] = perf_proc.prefs
						write_file("prefs.json", prefs)
						
						fig = create_plot(data)				
						return fig, not(perf_proc.plotter.have_data), not(perf_proc.show_outletTs), perf_proc.plotter.i3, perf_proc.outletTs
		
					if data['cmd'] == 'key-pressed':
						print(data)
						if data['key'] == 'Escape':
							print("clearing")
							perf_proc.plotter.clear_selected()
							perf_proc.plotter.draw(perf_proc.prefs)
							return perf_proc.plotter.fig, not(perf_proc.plotter.have_data), not(perf_proc.show_outletTs), perf_proc.plotter.i3, perf_proc.outletTs
						
		return no_update, no_update, no_update, no_update, no_update
	

	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('interp-sizes', 'hidden'),
			Input('interp-check', 'value'),
			prevent_initial_call=True
		)
	def change_interp(value):
		if 'interpolate' in value:
			perf_proc.prefs["interpolate"] = 1
		else:
			perf_proc.prefs["interpolate"] = 0
		perf_proc.plotter.draw(perf_proc.prefs)
		perf_proc.plotter.update_selected()
		perf_proc.plotter.update_marked()
		return perf_proc.plotter.fig, (perf_proc.prefs["interpolate"] == 0)
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('show-check', 'value'),
			prevent_initial_call=True
		)
	def change_show(value):
		if 'points' in value:
			perf_proc.prefs["show_points"] = 1
		else:
			perf_proc.prefs["show_points"] = 0
		perf_proc.plotter.draw(perf_proc.prefs)
		perf_proc.plotter.update_selected()
		perf_proc.plotter.update_marked()
		
		return perf_proc.plotter.fig
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('Nx-input', 'value'),
			Output('Ny-input', 'value'),
			Input('Nx-input', 'value'),
			Input('Ny-input', 'value'),
			prevent_initial_call=True
		)
	def set_Nxy(Nx, Ny):
		if Nx is not None:
			Nx = int(Nx)
			if Nx > 0:
				perf_proc.prefs["Nx"] = Nx
		if Ny is not None:
			Ny = int(Ny)
			if Ny > 0:
				perf_proc.prefs["Ny"] = Ny				
		perf_proc.plotter.draw(perf_proc.prefs)
		perf_proc.plotter.update_selected()
		perf_proc.plotter.update_marked()
		
		return perf_proc.plotter.fig, perf_proc.prefs["Nx"], perf_proc.prefs["Ny"]
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('display-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_variable(value):	
		perf_proc.prefs['contour_variable'] = value
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
			return perf_proc.plotter.fig
		return no_update
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('coloring-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_coloring(value):
		perf_proc.prefs['contour_coloring'] = value
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
			return perf_proc.plotter.fig
		return no_update
	
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
		return no_update
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('select-div', 'hidden'),
			Input('perf-graph', 'selectedData'),	
			State('perf-graph', 'figure'),
			prevent_initial_call=True
		)
	def select_data(selectedData, fig):
		hide_buttons = not(perf_proc.plotter.have_selected())
		if not selectedData:
			return no_update, hide_buttons
		if 'interpolate' in perf_proc.prefs:
			if perf_proc.prefs["interpolate"] == 1:
				return no_update, hide_buttons
		prev_layout = fig['layout']
		perf_proc.plotter.select_data(selectedData)
		perf_proc.plotter.update_selected()
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])

		return perf_proc.plotter.fig, hide_buttons
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('add-selected-to-marked', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def add_selected_to_marked(nclicks, fig):
		prev_layout = fig['layout']
		perf_proc.plotter.mark_selected()
		perf_proc.plotter.update_marked()
		#perf_proc.plotter.draw(perf_proc.prefs)
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])	
		return perf_proc.plotter.fig

	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('remove-selected-from-marked', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def remove_selected_from_marked(nclicks, fig):
		prev_layout = fig['layout']
		perf_proc.plotter.unmark_selected()
		perf_proc.plotter.update_marked()
		#perf_proc.plotter.draw(perf_proc.prefs)
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])		
		return perf_proc.plotter.fig
	
	@callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('clear-marked', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def clear_marked(nclicks, fig):
		perf_proc.plotter.clear_marked()	
		perf_proc.plotter.update_marked()	
		#perf_proc.plotter.draw(perf_proc.prefs)
		prev_layout = fig['layout']
		if 'dragmode' in prev_layout:		
			perf_proc.plotter.fig.update_layout(dragmode=prev_layout['dragmode'])		
		
		return perf_proc.plotter.fig
	
	@callback(
	    Output('perf-graph', 'figure', allow_duplicate=True),
	    Input('perf-graph', 'relayoutData'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def relayout_event(relayoutData, fig):
		if 'dragmode' in relayoutData:
			perf_proc.plotter.fig = go.Figure(fig)	
			perf_proc.plotter.clear_selected()		
			perf_proc.plotter.update_selected()
			if 'layout' in perf_proc.plotter.fig:
				perf_proc.plotter.fig.update_layout(dragmode=relayoutData['dragmode'])
		else:		
			prev_layout = fig['layout']
			if 'dragmode' in prev_layout:	
				if 'layout' in perf_proc.plotter.fig:	
					perf_proc.plotter.fig.update_layout(dragmode=prev_layout['dragmode'])	
	
		return 	perf_proc.plotter.fig

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

