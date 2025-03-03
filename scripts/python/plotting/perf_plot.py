import os
import pandas as pd  # type: ignore
import numpy as np
import json

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
		else:
			self.T2s = np.array(grid_vars["heat_source_temperature"])

		to_C = lambda x: x - 273.15
		to_C(self.T1s)
		to_C(self.T2s)

		self.Pins = np.array(lookup_vars["input_power"])
		self.Pouts = np.array(lookup_vars["heating_capacity"])
						
		self.have_data = True
 							   
	def draw(self):		
		if self.have_data:
			self.fig = go.Figure(data =
											 go.Contour(z = self.Pins, x = self.T1s, y = self.T2s))
		else:
			return
		
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
