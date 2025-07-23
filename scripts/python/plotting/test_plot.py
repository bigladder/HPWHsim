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

power_col_label_meas = "Power_W"
power_col_label_sim = "Power_W"

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

class DataSet:
    def __init__(self, variable_type):
        self.df = {}
        self.have_data = False
        self.show_plot = False
        self.energy_use_Wh = 0
        self.variable_type = variable_type
        self.filepath = ""

class TestPlotter:
	
	def filter_dataframe_range(self, data_set):
		column_time_name = self.variables["X-Variables"]["Time"]["Column Names"][data_set.variable_type]
		return data_set.df[(data_set.df[column_time_name] > 0) & (data_set.df[column_time_name] <= 1440)].reset_index()
	
	def __init__(self, data):
		self.plot = {}
		self.label = data['model_id']
		self.test_id = data['test_id']
		self.energy_data = {}
		self.measured = DataSet("Measured")
		self.simulated = DataSet("Simulated")
		self.have_fig = False
		self.metrics = {}
		self.model_id = ""

		self.variables = {
		    "Y-Variables": {
		        "Power Input": {
		            "Column Names": {"Measured": [power_col_label_meas], "Simulated": [power_col_label_sim]},
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
		            "Labels": [
		                f"Storage Tank Temperature {number}"
		                for number in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))
		            ],
		            "Units": f"{DEGREE_SIGN}F",
		            "Colors": list(reversed(RED_BLUE_DIVERGING_PALLETTE)),
		            "Line Mode": ["lines"] * NUMBER_OF_THERMOCOUPLES,
		            "Line Visibility": [False] * NUMBER_OF_THERMOCOUPLES,
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
		        "Storage Tank Average Temperature",
		        "OutletT(C)",
		        "InletT(C)",
		        "AmbientT(C)",
		    ],
		    "Simulated": [
		        "Storage Tank Average Temperature",
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
			    "Storage Tank Average Temperature",
			    "Storage Tank Outlet Temperature",
			    "Storage Tank Inlet Temperature",
			    "Ambient Temperature",
			],
			"Colors": ["black", "orange", "purple", "limegreen"],
			"Line Mode": ["lines", "lines+markers", "lines", "lines"],
			"Line Visibility": [True, True, False, False],
		}

		for key in OUTPUT_TEMPERATURE_DETAILS.keys():
			for index in range(len(OUTPUT_TEMPERATURE_DETAILS[key])):
					self.variables["Y-Variables"]["Temperature"][key].insert(index, OUTPUT_TEMPERATURE_DETAILS[key][index])


	def organize_tank_temperatures(self, data_set):
		data_set.df["Storage Tank Average Temperature"] = data_set.df[
		    self.temperature_profile_vars[data_set.variable_type]
		].mean(axis=1)

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
		self.measured.df[power_col_label_meas] = self.measured.df["PowerIn(W)"]
		self.measured.energy_use_Wh = self.measured.df[power_col_label_meas].sum()/60
		self.measured = self.organize_tank_temperatures(self.measured)
		self.measured.have_data = True

	def read_simulated(self, filepath):
		try:
			self.simulated.df = call_csv(filepath, 0)
			self.simulated.filepath = filepath
		except:
			self.simulated.df = {}
			return

		# remove rows from dataframes outside of inclusive range [1,1440]
		self.simulated.df = self.filter_dataframe_range(self.simulated)

		# sum sim power if multiple heat sources
		i = 1
		src_exists = True
		while src_exists:
			col_label = f"h_src{i}In (Wh)"
			src_exists = self.simulated.df.columns.isin([col_label]).any()
			if src_exists:
				if i == 1:
					self.simulated.df[power_col_label_sim] = self.simulated.df[col_label]
				else:
					self.simulated.df[power_col_label_sim] = self.simulated.df[power_col_label_sim] + self.simulated.df[col_label]
			i = i + 1

		self.energy_use_Wh = self.simulated.df[power_col_label_sim].sum()

		# convert simulated energy consumption (Wh) for every minute to power (W)
		self.simulated.df[power_col_label_sim] = convert_values(self.simulated.df[power_col_label_sim], "Wh/min", "W")
		self.simulated = self.organize_tank_temperatures(self.simulated)

		self.simulated.have_data = True

	def select_data(self, selectedData):
		self.metrics = []
		if self.measured.have_data:
			if "range" in selectedData:
				range = selectedData["range"]
				if "x" in range: # power
					t_mins = self.measured.df["Time(min)"]
					Pins = self.measured.df["PowerIn(W)"]
					for t_min, Pin in zip(t_mins, Pins):
						if Pin > range["y"][0] and Pin < range["y"][1] and t_min > range["x"][0]  and t_min < range["x"][1]:
							metric = {}
							metric['type'] = "measured"
							metric['model_id'] = self.label
							metric['test_id'] = self.test_id
							metric['variable'] = "Pin"
							metric['t_min'] = t_min
							self.metrics.append(metric)

	def click_data(self, clickData):
		self.metrics = []
		if self.measured.have_data:
			if "points" in clickData:
				for point in clickData["points"]:

					if "x" in point: # power
						t_mins = self.measured.df["Time(min)"]
						Pins = self.measured.df["PowerIn(W)"]
						for t_min, Pin in zip(t_mins, Pins):
							if Pin == point["y"] and t_min == point["x"]:
								metric = {}
								metric['type'] = "measured"
								metric['model_id'] = self.label
								metric['test_id'] = self.test_id
								metric['variable'] = "Pin(W)"
								metric['value'] = Pin
								metric['t_min'] = t_min
								self.metrics.append(metric)
								print(metric)
								break
								            
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

	def draw_variable_type(self, data_set):
		for row, variable in enumerate(self.variables["Y-Variables"].keys()):
			for value in range(
				len(self.variables["Y-Variables"][variable]["Column Names"][data_set.variable_type])
			):
				self.plot_graphs(data_set, variable, value, row + 1)

	def draw(self, data):
		have_traces = False
		draw_meas = self.measured.have_data
		draw_sim = self.simulated.have_data
		if 'show' in data:
			draw_meas = draw_meas and (data["show"] & 1 == 1)
			draw_sim = draw_sim and (data["show"] & 2 == 2)

			if draw_meas:
				self.plot = dimes.DimensionalPlot(
				    [x for x in self.measured.df[self.variables["X-Variables"]["Time"]["Column Names"]["Measured"]]]
				)
				self.draw_variable_type(self.measured)
				have_traces = True
				if draw_sim:
					self.draw_variable_type(self.simulated)
			elif draw_sim:
				self.plot = dimes.DimensionalPlot(
				    [x for x in self.simulated.df[self.variables["X-Variables"]["Time"]["Column Names"]["Simulated"]]]
				)
				have_traces = True
				self.draw_variable_type(self.simulated)
			else:
				return
			if have_traces:
				self.plot.finalize_plot()
				self.have_fig = True
		return self

def plot(data):
    plotter = TestPlotter(data)
    if "model_id" in data:
        plotter.model_id = data["model_id"]
    if "measured_filepath" in data:
        plotter.read_measured(data["measured_filepath"])
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
        write_plot(data)