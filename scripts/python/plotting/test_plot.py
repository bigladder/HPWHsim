"""
uv run test_plot.py \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/test/Villara/AquaThermAire/villara_24hr67/measured.csv" \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/build/test/output/villara_24hr67_Preset_AquaThermAire.csv" \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/build/test/output/villara_24hr67_Preset_AquaThermAire.html"
"""
"""
uv run test_plot.py \
	"AquaThermAire" \
	"villara_24hr67" \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/test/Villara/AquaThermAire/villara_24hr67/measured.csv" \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/build/test/output/villara_24hr67_Preset_AquaThermAire.csv" \
	"/Users/phil-ahrenkiel/Documents/GitHub/HPWHsim/build/test/output/villara_24hr67_Preset_AquaThermAire.html"
"""
import os
import sys
import dimes  # type: ignore
from dimes import LineProperties
import pandas as pd  # type: ignore
from koozie import convert  # type: ignore
import plotly.graph_objects as go
import math
from common import read_file, write_file, get_tank_volume

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

def call_csv(path, skip_rows):
    data = pd.read_csv(path, skiprows=skip_rows)
    df = pd.DataFrame(data)
    return df

def convert_values(df_column, from_units, to_units):
	converted_column = df_column.apply(lambda x: convert(x, from_units, to_units))
	return converted_column

def retrieve_line_type(variable_type):
	if variable_type == "Measured":
		return None
	elif variable_type == "Simulated":
		return "dot"

class EnergySummary:
	def __init__(self, tank_volume_L):
		self.energy_used_kJ = 0
		self.delivered_energy_kJ = 0
		self.volume_drawn_L = 0
		self.tank_volume_L = tank_volume_L
		self.ef = 0

class DataSet:
	def __init__(self, variable_type, tank_volume_L):
		self.df = {}
		self.have_data = False
		self.show_plot = False
		self.energy_summary = EnergySummary(tank_volume_L)
		self.variable_type = variable_type
		self.filepath = ""
				
