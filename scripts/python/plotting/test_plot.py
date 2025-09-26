"""
uv run test_plot.py \
	"AquaThermAire" \
	"villara_24hr67" \
	"" \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/build/test/output/villara_24hr67_JSON_BradfordWhiteAeroThermRE2HP50.csv" \
	"temp.html"
"""
import os
import sys
import dimes  # type: ignore
from dimes import LineProperties
import pandas as pd  # type: ignore
from koozie import convert  # type: ignore
import plotly.graph_objects as go
import math
import numpy as np
from common import read_file, write_file, get_tank_volume
from test_data import DataSet
import json

styles = {
    'pre': {
        'border': 'thin lightgrey solid',
        'overflowX': 'scroll'
    }
}

DEGREE_SIGN = "\N{DEGREE SIGN}"
GRID_LINE_WIDTH = 1.5
GRID_LINES_COLOR = "rgba(128,128,128,0.3)"
# TODO: reverse colors in list below, and revert reverse in variables dictionary
RED_BLUE_DIVERGING_PALLETTE = [
    "#750e13",
    "#da1e28",
    "#ff8389",
    "#33b1ff",
    "#0072c3",
    "#003a6d",
]

NUMBER_OF_THERMOCOUPLES = 6

def convert_values(df_column, from_units, to_units):
	converted_column = df_column.apply(lambda x: convert(x, from_units, to_units))
	return converted_column

def C_to_F(T_C):
	return 1.8 * T_C + 32

def retrieve_line_type(variable_type):
	if variable_type == "Measured":
		return 'solid'
	elif variable_type == "Simulated":
		return 'dot'
	return 'dashdot'

