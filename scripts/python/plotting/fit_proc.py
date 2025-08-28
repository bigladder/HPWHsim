
from pathlib import Path
import os, sys
import time
import multiprocessing as mp
from common import read_file, write_file, get_perf_map, set_perf_map, get_heat_source_configuration, set_heat_source_configuration
import numpy as np

class FitProc:
		
	def __init__(self):

		self.i_send = 0
		self.running = False
		self.prefs = read_file("prefs.json")
		self.build_dir = self.prefs['build_dir']
		self.constraints = []
		self.parameters = []
		self.metrics = [] 
		
	def update(self):
		fit_list = read_file("fit_list.json")
		self.constraints = [] if ('constraints' not in fit_list) else fit_list['constraints']
		self.parameters = []
		for parameter in fit_list['parameters']:
			if parameter['type'] == "bilinear-coeff":
				constraint = self.constraints[parameter['constraint']]
				self.parameters.append(constraint['value'][parameter['term']])
				
		self.metrics = [] 
		for metric in fit_list['metrics']:
			if metric['type']  == "test_point":
				self.metrics.append(metric['value'])	
	
	#
	def start(self, data):
		if not self.running:
			self.process = mp.Process(target=self.fit, args=(data, ), name='fit-proc')
			time.sleep(1)
			print("launching fit-proc...")
			self.process.start()
			time.sleep(2)	   
			self.running = True
		return {}

	#
	def stop(self):
		if self.running:
			print("killing current fit-proc..")
			self.process.kill()
			time.sleep(1)
			self.running = False
		return {}
	
	def read_cache_model(self, model_id):
			model_cache = read_file("./model_cache.json")	
			if model_id not in model_cache:
				ref_model_filepath = "../../../test/models_json/" + model_id + ".json"
				model_data = read_file(ref_model_filepath)
				model_cache[model_id] = self.prefs["build_dir"] + "/gui/" + model_id + ".json"
				write_file("./model_cache.json", model_cache)
				write_file(model_id, model_data)
				return model_data
			return read_file(model_cache[model_id])
	
	def write_cache_model(self, model_id, model_data):
			model_cache = read_file("./model_cache.json")	
			if model_id not in model_cache:
				model_cache[model_id] = self.prefs["build_dir"] + "/gui/" + model_id + ".json"
				write_file("./model_cache.json", model_cache)
			write_file(model_cache[model_id], model_data)
			
	def apply_bilinear_coefficients(self, model_data, coefficients, variable, dependent, slice):
			if len(coefficients) < 3:
				return
					
			is_central = "central_system" in model_data
			iT3 = 0 if not is_central else slice
			
			perf_map = get_perf_map(model_data)
			grid_vars = perf_map["grid_variables"]
			envTs = grid_vars["evaporator_environment_dry_bulb_temperature"]			
			envTs = [envT - 273.15 for envT in envTs]
					
			hsTs = 	grid_vars["condenser_entering_temperature" if is_central else "heat_source_temperature"]
			hsTs = [hsT - 273.15 for hsT in hsTs]
										
			nT1s = len(envTs)			
			nT2s = len(hsTs)
			nT3s = 1 if not is_central else len(grid_vars["condenser_leaving_temperature"])

			lookup_vars = perf_map["lookup_variables"]
			Pins = lookup_vars["input_power"]
			Pouts = lookup_vars["heating_capacity"]
			COPs = Pouts
			for (elem, value) in enumerate(COPs):
				value /= Pins[elem]
		
			if variable == "Pin":
				param_vars = Pins
			elif variable == "Pout":
				param_vars = Pouts
			elif variable == "COP":
				param_vars = COPs
				for (elem, value) in enumerate(param_vars):
					param_vars[elem] /= Pins[elem]
						
			for iT1 in range(nT1s):
				for iT2 in range(nT2s):
					elem = nT3s * (nT2s * iT1 + iT2) + iT3
					param_vars[elem] 	= coefficients[0] + coefficients[1] * envTs[iT1] + coefficients[2] * hsTs[iT2]

			if dependent == "Pin":
				for (elem, value) in enumerate(param_vars):
					Pins[elem] = Pouts[elem] / COPs[elem]
			elif dependent == "Pout":
				for (elem, value) in enumerate(param_vars):
					Pouts[elem] = COPs[elem] * Pins[elem]
	
	def apply_bilinear_points(self, model_data, variable, dependent, slice, points):
		if len(points) < 3:
			return
		A = []
		x = []
		for (i, point) in enumerate(points):
				x.append(point['value'])
				A.append([1.0, point['Te'], point['Ts']])
		xp = np.array(x)
		Ap = np.array(A)		
		ApT = Ap.transpose()
		ApT_Ap = np.matmul(ApT, Ap)	
		if np.linalg.det(ApT_Ap) == 0:
			return
		iApL = np.matmul(np.linalg.inv(ApT_Ap), ApT)			
		coefficients = iApL.dot(xp)
		self.apply_bilinear_coefficients(model_data, coefficients, variable, dependent, slice)	

	def apply_condenser_distribution(self, new_dist, model_data):
			heat_source_config = get_heat_source_configuration(model_data, "compressor")
			heat_source_config['heat_distribution'] = new_dist
			
			turn_on_logic = heat_source_config['turn_on_logic'][0]
			turn_on_logic["heating_logic"]["temperature_weight_distribution"] = new_dist
			set_heat_source_configuration(model_data, "compressor", heat_source_config)

	def apply_coil_configuration(self, new_config, model_data):
			heat_source_config = get_heat_source_configuration(model_data, "compressor")		
			heat_source_config['heat_source']['performance']['coil_configuration'] = new_config
			set_heat_source_configuration(model_data, "compressor", heat_source_config)
	
	def apply_condenser_turn_on_logic_temperature(self, new_T, model_data):
			heat_source_config = get_heat_source_configuration(model_data, "compressor")
			turn_on_logic = heat_source_config['turn_on_logic'][0]
			turn_on_logic["heating_logic"]["differential_temperature"] = new_T
			set_heat_source_configuration(model_data, "compressor", heat_source_config)

	def apply_number_of_nodes(self, new_nodes, model_data):
			model_data["number_of_nodes"] = new_nodes
			
	def apply_standby_power(self, new_power, model_data):
			heat_source_config = get_heat_source_configuration(model_data, "compressor")		
			heat_source_config['heat_source']['performance']["standby_power"] = new_power
			set_heat_source_configuration(model_data, "compressor", heat_source_config)
										
	def get_performance_point(self, model_data, coordinates, variable):
		is_central = "central_system" in model_data		
		perf_map = get_perf_map(model_data)
		grid_vars = perf_map["grid_variables"]		
		nT1s = len(grid_vars["evaporator_environment_dry_bulb_temperature"])
		col2_name = "condenser_entering_temperature" if is_central else "heat_source_temperature"
		nT2s = len(grid_vars[col2_name])
		nT3s = 1 if not is_central else len(grid_vars["condenser_leaving_temperature"])
	
		lookup_vars = perf_map["lookup_variables"]
		elem = nT3s * (nT2s * coordinates[0] + coordinates[1]) + coordinates[2]
		if variable == "Pin":
			return lookup_vars["input_power"][elem]
		if variable == "Pout":
			return lookup_vars["heating_capacity"][elem]
		if variable == "COP":
				return lookup_vars["heating_capacity"][elem] / lookup_vars["input_power"][elem]
		
	def set_performance_point(self, model_data, coordinates, variable, dependent, value):
		is_central = "central_system" in model_data
		
		orig_perf_map = get_perf_map(model_data)
		perf_map = orig_perf_map
		grid_vars = perf_map["grid_variables"]
		
		nT1s = len(grid_vars["evaporator_environment_dry_bulb_temperature"])
		col2_name = "condenser_entering_temperature" if is_central else "heat_source_temperature"
		nT2s = len(grid_vars[col2_name])
		nT3s = 1 if not is_central else len(grid_vars["condenser_leaving_temperature"])
	
		lookup_vars = perf_map["lookup_variables"]
		elem = nT3s * (nT2s * coordinates[0] + coordinates[1]) + coordinates[2]
		if variable == "Pin":
			lookup_vars["input_power"][elem] = value
		if variable == "Pout":
			lookup_vars["heating_capacity"][elem] = value
		if variable == "COP":
			if dependent == "Pin":
				lookup_vars["input_power"][elem] = lookup_vars["heating_capacity"][elem] / value
			else:
				lookup_vars["heating_capacity"][elem] = value * lookup_vars["input_power"][elem]
		
		perf_map["lookup_variables"] = lookup_vars		
		set_perf_map(model_data, perf_map)
				
	def apply_constraint(self, constraint):
		if constraint['type'] == 'bilinear-points':		
			variable = constraint['variable']
			dependent = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent == variable:
				return
			slice = 0 if not 'slice' in constraint else constraint['slice']
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)							
				self.apply_bilinear_points(model_data, variable, dependent, slice, constraint['value'])							
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'bilinear-coeffs':		
			variable = constraint['variable']
			dependent = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent == variable:
				return
			slice = 0 if not 'slice' in constraint['slice'] else constraint['slice']
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_bilinear_coefficients(constraint['value'], model_data, variable, dependent, slice)				
				self.write_cache_model(model_id, model_data)
					
		if constraint['type'] == 'condenser-distribution':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_condenser_distribution(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'coil-configuration':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)

				self.apply_coil_configuration(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'condenser-turn-on-logic-temperature':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_condenser_turn_on_logic_temperature(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)

		if constraint['type'] == 'number-of-tank-nodes':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				print(f"nodes: {constraint['value']}")
				self.apply_number_of_nodes(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'condenser-standby-power':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				print(f"standby_power: {constraint['value']}")
				self.apply_standby_power(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
											
	def apply_constraints(self):
		for constraint in self.constraints:
			self.apply_constraint(self.constraints[constraint])

	def apply_parameter(self, parameter, new_value):
		if parameter['type'] == 'bilinear-point':					
			constraint = self.constraints[parameter['constraint']]
			constraint['value'][parameter['point']] = new_value
			self.apply_constraint(constraint)
		
		if parameter['type'] == 'bilinear-coeff':		
			constraint = self.constraints[parameter['constraint']]
			constraint['value'][parameter['term']] = new_value	
			self.apply_constraint(constraint)			
									
		if parameter['type'] == 'performance-point':		
			variable = parameter['variable']
			dependent = "COP" if ('dependent' not in parameter) else parameter['dependent']
			if dependent == variable:
				return
			model_data = self.read_cache_model(parameter['model_id'])		
			self.set_performance_point(model_data, parameter['coordinates'], variable, dependent, new_value)
			self.write_cache_model(parameter['model_id'], model_data)

	def get_parameter(self, parameter):
		if parameter['type'] == 'bilinear-point':					
			constraint = self.constraints[parameter['constraint']]
			variable = constraint['variable']
			dependent = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent == variable:
				return			
			model_data = self.read_cache_model(constraint['model_id'])					
			return constraint['value'][parameter['point']]	
		
		if parameter['type'] == 'bilinear-coeff':		
			constraint = self.constraints[parameter['constraint']]
			variable = constraint['variable']
			dependent = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent == variable:
				return
			model_data = self.read_cache_model(constraint['model_id'])
			return constraint['value'][parameter['term']]								
					
		if parameter['type'] == 'performance-point':		
			variable =  parameter['variable']
			dependent = "COP" if ('dependent' not in parameter) else parameter['dependent']
			if dependent == variable:
				return
			model_data = self.read_cache_model(parameter['model_id'])	
			return self.get_performance_point(self, model_data, parameter['coordinates'], variable)	

	def get_parameters(self):
		parameterV = []
		for parameter in self.parameters:
			parameterV.append(self.get_parameter(parameter))
		return parameterV
				
	def apply_parameters(self):
		for parameter in self.parameters:
			self.apply_parameter(parameter)
			
	def fit(self, data):
		self.update()
		self.apply_constraints()
		
		parameterV = self.get_parameters()
		print(parameterV)
		self.apply_parameters()
		
	# Runs the fitting process
	
		n_params = len(self.parameters)
		n_metrics = len(self.metrics)

		done = False
		i_iter = 0


fit_proc = FitProc()


# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1
	if n_args == 0:

		result = fit_proc.start({})

