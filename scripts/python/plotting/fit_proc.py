
from pathlib import Path
import os, sys
from common import read_file, write_file, get_perf_map, set_perf_map

class FitProc:
		
	def __init__(self):

		self.i_send = 0



		self.prefs = read_file("prefs.json")
		self.build_dir = self.prefs['build_dir']
		
		fit_list = read_file("fit_list.json")
		self.constraints = [] if ('constraints' not in fit_list) else fit_list['constraints']
		self.parameters = [] if ('parameters' not in fit_list) else fit_list['parameters']
		self.metrics = [] if 'metrics' not in fit_list else fit_list['metrics']
	
	def apply_constraint(self, constraint):
		print(constraint)
		if constraint['type'] == 'bilinear-coeffs':		
			param_var = constraint['variable']
			dependent_var = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent_var == param_var:
				return	
			
			model_cache = read_file("./model_cache.json")	
			if constraint['model_id'] not in model_cache:
				ref_model_filepath = "../../../test/models_json/" + constraint['model_id'] + ".json";	
				model_data = read_file(ref_model_filepath);
				model_cache[constraint['model_id']] = self.prefs["build_dir"] + "/test/output/gui/" + constraint['model_id'] + ".json";
				write_file("./model_cache.json", model_cache);
				write_file(model_cache[constraint['model_id']], model_data);

			model_data = read_file(model_cache[constraint['model_id']])					
			coefficients = constraint['value']
			
			orig_perf_map = get_perf_map(model_data)
			perf_map = orig_perf_map
					
			is_central = "central_system" in model_data
			grid_vars = perf_map["grid_variables"]
			envTs = grid_vars["evaporator_environment_dry_bulb_temperature"]			
			for envT in envTs:
				envT -= 273.15
				
			hsTs = 	grid_vars["condenser_entering_temperature" if is_central else "heat_source_temperature"]
			for hsT in hsTs:
				hsT -= 273.15
										
			nT1s = len(envTs)			
			nT2s = len(hsTs)
			nT3s = 1 if not is_central else len(grid_vars["condenser_leaving_temperature"])
		
			lookup_vars = perf_map["lookup_variables"]
			Pins = lookup_vars["input_power"]
			Pouts = lookup_vars["heating_capacity"]
			COPs = Pouts
			for (elem, value) in enumerate(COPs):
				value /= Pins[elem]
					
			if param_var == "Pin":
				param_vars = Pins
			elif param_var == "Pout":
				param_vars = Pouts
			elif param_var == "COP":
				param_vars = COPs
				for (elem, value) in enumerate(param_vars):
					param_vars[elem] /= Pins[elem]
						
			for iT1 in range(nT1s):
				for iT2 in range(nT2s):
					for iT3 in range(nT3s):
						elem = nT3s * (nT2s * iT1 + iT2) + iT3
						param_vars[elem] 	= coefficients[0] + coefficients[1] * envTs[iT1] + coefficients[2] * hsTs[iT2]

			if dependent_var == "Pin":
				for (elem, value) in enumerate(param_vars):
					Pins[elem] = Pouts[elem] / COPs[elem]
			elif dependent_var == "Pout":
				for (elem, value) in enumerate(param_vars):
					Pouts[elem] = COPs[elem] * Pins[elem]
				
			#set_perf_map(					
			write_file(model_cache[constraint['model_id']], model_data)
					
	def apply_constraints(self):
		for constraint in self.constraints:
			self.apply_constraint(self.constraints[constraint])

	def set_param(self, param, new_value):	
		if param['type'] == 'bilinear-coeff':		
			term = param['term']
					
			constraint = self.constraints[param['constraint']]
			param_var = constraint['variable']
			dependent_var = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent_var == param_var:
				return	
			
			ref_model_filepath = "../../../test/models_json/" + constraint['model_id'] + ".json";	
			model_cache = read_file("./model_cache.json")	
			model_filepath = model_cache[constraint['model_id']]	
			model_data = read_file(model_filepath)
						
			self.constraints[param['constraint']]['value'][term] = new_value
			coefficients = self.constraints[param['constraint']]['value']		
			
			orig_perf_map = get_perf_map(model_data)
			perf_map = orig_perf_map
					
			is_central = "central_system" in model_data
			grid_vars = perf_map["grid_variables"]
			envTs = grid_vars["evaporator_environment_dry_bulb_temperature"]			
			for envT in envTs:
				envT -= 273.15
				
			hsTs = 	grid_vars["condenser_entering_temperature" if is_central else "heat_source_temperature"]
			for hsT in hsTs:
				hsT -= 273.15
										
			nT1s = len(envTs)			
			nT2s = len(hsTs)
			nT3s = 1 if not is_central else len(grid_vars["condenser_leaving_temperature"])
		
			lookup_vars = perf_map["lookup_variables"]
			Pins = lookup_vars["input_power"]
			Pouts = lookup_vars["heating_capacity"]
			COPs = Pouts
			for (elem, value) in enumerate(COPs):
				value /= Pins[elem]
					
			if param_var == "Pin":
				param_vars = Pins
			elif param_var == "Pout":
				param_vars = Pouts
			elif param_var == "COP":
				param_vars = COPs
				for (elem, value) in enumerate(param_vars):
					param_vars[elem] /= Pins[elem]
						
			for iT1 in range(nT1s):
				for iT2 in range(nT2s):
					for iT3 in range(nT3s):
						elem = nT3s * (nT2s * iT1 + iT2) + iT3
						param_vars[elem] 	= coefficients[0] + coefficients[1] * envTs[iT1] + coefficients[2] * hsTs[iT2]

			if dependent_var == "Pin":
				for (elem, value) in enumerate(param_vars):
					Pins[elem] = Pouts[elem] / COPs[elem]
			elif param_var == "Pout":
				for (elem, value) in enumerate(param_vars):
					Pouts[elem] = COPs[elem] * Pins[elem]
				
			#set_perf_map(					
			write_file(model_filepath, model_data)
					
		if param['type'] == 'perf-point':		
			param_var =  param['variable']
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
		
			dependent_var = param['dependent']
			if dependent_var == param_var:
				return

			lookup_vars = perf_map["lookup_variables"]
			coords = param["coords"]		
			elem = nT2s * (nT3s * coords[0] + coords[2]) + coords[1]
			if param_var == "Pin":
				lookup_vars["input_power"][elem] = value
			if param_var == "Pout":
				lookup_vars["heating_capacity"][elem] = value
			if param_var == "COP":
				if dependent_var == "Pin":
					lookup_vars["input_power"][elem] = lookup_vars["heating_capacity"][elem] / value
				else:
					lookup_vars["heating_capacity"][elem] = value * lookup_vars["input_power"][elem]
			
			perf_map["lookup_variables"] = lookup_vars		
			set_perf_map(perf_map)
			write_file(model_data_filepath, model_data)		
	
# Runs the fitting process
def launch_fit_proc():

	#launch_fit_proc.proc = mp.Process(target=fit_proc, args=(data, ), name='fit-proc')
	print("launching fit process...")
	
	fit_proc = FitProc()
	result = fit_proc.apply_constraints()
	#await launch_fit_proc.proc.start()
	return result
	   

launch_fit_proc.proc = -1


# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1
	if n_args == 0:

		result = launch_fit_proc()

