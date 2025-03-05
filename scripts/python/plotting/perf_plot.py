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
		self.iT3 = 0
		
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
						
		self.variables = ['Input Power (W)', 'HeatingCapacity (W)', 'COP']
		self.variables_names = ['Pin', 'Pout', 'COP']
		self.variable = 0
		self.dependent = []
		
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
				
				elem = nT2s * (nT3s * i1x + self.iT3) + i2y																
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
										
			self.selected = np.zeros((nT1s, nT2s), dtype=np.uint32)
			self.marked = np.zeros((nT1s, nT2s, nT3s), dtype=np.uint32)
			self.dependent = np.full((nT1s, nT2s, nT3s), 2, dtype=np.uint32)
			self.iT3 = 0 if not self.is_central else math.floor(len(self.T3s) / 2)
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
					self.selected[iT1, iT2] = 1
	
	def have_selected(self):			
		for iT1, T1 in enumerate(self.T1s):
			for iT2, T2 in enumerate(self.T2s):
				if self.selected[iT1, iT2] == 1:
					return True
		return False
	
	def clear_selected(self):
		nT1s = len(self.T1s)
		nT2s = len(self.T2s)			
		self.selected = np.zeros((nT1s, nT2s))
	
	def clear_marked(self):	
		nT1s = len(self.T1s)
		nT2s = len(self.T2s)
		nT3s = 1 if not self.is_central else len(self.T3s)	
		self.marked = np.zeros((nT1s, nT2s, nT3s), dtype=np.uint32)
								 
	def update_marks(self, value, prefs):
		for iT1, T1 in enumerate(self.T1s):
				for iT2, T2 in enumerate(self.T2s):
					this_mark = self.marked[iT1, iT2, self.iT3] & (1 << prefs['contour_variable'])
					other_marks = self.marked[iT1, iT2, self.iT3] & (~this_mark)
					if self.selected[iT1, iT2] and not self.dependent[iT1, iT2, self.iT3] == prefs['contour_variable']:
						this_mark = value * (1 << prefs['contour_variable'])						
					self.marked[iT1, iT2, self.iT3] = other_marks | this_mark
					
	def mark_selected(self, prefs):
		self.update_marks(1, prefs)
		
	def unmark_selected(self, prefs):
		self.update_marks(0, prefs)

	def reset_selected(self):
			nT1s = len(self.T1s)
			nT2s = len(self.T2s)
			nT3s = 1 if not self.is_central else len(self.T3s)
			self.selected = np.zeros((nT1s, nT2s, nT3s))

	def make_selected_dependent(self, prefs):
		for iT1, T1 in enumerate(self.T1s):
			for iT2, T2 in enumerate(self.T2s):
				if self.selected[iT1, iT2]:
					self.dependent[iT1, iT2, self.iT3] = prefs['contour_variable']
			
	def interpolate(self, refs, prefs):
		# define RGI
		xr = np.array(refs[0])
		yr = np.array(refs[1])
		zr = np.array(refs[2]).reshape(np.size(yr), np.size(xr))
		zr = np.array(zr).transpose()
		rgi = RegularGridInterpolator((xr, yr), zr, method='linear')
		
		# generate mesh and interpolate	
		nX = prefs['Nx']
		nY = prefs['Ny']
		
		xp = np.linspace(xr[0], xr[-1], nX)
		yp = np.linspace(yr[0], yr[-1], nY)
		xg, yg = np.meshgrid(xp, yp)
		zp = rgi((xg, yg))
	
		results = [xp.flatten(), yp.flatten(), zp.flatten()]
		return results
			
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
			graph_title += f" - Toutlet (\u00B0C) = {self.T3s[self.iT3]:8.2f}"	
	

		self.refs = [self.T1s, self.T2s, plotVals] 
		self.vals = self.refs	
					
		# data as lists of point coordinates	
		is_interp = ('interpolate' in prefs) and (prefs['interpolate'] == 1)
		if is_interp:
			self.vals = self.interpolate(self.refs, prefs)
		#print(f"refs:\n{self.refs}\n\nvals:\n{self.vals}")
	
		
