# From enclosing folder launch with "poetry run python wstest/dash_ws.py".

from dash_extensions.enrich import DashProxy, html, dcc, Input, Output
from dash_extensions import WebSocket
import multiprocessing as mp
import math
import time

# Create example app.
def perf_proc(data):
	print("launching")
	app = DashProxy(prevent_initial_callbacks=True)
	app.layout = html.Div([
	    dcc.Input(id="input", autoComplete="off"), html.Div(id="message"),
	    WebSocket(url="ws://localhost:8600", id="ws"),
	    html.Button("send", id='send-btn', n_clicks=0),
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
