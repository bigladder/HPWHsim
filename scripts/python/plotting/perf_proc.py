from dash import Dash, html, dcc, Input, Output, State, no_update
from dash_extensions import WebSocket
import multiprocessing as mp
from pathlib import Path
import os
import json
import time
from common import read_file, write_file
from perf_plot import PerfPlotter

# Global variable to store the WebSocket
def perf_proc(data):

	perf_proc.ws = None

	perf_proc.i_send = 0

	orig_dir = str(Path.cwd())
	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	def replot(data):
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
			perf_proc.plotter.update_markers(perf_proc.prefs)
			perf_proc.plotter.update_dependent(perf_proc.prefs)
		
			perf_proc.plotter.fig.update_layout(clickmode='event+select')

			show_outletTs = perf_proc.plotter.is_central
			outletTs = []
			if show_outletTs:
				i = 0
				for outletT in perf_proc.plotter.T3s:
					outletTs.append({'label': f"{outletT:.2f} \u00B0C", 'value': i})
					i = i + 1
				
			show_list = []
			if perf_proc.prefs["show_points"] == 1:
				show_list= ['points']
				
			interp_list = []
			if perf_proc.prefs["interpolate"] == 1:
				interp_list= ['interpolate']

			return perf_proc.plotter.fig, not(perf_proc.plotter.have_data), not(show_outletTs), perf_proc.plotter.iT3, outletTs, show_list, interp_list, perf_proc.prefs['contour_variable'], perf_proc.prefs['contour_coloring'], perf_proc.prefs["Nx"], perf_proc.prefs["Ny"]

		return no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update
	
	external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']		
	app = Dash(__name__, external_stylesheets=external_stylesheets)

	styles = {
		'pre': {
			'border': 'thin lightgrey solid',
			'overflowX': 'scroll'
		}
	}

	app.layout = [
   	WebSocket(url="ws://localhost:8600", id="ws"),
 		html.Div(html.Button("send", id='send-btn', n_clicks=0), hidden=True),
		 
		html.Div([
			html.P("display variable:", style={'fontSize': '12px', 'margin': '4px', 'display': 'inline-block'}),
			dcc.Dropdown(
						options = [
							{'label': 'Input Power (W)', 'value': 0}, 
					 		{'label': 'Heating Capacity (W)', 'value': 1},
							{'label': 'COP', 'value': 2}],
						id='display-dropdown',
						style={'width': '50%', 'display': 'inline-block', 'verticalAlign': 'middle'},
						clearable=False)
			]),
						
		html.Div([
			html.P("condenser outlet temperature (\u00B0C)", style={'fontSize': '12px', 'margin': '4px', 'display': 'inline-block'}),							
			dcc.Dropdown(
					id='outletT-dropdown',
					style={'width': '50%', 'display': 'inline-block', 'verticalAlign': 'middle'},
					clearable=False)
			], 
			id="outletT-p", 
			hidden = True, 
		),
					
		html.Div([
			html.P("coloring", style={'fontSize': '12px', 'margin': '4px', 'display': 'inline-block'}),
			dcc.Dropdown(options = [{'label': 'none', 'value': 0}, {'label': 'heatmap', 'value': 1}, {'label': 'lines', 'value': 2}],
				id='coloring-dropdown',
				style= {'width': '50%', 'display': 'inline-block', 'verticalAlign': 'middle'},
				clearable=False)
				]),
			
		html.P("show"),
		dcc.Checklist(
			    options = [{'label': 'points', 'value': 'points'}],
					id="show-check"
			),
			
		dcc.Checklist(
			    options = [{'label': 'interpolate', 'value': 'interpolate'}],
					id="interp-check"
			),
		html.Div(	[		
				dcc.Input(id='Nx-input', type='number'),
				dcc.Input(id='Ny-input', type='number')
			], id='interp-sizes', hidden=True
		),				
			
		html.Div([
				html.Button("x", id='make-dependent', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),
				html.P("marked:", style={'fontSize': '12px', 'margin': '4px', 'display': 'inline-block'}),
				html.Button("+", id='add-selected-to-marked', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),
				html.Button("-", id='remove-selected-from-marked', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),
				html.Button("clear", id='clear-marked', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),	
				html.Button("vary", id='vary-marked', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),				
				html.Button("hold", id='hold-marked', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),				
				],
				id='select-div',
				hidden = True
			),
					
		html.Br(),
		html.Div(
			dcc.Graph(id='perf-graph', figure={}, style ={'width': '1200px', 'height': '800px', 'display': 'block'},
				config={
	        'modeBarButtonsToAdd': [
	        "drawrect",
	        "eraseshape"
	        ]
	    	}),
				id="graph-div",
				hidden = True)

	]

	@app.callback(
		Output("ws", "send"),
		Input("send-btn", "n_clicks")
	)
	def send_message(n_clicks):
		perf_proc.i_send = perf_proc.i_send + 1
		message = json.dumps({
			"source": "perf-proc",
			"dest": "index",
			"cmd": "init-perf-proc",
			"index": perf_proc.i_send})
		return message

	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('graph-div', 'hidden'),
			Output('outletT-p', 'hidden'),
			Output('outletT-dropdown', 'value'),
			Output('outletT-dropdown', 'options'),
			Output('show-check', 'value'),
			Output('interp-check', 'value'),
			Output('display-dropdown', 'value'),
			Output('coloring-dropdown', 'value'),
			Output('Nx-input', 'value', allow_duplicate=True),
			Output('Ny-input', 'value', allow_duplicate=True),
			[Input("ws", "message")],
			prevent_initial_call=True
			)
	def message(msg):
		if 'data' in msg:
			data = json.loads(msg['data'])
			if 'dest' in data:
				if data['dest'] == 'perf-proc':
					print(f"received by perf-proc:\n{data}")
					if 'cmd' in data:
						if data['cmd'] == 'replot':										
							return replot(data)
								
		return no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update, no_update

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
		perf_proc.plotter.update_markers(perf_proc.prefs)
		perf_proc.plotter.update_selected(perf_proc.prefs)
		perf_proc.plotter.update_marked(perf_proc.prefs)
		perf_proc.plotter.update_dependent(perf_proc.prefs)
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
		perf_proc.plotter.update_markers(perf_proc.prefs)
		perf_proc.plotter.update_selected(perf_proc.prefs)
		perf_proc.plotter.update_marked(perf_proc.prefs)
		perf_proc.plotter.update_dependent(perf_proc.prefs)
		
		return perf_proc.plotter.fig
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('Nx-input', 'value', allow_duplicate=True),
			Output('Ny-input', 'value', allow_duplicate=True),
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
		perf_proc.plotter.update_markers(perf_proc.prefs)
		perf_proc.plotter.update_selected(perf_proc.prefs)
		perf_proc.plotter.update_dependent(perf_proc.prefs)
		perf_proc.plotter.update_marked(perf_proc.prefs)
		return perf_proc.plotter.fig, perf_proc.prefs["Nx"], perf_proc.prefs["Ny"]
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('display-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_variable(value):	
		perf_proc.prefs['contour_variable'] = value
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
			perf_proc.plotter.update_markers(perf_proc.prefs)
			perf_proc.plotter.update_selected(perf_proc.prefs)
			perf_proc.plotter.update_dependent(perf_proc.prefs)
			perf_proc.plotter.update_marked(perf_proc.prefs)
			return perf_proc.plotter.fig
		return no_update
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('coloring-dropdown', 'value'),
			prevent_initial_call=True
		)
	def select_coloring(value):
		perf_proc.prefs['contour_coloring'] = value
		if perf_proc.plotter.have_data:
			perf_proc.plotter.draw(perf_proc.prefs)
			perf_proc.plotter.update_markers(perf_proc.prefs)
			perf_proc.plotter.update_selected(perf_proc.prefs)
			perf_proc.plotter.update_dependent(perf_proc.prefs)
			perf_proc.plotter.update_marked(perf_proc.prefs)
			return perf_proc.plotter.fig
		return no_update
	
	@app.callback(
				Output('perf-graph', 'figure', allow_duplicate=True),
				Input('outletT-dropdown', 'value'),
				prevent_initial_call=True
			)
	def select_outletT(value):
		if perf_proc.plotter.have_data:
			if value == None:
				return perf_proc.plotter.fig
			if perf_proc.plotter.is_central:
				perf_proc.plotter.iT3 = value
			else:
				perf_proc.plotter.iT3 = 0
			perf_proc.plotter.get_slice()
			perf_proc.plotter.draw(perf_proc.prefs)
			perf_proc.plotter.update_markers(perf_proc.prefs)
			perf_proc.plotter.update_selected(perf_proc.prefs)
			perf_proc.plotter.update_dependent(perf_proc.prefs)
			perf_proc.plotter.update_marked(perf_proc.prefs)	
			return perf_proc.plotter.fig
		return no_update

	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Output('select-div', 'hidden', allow_duplicate=True),
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
		perf_proc.plotter.update_selected(perf_proc.prefs)
		if 'range' in prev_layout:
			perf_proc.plotter.fig.update_layout(range = prev_layout['range'])
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])
		return perf_proc.plotter.fig, hide_buttons
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('add-selected-to-marked', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def add_selected_to_marked(nclicks, fig):
		prev_layout = fig['layout']
		perf_proc.plotter.mark_selected(perf_proc.prefs)
		perf_proc.plotter.update_marked(perf_proc.prefs)
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])	
		return perf_proc.plotter.fig

	@app.callback(
		Output('perf-graph', 'figure', allow_duplicate=True),
		Output('select-div', 'hidden', allow_duplicate=True),
		Input('perf-graph', 'clickData'),
		prevent_initial_call=True
	)
	def click_data(clickData):
		hide_buttons = not(perf_proc.plotter.have_selected())
		if not clickData:
			return no_update, hide_buttons
		if 'interpolate' in perf_proc.prefs:
			if perf_proc.prefs["interpolate"] == 1:
				return no_update, hide_buttons
		prev_layout = fig['layout']
		perf_proc.plotter.click_data(clickData)
		perf_proc.plotter.update_selected(perf_proc.prefs)
		if 'range' in prev_layout:
			perf_proc.plotter.fig.update_layout(range = prev_layout['range'])
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])
		return perf_proc.plotter.fig, hide_buttons
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('make-dependent', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def make_selected_dependent(nclicks, fig):
		prev_layout = fig['layout']
		perf_proc.plotter.make_selected_dependent(perf_proc.prefs)
		perf_proc.plotter.update_dependent(perf_proc.prefs)
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])	
		return perf_proc.plotter.fig
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('remove-selected-from-marked', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def remove_selected_from_marked(nclicks, fig):
		prev_layout = fig['layout']
		perf_proc.plotter.unmark_selected(perf_proc.prefs)
		perf_proc.plotter.update_marked(perf_proc.prefs)
		if 'dragmode' in prev_layout:
			perf_proc.plotter.fig.update_layout(dragmode = prev_layout['dragmode'])		
		return perf_proc.plotter.fig
	
	@app.callback(
			Output('perf-graph', 'figure', allow_duplicate=True),
			Input('clear-marked', 'n_clicks'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def clear_marked(nclicks, fig):
		perf_proc.plotter.clear_marked()	
		perf_proc.plotter.update_marked(perf_proc.prefs)	
		prev_layout = fig['layout']
		if 'dragmode' in prev_layout:		
			perf_proc.plotter.fig.update_layout(dragmode=prev_layout['dragmode'])		
		
		return perf_proc.plotter.fig

	@app.callback(
			Output("ws", "send", allow_duplicate=True),
			Input('vary-marked', 'n_clicks'),
			prevent_initial_call=True
	)
	def vary_marked(nclicks):
		add_points = perf_proc.plotter.get_marked_list(perf_proc.prefs)
		
		fit_list = read_file("fit_list.json")
		if 'parameters' in fit_list:
			params = fit_list['parameters']
		else:
			params = {}			

		new_params = params
		for point in add_points:
			found_point = False
			for param in params:
				if 'type' not in param or param['type'] != 'perf-point':
					continue
				if 'variable' not in param or param['variable'] != point['variable']:
					continue
				if 'model_id' not in param or param['model_id'] != point['model']:
					continue
					
				if 'coords' not in param:
					continue

				coords = param['coords']
				if coords[0] != point['coords'][0]:
						continue
				if coords[1] != point['coords'][1]:
						continue
				if coords[2] != point['coords'][2]:
						continue
		
				found_point = True
				break
			
			if not found_point:
				new_params.append(point)
		
		fit_list['parameters'] = new_params			
		write_file("fit_list.json", fit_list)
		perf_proc.i_send = perf_proc.i_send + 1
		msg = {"source": "perf-proc", "dest": "index", "cmd": "refresh-fit", "index": perf_proc.i_send}
		return json.dumps(msg)

	@app.callback(
			Output('ws', 'send', allow_duplicate=True),
			Input('hold-marked', 'n_clicks'),
			prevent_initial_call=True
	)
	def hold_marked(nclicks):
		remove_points = perf_proc.plotter.get_marked_list(perf_proc.prefs)
		
		fit_list = read_file("fit_list.json")
		if 'parameters' in fit_list:
			params = fit_list['parameters']
		else:
			params = []			

		new_params = params
		for point in remove_points:
			for index, param in reversed(list(enumerate(params))):
				if 'type' not in param or param['type'] != 'perf-point':
					continue
				if 'variable' not in param or param['variable'] != point['variable']:
					continue
				if 'model' not in param or param['model'] != point['model']:
					continue
					
				if 'coords' not in param:
					continue
				
				coords = param['coords']
				if coords[0] != point['coords'][0]:
						continue
				if coords[1] != point['coords'][1]:
						continue
				if coords[2] != point['coords'][2]:
						continue
								
				del new_params[index]
		
		fit_list['parameters'] = new_params				
		write_file("fit_list.json", fit_list)
		perf_proc.i_send = perf_proc.i_send + 1
		msg = {"source": "perf-proc", "dest": "index", "cmd": "refresh-fit", "index": perf_proc.i_send}
		return json.dumps(msg)

	@app.callback(
	    Output('perf-graph', 'figure', allow_duplicate=True),
	    Input('perf-graph', 'relayoutData'),
			State('perf-graph', 'figure'),
			prevent_initial_call=True
	)
	def relayout_event(relayoutData, fig):
		if 'dragmode' in relayoutData:
			if relayoutData['dragmode'] == 'select' or relayoutData['dragmode'] == 'lasso':
				perf_proc.plotter.clear_selected()		
				perf_proc.plotter.update_selected(perf_proc.prefs)
				perf_proc.plotter.fig.update_layout(dragmode= relayoutData['dragmode'])
				return 	perf_proc.plotter.fig
		elif 'range' in fig:
			perf_proc.plotter.fig.update_layout(range = fig['range'])
			return 	perf_proc.plotter.fig
		return fig
			
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