# original data as np.arrays referred to a regular grid
		xc = np.array(self.vals[0])
		yc = np.array(self.vals[1])
		zc = np.array(self.vals[2]).reshape(np.size(self.vals[1]), np.size(self.vals[0]))	
		self.coords = [xc, yc, zc]
								
		coloring = 'lines'	
		if 'contour_coloring' in prefs:
			if prefs['contour_coloring'] == 0:
				coloring = 'heatmap'

		self.fig = go.Figure(data =
										go.Contour(
											x = self.coords[0],
											y = self.coords[1],
											z = self.coords[2],									
											hoverinfo="skip",
											showscale = False,
											contours=dict(
						            coloring = coloring,
						            showlabels = True, # show labels on contours
						            labelfont = dict( # label font properties
						                size = 14,
						                color = 'black',
	          							)																					
											),
											line_width = 2,
											line_color = 'black',
											showlegend = False,
											)
										)									
		
		points_visible = ('show_points' in prefs) and 	(prefs["show_points"] == 1)
								
		# create point trace (data or interpolated) (initially empty)
		trace_all = go.Scatter(
			name = "all points", 
			x = [], 
			y = [],
			mode="markers", 
			marker_size=[],
			marker_symbol='circle',
			marker_color='green',
			marker_line_color= 'green',
			marker_line_width = 0,
				
			showlegend = False,
			hoverinfo="skip",			 
			hovertemplate = "<extra></extra>",
			visible = points_visible
				
		)						
		self.fig.add_trace(trace_all)			
	
		# create selected trace (empty)
		trace_selected = go.Scatter(
				name = "selected points", 
				x = [], 
				y = [],
				mode="markers", 
				marker_size= [],
				marker_symbol='circle-open',
				marker_color = 'black',
				marker_line_color= 'black',
				marker_line_width = 2,
					
				showlegend = False,
				hoverinfo="skip",
				visible = points_visible
			)	
		self.fig.add_trace(trace_selected)				
		
		# create dependent trace (empty)				
		trace_dependent = go.Scatter(
			name = "dependent points", 
			x = [], 
			y = [],
			mode="markers", 
			marker_size = 6,
			marker_symbol='x-thin',
			marker_color = 'black',
			marker_line_color= 'black',
			marker_line_width = 1,
				
			showlegend = False,
			hoverinfo="skip",
			visible = points_visible
		)	
		self.fig.add_trace(trace_dependent)
			
		# show marked (empty)
		trace_marked = go.Scatter(
			name = "marked points", 
			x = [], 
			y = [],
			mode="markers", 
			marker_size = 3,
			marker_symbol='circle',
			marker_color = 'black',
			marker_line_color= 'black',
			marker_line_width = 0,
				
			showlegend = False,
			hoverinfo="skip",
			visible = points_visible
		)	
		self.fig.add_trace(trace_marked)	

		# axis labels
		x_title = "environment temperature (\u00B0C)"
		y_title = "condenser inlet temperature (\u00B0C)" if self.is_central else "condenser temperature (C)"		
		self.fig.update_layout(
					xaxis_title = x_title,
					yaxis_title = y_title,
					title = graph_title,
					title_x=0.5
				)
			
		# init axes to data range
		dX = 0.05 * (self.vals[0][-1] - self.vals[0][0])
		dY = 0.05 * (self.vals[1][-1] - self.vals[1][0])
		self.fig.update_xaxes(range=[self.vals[0][0] - dX, self.vals[0][-1] + dX])
		self.fig.update_yaxes(range=[self.vals[1][0] - dY, self.vals[1][-1] + dY])
														
		return self

	# show points (data or interpolated)
	def update_markers(self, prefs):
		markers = {}
		markers['x'] = []
		markers['y'] = []
		markers['size'] = []
		markers['hover_labels'] = []
		
		i = 0
		self.points = []
		for T1 in self.vals[1]:
			for T0 in self.vals[0]:
				point = [T0, T1, self.vals[2][i]]
				self.points.append(point)
				i = i + 1

		fac = 0.5
		zMin = min(self.vals[2])
		zMax = max(self.vals[2])	
						
		x_label = f"Tenv (\u00B0C)"
		y_label = f"Tinlet (\u00B0C)" if self.is_central else f"Tcond (\u00B0C)"	
		value_label = self.variables[prefs['contour_variable']]
		
		# create markers
		for point in self.points:
			markers['x'].append(point[0])
			markers['y'].append(point[1])
			diam = self.maxSize * ((1 - fac) * (point[2] - zMin) / (zMax - zMin) + fac)
			markers['size'].append(diam)
			if prefs['contour_variable'] == 2:
				markers['hover_labels'].append([x_label, y_label, value_label, f"{point[2]:8.4f}"])
			else:
				markers['hover_labels'].append([x_label, y_label, value_label, f"{point[2]:8.2f}"])
			i = i + 1	
			   	
		points_visible = ('show_points' in prefs) and 	(prefs["show_points"] == 1)
							
		self.fig.update_traces(
			x = markers['x'],
			y = markers['y'],
			marker_size = markers['size'],
			customdata = markers['hover_labels'],
			hovertemplate = "%{customdata[0]}: %{x}<br>" +
					"%{customdata[1]}: %{y}<br>" +
					"%{customdata[2]}: %{customdata[3]}" +
	        "<extra></extra>",
			selector = dict(name="all points")
			)
