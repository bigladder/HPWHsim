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
		return None
	elif variable_type == "Simulated":
		return None#"dot"

class TestPlotter:

	def __init__(self, data):
		self.plot = {}
		
		tank_volume_L = 173
		if 'model_filepath' in data:
			model_data = read_file(data['model_filepath'])		
			tank_volume_L = get_tank_volume(model_data)
		
		self.datasets = []
		if "dataset_specs" in data:
			for dataset_spec in data['dataset_specs']:
				self.datasets.append(DataSet(dataset_spec))
											 
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
		        },
		        "Flow Rate": {
								"Columns": ["Flow Rate"],
		            "Labels": ["Flow Rate"],
		            "Units": "gal/min",
		            "Colors": ["green"],
		            "Line Mode": ["lines"],
		            "Line Visibility": [True],
	
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
		            "Colors": ["red", "green", "blue","cyan"].append(list(reversed(RED_BLUE_DIVERGING_PALLETTE))), 
		            "Line Mode": {"Tank": ["lines"] * (4 + NUMBER_OF_THERMOCOUPLES)},
		            "Line Visibility": [False] * 4 + 
									[True if i == 0 or i == NUMBER_OF_THERMOCOUPLES - 1 else False for i in range(NUMBER_OF_THERMOCOUPLES)]
								}							
		        	},
						
		    "X-Variables": {
		        "Time": {
		            "Labels": ["Time"],
		            "Units": "Min",
		        }
		    },
		}

		have_traces = False
		for dataset in self.datasets:
			print(dataset.variable_type)
			self.plot = dimes.DimensionalPlot(
			  [x for x in dataset.df["Time"]],
					f"Model: {dataset.model_id}, Test: {dataset.test_id}"					
			)
			self.plot.x_axis.name = "Time [min]"
			have_traces = True
			break
			
		if have_traces:			
			for dataset in self.datasets:	

				self.plot_dataset(dataset)	
			
			self.plot.finalize_plot()
			
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
			
			self.plot.finalize_plot()
			self.plot.figure['layout']['yaxis']['title'] = "Power Input [W]"
			self.plot.figure['layout']['yaxis2']['title'] = "Flow Rate [gal/min]"
			self.plot.figure['layout']['yaxis3']['title'] = "Temperature [°F]"
			self.plot.figure['layout']['title']['x'] = 0.25
			self.have_fig = True
	
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
						if "Measured" in curve["name"]:
							if "Power" in curve["name"]:
								curveSums[curveNumber] = [self.measured.df["PowerIn(W)"], 0, "Measured total input energy (kJ)", 60 / 1000]
							if "Flow Rate" in curve["name"]:
								curveSums[curveNumber] = [self.measured.df["FlowRate(gal/min)"], 0, "Measured total volume drawn (gal)", 1]
						if "Simulated" in curve["name"]:
							if "Power" in curve["name"]:
								curveSums[curveNumber] = [self.simulated.df["Power_W"], 0, "Simulated total input energy (kJ)", 60 / 1000]
							if "Flow Rate" in curve["name"]:
								curveSums[curveNumber] = [self.simulated.df["draw"], 0, "Simulated total volume drawn (gal)", 1]
					
												
				for point in selectedData["points"]:
					if "curveNumber" in point and "pointNumber" in point:
						if point["curveNumber"] in curveSums:
							val = curveSums[point["curveNumber"]][0][point["pointNumber"]]
							if not math.isnan(val):	
								curveSums[point["curveNumber"]][1] += val
								
				for curveNumber in curveSums:
					curveSum = curveSums[curveNumber]
					result.append([curveSum[2], curveSum[3] * curveSum[1]])													
					
		elif "range" in selectedData:
			range = selectedData["range"]					
			if "x" in range: # power
				t_mins = self.measured.df["Time(min)"]
				Pins = self.measured.df["PowerIn(W)"]
				flow_rates = self.measured.df["FlowRate(gal/min)"]
				flowV_gal = 0
				Ein_kJ = 0
				
				for t_min, Pin, flow_rate in zip(t_mins, Pins, flow_rates):
					if Pin > range["y"][0] and Pin < range["y"][1] and t_min > range["x"][0]  and t_min < range["x"][1]:
						test_point = {}
						test_point['model_id'] = self.model_id
						test_point['test_id'] = self.test_id
						test_point['variable'] = "Pin"
						test_point['value'] = Pin
						test_point['t_min'] = t_min
						self.test_points.append(test_point)
						
						if not(math.isnan(flow_rate)):
							flowV_gal += flow_rate
						Ein_kJ += Pin * 60 / 1000
					
				result = []
		return result
				
	def click_data(self, clickData):
		if self.measured.have_data:
			if "points" in clickData:
				for point in clickData["points"]:
					if "curveNumber" in point:
						self.click_point['curve_name'] =self.plot.figure["data"][point["curveNumber"]]["name"]
						self.click_point['point_number'] = point["pointNumber"]
						return
		self.click_point = {}
								            
	def plot_dataset(self, dataset):
		for row, variable in enumerate(self.variables["Y-Variables"].keys()):
			for value in enumerate(self.variables["Y-Variables"][variable]["Labels"]):
				if dataset.variable_type == "Measured":
					marker_symbol = "circle"
					marker_size = 7
					marker_fill_color = "white"
					marker_line_color = self.variables["Y-Variables"][variable]["Colors"][0]
				elif dataset.variable_type == "Simulated":
					marker_symbol = "circle"
					marker_size = 7
					marker_fill_color = self.variables["Y-Variables"][variable]["Colors"][0]
					marker_line_color = self.variables["Y-Variables"][variable]["Colors"][0]
				else:
					marker_symbol = None
					marker_size = None
					marker_fill_color = None
					marker_line_color = None		

				#replace edge NaNs with 0			
				x_df = dataset.df["Time"]
				y_df = dataset.df[self.variables["Columns"][value]]
				prevNan = False		
				x_arr = []
				y_arr = []
				for i, y in enumerate(y_df):
					if np.isnan(y):
						if not prevNan:
							x_arr.append(x_df.iloc[i])
							y_arr.append(0)					
						prevNan = True	
					else:
						if prevNan:
							x_arr.append(x_df.iloc[i-1])
							y_arr.append(0)
						x_arr.append(x_df.iloc[i])
						y_arr.append(y_df.iloc[i])
						prevNan = False
				
				displayData = dimes.DisplayData(
		      y_arr,
		      name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {dataset.variable_type}",
		      native_units=self.variables["Y-Variables"][variable]["Units"],
		      is_visible=self.variables["Y-Variables"][variable]["Line Visibility"][value],
					x_axis = x_arr,
					line_properties=LineProperties(
		        color=self.variables["Y-Variables"][variable]["Colors"][value],
		        line_type=retrieve_line_type(dataset.variable_type),
		        marker_symbol=marker_symbol,
		        marker_size=marker_size,
		        marker_line_color=marker_line_color,
		        marker_fill_color=marker_fill_color,
				 	)
				)
				self.plot.add_display_data(
					displayData,
			    subplot_number=row,
			    #axis_name=variable,
			  )

	def update_graphs(self, data_set, variable, value, row):
		self.plot.figure.update_traces(
			y = [x for x in data_set.df[
		            self.variables["Y-Variables"][variable]["Columns"][data_set.variable_type][value][row]
		        ]],
			selector = dict(name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {data_set.variable_type}")
			)
	
	def update_variable_type(self, data_set):
			for row, variable in enumerate(self.variables["Y-Variables"].keys()):
				for value in range(
					len(self.variables["Y-Variables"][variable]["Columns"][data_set.variable_type])
				):
					self.update_graphs(data_set, variable, value, row + 1)

	def update_simulated(self):
		for row, variable in enumerate(self.variables["Y-Variables"].keys()):
			for value in range(
				len(self.variables["Y-Variables"][variable]["Columns"][self.simulated.variable_type])
				):

				self.plot.figure.update_traces(
					y = [x for x in self.simulated.df[
				            self.variables["Y-Variables"][variable]["Columns"][self.simulated.variable_type][value]
				        ]],
					selector = dict(name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {self.simulated.variable_type}")
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
				
	def getSummaryDataList(self):	
		summary_data_dict = {}
		for data_set in [self.measured, self.simulated]:
			if data_set.have_data:
				self.analyze(data_set)
				for summary in ['first-recovery_period', 'standby_period', '24-hr-test']:
					for item in data_set.test_summary[summary]:
						if item not in summary_data_dict:
							summary_data_dict[item] = []
							
		for dataset in self.datasets:
			for item in summary_data_dict:
				have_item = False
				for summary in ['first-recovery_period', 'standby_period', '24-hr-test']:
					if item in dataset.test_summary[summary]:
						have_item = True
						summary_data_dict[item].append(data_set.test_summary[summary][item])
						break

				if not(have_item):
					summary_data_dict[item].append("")	
									
		summary_data_list = []
		for item in summary_data_dict:
			summary_data_list.append([item, summary_data_dict[item][0], summary_data_dict[item][1]])		
		
		return summary_data_list

def plot(data):
	print(data)
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
			data['dataset_specs'].append({'model_id': model_id, 'test_id': test_id, 'type': "Measured", 'filepath': sys.argv[3]})
			data['dataset_specs'].append({'model_id': model_id, 'test_id': test_id, 'type': "Simulated", 'filepath': sys.argv[4]	})
		if n_args > 4:
			data['plot_filepath'] = sys.argv[5]
		if n_args > 5:
			data['model_filepath'] = sys.argv[6]
		
		write_plot(data)