class TestPlotter:

	def __init__(self, data):
		self.plot = {}
		self.model_id = data['model_id']
		self.test_id = data['test_id']
		
		tank_volume_L = 173
		if 'model_filepath' in data:
			model_data = read_file(data['model_filepath'])		
			tank_volume_L = get_tank_volume(model_data)
		
		self.measured = DataSet("Measured", tank_volume_L)
		self.simulated = DataSet("Simulated", tank_volume_L)
		self.have_fig = False
		self.test_points = []
		self.model_id = ""

		
		visibleT = []
		for i in range(NUMBER_OF_THERMOCOUPLES):
			visibleT.append(True if i == 0 or i == NUMBER_OF_THERMOCOUPLES - 1 else False)
			
		self.variables = {
		    "Y-Variables": {
		        "Power Input": {
		            "Column Names": {"Measured": ["PowerIn(W)"], "Simulated": ["Power_W"]},
		            "Labels": ["Power Input"],
		            "Units": "W",
		            "Colors": ["red"],
		            "Line Mode": ["lines"],
		            "Line Visibility": [True],
		        },
		        "Flow Rate": {
		            "Column Names": {"Measured": ["FlowRate(gal/min)"], "Simulated": ["draw"]},
		            "Labels": ["Flow Rate"],
		            "Units": "gal/min",
		            "Colors": ["green"],
		            "Line Mode": ["lines"],
		            "Line Visibility": [True],
	
		        },
		        "Temperature": {
		            "Column Names": {
		                "Measured": [
		                    f"TankT{number}(C)"
		                    for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
		                ],
		                "Simulated": [
		                    f"tcouple{number} (C)"
		                    for number in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))
		                ],
		            },
								"Inlet": { "Measured": "InletT(C)", "Simulated": "inletT"},
								"Outlet": { "Measured": "OutletT(C)", "Simulated": "toutlet (C)"},			
								"Ambient": { "Measured": "AmbientT(C)", "Simulated": "Ta"},						
		            "Labels": [
		                f"Tank Temperature {number}"
		                for number in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))
		            ],
		            "Units": f"{DEGREE_SIGN}F",
		            "Colors": list(reversed(RED_BLUE_DIVERGING_PALLETTE)), 
		            "Line Mode": ["lines"] * NUMBER_OF_THERMOCOUPLES,
		            "Line Visibility": visibleT#[False] * NUMBER_OF_THERMOCOUPLES,
		        },
		    },
		    "X-Variables": {
		        "Time": {
		            "Column Names": {"Measured": "Time(min)", "Simulated": "minutes"},
		            "Units": "Min",
		        }
		    },
		}

		self.temperature_profile_vars = {
		    "Measured": [
		        f"TankT{number}(C)"
		        for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
		    ],
		    "Simulated": [
		        f"tcouple{number} (C)"
		        for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
		    ],
		}

		INPUT_TEMPERATURE_DETAILS = {
		    "Measured": [
		        "Tank Average Temperature",
		        "OutletT(C)",
		        "InletT(C)",
		        "AmbientT(C)",
		    ],
		    "Simulated": [
		        "Tank Average Temperature",
		        "toutlet (C)",
		        "inletT",
		        "Ta",
		    ],
		}

		for variable_type in ["Measured", "Simulated"]:
			for index, label in enumerate(INPUT_TEMPERATURE_DETAILS[variable_type]):
				self.variables["Y-Variables"]["Temperature"]["Column Names"][variable_type].insert(
		            index, label)

		# add average, inlet, and outlet temperature details (ex. visibility, color, etc.) to variables dictionary
		OUTPUT_TEMPERATURE_DETAILS = {
			"Labels": [
			    "Tank Average Temperature",
			    "Tank Outlet Temperature",
			    "Tank Inlet Temperature",
			    "Ambient Temperature",
			],
			"Colors": ["black", "orange", "purple", "limegreen"],
			"Line Mode": ["lines", "lines+markers", "lines", "lines"],
			"Line Visibility": [False, False, False, False],
		}

		for key in OUTPUT_TEMPERATURE_DETAILS.keys():
			for index in range(len(OUTPUT_TEMPERATURE_DETAILS[key])):
					self.variables["Y-Variables"]["Temperature"][key].insert(index, OUTPUT_TEMPERATURE_DETAILS[key][index])

	
	def filter_dataframe_range(self, data_set):
		column_time_name = self.variables["X-Variables"]["Time"]["Column Names"][data_set.variable_type]
		return data_set.df[(data_set.df[column_time_name] > -120) & (data_set.df[column_time_name] <= 2880)].reset_index()
	
	def organize_tank_temperatures(self, data_set):
		data_set.df["Tank Average Temperature"] = data_set.df[
		    self.temperature_profile_vars[data_set.variable_type]
		].mean(axis=1)

		initialTankAvgT_C = data_set.df["Tank Average Temperature"][0]
		finalTankAvgT_C = data_set.df["Tank Average Temperature"].iloc[-1]
		data_set.volume_drawn_L = data_set.df[self.variables["Y-Variables"]["Flow Rate"]["Column Names"][data_set.variable_type][0]].sum() * 3.78541
		data_set.energy_summary.energy_used_kJ = data_set.df[self.variables["Y-Variables"]["Power Input"]["Column Names"][data_set.variable_type][0]].sum() * 60 / 1000
		water_specific_heat_kJperkgC = 4.180
		water_density_kgperL = 0.995
		tank_heat_capacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * data_set.energy_summary.tank_volume_L
		
		sumDrawOutletT = 0
		sumDrawInletT = 0
		sumDrawDiffT = 0
		sumDraw_L = 0
		sumDrawHeatCap_kJperC = 0
		sumInputEnergy_kJ = 0
		sumNoDrawTime_min = 0
		noDrawSumAmbientTTime = 0
		
		standbySumTime_min = 0
		standbySumTimeTankT_minC = 0
		standbySumTimeAmbientT_minC = 0
		hasStandbyPeriodStarted = False
		hasStandbyPeriodEnded = False	
		
		deliveredEnergy_kJ = 0
		recoveryUsedEnergy_kJ = 0
		recoveryStoredEnergy_kJ = 0
		recoveryDeliveredEnergy_kJ = 0
		isFirstRecoveryPeriod = True
		has_heated = False
		has_drawn = False
		
		maxTankAfterFirstRecoveryT_C = -10

		print("\n")
		print(data_set.variable_type)
		for index in range(len(data_set.df)):
			ambientT_C = data_set.df.loc[index, self.variables["Y-Variables"]["Temperature"]["Ambient"][data_set.variable_type]]
			tankAvgT_C = data_set.df.loc[index, "Tank Average Temperature"]
			draw_volume_L = data_set.df.loc[index, self.variables["Y-Variables"]["Flow Rate"]["Column Names"][data_set.variable_type][0]] * 3.78541	
			input_energy_kJ = data_set.df.loc[index, self.variables["Y-Variables"]["Power Input"]["Column Names"][data_set.variable_type][0]] * 60 / 1000
			inletT_C = 0 if draw_volume_L == 0 else data_set.df.loc[index, self.variables["Y-Variables"]["Temperature"]["Inlet"][data_set.variable_type]]
			outletT_C = 0 if draw_volume_L == 0 else data_set.df.loc[index, self.variables["Y-Variables"]["Temperature"]["Outlet"][data_set.variable_type]]

			sumInputEnergy_kJ += input_energy_kJ
			draw_heat_capacity_kJperC = 0
			if math.isnan(draw_volume_L):
				draw_volume_L = 0
			else:
				draw_heat_capacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * draw_volume_L
				sumDrawInletT += draw_volume_L * inletT_C		
				sumDrawOutletT += draw_volume_L * outletT_C		
				sumDrawDiffT += draw_volume_L * (outletT_C - inletT_C)
				sumDraw_L += draw_volume_L
				sumDrawHeatCap_kJperC += draw_heat_capacity_kJperC
				deliveredEnergy_kJ += draw_heat_capacity_kJperC * (outletT_C - inletT_C)
				
			is_heating = input_energy_kJ > 0.5
			has_heated = has_heated or is_heating
			is_drawing = draw_volume_L > 0
			has_drawn = has_drawn or is_drawing
			if not(is_drawing):
				noDrawSumAmbientTTime += ambientT_C
				sumNoDrawTime_min += 1

			if isFirstRecoveryPeriod:
				if has_drawn and not(is_drawing):
					if not(is_heating):	
						print(f"End first recovery: {index}")
						isFirstRecoveryPeriod = False					
							
						recoveryTotalDraw_L = sumDraw_L		
						recoveryAvgInletT_C =	sumDrawInletT / sumDraw_L
						recoveryAvgOutletT_C =	sumDrawOutletT / sumDraw_L
						recoveryUsedEnergy_kJ = sumInputEnergy_kJ	
						hasStandbyPeriodStarted = True
						standbyStartT_C = tankAvgT_C
						standbyStartTime_min = index
						continue				
													
			if hasStandbyPeriodStarted:	
				if not(hasStandbyPeriodEnded):
					if not(is_drawing) and not(is_heating):				
							standbySumTimeTankT_minC += (1.) * tankAvgT_C 
							standbySumTimeAmbientT_minC += (1.) * ambientT_C
							standbySumTime_min += 1.
							if tankAvgT_C > maxTankAfterFirstRecoveryT_C:
								maxTankAfterFirstRecoveryT_C = tankAvgT_C
					else:
						hasStandbyPeriodEnded = True
						standbyEndT_C = tankAvgT_C
						tauStandby_min = index - standbyStartTime_min
						print(f"End standby period: {index}")
		
		recoveryStoredEnergy_kJ = tank_heat_capacity_kJperC * (maxTankAfterFirstRecoveryT_C - initialTankAvgT_C)
		recoveryDeliveredEnergy_kJ = water_specific_heat_kJperkgC * water_density_kgperL * recoveryTotalDraw_L * (recoveryAvgOutletT_C - recoveryAvgInletT_C)

		print(f"initial tank T_C: {initialTankAvgT_C}")
		print(f"max tank T after first recovery_C: {maxTankAfterFirstRecoveryT_C}")
		print(f"recoveryTotalDraw_L: {recoveryTotalDraw_L}")
		print(f"recovery avg inlet T_C: {recoveryAvgInletT_C}")
		print(f"recovery avg outlet T_C: {recoveryAvgOutletT_C}")
		print(f"recoveryStoredEnergy_kJ : {recoveryStoredEnergy_kJ}")
		print(f"recoveryDeliveredEnergy_kJ : {recoveryDeliveredEnergy_kJ}")
		print(f"recoveryUsedEnergy_kJ : {recoveryUsedEnergy_kJ}")
		print("\n")
						
		avgInletT_C = sumDrawInletT / sumDraw_L	
		avgOutletT_C =	sumDrawOutletT / sumDraw_L
		print(f"sumDraw_L : {sumDraw_L}")
		print(f"avgInletT_C : {avgInletT_C}")
		print(f"avgOutletT_C : {avgOutletT_C}")
						
		print(f"inputEnergy_kJ : {sumInputEnergy_kJ}")

		standardSetpointT_C = 51.7
		standardInletT_C = 14.4
		standardAmbientT_C = 19.7

		if sumNoDrawTime_min > 0:
			noDrawAvgAmbientT_C = noDrawSumAmbientTTime / sumNoDrawTime_min
			
		recoveryEfficiency = 0
		if recoveryUsedEnergy_kJ > 0:
			recoveryEfficiency = (recoveryStoredEnergy_kJ + recoveryDeliveredEnergy_kJ) / recoveryUsedEnergy_kJ
			waterHeatingEnergy_kJ = deliveredEnergy_kJ / recoveryEfficiency
			standardRemovedEnergy_kJ = sumDrawHeatCap_kJperC * (standardSetpointT_C - standardInletT_C)
			standardWaterHeatingEnergy_kJ = standardRemovedEnergy_kJ / recoveryEfficiency
			waterHeatingDifferenceEnergy_kJ = standardWaterHeatingEnergy_kJ - waterHeatingEnergy_kJ
			removedHeatCapacity_kJperC = water_specific_heat_kJperkgC * water_density_kgperL * sumDraw_L		
			standardDeliveredEnergy_kJ = removedHeatCapacity_kJperC * (standardSetpointT_C - standardInletT_C)
			
			standbyAvgTankT_C = standbySumTimeTankT_minC / standbySumTime_min
			standbyAvgAmbientT_C = standbySumTimeAmbientT_minC / standbySumTime_min
			
			standbyHourlyLossEnergy_kJperh = tank_heat_capacity_kJperC * (standbyStartT_C - standbyEndT_C) / recoveryEfficiency / (standbySumTime_min / 60)
			standbyLossCoefficient_kJperhC = standbyHourlyLossEnergy_kJperh / (standbyAvgTankT_C - standbyAvgAmbientT_C)
			consumedHeatingEnergy_kJ = sumInputEnergy_kJ + tank_heat_capacity_kJperC * (finalTankAvgT_C - initialTankAvgT_C) / recoveryEfficiency
			adjustedConsumedWaterHeatingEnergy_kJ = consumedHeatingEnergy_kJ - (standardAmbientT_C - noDrawAvgAmbientT_C) * standbyLossCoefficient_kJperhC * (sumNoDrawTime_min / 60)
			waterHeatingDifferenceEnergy_kJ = standardWaterHeatingEnergy_kJ - waterHeatingEnergy_kJ
			modifiedConsumedWaterHeatingEnergy_kJ = adjustedConsumedWaterHeatingEnergy_kJ + waterHeatingDifferenceEnergy_kJ
			data_set.energy_summary.ef = standardDeliveredEnergy_kJ / modifiedConsumedWaterHeatingEnergy_kJ
			data_set.energy_summary.energy_used_kJ = sumInputEnergy_kJ
			print(f"recovery efficiency : {recoveryEfficiency}")
			print(f"standbyLossCoefficient_kJperhC : {standbyLossCoefficient_kJperhC}")
			print(f"standardDeliveredEnergy_kJ : {standardDeliveredEnergy_kJ}")
			print(f"waterHeatingDifferenceEnergy_kJ  : {waterHeatingDifferenceEnergy_kJ}")
			print(f"adjustedConsumedWaterHeatingEnergy_kJ : {adjustedConsumedWaterHeatingEnergy_kJ}")
			print(f"modifiedConsumedWaterHeatingEnergy_kJ : {modifiedConsumedWaterHeatingEnergy_kJ}")
			print(f"ef: {data_set.energy_summary.ef}")

		for temperature_column in self.variables["Y-Variables"]["Temperature"]["Column Names"][data_set.variable_type]:
			for index in range(len(data_set.df)):
				data_set.df.loc[index, temperature_column] = 1.8 * data_set.df.loc[index, temperature_column] + 32 #convert(df.loc[index, temperature_column], "degC", "degF")

		return data_set

	def read_measured(self, filepath):
		try:
			self.measured.df = call_csv(filepath, 0)
			self.measured.filepath = filepath
		except:
			self.measured.df = {}
			return	

		# remove rows from dataframes outside of inclusive range [1,1440]
		self.measured.df = self.filter_dataframe_range(self.measured)
		#self.measured.energy_use_Wh = self.measured.df[power_col_label_meas].sum()/60
		self.measured.energy_summary.energy_used_kJ = self.measured.df[self.variables["Y-Variables"]["Power Input"]["Column Names"]["Measured"][0]].sum() * 60 / 1000

		self.measured = self.organize_tank_temperatures(self.measured)
		
		self.measured.have_data = True

	def prepare_simulated(self):
	# remove rows from dataframes outside of inclusive range [1,1440]
		self.simulated.df = self.filter_dataframe_range(self.simulated)

		# sum sim power if multiple heat sources
		i = 1
		src_exists = True
		while src_exists:
			col_label = f"h_src{i}In (Wh)"
			src_exists = self.simulated.df.columns.isin([col_label]).any()
			if src_exists:
				energy = self.simulated.df[col_label]
				if i == 1:
					self.simulated.df[self.variables["Y-Variables"]["Power Input"]["Column Names"]["Simulated"][0]] = energy
				else:
					self.simulated.df[self.variables["Y-Variables"]["Power Input"]["Column Names"]["Simulated"][0]] += energy
			i = i + 1
		
		# convert simulated energy consumption (Wh) for every minute to power (W)
		self.simulated.df[self.variables["Y-Variables"]["Power Input"]["Column Names"]["Simulated"][0]] = convert_values(self.simulated.df[self.variables["Y-Variables"]["Power Input"]["Column Names"]["Simulated"][0]], "Wh/min", "W")

		self.simulated = self.organize_tank_temperatures(self.simulated)
		self.simulated.have_data = True
		
	def read_simulated(self, filepath):
		try:
			self.simulated.df = call_csv(filepath, 0)
			self.simulated.filepath = filepath
		except:
			self.simulated.df = {}
			return
		self.prepare_simulated()
	
	def reread_simulated(self):
		try:
			self.simulated.df = call_csv(self.simulated.filepath, 0)
		except:
			self.simulated.df = {}
		self.prepare_simulated()

	def select_data(self, selectedData):
		self.test_points = []
		result = []
		if "points" in selectedData:
				curves = {}
				for point in selectedData["points"]:
					if not point["curveNumber"] in curves:
						curves[point["curveNumber"]] = self.plot.figure["data"][point["curveNumber"]]
				
				print("\n")
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
					if "x" in point: # power
						t_mins = self.measured.df["Time(min)"]
						for i_min, t_min in enumerate(t_mins):
							if t_min == point["x"]:
								test_point = {}
								test_point['model_id'] = self.model_id
								test_point['test_id'] = self.test_id
								test_point['variable']= "Pin"
								test_point['value'] =  float(self.measured.df["PowerIn(W)"][i_min])
								test_point['t_min'] = t_min
								self.test_points.append(test_point)
								return
								            
	def plot_graphs(self, data_set, variable, value, row):
		if (value in [1, 2]) and (data_set.variable_type == "Measured"):
			marker_symbol = "circle"
			marker_size = 7
			marker_fill_color = "white"
			marker_line_color = self.variables["Y-Variables"][variable]["Colors"][value]
		elif (value == 1) and (data_set.variable_type == "Simulated"):
			marker_symbol = "circle"
			marker_size = 7
			marker_fill_color = self.variables["Y-Variables"][variable]["Colors"][value]
			marker_line_color = self.variables["Y-Variables"][variable]["Colors"][value]
		else:
			marker_symbol = None
			marker_size = None
			marker_fill_color = None
			marker_line_color = None

		self.plot.add_display_data(
		    dimes.DisplayData(

		        [x for x in data_set.df[
		            self.variables["Y-Variables"][variable]["Column Names"][data_set.variable_type][value]
		        ]],
		        name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {data_set.variable_type}",
		        native_units=self.variables["Y-Variables"][variable]["Units"],
		        line_properties=LineProperties(
		            color=self.variables["Y-Variables"][variable]["Colors"][value],
		            line_type=retrieve_line_type(data_set.variable_type),
		            marker_symbol=marker_symbol,
		            marker_size=marker_size,
		            marker_line_color=marker_line_color,
		            marker_fill_color=marker_fill_color,
		        ),
		        is_visible=self.variables["Y-Variables"][variable]["Line Visibility"][value],
		    ),
		    subplot_number=row,
		    #axis_name=variable,
	    )
		
	def update_graphs(self, data_set, variable, value, row):
		self.plot.figure.update_traces(
			y = [x for x in data_set.df[
		            self.variables["Y-Variables"][variable]["Column Names"][data_set.variable_type][value]
		        ]],
			selector = dict(name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {data_set.variable_type}")
			)
	
	def draw_variable_type(self, data_set):
		for row, variable in enumerate(self.variables["Y-Variables"].keys()):
			for value in range(
				len(self.variables["Y-Variables"][variable]["Column Names"][data_set.variable_type])
			):
				self.plot_graphs(data_set, variable, value, row + 1)

	def update_variable_type(self, data_set):
			for row, variable in enumerate(self.variables["Y-Variables"].keys()):
				for value in range(
					len(self.variables["Y-Variables"][variable]["Column Names"][data_set.variable_type])
				):
					self.update_graphs(data_set, variable, value, row + 1)

#
	def update_simulated(self):
		for row, variable in enumerate(self.variables["Y-Variables"].keys()):
			for value in range(
				len(self.variables["Y-Variables"][variable]["Column Names"][self.simulated.variable_type])
				):
			
				self.plot.figure.update_traces(
					y = [x for x in self.simulated.df[
				            self.variables["Y-Variables"][variable]["Column Names"][self.simulated.variable_type][value]
				        ]],
					selector = dict(name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {self.simulated.variable_type}")
					)
		
#
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
		
	def draw(self, data):
		have_traces = False
		draw_meas = self.measured.have_data
		draw_sim = self.simulated.have_data
		if 'show' in data:
			draw_meas = draw_meas and (data["show"] & 1 == 1)
			draw_sim = draw_sim and (data["show"] & 2 == 2)

			if draw_meas:
				self.plot = dimes.DimensionalPlot(
				    [x for x in self.measured.df[self.variables["X-Variables"]["Time"]["Column Names"]["Measured"]]],
						f"Model: {self.model_id}, Test: {self.test_id}"					
				)
				self.plot.x_axis.name = "Time [min]"
				self.draw_variable_type(self.measured)
				have_traces = True
				if draw_sim:
					self.draw_variable_type(self.simulated)
			elif draw_sim:
				self.plot = dimes.DimensionalPlot(
				    [x for x in self.simulated.df[self.variables["X-Variables"]["Time"]["Column Names"]["Simulated"]]],
						f"Model: {self.model_id}, Test: {self.test_id}"
				)
				self.plot.x_axis.name = "Time [min]"
				have_traces = True
				self.draw_variable_type(self.simulated)
			else:
				return
			
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
			if have_traces:
				self.plot.finalize_plot()
				self.plot.figure['layout']['yaxis']['title'] = "Power Input [W]"
				self.plot.figure['layout']['yaxis2']['title'] = "Flow Rate [gal/min]"
				self.plot.figure['layout']['yaxis3']['title'] = "Temperature [°F]"
				self.plot.figure['layout']['title']['x'] = 0.25
				self.have_fig = True
		return self

def plot(data):
	plotter = TestPlotter(data)
	if "model_id" in data:
		plotter.model_id = data["model_id"]
	if "measured_filepath" in data:
			plotter.read_measured(data["measured_filepath"])
	if "test_points" in data:
		plotter.test_points = data['test_points']
	if "simulated_filepath" in data:
		plotter.read_simulated(data["simulated_filepath"])
	plotter.draw({'show': 3})
	return plotter

def write_plot(data):
    plotter = plot(data)
    if "plot_filepath" in data:
        plotter.plot.write_html_plot(data['plot_filepath'])

# main
if __name__ == "__main__":
		n_args = len(sys.argv) - 1
		if n_args > 4:
			data = {}
			data['model_id'] = sys.argv[1]
			data['test_id'] = sys.argv[2]
			data['measured_filepath'] = sys.argv[3]
			data['simulated_filepath'] = sys.argv[4]
			data['plot_filepath'] = sys.argv[5]
			if n_args > 5:
				data['model_filepath'] = sys.argv[6]
			write_plot(data)