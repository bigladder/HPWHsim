# From enclosing folder launch with "poetry run python wstest/dash_ws.py".

from dash_extensions.enrich import Dash, html, dcc, Input, Output
from dash_extensions import WebSocket
import multiprocessing as mp
import math
import time
from perf_plot import PerfPlotter
from common import read_file, write_file

# Create example app.
def perf_proc(data):
	print("launching")
	
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
			perf_proc.plotter.update_markers(perf_proc.prefs)
		
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

	app = Dash(prevent_initial_callbacks=True)
	app.layout = html.Div([
	    dcc.Input(id="input", autoComplete="off"), html.Div(id="message"),
	    WebSocket(url="ws://localhost:8600", id="ws"),
	    html.Button("send", id='send-btn', n_clicks=0),
			html.Div(
				dcc.Graph(
						id='perf-graph',
						figure=fig,
						style ={'width': '1200px', 'height': '800px', 'display': 'block'}
					),
					id="graph-div",
					hidden = not(perf_proc.plotter.have_data)
				)

	])

	@app.callback(Output("ws", "send"), [Input("input", "value")])
	def send(value):
		print("sent from dash app")
		return value

	@app.callback(Output("input", "value"), Input("send-btn", "n_clicks"))
	def send_btn(n_clicks):
		return n_clicks

	@app.callback(Output("message", "children"), [Input("ws", "message")])
	def message(e):
		print("received by dash app")
		return f"Response from websocket: {e['data']}"

	app.run_server(port = perf_proc.port_num)
			
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
