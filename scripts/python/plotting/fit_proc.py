from dash import Dash, html, dcc, Input, Output, State, no_update
from dash_extensions import WebSocket
import multiprocessing as mp
from pathlib import Path
import os
import json
import time
from common import read_file, write_file, get_perf_map
import websockets
from scipy.optimize import least_squares

async def send_message(uri, message):
    async with websockets.connect(uri) as websocket:
        await websocket.send(message)
        print(f"Sent: {message}")
        response = await websocket.recv()
        print(f"Received: {response}")

def fit_proc(data):

	fit_proc.i_send = 0

	orig_dir = str(Path.cwd())
	os.chdir("../../../test")
	abs_repo_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	build_dir = data['build_dir']
	prefs = read_file("prefs.json")
	
	fit_list = read_file("fit_list.json")
	if 'parameters' in fit_list:
		params = fit_list['parameters']
	else:
		params = {}
		
	if 'data' in fit_list:
		data = fit_list['data']
	else:
		data = {}		
		
			
	def get_UEF(param):
		model_spec = 'JSON'
		model_id = param['model_id']
		draw_profile = "auto"
		
		model_data_filepath = "../../../test/models_json/" + model_id + ".json";
		model_data = read_file(model_data_filepath)
		is_central = "central_system" in model_data
		
		orig_perf_map = get_perf_map(model_data)
		perf_map = orig_perf_map
		grid_vars = perf_map["grid_variables"]
		
		nT1s = len(grid_vars["evaporator_environment_dry_bulb_temperature"])
		col2_name = "condenser_entering_temperature" if is_central else "heat_source_temperature"
		nT2s = len(grid_vars[col2_name])
		nT3s = 1 if not is_central else len(grid_vars["condenser_leaving_temperature"])
		
		param_name = param['variable']
		quant_label = "input_power"
		if param_name == "Pout":
			quant_label = "heating_capacity"
		lookup_vars = perf_map["lookup_variables"]
		orig_val = lookup_vars[quant_label]
		zPouts = lookup_vars["heating_capacity"]
		zCOPs = []
		
		return uef

	def diff_val(params, datum):
		if 'type' in datum:
			if datum['type'] == 'UEF':
				for param in params:
					if 'type' in param:
						if param['type'] == 'UEF':
							if param['model_id'] == datum['model_id']:
								new_uef = get_UEF(param)
								targ_uef = datum['UEF']
								return (new_uef - targ_uef) / 1.
							
			return 1.e8

	dataV = []
	for datum in data:
		if 'type' in dataum:
			if datum['type'] == 'UEF':
				dataV.append(datum['target'])

	paramV = []
	for param in params:
		if 'type' in param:
			if param['type'] == 'perf-point':
				paramV.append(param['target'])

	res = least_squares(diffT_t, data, args=params)
	#uri = "ws://localhost:8600"  # Replace with your WebSocket server URI
	#fit_proc.i_send = fit_proc.i_send + 1
	#msg = {"source": "fit-proc", "dest": "index", "cmd": "refresh-fit", "index": fit_proc.i_send}
	result = {"chi-sq": 1000.}
	return result
	#await send_message(uri, msg)

# Runs the fitting process
def launch_fit_proc(data):

	#launch_fit_proc.proc = mp.Process(target=fit_proc, args=(data, ), name='fit-proc')
	print("launching fit process...")
	result = fit_proc(data)
	#await launch_fit_proc.proc.start()
	return result
	   

launch_fit_proc.proc = -1