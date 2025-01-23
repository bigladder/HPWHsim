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

class TestPlotter:
	def __init__(self):
		self.plot = {}
		self.energy_data = {}
		self.df_measured = {}
		self.df_simulated = {}
		self.have_measured = False
		self.have_simulated = False

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

	def retrieve_dataframe(self, variable_type):
		if variable_type == "Measured":
			return self.df_measured
		elif variable_type == "Simulated":
			return self.df_simulated

	def filter_dataframe_range(self, df, variable_type):
		column_time_name = self.variables["X-Variables"]["Time"]["Column Names"][variable_type]
		return df[(df[column_time_name] > 0) & (df[column_time_name] <= 1440)].reset_index()

	def organize_tank_temperatures(self, variable_type):
		if variable_type == "Measured":
			df = self.df_measured
		elif variable_type == "Simulated":
			df = self.df_simulated

		df["Storage Tank Average Temperature"] = df[
	        self.temperature_profile_vars[variable_type]
	    ].mean(axis=1)

		for temperature_column in self.variables["Y-Variables"]["Temperature"]["Column Names"][variable_type]:
				for index in range(len(df)):
					df.loc[index, temperature_column] = 1.8 * df.loc[index, temperature_column] + 32 #convert(df.loc[index, temperature_column], "degC", "degF")

		return df

	def read_measured(self, measured_path):
		try:
			self.df_measured = call_csv(measured_path, 0)
		except:
			return

	  # remove rows from dataframes outside of inclusive range [1,1440]
		self.df_measured = self.filter_dataframe_range(self.df_measured, "Measured")
		self.df_measured[power_col_label_meas] = self.df_measured["PowerIn(W)"]
		self.energy_data['measuredE_Wh'] = self.df_measured[power_col_label_meas].sum()/60
		self.df_measured = self.organize_tank_temperatures("Measured")

		self.have_measured = True
		
	def read_simulated(self, simulated_path):
		try:
			self.df_simulated = call_csv(simulated_path, 0)
		except:
			return

		# remove rows from dataframes outside of inclusive range [1,1440]
		self.df_simulated = self.filter_dataframe_range(self.df_simulated, "Simulated")

		# sum sim power if multiple heat sources
		i = 1
		src_exists = True
		while src_exists:
			col_label = f"h_src{i}In (Wh)"
			src_exists = self.df_simulated.columns.isin([col_label]).any()
			if src_exists:
				if i == 1:
					self.df_simulated[power_col_label_sim] = self.df_simulated[col_label]
				else:
					self.df_simulated[power_col_label_sim] = self.df_simulated[power_col_label_sim] + self.df_simulated[col_label]
			i = i + 1

		self.energy_data['simulatedE_Wh'] = self.df_simulated[power_col_label_sim].sum()

		# convert simulated energy consumption (Wh) for every minute to power (W)
		self.df_simulated[power_col_label_sim] = convert_values(self.df_simulated[power_col_label_sim], "Wh/min", "W")
		self.df_simulated = self.organize_tank_temperatures("Simulated")
	
		self.have_simulated = True
 					
	def plot_graphs(self, variable_type, variable, value, row):
		df = self.retrieve_dataframe(variable_type)

		if (value in [1, 2]) and (variable_type == "Measured"):
			marker_symbol = "circle"
			marker_size = 7
			marker_fill_color = "white"
			marker_line_color = self.variables["Y-Variables"][variable]["Colors"][value]
		elif (value == 1) and (variable_type == "Simulated"):
			marker_symbol = "circle"
			marker_size = 7
			marker_fill_color = self.variables["Y-Variables"][variable]["Colors"][value]
			marker_line_color = self.variables["Y-Variables"][variable]["Colors"][value]
		else:
			marker_symbol = None
			marker_size = None
			marker_fill_color = None
			marker_line_color = None

		self.plot.add_time_series(
		    dimes.TimeSeriesData(
		        df[
		            self.variables["Y-Variables"][variable]["Column Names"][variable_type][value]
		        ],
		        name=f"{self.variables['Y-Variables'][variable]['Labels'][value]} - {variable_type}",
		        native_units=self.variables["Y-Variables"][variable]["Units"],
		        line_properties=LineProperties(
		            color=self.variables["Y-Variables"][variable]["Colors"][value],
		            line_type=retrieve_line_type(variable_type),
		            marker_symbol=marker_symbol,
		            marker_size=marker_size,
		            marker_line_color=marker_line_color,
		            marker_fill_color=marker_fill_color,
		            is_visible=self.variables["Y-Variables"][variable]["Line Visibility"][value],
		        ),
		    ),
		    subplot_number=row,
		    axis_name=variable,
	  )

	def draw_variable_type(self, variable_type):
			for row, variable in enumerate(self.variables["Y-Variables"].keys()):
				for value in range(
					len(self.variables["Y-Variables"][variable]["Column Names"][variable_type])
				):
					self.plot_graphs(variable_type, variable, value, row + 1)

	def draw(self):		
		if self.have_measured:
			self.plot = dimes.TimeSeriesPlot(
				self.df_measured[self.variables["X-Variables"]["Time"]["Column Names"]["Measured"]]
			)
			self.draw_variable_type("Measured")
			if self.have_simulated:
				self.draw_variable_type("Simulated")
		elif self.have_simulated:
			self.plot = dimes.TimeSeriesPlot(
				self.df_simulated[self.variables["X-Variables"]["Time"]["Column Names"]["Simulated"]]
			)
			self.draw_variable_type("Simulated")
			if self.have_measured:
				self.draw_variable_type("Measured")
		else:
			return

		self.plot.finalize_plot()
		return self

def plot(measured_path, simulated_path):
	plotter = TestPlotter()
	plotter.read_measured(measured_path)
	plotter.read_simulated(simulated_path)
	plotter.draw()
	return plotter

def write_plot(measured_path, simulated_path, plot_path):
	plotter = TestPlotter()
	plotter.read_measured(measured_path)
	plotter.read_simulated(simulated_path)
	plotter.draw()
	plotter.plot.write_html_plot(plot_path)
	
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args > 2:
        measured_path = sys.argv[1]
        simulated_path = sys.argv[2]
        plot_path = sys.argv[3]
        write_plot(measured_path, simulated_path, plot_path)
