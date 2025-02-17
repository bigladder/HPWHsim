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
		
		# grid vars
		self.T1s = []
		self.T2s = []
		self.T3s = []
		
		# lookup vars
		self.Pins = []
		self.Pouts = []
		self.COPs= []
		
		# extended lists of grid vars
		self.extT1s = []
		self.extT2s = []

		# extended lists of grid vars
		self.extPins = []
		self.extPouts = []
		self.extCOPs = []
						
		self.variables = ['InputPower', 'HeatingCapacity', 'COP']
	
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
		
	def get_slice(self):	
					
		nT1s = len(self.T1s)
		nT2s = len(self.T2s)
		nT3s = 1 if not self.is_central else len(self.T3s)
		
		self.extT1s = []
		self.extT2s = []
		self.Pins = []
		self.Pouts = []
		self.COPs = []
	
		lookup_vars = self.perf_map["lookup_variables"]
		zPins = lookup_vars["input_power"]
		zPouts = lookup_vars["heating_capacity"]
		zCOPs = []
		for Pin, Pout in zip(zPins, zPouts):
			zCOPs.append(Pout / Pin)
			
		# extend grid vars
		for i2y in range(nT2s):
			for i1x in range(nT1s):		
				self.extT1s.append(self.T1s[i1x])
				self.extT2s.append(self.T2s[i2y])
				
				elem = nT2s * (nT3s * i1x + self.i3) + i2y																
				self.Pins.append(zPins[elem])
				self.Pouts.append(zPouts[elem])
				self.COPs.append(zCOPs[elem])
				
		self.have_data = True

	def prepare(self, model_data):
		try:
			self.is_central = "central_system" in model_data
			self.perf_map = self.get_perf_map(model_data)
			grid_vars = self.perf_map["grid_variables"]

			self.T1s = [x - 273.15 for x in grid_vars["evaporator_environment_dry_bulb_temperature"]]
			if self.is_central:
				self.T2s = [x - 273.15 for x in grid_vars["condenser_entering_temperature"]]
				self.T3s = [x - 273.15 for x in grid_vars["condenser_leaving_temperature"]]
			else:
				self.T2s = [x - 273.15 for x in grid_vars["heat_source_temperature"]]			

			nT1s = len(self.T1s)
			nT2s = len(self.T2s)
			nT3s = 1 if not self.is_central else len(self.T3s)
										
			self.selected = np.zeros((nT1s, nT2s, nT3s))
			self.i3 = 0 if not self.is_central else math.floor(len(self.T3s) / 2)
			self.get_slice()
			
		except:
			self.have_data = False
 	
	def select(self, x0, y0, x1, y1):
			nT1s = np.size(self.T1s)
			nT2s = np.size(self.T2s)
			for i2y in range(nT2s):				
				y = self.T2s[i2y]
				if y >= y0 and y <= y1:
					for i1x in range(nT1s):
						x = self.T1s[i1x]
						if x >= x0 and x <= x1:
							self.selected[i1x, i2y, self.i3] = 1
											   
	def draw(self, prefs):
		if not self.have_data:
			self.fig = {}
			return
		
		graph_title = ""
		value_label = ""
		if 'contour_variable' in prefs:	
			if prefs['contour_variable'] == 0:
				plotVals = self.Pins
				value_label = "Pin (W)"
				graph_title = "Input Power (W)"
			elif prefs['contour_variable'] == 1:
				plotVals = self.Pouts
				value_label = "Pout (W)"
				graph_title = "Heating Capacity (W)"
			else:
				plotVals = self.COPs
				value_label = "COP"
				graph_title = "COP"
		else:
			plotVals = self.Pouts
			value_label = "Pout"
			graph_title = "Heating Capacity (W)"

# original data as np.arrays referred to a regular grid
		xc = np.array(self.T1s)
		yc = np.array(self.T2s)
		zc = np.array(plotVals).reshape(np.size(yc), np.size(xc))
		
# original data as lists of point coordinates
		xp = self.extT1s
		yp = self.extT2s
		zp = plotVals
		
		if 'interpolate' in prefs:
			if prefs['interpolate'] == 1:

				# define RGI
				zs = np.array(zc).transpose()
				interp = RegularGridInterpolator((xc, yc), zs, method='linear')
				
				# generate mesh and interpolate	
				nX = prefs['Nx']
				nY = prefs['Ny']
				xc = np.linspace(xc[0], xc[-1], nX)
				yc = np.linspace(yc[0], yc[-1], nY)
				xg, yg = np.meshgrid(xc, yc)
				zc = interp((xg, yg))
			
				# modify xp, yp, zp
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
		
		# show points
		fac = 0.5
		zMin = min(zp)
		zMax = max(zp)	
		markerSize = []
		if 'show_points' in prefs:
			if prefs['show_points'] == 1:
	
				for z in zp:
					diam = 20 * ((1 - fac) * (z - zMin) / (zMax - zMin) + fac)
					markerSize.append(diam)
			else:
				for z in zp:
					markerSize.append(1)
	
		x_label = f"Tenv (\u00B0C)"
		y_label = f"Tinlet (\u00B0C)" if self.is_central else f"Tcond (\u00B0C)"	
		hover_labels = []
		for z in zp:
			hover_labels.append([x_label, y_label, value_label, z])			

		if markerSize:
			self.fig = go.Figure(self.fig)
			trace = go.Scatter(
				name = "points", 
				x = xp, 
				y = yp, 
				marker_size=markerSize,
				mode="markers",			
				showlegend = False,
				customdata = hover_labels,			 
				hovertemplate = 
					"%{customdata[0]}: %{x}<br>" +
					"%{customdata[1]}: %{y}<br>" +
					"%{customdata[2]}: %{customdata[3]}" +
          "<extra></extra>"
			)	
			
			self.fig.add_trace(trace)
			
			# show selected points
			xsel = []
			ysel = []
			zsel = []
			nT1s = len(self.T1s)
			nT2s = len(self.T2s)
			for i2y in range(nT2s):
				for i1x in range(nT1s):		
					if self.selected[i1x, i2y, self.i3] == 1:
						xsel.append(self.T1s[i1x])
						ysel.append(self.T2s[i2y])
						elem = nT1s * i2y + i1x
						z =  plotVals[elem]
						diam = 20 * ((1 - fac) * (z - zMin) / (zMax - zMin) + fac) + 2
						zsel.append(diam)

			if xsel and ysel and zsel:
				self.fig = go.Figure(self.fig)
				trace = go.Scatter(name = "points", x = xsel, y = ysel,
					mode='markers',
					marker=dict(
            size=zsel,					
						color="black", symbol="circle-open"
            ),
					showlegend = False
					)		
				self.fig.add_trace(trace)
				
							
			x_title = "environment temperature (\u00B0C)"
			y_title = "condenser inlet temperature (\u00B0C)" if self.is_central else "condenser temperature (C)"		
			self.fig.update_layout(
						xaxis_title = x_title,
						yaxis_title = y_title,
						title = graph_title,
						title_x=0.5
					)
		
		
		# fix to data range
		dX = 0.05 * (self.T1s[-1] - self.T1s[0])
		dY = 0.05 * (self.T2s[-1] - self.T2s[0])
		self.fig.update_xaxes(range=[self.T1s[0] - dX, self.T1s[-1] + dX])
		self.fig.update_yaxes(range=[self.T2s[0] - dY, self.T2s[-1] + dY])
		return self
	
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args > 1:
        model_path = sys.argv[1]
        write_plot(model_path)
