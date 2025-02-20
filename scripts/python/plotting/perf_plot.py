import os
import pandas as pd  # type: ignore
import numpy as np
import json
import math
from scipy.interpolate import RegularGridInterpolator

import plotly.graph_objects as go
from common import read_file, write_file, get_perf_map

styles = {
    'pre': {
        'border': 'thin lightgrey solid',
        'overflowX': 'scroll'
    }
}

def ff(x, y):
    	return x**2 + y**2

class PerfPlotter():
	def __init__(self, label):
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
		
		self.label = label
		
		self.maxSize = 24
						
		self.variables = ['InputPower', 'HeatingCapacity', 'COP']
		
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

	def prepare(self, data):
		try:
			model_data = data['model_data']
			self.is_central = "central_system" in model_data
			self.perf_map = get_perf_map(model_data)
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
										
			self.selected = np.zeros((nT1s, nT2s))
			self.marked = np.zeros((nT1s, nT2s, nT3s))
			self.i3 = 0 if not self.is_central else math.floor(len(self.T3s) / 2)
			self.get_slice()
			
		except:
			self.have_data = False
	
	def select_data(self, selectedData):
		if 'points' in selectedData:
			if selectedData['points']:
				self.clear_selected()
				nT1s = len(self.T1s)
				for point in selectedData['points']:
					idx = point['pointIndex']
					iT2 = int(idx / nT1s)
					iT1 = idx % nT1s
					#print(f"idx: {idx}, iT1: {iT1}, iT2: {iT2}")
					self.selected[iT1, iT2] = 1
	
	def have_selected(self):			
		for iT1, T1 in enumerate(self.T1s):
			for iT2, T2 in enumerate(self.T2s):
				if self.selected[iT1, iT2] == 1:
					return True
		return False
	
	def clear_selected(self):			
		for iT1, T1 in enumerate(self.T1s):
			for iT2, T2 in enumerate(self.T2s):
				self.selected[iT1, iT2] = 0
					 
	def update_marks(self, value):
		for iT1, T1 in enumerate(self.T1s):
				for iT2, T2 in enumerate(self.T2s):
					if self.selected[iT1, iT2]:
						self.marked[iT1, iT2, self.i3] = value
	
	def mark_selected(self):
		self.update_marks(1)
		
	def unmark_selected(self):
		self.update_marks(0)

	def reset_selected(self):
			nT1s = len(self.T1s)
			nT2s = len(self.T2s)
			nT3s = 1 if not self.is_central else len(self.T3s)
			self.selected = np.zeros((nT1s, nT2s, nT3s))
	
	def reset_marked(self):
			nT1s = len(self.T1s)
			nT2s = len(self.T2s)
			self.selected = np.zeros((nT1s, nT2s))
	
						   
	def draw(self, prefs):
		if not self.have_data:
			self.fig = {}
			return
			
		graph_title = self.label
		if graph_title != "":
			graph_title += " - "
			
		value_label = ""
		if 'contour_variable' in prefs:	
			if prefs['contour_variable'] == 0:
				plotVals = self.Pins
				value_label = "Pin (W)"
				graph_title += "Input Power (W)"
			elif prefs['contour_variable'] == 1:
				plotVals = self.Pouts
				value_label = "Pout (W)"
				graph_title += "Heating Capacity (W)"
			else:
				plotVals = self.COPs
				value_label = "COP"
				graph_title += "COP"
		else:
			plotVals = self.Pouts
			value_label = "Pout"
			graph_title += " - Heating Capacity (W)"

		if self.is_central:
			graph_title += f" - Toutlet (\u00B0C) = {self.T3s[self.i3]:8.2f}"
			
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
          						)))
										)
		
		if "show_points" in prefs:
			if prefs["show_points"] == 1:
				
				# set marker sizes
				markerSizes = []
				fac = 0.5
				zMin = min(zp)
				zMax = max(zp)	
				for z in zp:
					diam = self.maxSize * ((1 - fac) * (z - zMin) / (zMax - zMin) + fac)
					markerSizes.append(diam)
			
				selectedMarkers = {}
				selectedMarkers['x'] = []
				selectedMarkers['y'] = []
				selectedMarkers['size'] = []
				
				markedMarkers = {}
				markedMarkers['x'] = []
				markedMarkers['y'] = []
				
				i=0
				for iT2, T2 in enumerate(self.T2s):
					for iT1, T1 in enumerate(self.T1s):
						
						if self.selected[iT1, iT2]:
								selectedMarkers['x'].append(T1)
								selectedMarkers['y'].append(T2)
								selectedMarkers['size'].append(markerSizes[i] + 5)							
											
						if self.marked[iT1, iT2, self.i3]:
							markedMarkers['x'].append(T1)
							markedMarkers['y'].append(T2)
						
						i = i + 1									

				x_label = f"Tenv (\u00B0C)"
				y_label = f"Tinlet (\u00B0C)" if self.is_central else f"Tcond (\u00B0C)"	
				hover_labels = []
				if prefs['contour_variable'] == 2:
					for z in zp:
						hover_labels.append([x_label, y_label, value_label, f"{z:8.4f}"])
				else:
					for z in zp:
						hover_labels.append([x_label, y_label, value_label, f"{z:8.2f}"])				

				if markerSizes:
					self.fig = go.Figure(self.fig)

					trace_all = go.Scatter(
						name = "all points", 
						x = xp, 
						y = yp,
						mode="markers", 
						marker_size=markerSizes,
						marker_symbol='circle',
						marker_color='green',
						marker_line_color= 'green',
						marker_line_width = 0,
							
						showlegend = False,
						customdata = hover_labels,
						hoverinfo="skip",			 
						hovertemplate = 
							"%{customdata[0]}: %{x}<br>" +
							"%{customdata[1]}: %{y}<br>" +
							"%{customdata[2]}: %{customdata[3]}" +
		          "<extra></extra>"
					)						
					self.fig.add_trace(trace_all)			
					
					trace_selected = go.Scatter(
						name = "selected points", 
						x = selectedMarkers['x'], 
						y = selectedMarkers['y'],
						mode="markers", 
						marker_size= selectedMarkers['size'],
						marker_symbol='circle-open',
						marker_color = 'black',
						marker_line_color= 'black',
						marker_line_width = 2,
							
						showlegend = False,
						hoverinfo="skip",
					)	
					self.fig.add_trace(trace_selected)				

					trace_marked = go.Scatter(
						name = "marked points", 
						x = markedMarkers['x'], 
						y = markedMarkers['y'],
						mode="markers", 
						marker_size = 3,
						marker_symbol='circle',
						marker_color = 'black',
						marker_line_color= 'black',
						marker_line_width = 0,
							
						showlegend = False,
						hoverinfo="skip"
					)	
					self.fig.add_trace(trace_marked)	
												
				x_title = "environment temperature (\u00B0C)"
				y_title = "condenser inlet temperature (\u00B0C)" if self.is_central else "condenser temperature (C)"		
				self.fig.update_layout(
							xaxis_title = x_title,
							yaxis_title = y_title,
							title = graph_title,
							title_x=0.5
						)
		
		
		# fix axes to data range
		dX = 0.05 * (self.T1s[-1] - self.T1s[0])
		dY = 0.05 * (self.T2s[-1] - self.T2s[0])
		self.fig.update_xaxes(range=[self.T1s[0] - dX, self.T1s[-1] + dX])
		self.fig.update_yaxes(range=[self.T2s[0] - dY, self.T2s[-1] + dY])
		return self


def write_plot(prefs, model_data, plot_filepath):
	plotter = PerfPlotter()
	plotter.prepare(prefs, model_data)
	if "iT3" in prefs:
		plotter.i3 = prefs["iT3"]
		plotter.get_slice()	
		
	if plotter.have_data:
		plotter.draw(prefs)
		plotter.fig.write_html(plot_filepath)
		
# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1

	if n_args > 1:
		model_id = sys.argv[1]
		model_data_filepath = sys.argv[2]
		variable = sys.argv[3]
		plot_filepath = sys.argv[4]

		model_data = read_file(model_data_filepath)
		write_plot(model_id, model_data, {'contour_variable': variable}, plot_filepath)
