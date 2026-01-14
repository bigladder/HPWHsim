
import json

def read_file(filename):
	with open(filename, 'r') as json_file:
		return json.load(json_file)
	
def write_file(filename, json_data):
	with open(filename, 'w') as json_file:
		json.dump(json_data, json_file, indent=2)			


def get_performance(model_data):
	is_central = "central_system" in model_data
	if is_central:
		return model_data["central_system"]	
	else:
		wh = model_data["integrated_system"]
		return wh["performance"]	 

def get_perf_map(model_data):
	perf = get_performance(model_data)		 
	hscs = perf["heat_source_configurations"]	
	for hsc in hscs:
		if "heat_source_type" in hsc:
			if hsc["heat_source_type"] in {"CONDENSER", "AIRTOWATERHEATPUMP"}:
				hs = hsc["heat_source"]
				hs_perf = hs["performance"]
				perf_map = hs_perf["performance_map"]
				return perf_map

	return {}

def set_perf_map(model_data, perf_map):
	perf = get_performance(model_data)
	hscs = perf["heat_source_configurations"]	
	for ihs, hsc in enumerate(hscs):
		if "heat_source_type" in hsc:
			if hsc["heat_source_type"] in {"CONDENSER", "AIRTOWATERHEATPUMP"}:
				hsc["heat_source"]["performance"]["performance_map"] = perf_map							
				perf["heat_source_configurations"][ihs] = hsc
				if "central_system" in model_data: 
					model_data["central_system"] = perf
				else:
					model_data["integrated_system"]["performance"] = perf
				return

def get_tank(model_data):
	if "central_system" in model_data:
		return model_data["central_system"]["tank"]
	else:
		return model_data["integrated_system"]["performance"]["tank"]
					
def get_tank_volume(model_data):
	return get_tank(model_data)["performance"]["volume"] * 1000	 
		
def get_heat_source_configuration(model_data, heat_source_id):
	perf = get_performance(model_data)
	hscs = perf["heat_source_configurations"]	
	for hsc in hscs:
		if "id" in hsc:
			if hsc["id"] == heat_source_id:
				return hsc
			
def set_heat_source_configuration(model_data, heat_source_id, heat_source_config):
	perf = get_performance(model_data)

	hscs = perf["heat_source_configurations"]	
	for hsc in hscs:
		if "id" in hsc:
			if hsc["id"] == heat_source_id:
				hsc = heat_source_config
				if is_central: 
					model_data["central_system"] = perf
				else:
					model_data["integrated_system"]["performance"] = perf
				return
