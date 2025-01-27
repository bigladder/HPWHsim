import os
import pandas as pd  # type: ignore
import numpy as np
import json
import math

import plotly.graph_objects as go

styles = {
    'pre': {
        'border': 'thin lightgrey solid',
        'overflowX': 'scroll'
    }
}


class PerfPlotter:
	def __init__(self):
		self.fig = {}
		self.data = {}
		self.have_data = False
		self.is_central = False
		self.i3 = 0
		
		self.variables = ['InputPower', 'HeatingCapacity', 'COP']
		self.variable = self.variables[1]
		
	def get_slice(self):
		self.Pins = []
		self.Pouts = []
		self.COPs= []
		i = 0
		nT1s = np.size(self.T1s)
		nT2s = np.size(self.T2s)
		nT3s = 1 if not self.is_central else np.size(self.T3s)
		for i2y in range(nT2s):
			rowPin = []
			rowPout = []
			rowCOP = []
			for i1x in range(nT1s):
				elem = nT2s * (nT3s * i1x + self.i3) + i2y
				rowPin.append(self.vPins[elem])
				rowPout.append(self.vPouts[elem])
				rowCOP.append(self.vCOPs[elem])
			self.Pins.append(rowPin)
			self.Pouts.append(rowPout)
			self.COPs.append(rowCOP)						
		self.have_data = True
	
	def read_data(self, model_path):
		try:
			with open(model_path) as json_data:
				self.data = json.load(json_data)
		except:
			return

		if "integrated_system" in self.data:
			self.is_central = False
			wh = self.data["integrated_system"]
			perf = wh["performance"]
		else:
			self.is_central = True
			perf = self.data["central_system"]			 
	
		hscs = perf["heat_source_configurations"]
		
		for hsc in hscs:
			if "heat_source_type" in hsc:
				if hsc["heat_source_type"] in {"CONDENSER", "AIRTOWATERHEATPUMP"}:
					hs = hsc["heat_source"]				
					self.perf_map = hs["performance"]["performance_map"]
					break
				
		grid_vars = self.perf_map["grid_variables"]
		lookup_vars = self.perf_map["lookup_variables"]
	
		self.T1s = np.array(grid_vars["evaporator_environment_dry_bulb_temperature"])
		if self.is_central:
			self.T2s = np.array(grid_vars["condenser_entering_temperature"])
			self.T3s = np.array(grid_vars["condenser_leaving_temperature"])
		else:
			self.T2s = np.array(grid_vars["heat_source_temperature"])
		
		to_C = lambda x: x - 273.15
		self.T1s -= 273.15
		self.T2s -= 273.15

		self.vPins = np.array(lookup_vars["input_power"])
		self.vPouts = np.array(lookup_vars["heating_capacity"])
		self.vCOPs = np.zeros(np.size(self.vPins))
		i = 0
		for Pin in self.vPins:
			self.vCOPs[i] = self.vPouts[i] / Pin
			i = i + 1
			
		self.i3 = 0 if not self.is_central else math.floor(np.size(self.T3s) / 2)
		self.get_slice()
 							   
	def draw(self):	
		if self.variable == self.variables[0]:
			zPlot = self.Pins
		elif self.variable == self.variables[1]:
			zPlot = self.Pouts
		else:
			zPlot = self.COPs	
		
		self.fig = go.Figure(data =
										 go.Contour(z = zPlot, x = self.T1s, y = self.T2s))
		self.fig.update_layout()
		return self

def plot(model_path):
	plotter = PerfPlotter()
	plotter.read_data(model_path)
	plotter.draw()
	return plotter

def write_plot(model_path):
	plotter = PerfPlotter()
	plotter.read_data(model_path)
	#plotter.draw()
	#plotter.plot.write_html_plot(plot_path)
	
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args > 1:
        model_path = sys.argv[1]
        write_plot(model_path)
