from dash import Dash, html, dcc, Input, Output, State
from dash_extensions import WebSocket
import multiprocessing as mp
import json
import time

# Global variable to store the WebSocket
def perf_proc(data):

	perf_proc.ws = None

	perf_proc.i_send = 0
	app = Dash(__name__)

	app.layout = html.Div([
	    WebSocket(id="ws", url="ws://localhost:8600"),
	    dcc.Input(id="input-msg", type="text", placeholder="Enter message"),
	    html.Button("Send", id="send-button", n_clicks=0),
	    html.Div(id="output-msg")
	])

	@app.callback(
		Output("ws", "send"),
		Input("send-button", "n_clicks"),
		prevent_initial_call=True
	)
	def send_message(n_clicks):
		perf_proc.i_send = perf_proc.i_send +1
		message = json.dumps({"source": "perf-proc", "dest": "perf-proc", "index": perf_proc.i_send})
		return message

	@app.callback(
		Output("output-msg", "children"),
		Input("ws", "message"),
		prevent_initial_call=True
	)
	def receive_message(msg):
		if 'data' in msg:
			data = msg['data']
			return f"Received: {data}"
		return ""

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

	print("starting dash")
	launch_perf_proc.proc.start()
	time.sleep(2)
	print("launched dash")
	   

launch_perf_proc.proc = -1