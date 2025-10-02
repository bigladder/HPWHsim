
from pathlib import Path
import os, sys
import time
import multiprocessing as mp
from common import read_file, write_file, get_perf_map, set_perf_map, get_heat_source_configuration, set_heat_source_configuration,get_tank
import numpy as np
from simulate import simulate
from test_data import DataSet

def get_damped_inverse_left(M, nu):
	MT = M.transpose()
	MT_M = np.matmul(MT, M)	
	num = np.size(MT_M, 0)
	wMT_M = MT_M + nu * np.diag(np.diag(MT_M))
	d = np.linalg.det(wMT_M)
	if d == 0:
		print("det = 0")
		return M, False
	return np.matmul(np.linalg.inv(wMT_M), MT).astype(float), True

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
		self.parameters = [] if ('parameters' not in fit_list) else fit_list['parameters']		
		self.metrics = [] if ('metrics' not in fit_list) else fit_list['metrics']
	
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
	
	def find_cache_model(self, model_id):
		model_cache = read_file("./model_cache.json")	
		if model_id not in model_cache:
			ref_model_filepath = "../../../test/models_json/" + model_id + ".json"
			model_data = read_file(ref_model_filepath)
			model_cache[model_id] = self.prefs["build_dir"] + "/gui/" + model_id + ".json"
			write_file("./model_cache.json", model_cache)
			write_file(model_id, model_data)
		return model_cache[model_id]


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

	def apply_heat_distribution(self, heat_source_id, new_dist, model_data):
			heat_source_config = get_heat_source_configuration(model_data, heat_source_id)
			heat_source_config['heat_distribution'] = new_dist	
			set_heat_source_configuration(model_data, heat_source_id, heat_source_config)

	def apply_on_logic_distribution(self, heat_source_id, on_logic_index, new_dist, model_data):
			heat_source_config = get_heat_source_configuration(model_data, heat_source_id)
			turn_on_logic = heat_source_config['turn_on_logic'][on_logic_index]
			turn_on_logic["heating_logic"]["temperature_weight_distribution"] = new_dist
			set_heat_source_configuration(model_data, heat_source_id, heat_source_config)

	def apply_coil_configuration(self, new_config, model_data):
			heat_source_config = get_heat_source_configuration(model_data, "compressor")		
			heat_source_config['heat_source']['performance']['coil_configuration'] = new_config
			set_heat_source_configuration(model_data, "compressor", heat_source_config)
	
	def apply_turn_on_logic_temperature(self, heat_source_id, new_T, model_data):
			heat_source_config = get_heat_source_configuration(model_data,heat_source_id)
			turn_on_logic = heat_source_config['turn_on_logic'][0]
			turn_on_logic["heating_logic"]["differential_temperature"] = new_T
			set_heat_source_configuration(model_data, heat_source_id, heat_source_config)

	def apply_number_of_nodes(self, new_nodes, model_data):
			model_data["number_of_nodes"] = new_nodes
			
	def apply_standby_power(self, new_power, model_data):
			heat_source_config = get_heat_source_configuration(model_data, "compressor")		
			heat_source_config['heat_source']['performance']["standby_power"] = new_power
			set_heat_source_configuration(model_data, "compressor", heat_source_config)

	def apply_tank_mixing(self, new_value, model_data):
			tank = get_tank(model_data)		
			tank["bottom_fraction_of_tank_mixing_on_draw"] = new_value
										
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

	def set_performance_points_offset(self, model_data, variable, dependent, value):
		is_central = "central_system" in model_data
		
		orig_perf_map = get_perf_map(model_data)
		perf_map = orig_perf_map
		grid_vars = perf_map["grid_variables"]
		
		nTs = [len(grid_vars["evaporator_environment_dry_bulb_temperature"])]
		col2_name = "condenser_entering_temperature" if is_central else "heat_source_temperature"
		nTs.append(len(grid_vars[col2_name]))
		nTs.append(1 if not is_central else len(grid_vars["condenser_leaving_temperature"]))
	
		lookup_vars = perf_map["lookup_variables"]
		for x0 in range(nTs[0]):
			for x1 in range(nTs[1]):
				for x2 in range(nTs[2]):
					elem = nTs[2] * (nTs[1] * x0 + x1) + x2
					if variable == "Pin":
						lookup_vars["input_power"][elem] += value
					if variable == "Pout":
						lookup_vars["heating_capacity"][elem] += value
					if variable == "COP":
						if dependent == "Pin":
							Pin = lookup_vars["input_power"][elem]
							lookup_vars["input_power"][elem] = 1 / ((1 / Pin) + (value / lookup_vars["heating_capacity"][elem])) - Pin
						else:
							lookup_vars["heating_capacity"][elem] += value * lookup_vars["input_power"][elem]
		
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
					
		if constraint['type'] == 'heat-distribution':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_heat_distribution(constraint['heat_source'], constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'on-logic-distribution':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				on_logic_index = 0 if "on_logic_index" not in constraint else constraint["on_logic_index"]
				self.apply_on_logic_distribution(constraint['heat_source'], on_logic_index, constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'heat-and-on-logic-distribution':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_heat_distribution(constraint['value'], model_data)
				self.apply_on_logic_distribution(constraint['heat_source'], constraint['value'], model_data)					
				self.write_cache_model(model_id, model_data)
			
		if constraint['type'] == 'turn-on-logic-temperature':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_turn_on_logic_temperature(constraint['heat_source'], constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
							
		if constraint['type'] == 'coil-configuration':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_coil_configuration(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)

		if constraint['type'] == 'number-of-tank-nodes':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_number_of_nodes(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
				
		if constraint['type'] == 'condenser-standby-power':
			for model_id in constraint['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_standby_power(constraint['value'], model_data)				
				self.write_cache_model(model_id, model_data)
											
	def apply_constraints(self):
		for constraint in self.constraints:
			self.apply_constraint(self.constraints[constraint])
	
	def set_parameter_value(self, parameter, x):
		if parameter['type'] == 'bilinear-point':					
			constraint = self.constraints[parameter['constraint']]
			variable = constraint['variable']
			dependent = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent == variable:
				return			
			model_data = self.read_cache_model(constraint['model_id'])					
			constraint['value'][parameter['point']]	= x
			self.apply_constraint(constraint)
		
		if parameter['type'] == 'bilinear-coeff':		
			constraint = self.constraints[parameter['constraint']]
			variable = constraint['variable']
			dependent = "COP" if ('dependent' not in constraint) else constraint['dependent']
			if dependent == variable:
				return
			model_data = self.read_cache_model(constraint['model_id'])
			constraint['value'][parameter['term']]	= x							
			self.apply_constraint(constraint)
						
		if parameter['type'] == 'performance-point':		
			variable =  parameter['variable']
			dependent = "COP" if ('dependent' not in parameter) else parameter['dependent']
			if dependent == variable:
				return
			for model_id in parameter['models']:
				model_data = self.read_cache_model(model_id)
				self.set_performance_point(model_data, parameter['coordinates'], variable, dependent, x)	
				self.write_cache_model(model_id, model_data)
				
		if parameter['type'] == 'performance-points-offset':		
			variable =  parameter['variable']
			dependent = "COP" if ('dependent' not in parameter) else parameter['dependent']
			if dependent == variable:
				return
			for model_id in parameter['models']:
				model_data = self.read_cache_model(model_id)
				self.set_performance_points_offset(model_data, variable, dependent, x - parameter['value'])	
				self.write_cache_model(model_id, model_data)
			parameter['value'] = x
			
		if parameter['type'] == 'tank-mixing':		
			for model_id in parameter['models']:
				model_data = self.read_cache_model(model_id)
				self.apply_tank_mixing(x, model_data)
				self.write_cache_model(model_id, model_data)
		
	def get_parameter_value(self, parameter):
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
			for model_id in parameter['models']:
				model_data = self.read_cache_model(model_id)
				parameter['value'] = self.get_performance_point(model_data, parameter['coordinates'], variable)
				return parameter['value']

		if parameter['type'] == 'performance-points-offset':		
			return parameter['value']
		
		if parameter['type'] == 'tank-mixing':		
				return parameter['value']

	def get_metric_value(self, metric):
		if metric['type'] == 'analysis':		
			test_index = read_file("./test_index.json");
			test_data = test_index['tests'][metric['test_id']]
			test_dir = "../../../test/" 											 
			if 'path' in test_data:
					test_dir = os.path.join(test_dir, test_data['path' ])			 
			test_dir = os.path.join(test_dir, metric['test_id'])
			model_filepath = self.find_cache_model(metric['model_id'])
			data = {	
				"model_spec": "JSON",		
				"model_id_or_filepath": model_filepath,
				"is_standard_test": 1 if "is_standard_test" in metric else 0,
				'test_dir': test_dir,
				'build_dir': self.prefs['build_dir']
			}
			simulate(data)

			data_filename = metric['test_id'] + "_JSON_" + metric["model_id"] + ".csv";
			data_filepath = os.path.join(self.prefs["build_dir"], "test", "output", data_filename)	
			dataset = DataSet({'model_id': metric['model_id'], 'test_id': metric['test_id'], 'type': "Simulated", 'filepath': data_filepath})
			dataset.analyze()
			if metric['group'] in dataset.test_summary:
				if metric['item'] in dataset.test_summary[metric['group']]:
					return dataset.test_summary[metric['group']][metric['item']]
				
		if metric['type'] == 'performance-point':
			test_index = read_file("./test_index.json");
			test_data = test_index['tests'][metric['test_id']]
			test_dir = "../../../test/" 											 
			if 'path' in test_data:
					test_dir = os.path.join(test_dir, test_data['path' ])			 
			test_dir = os.path.join(test_dir, metric['test_id'])
			model_filepath = self.find_cache_model(metric['model_id'])
			data = {	
				"model_spec": "JSON",		
				"model_id_or_filepath": model_filepath,
				"is_standard_test": 1 if "is_standard_test" in metric else 0,
				'test_dir': test_dir,
				'build_dir': self.prefs['build_dir']
			}
			simulate(data)

			data_filename = metric['test_id'] + "_JSON_" + metric["model_id"] + ".csv";
			data_filepath = os.path.join(self.prefs["build_dir"], "test", "output", data_filename)	
			dataset = DataSet({'model_id': metric['model_id'], 'test_id': metric['test_id'], 'type': "Simulated", 'filepath': data_filepath})
			return dataset.df[metric['variable']].iloc[metric['i_min']]
		
		return 0	
	
	def get_parameter_values(self):
		paramV = []
		for parameter in self.parameters:
			paramV.append(self.get_parameter_value(parameter))
		return paramV
				
	def get_parameters(self):
		paramV = []
		for parameter in self.parameters:
			param = self.get_parameter(parameter)
			paramV.append({"value": param, "increment": parameter['increment']})
		return paramV
				
	def apply_parameters(self):
		for parameter in self.parameters:
			self.apply_parameter(parameter)
	
	def get_metric_values(self):
		metricV = []
		for metric in self.metrics:
			metricV.append(self.get_metric_value(metric))
		return metricV
			
	def fit(self, data):
		self.update()	
	
		paramsV = self.get_parameter_values()	
		metricsV = self.get_metric_values()
		diff0V = [0] * len(metricsV)
		for i_metric, metric in enumerate(self.metrics):
			diff0V[i_metric] = (metricsV[i_metric] - metric['target']) / metric['tolerance']
		FOM = np.matmul(diff0V, diff0V)								
		print(f"parameters: {paramsV}")		
		print(f"metrics: {metricsV}")
		print(f"FOM: {FOM}")
				
		nu = 0.1
		for iter in range(10):
			print(f"\niteration: {iter}, nu = {nu}")
				
			# get jacobian
			print("\nget jacobian")
			jacobiM = np.zeros([len(metricsV), len(paramsV)])
			for (i_param, parameter) in enumerate(self.parameters):
				self.set_parameter_value(parameter, paramsV[i_param] + parameter['increment'])
				#print(f"parameter {i_param}: {paramsV[i_param] + parameter['increment']}, inc: {parameter['increment']}")	
				metricsV = self.get_metric_values()
				#print(f"metrics: {metricsV}")	
				for i_metric, metric in enumerate(self.metrics):
					diff = (metricsV[i_metric] - metric['target']) / metric['tolerance']
					jacobiM[i_metric][i_param] = (diff - diff0V[i_metric]) / parameter['increment']									
				self.set_parameter_value(parameter, paramsV[i_param])
										
			print(f"jacobian:\n{jacobiM}")	
			
			
			# invert with damping nu
			print("\nusing nu")
			FOM_1 = 1e12
			ijML, got1 = get_damped_inverse_left(jacobiM, nu)
			if got1:	
				p_inc1V = -np.matmul(ijML, np.array(diff0V)).astype(float)
				print(f"p_inc: {p_inc1V}")
				for (i_param, parameter) in enumerate(self.parameters):
					self.set_parameter_value(parameter, paramsV[i_param] + p_inc1V[i_param])
					#print(f"parameter {i_param}: {paramsV[i_param] + p_inc1V[i_param]}")				
				metricsV = self.get_metric_values()
				#print(f"metrics: {metricsV}")			
				diffV = [0] * len(metricsV)
				for i_metric, metric in enumerate(self.metrics):
					diffV[i_metric] = (metricsV[i_metric] - metric['target']) / metric['tolerance']
				FOM_1 = np.matmul(diffV, diffV)
				print(f"FOM_1: {FOM_1}")				
				for (i_param, parameter) in enumerate(self.parameters):
					self.set_parameter_value(parameter, paramsV[i_param])
			
			# invert with damping nu/2
			print("\nusing nu/2")
			FOM_2 = 1e12
			ijML, got2 = get_damped_inverse_left(jacobiM, nu / 2)
			if got2:
				p_inc2V = -np.matmul(ijML, np.array(diff0V)).astype(float)
				print(f"p_inc: {p_inc2V}")
				for (i_param, parameter) in enumerate(self.parameters):
					self.set_parameter_value(parameter, paramsV[i_param] + p_inc2V[i_param])
					#print(f"parameter {i_param}: {paramsV[i_param] + p_inc2V[i_param]}")	
				metricsV = self.get_metric_values()
				#print(f"metrics: {metricsV}")
				diffV = [0] * len(metricsV)
				for i_metric, metric in enumerate(self.metrics):
					diffV[i_metric] = (metricsV[i_metric] - metric['target']) / metric['tolerance']
				FOM_2 = np.matmul(diffV, diffV)
				print(f"FOM_2: {FOM_2}\n")	
				for (i_param, parameter) in enumerate(self.parameters):
					self.set_parameter_value(parameter, paramsV[i_param])
			
			if got1 and got2:
				p_incV = []					
				if FOM_1 < FOM or FOM_2 < FOM:
					if FOM_1 < FOM_2:
						p_incV = p_inc1V
						FOM = FOM_1
						print("accept 1")
					else:
						p_incV = p_inc2V
						FOM = FOM_2
						nu /= 2		
						print("accept 2")	
					for (i_param, parameter) in enumerate(self.parameters):
						paramsV[i_param] +=  p_incV[i_param]
						self.set_parameter_value(parameter, paramsV[i_param])				
						parameter['increment'] = 	p_incV[i_param] / 100		
				else:
					nu *= 10
					print("no improvement")
					if nu > 1e6:
						print(f"Unable to improve fit.")
						return
			else:
				print(f"Unable to invert.")
				break
									
			paramsV = self.get_parameter_values()									
			metricsV = self.get_metric_values()
			diff0V = [0] * len(metricsV)
			for i_metric, metric in enumerate(self.metrics):
				diff0V[i_metric] = (metricsV[i_metric] - metric['target']) / metric['tolerance']
			FOM = np.matmul(diff0V, diff0V)

			print(f"parameters: {paramsV}")
			print(f"metrics: {metricsV}")	
			print(f"FOM: {FOM}")										
fit_proc = FitProc()


# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1
	if n_args == 0:

		result = fit_proc.start({})