#
	def update_selected(self, prefs):
		selectedMarkers = {}
		selectedMarkers['x'] = []
		selectedMarkers['y'] = []
		selectedMarkers['size'] = []
		
		i = 0
		selected_points = []								
		for iT2, T2 in enumerate(self.refs[1]):
			for iT1, T1 in enumerate(self.refs[0]):					
				if self.selected[iT1, iT2]:
					point = [T1, T2, self.vals[2][i]]
					selected_points.append(point)
				i = i + 1
			
		fac = 0.5
		zMin = min(self.refs[2])
		zMax = max(self.refs[2])				

		for point in selected_points:
			selectedMarkers['x'].append(point[0])
			selectedMarkers['y'].append(point[1])					
			diam = self.maxSize * ((1 - fac) * (point[2] - zMin) / (zMax - zMin) + fac) + 5
			selectedMarkers['size'].append(diam)	
					
		self.fig.update_traces(
			x = selectedMarkers['x'],
			y = selectedMarkers['y'],
			marker_size = selectedMarkers['size'],
			selector = dict(name="selected points")
			)

#
	def update_dependent(self, prefs):

		dependMarkers = {}
		dependMarkers['x'] = []
		dependMarkers['y'] = []
		dependMarkers['size'] = []
		
		fac = 0.5
		zMin = min(self.refs[2])
		zMax = max(self.refs[2])	
		
		i = 0
		dependent_points = []								
		for iT2, T2 in enumerate(self.refs[1]):
			for iT1, T1 in enumerate(self.refs[0]):
				depends = self.dependent[iT1, iT2, self.iT3]
				if depends == prefs['contour_variable']:			
					point = [T1, T2, self.refs[2][i]]
					dependent_points.append(point)
				i = i + 1
			
		for point in dependent_points:
			dependMarkers['x'].append(point[0])
			dependMarkers['y'].append(point[1])
			diam = 0.8 * self.maxSize * ((1 - fac) * (point[2] - zMin) / (zMax - zMin) + fac)
			dependMarkers['size'].append(diam)
		
		self.fig.update_traces(
			x = dependMarkers['x'],
			y = dependMarkers['y'],
			marker_size = dependMarkers['size'],
			selector = dict(name="dependent points"))			
		
#
	def update_marked(self, prefs):
		markedMarkers = {}
		markedMarkers['x'] = []
		markedMarkers['y'] = []
		markedMarkers['color'] = []
		
		for iT2, T2 in enumerate(self.refs[1]):
			for iT1, T1 in enumerate(self.refs[0]):

					marks = self.marked[iT1, iT2, self.iT3]
					if  marks > 0:
						markedMarkers['x'].append(T1)
						markedMarkers['y'].append(T2)
						if (marks & (1 << prefs['contour_variable'])) > 0:
							markedMarkers['color'].append('black')
						else:
							markedMarkers['color'].append('white')
		
		self.fig.update_traces(
			x = markedMarkers['x'],
			y = markedMarkers['y'],
			marker_color = markedMarkers['color'],
			selector = dict(name="marked points"))
		

	def get_marked_list(self, prefs):
		nT3s = 1 if not self.is_central else len(self.T3s)
		marked_list = []
		if self.is_central:
				for iT2, T2 in enumerate(self.refs[1]):
					for iT1, T1 in enumerate(self.refs[0]):
						for i in range(3):
							if (self.marked[iT1, iT2, self.iT3] & (1 << i)) & (1 << prefs['contour_variable']) > 0:
								entry = {'type': 'perf-point'}
								entry['model'] = self.label
								entry['variable'] = self.variables_names[i]
								entry['dependent'] = self.variables_names[self.dependent[iT1, iT2, self.iT3]]						
								entry['coords'] = [iT1, iT2, self.iT3]
								marked_list.append(entry)
								
		else:
			for iT2, T2 in enumerate(self.refs[1]):
				for iT1, T1 in enumerate(self.refs[0]):
					for i in range(3):
						if self.marked[iT1, iT2, self.iT3] & (1 << i) & (1 << prefs['contour_variable']) > 0:
							entry = {'type': 'perf-point'}
							entry['model'] = self.label
							entry['variable'] = self.variables_names[i]
							entry['dependent'] = self.variables_names[self.dependent[iT1, iT2, self.iT3]]
							entry['coords'] = [iT1, iT2]
							marked_list.append(entry)
							
		return marked_list