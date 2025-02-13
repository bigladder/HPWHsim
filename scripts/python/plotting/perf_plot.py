import os
import pandas as pd  # type: ignore
import numpy as np
import json
import math
from scipy.interpolate import RegularGridInterpolator

import plotly.graph_objects as go

styles = {
    'pre': {
        'border': 'thin lightgrey solid',
        'overflowX': 'scroll'
    }
}

def ff(x, y):
    	return x**2 + y**2

class PerfPlotter:
	def __init__(self):
		self.fig = {}
		self.data = {}
		self.have_data = False
		self.is_central = False
		self.i3 = 0
		
		self.variables = ['InputPower', 'HeatingCapacity', 'COP']
		
	def get_slice(self):
		self.Pins = []
		self.Pouts = []
		self.COPs= []
		
		self.xPoint = []
		self.yPoint = []
		
		self.zPins = []
		self.zPouts = []
		self.zCOPs= []
		
		nT1s = np.size(self.T1s)
		nT2s = np.size(self.T2s)
		nT3s = 1 if not self.is_central else np.size(self.T3s)
		
		for i2y in range(nT2s):
			rowPin = []
			rowPout = []
			rowCOP = []
			for i1x in range(nT1s):
				elem = nT2s * (nT3s * i1x + self.i3) + i2y
				
				self.xPoint.append(self.T1s[i1x])
				self.yPoint.append(self.T2s[i2y])
																
				self.zPins.append(self.vPins[elem])
				self.zPouts.append(self.vPouts[elem])
				self.zCOPs.append(self.vCOPs[elem])
				
				rowPin.append(self.vPins[elem])
				rowPout.append(self.vPouts[elem])
				rowCOP.append(self.vCOPs[elem])

			self.Pins.append(rowPin)
			self.Pouts.append(rowPout)
			self.COPs.append(rowCOP)						
		self.have_data = True

	def get_perf_map(self, model_data):
		if "integrated_system" in model_data:
			wh = model_data["integrated_system"]
			perf = wh["performance"]
		else:
			perf = model_data["central_system"]			 
	
		hscs = perf["heat_source_configurations"]	
		for hsc in hscs:
			if "heat_source_type" in hsc:
				if hsc["heat_source_type"] in {"CONDENSER", "AIRTOWATERHEATPUMP"}:
					hs = hsc["heat_source"]
					hs_perf = hs["performance"]
					perf_map = hs_perf["performance_map"]
					return perf_map
					break		
		return {}

	def prepare(self, model_data):
		try:
			self.is_central = "central_system" in model_data
			self.perf_map = self.get_perf_map(model_data)
			grid_vars = self.perf_map["grid_variables"]
			lookup_vars = self.perf_map["lookup_variables"]

			self.T1s = np.array(grid_vars["evaporator_environment_dry_bulb_temperature"]) - 273.15
			if self.is_central:
				self.T2s = np.array(grid_vars["condenser_entering_temperature"]) - 273.15
				self.T3s = np.array(grid_vars["condenser_leaving_temperature"]) - 273.15
			else:
				self.T2s = np.array(grid_vars["heat_source_temperature"]) - 273.15

			
			self.vPins = np.array(lookup_vars["input_power"])
			self.vPouts = np.array(lookup_vars["heating_capacity"])
			self.vCOPs = np.zeros(np.size(self.vPins))
			i = 0
			for Pin in self.vPins:
				self.vCOPs[i] = self.vPouts[i] / Pin
				i = i + 1
				
			self.i3 = 0 if not self.is_central else math.floor(np.size(self.T3s) / 2)
			self.get_slice()
		except:
			self.have_data = False
 							   
	def draw(self, prefs):
		if not self.have_data:
			self.fig = {}
			print("no data")
			return
		
		if 'contour_variable' in prefs:	
			if prefs['contour_variable'] == 0:
				zPlot = self.Pins
				zSizes = self.zPins
			elif prefs['contour_variable']  == 1:
				zPlot = self.Pouts
				zSizes = self.zPouts
			else:
				zPlot = self.COPs	
				zSizes = self.zCOPs
		else:
			zPlot = self.Pouts
			zSizes = self.zPouts
	

		xc = self.T1s
		yc = self.T2s
		zc = zPlot
		
		xp = self.xPoint
		yp = self.yPoint
		zp = zSizes
		
		if 'interpolate' in prefs:
			if prefs['interpolate'] == 1:

				zs = np.array(zPlot)
				#zs = z0.reshape(np.size(self.T1s), np.size(self.T2s))

				interp = RegularGridInterpolator((self.T1s, self.T2s), zs.transpose(), method='linear')
				
				#interp				
				nX = prefs['Nx']
				nY = prefs['Ny']
				xc = np.linspace(self.T1s[0], self.T1s[-1], nX)
				yc = np.linspace(self.T2s[0], self.T2s[-1], nY)
				xg, yg = np.meshgrid(xc, yc)
				zc = interp((xg, yg))
			
				xp = []
				yp = []
				for y in yc:
					for x in xc:				
						xp.append(x)
						yp.append(y)
				
				xp = np.array(xp)
				yp = np.array(yp)		
				zp = zc.flatten()
						
		coloring = 'lines'
		if 'contour_coloring' in prefs:
			if prefs['contour_coloring'] == 0:
				coloring = 'heatmap'
					
		self.fig = go.Figure(data =
										 go.Contour(z = zc, x = xc, y = yc,
											contours=dict(
					            coloring = coloring,
					            showlabels = True, # show labels on contours
					            labelfont = dict( # label font properties
					                size = 14,
					                color = 'black',
            					),
											)))
		
		xaxis_title = "environment temperature (\u00B0C)"
		yaxis_title = "condenser inlet temperature (\u00B0C)" if self.is_central else "condenser temperature (C)"
		
		if True:#self.show_points:

			fac = 0.5
			markerSize = []
			zMin = min(zp)
			zMax = max(zp)			
			for z in zp:
				markerSize.append(20 * ((1 - fac) * (z - zMin) / (zMax - zMin) + fac))

			self.fig = go.Figure(self.fig)

			trace = go.Scatter(name = "points", x = xp, y = yp, marker_size=markerSize, mode="markers")		
			self.fig.add_trace(trace)

		self.fig.update_layout({
    	"xaxis": {"title_text": "environment temperature (\u00B0C)", 'title_font': {"size": 14, "color": "black"}},
    	"yaxis": {"title_text": yaxis_title, 'title_font': {"size": 14, "color": "black"}}
			}
			)
		return self
	
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args > 1:
        model_path = sys.argv[1]
        write_plot(model_path)