class TestPlotter:
	def __init__(self, data):
		self.plot = {}
		tank_volume_L = 173
		if 'model_filepath' in data:
			model_data = read_file(data['model_filepath'])		
			tank_volume_L = get_tank_volume(model_data)

		self.have_fig = False
		self.test_points = []
		self.model_id = ""
		self.click_point = {}
			
		self.variables = {
	    "Y-Variables": {
	        "Power Input": {
							"Columns": ["Power Input"],
	            "Labels": ["Power Input"],
	            "Units": "W",
	            "Colors": ["red"],
	            "Line Mode": ["lines"],
	            "Line Visibility": [True],
							"Marker Size": [4]
	        },
	        "Flow Rate": {
							"Columns": ["Flow Rate"],
	            "Labels": ["Flow Rate"],
	            "Units": "gal/min",
	            "Colors": ["green"],
	            "Line Mode": ["lines"],
	            "Line Visibility": [True],
							"Marker Size": [7]

	        },
	        "Temperature": {
							"Columns": [
									"Tank Average Temperature",
									"Tank Outlet Temperature",
									"Tank Inlet Temperature",
									"Ambient Temperature"
									] + [f"Tank Temperature - #{i}" for i in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))],
	            "Labels": [
								"Tank Average Temperature",
						    "Tank Outlet Temperature",
						    "Tank Inlet Temperature",
						    "Ambient Temperature"
							] + [f"Tank Temperature - #{i}" for i in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))],
	            "Units": f"{DEGREE_SIGN}F",
	            "Colors": ["#e1141e", "#53a05d", "#6e62a4","#b05593","#cbc062"] + list(reversed(RED_BLUE_DIVERGING_PALLETTE)),
	            "Line Mode": {"Tank": ["lines"] * (4 + NUMBER_OF_THERMOCOUPLES)},
	            "Line Visibility": [False] * 4 +
								[True if i == 0 or i == NUMBER_OF_THERMOCOUPLES - 1 else False for i in range(NUMBER_OF_THERMOCOUPLES)],
							"Marker Size": [None, 7, 7, None] + [4] * NUMBER_OF_THERMOCOUPLES
							}
	        	},

		    "X-Variables": {
		        "Time": {
		            "Labels": ["Time"],
		            "Units": "Min",
		        }
		    },
		}

		self.datasets = []
		if "dataset_specs" in data:
			for dataset_spec in data['dataset_specs']:
				if 'tank_volume_L' not in dataset_spec :
					dataset_spec['tank_volume_L'] = tank_volume_L
				self.datasets.append(DataSet(dataset_spec))
				have_dataset = True
		
		if have_dataset:
			self.draw()

	def draw(self):		
		for dataset in self.datasets:
			self.model_id = dataset.model_id
			self.test_id = dataset.test_id
			self.plot = dimes.DimensionalPlot(
			  [x for x in dataset.df["Time"]],
					f"Model: {dataset.model_id}, Test: {dataset.test_id}"
			)
			self.plot.x_axis.name = "Time [min]"
			break

		for dataset in self.datasets:
			self.plot_dataset(dataset)
			
		self.plot.finalize_plot()
		self.plot.figure['layout']['yaxis']['title'] = "Power Input [W]"
		self.plot.figure['layout']['yaxis2']['title'] = "Flow Rate [gal/min]"
		self.plot.figure['layout']['yaxis3']['title'] = "Temperature [°F]"
		self.plot.figure['layout']['title']['x'] = 0.25
		
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
				visible = True
			)
		self.plot.figure.add_trace(trace_selected)
		self.update_selected()

		# create selected trace (empty)
		trace_clicked = go.Scatter(
				name = "clicked points",
				x = [],
				y = [],
				mode="markers",
				marker_size= [],
				marker_symbol='circle-open',
				marker_color = 'blue',
				marker_line_color= 'blue',
				marker_line_width = 6,
				showlegend = False,
				hoverinfo="skip",
				visible = True
			)
		self.plot.figure.add_trace(trace_clicked)
		self.update_clicked()
		self.have_fig = True

	def plot_dataset(self, dataset):
		for i_variable, variable in enumerate(self.variables["Y-Variables"]):
			for i_val, column in enumerate(self.variables["Y-Variables"][variable]["Columns"]):
				marker_symbol = None
				marker_size = 7
				marker_fill_color = None
				marker_line_color = None
				if dataset.variable_type == "Measured":
					marker_symbol = "circle"
					marker_size = self.variables["Y-Variables"][variable]["Marker Size"][i_val]
					marker_fill_color = self.variables["Y-Variables"][variable]["Colors"][i_val]
					marker_line_color = self.variables["Y-Variables"][variable]["Colors"][i_val]
				elif dataset.variable_type == "Simulated":
					marker_symbol = "circle" 
					marker_size = self.variables["Y-Variables"][variable]["Marker Size"][i_val]
					marker_fill_color = self.variables["Y-Variables"][variable]["Colors"][i_val]
					marker_line_color = self.variables["Y-Variables"][variable]["Colors"][i_val]

				x_list = [x for x in dataset.df["Time"]]
				y_list = [y for y in dataset.df[column]]
				if variable == 'Temperature':
					y_list = [1.8 * y + 32 for y in y_list]

				trace_name = f"{self.variables['Y-Variables'][variable]['Labels'][i_val]}-{dataset._id}"
				displayData = dimes.DisplayData(
		      y_list,
		      name=trace_name,
					#native_units=self.variables["Y-Variables"][variable]["Units"],
		      is_visible=self.variables["Y-Variables"][variable]["Line Visibility"][i_val],
					x_axis = x_list,
					line_properties=LineProperties(
		        color=self.variables["Y-Variables"][variable]["Colors"][i_val],
		        line_type=retrieve_line_type(dataset.variable_type),
		        marker_symbol=marker_symbol,
		        marker_size=marker_size,
		        marker_line_color=marker_line_color,
		        marker_fill_color=marker_fill_color,
					)
				)

				self.plot.add_display_data(
					displayData,
			    subplot_number=i_variable + 1,
			    #axis_name=variable,
			  )
				dataset.trace_visible[trace_name] = displayData.is_visible
		dataset.traces_hidden = False
		
	def select_data(self, selectedData):
		self.test_points = []
		result = []
		if "points" in selectedData:
				curves = {}
				for point in selectedData["points"]:
					if not point["curveNumber"] in curves:
						curves[point["curveNumber"]] = self.plot.figure["data"][point["curveNumber"]]
				
				curveSums = {}
				for curveNumber in curves:
					curve = curves[curveNumber]
					if "name" in curve:
						for dataset in self.datasets:
							if dataset._id in curve["name"]:
								if "Power" in curve["name"]:
									curveSums[curveNumber] = [curve, 0, f"{dataset._id} total input energy (kJ)", 60 / 1000]
								if "Flow Rate" in curve["name"]:
									curveSums[curveNumber] = [curve, 0, f"{dataset._id} total volume drawn (gal)", 1]
					
						for dataset in self.datasets:
							if dataset._id in curve["name"]:
								name = dataset.variable_type
								if "Power" in curve["name"]:
									curveSums[curveNumber] = [curve, 0, f"Total input energy (kJ) - {name}", 60 / 1000]
								if "Flow Rate" in curve["name"]:
									curveSums[curveNumber] = [curve, 0, f"Total volume drawn (gal) - {name}", 1]

				for point in selectedData["points"]:
					if "curveNumber" in point and "pointNumber" in point:
						if point["curveNumber"] in curveSums:
							curve = curveSums[point["curveNumber"]][0]
							val = curve['y'][point["pointNumber"]]
							if not math.isnan(val):	
								curveSums[point["curveNumber"]][1] += val

				for curveNumber in curveSums:
					curveSum = curveSums[curveNumber]

					result.append([curveSum[2], curveSum[3] * curveSum[1]])

		return result
				
	def click_data(self, clickData):
		for dataset in self.datasets:
			if dataset.variable_type == 'Measured':
				if "points" in clickData:
					for point in clickData["points"]:
						if "curveNumber" in point:
							self.click_point['curve_name'] =self.plot.figure["data"][point["curveNumber"]]["name"]
							self.click_point['point_number'] = point["pointNumber"]
							return
		self.click_point = {}

	def update_graphs(self, dataset, variable, value, row):
		self.plot.figure.update_traces(
			y = [x for x in dataset.df[
		            self.variables["Y-Variables"][variable]["Columns"][dataset.variable_type][value][row]
		        ]],
			selector = dict(name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {dataset._id}")
			)

	def update_dataset(self, dataset):
			for row, variable in enumerate(self.variables["Y-Variables"].keys()):
				for value in range(
					len(self.variables["Y-Variables"][variable]["Columns"][dataset.variable_type])
				):
					self.plot.figure.update_traces(
						y = [x for x in dataset.df[
		            self.variables["Y-Variables"][variable]["Columns"][dataset.variable_type][value][row + 1]
		        ]],
						selector = dict(name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {dataset._id}")
					)

	def update_selected(self):
		selected_points = []								
		for test_point in self.test_points:
				selected_points.append([test_point['t_min'], test_point['value']])

		selectedMarkers = {}
		selectedMarkers['x'] = []
		selectedMarkers['y'] = []
		selectedMarkers['size'] = []
		for point in selected_points:
			selectedMarkers['x'].append(point[0])
			selectedMarkers['y'].append(point[1])					
			diam = 15
			selectedMarkers['size'].append(diam)	
		
		self.plot.figure.update_traces(
			x = selectedMarkers['x'],
			y = selectedMarkers['y'],
			marker_size = selectedMarkers['size'],
			selector = dict(name="selected points") 
			)

	def update_clicked(self):
		if 'curve_name' in self.click_point:
			if 'point_number' in self.click_point:
				for curve in self.plot.figure["data"]:
					if curve["name"] == self.click_point['curve_name']:
						xc = [curve.x[self.click_point['point_number']]]
						yc = [curve.y[self.click_point['point_number']]]
						self.plot.figure.update_traces(
							x = xc,
							y = yc,
							marker_size = 20,
							selector = dict(name="clicked points") 
						)

	def update(self, data):
		if "dataset_specs" in data:
			for dataset_spec in data['dataset_specs']:
				for dataset in self.datasets:
					if dataset_spec['filepath'] == dataset.filepath:
						self.update_dataset(dataset)
					else:
						self.plot_dataset(DataSet(dataset_spec))
				
	def set_datasets_visible(self, dataset_ids):				
		for dataset in self.datasets:
			if dataset._id in dataset_ids:
				if dataset.traces_hidden:
					dataset.traces_hidden = False
					for curve in self.plot.figure["data"]:
						if curve['name'] in dataset.trace_visible:
							self.plot.figure.update_traces(
								visible = dataset.trace_visible[curve['name']],
								selector = dict(name=curve['name'])
							)
			else:
				if not dataset.traces_hidden:
					dataset.traces_hidden = True
					for curve in self.plot.figure["data"]:
						if curve['name'] in dataset.trace_visible:
							dataset.trace_visible[curve['name']] = curve['visible']
							self.plot.figure.update_traces(
								visible = False,
								selector = dict(name=curve['name'])
							)
 
	def getSummaryDataDict(self):
		summary_data_dict = {}
		for dataset in self.datasets:
				dataset.analyze()
				for summary in ['first-recovery-period', 'standby-period', '24-hr-test']:
					for item in dataset.test_summary[summary]:
						if item not in summary_data_dict:
							summary_data_dict[item] = []
							
		for dataset in self.datasets:
			for item in summary_data_dict:
				have_item = False
				for summary in ['first-recovery-period', 'standby-period', '24-hr-test']:
					if item in dataset.test_summary[summary]:
						have_item = True
						summary_data_dict[item].append(dataset.test_summary[summary][item])
						break
				if not(have_item):
					summary_data_dict[item].append("")	

		return summary_data_dict

	def getSummaryDataList(self):
		summary_data_dict = self.getSummaryDataDict()
		summary_data_list = []
		for item in summary_data_dict:
			summary_data_row = [item]
			for col in summary_data_dict[item]:
				summary_data_row.append(col)				
			summary_data_list.append(summary_data_row)
		return summary_data_list

def plot(data):
	return TestPlotter(data)

def write_plot(data):
    plotter = TestPlotter(data)
    if "plot_filepath" in data:
     	plotter.plot.write_html_plot(data['plot_filepath'])

# main
if __name__ == "__main__":
		n_args = len(sys.argv) - 1
		data = {}

		if n_args > 3:
			model_id = sys.argv[1]
			test_id = sys.argv[2]
			data['dataset_specs'] = []

			if sys.argv[3] != '':
				data['dataset_specs'].append({'model_id': model_id, 'test_id': test_id, 'type': "Measured", 'filepath': sys.argv[3]})
			if sys.argv[4] != '':
				data['dataset_specs'].append({'model_id': model_id, 'test_id': test_id, 'type': "Simulated", 'filepath': sys.argv[4]})
		if n_args > 4:
			data['plot_filepath'] = sys.argv[5]
		if n_args > 5:
			data['model_filepath'] = sys.argv[6]

		write_plot(data)