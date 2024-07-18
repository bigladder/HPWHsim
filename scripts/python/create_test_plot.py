import sys
from pathlib import Path

import dimes  # type: ignore
from dimes import LineProperties
import pandas as pd  # type: ignore
from koozie import convert  # type: ignore

if len(sys.argv) != 4:
    sys.exit(
        "Incorrect number of arguments. Must be two: Measured Path, Simulated Path, Output Path"
    )

measured_path = Path(sys.argv[1])
simulated_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])

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


def filter_dataframe_range(df, variable_type):
    global variables
    column_time_name = variables["X-Variables"]["Time"]["Column Names"][variable_type]
    return df[(df[column_time_name] > 0) & (df[column_time_name] <= 1440)].reset_index()


def calculate_average_tank_temperature(variable_type):
    global df_measured, df_simulated

    if variable_type == "Measured":
        df = df_measured
    elif variable_type == "Simulated":
        df = df_simulated

    df["Storage Tank Average Temperature"] = df[
        variables["Y-Variables"]["Temperature"]["Column Names"][variable_type]
    ].mean(axis=1)

    TEMPERATURE_DETAILS = {
        "Measured": [
            "Storage Tank Average Temperature",
            "T_Out_water",
            "T_In_water",
            "T_Plenum_In",
        ],
        "Simulated": [
            "Storage Tank Average Temperature",
            "toutlet (C)",
            "inletT",
            "Ta",
        ],
    }

    for index, label in enumerate(TEMPERATURE_DETAILS[variable_type]):
        variables["Y-Variables"]["Temperature"]["Column Names"][variable_type].insert(
            index, label
        )

    for temperature_column in variables["Y-Variables"]["Temperature"]["Column Names"][
        variable_type
    ]:
        for index in range(len(df)):
            df.loc[index, temperature_column] = convert(
                df.loc[index, temperature_column], "degC", "degF"
            )

    return df


def add_temperature_details():
    TEMPERATURE_DETAILS = {
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

    for key in TEMPERATURE_DETAILS.keys():
        for index in range(len(TEMPERATURE_DETAILS[key])):
            variables["Y-Variables"]["Temperature"][key].insert(
                index, TEMPERATURE_DETAILS[key][index]
            )


df_measured = call_csv(measured_path, 0)
df_simulated = call_csv(simulated_path, 0)

# convert measured power from kW to W
df_measured["Power_kW"] = convert_values(df_measured["Power_kW"], "kW", "W")

i = 1
src_exists = True
while src_exists:

    col_label = f"h_src{i}In (Wh)"
    src_exists = df_simulated.columns.isin([col_label]).any()
    if src_exists:
        if i == 1:
         df_simulated["Power_kW"]= df_simulated[col_label]
        else:
         df_simulated["Power_kW"] = df_simulated["Power_kW"] + df_simulated[col_label]
    i = i + 1

# convert simulated energy consumption (Wh) for every minute to power (W)
df_simulated["Power_kW"] = convert_values(df_simulated["Power_kW"], "Wh/min", "W")

variables = {
    "Y-Variables": {
        "Power Input": {
            "Column Names": {"Measured": ["Power_kW"], "Simulated": ["Power_kW"]},
            "Labels": ["Power Input"],
            "Units": "W",
            "Colors": ["red"],
            "Line Mode": ["lines"],
            "Line Visibility": [True],
        },
        "Flow Rate": {
            "Column Names": {"Measured": ["flow_out_gpm"], "Simulated": ["draw"]},
            "Labels": ["Flow Rate"],
            "Units": "gal/min",
            "Colors": ["green"],
            "Line Mode": ["lines"],
            "Line Visibility": [True],
        },
        "Temperature": {
            "Column Names": {
                "Measured": [
                    f"T_Tank_{str(number)}"
                    for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
                ],
                "Simulated": [
                    f"tcouple{number} (C)"
                    for number in reversed(range(1, NUMBER_OF_THERMOCOUPLES + 1))
                ],
            },
            "Labels": [
                f"Storage Tank Temperature {number}"
                for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
            ],
            "Units": f"{DEGREE_SIGN}F",
            "Colors": list(reversed(RED_BLUE_DIVERGING_PALLETTE)),
            "Line Mode": ["lines"] * NUMBER_OF_THERMOCOUPLES,
            "Line Visibility": [False] * NUMBER_OF_THERMOCOUPLES,
        },
    },
    "X-Variables": {
        "Time": {
            "Column Names": {"Measured": "minutes", "Simulated": "minutes"},
            "Units": "Min",
        }
    },
}

# remove rows from dataframes outside of inclusive range [1,1440]
df_measured = filter_dataframe_range(df_measured, "Measured")
df_simulated = filter_dataframe_range(df_simulated, "Simulated")

df_measured = calculate_average_tank_temperature("Measured")
df_simulated = calculate_average_tank_temperature("Simulated")

# add average, inlet, and outlet temperature details (ex. visibility, color, etc.) to variables dictionary
add_temperature_details()


def retrieve_dataframe(variable_type):
    if variable_type == "Measured":
        return df_measured
    elif variable_type == "Simulated":
        return df_simulated


def retrieve_line_type(variable_type):
    if variable_type == "Measured":
        return None
    elif variable_type == "Simulated":
        return "dot"


def plot_graphs(variable_type, variable, value, row):
    df = retrieve_dataframe(variable_type)

    if (value in [1, 2]) and (variable_type == "Measured"):
        marker_symbol = "circle"
        marker_size = 7
        marker_fill_color = "white"
        marker_line_color = variables["Y-Variables"][variable]["Colors"][value]
    elif (value == 1) and (variable_type == "Simulated"):
        marker_symbol = "circle"
        marker_size = 7
        marker_fill_color = variables["Y-Variables"][variable]["Colors"][value]
        marker_line_color = variables["Y-Variables"][variable]["Colors"][value]
    else:
        marker_symbol = None
        marker_size = None
        marker_fill_color = None
        marker_line_color = None

    plot.add_time_series(
        dimes.TimeSeriesData(
            df[
                variables["Y-Variables"][variable]["Column Names"][variable_type][value]
            ],
            name=f"{variables['Y-Variables'][variable]['Labels'][value]} - {variable_type}",
            native_units=variables["Y-Variables"][variable]["Units"],
            line_properties=LineProperties(
                color=variables["Y-Variables"][variable]["Colors"][value],
                line_type=retrieve_line_type(variable_type),
                marker_symbol=marker_symbol,
                marker_size=marker_size,
                marker_line_color=marker_line_color,
                marker_fill_color=marker_fill_color,
                is_visible=variables["Y-Variables"][variable]["Line Visibility"][value],
            ),
        ),
        subplot_number=row,
        axis_name=variable,
    )


# remove rows from dataframes outside of inclusive range [1,1440]
df_measured = filter_dataframe_range(df_measured, "Measured")
df_simulated = filter_dataframe_range(df_simulated, "Simulated")

plot = dimes.TimeSeriesPlot(
    df_measured[variables["X-Variables"]["Time"]["Column Names"]["Measured"]]
)

for row, variable in enumerate(variables["Y-Variables"].keys()):
    for variable_type in variables["Y-Variables"][variable]["Column Names"].keys():
        for value in range(
            len(variables["Y-Variables"][variable]["Column Names"][variable_type])
        ):
            plot_graphs(variable_type, variable, value, row + 1)

plot.write_html_plot(output_path)
