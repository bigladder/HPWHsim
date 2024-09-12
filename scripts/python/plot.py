import sys
from pathlib import Path

import dimes  # type: ignore
from dimes import LineProperties
import pandas as pd  # type: ignore
from koozie import convert  # type: ignore
import json

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
    print(path)

    data = pd.read_csv(path, skiprows=skip_rows)
    df = pd.DataFrame(data)
    return df


def convert_values(df_column, from_units, to_units):
    converted_column = df_column.apply(lambda x: convert(x, from_units, to_units))
    return converted_column


def filter_dataframe_range(df, variable_type, variables):
    column_time_name = variables["X-Variables"]["Time"]["Column Names"][variable_type]
    return df[(df[column_time_name] > 0) & (df[column_time_name] <= 1440)].reset_index()


def calculate_average_tank_temperature(df_measured, df_simulated, variable_type, variables):
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

def add_temperature_details(variables):
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

def retrieve_dataframe(df_measured, df_simulated, variable_type):
    if variable_type == "Measured":
        return df_measured
    elif variable_type == "Simulated":
        return df_simulated


def retrieve_line_type(variable_type):
    if variable_type == "Measured":
        return None
    elif variable_type == "Simulated":
        return "dot"


def plot_graphs(plot, df_measured, df_simulated, variable_type, variable, variables, value, row):
    df = retrieve_dataframe(df_measured, df_simulated, variable_type)

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

#
def plot(measured_path, simulated_path):
    power_col_label_meas = "Power_W"
    power_col_label_sim = "Power_W"

    df_measured = call_csv(measured_path, 0)
    df_simulated = call_csv(simulated_path, 0)

    variables = {
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
                "Column Names": {"Measured": "Time(min)", "Simulated": "minutes"},
                "Units": "Min",
            }
        },
    }

    # remove rows from dataframes outside of inclusive range [1,1440]
    df_measured = filter_dataframe_range(df_measured, "Measured", variables)
    df_simulated = filter_dataframe_range(df_simulated, "Simulated", variables)

    df_measured = calculate_average_tank_temperature(df_measured, df_simulated, "Measured", variables)
    df_simulated = calculate_average_tank_temperature(df_measured, df_simulated, "Simulated", variables)

    df_measured[power_col_label_meas] = df_measured["PowerIn(W)"]

    # sum sim power if multiple heat sources
    i = 1
    src_exists = True
    while src_exists:
        col_label = f"h_src{i}In (Wh)"
        src_exists = df_simulated.columns.isin([col_label]).any()
        if src_exists:
            if i == 1:
                df_simulated[power_col_label_sim] = df_simulated[col_label]
            else:
                df_simulated[power_col_label_sim] = df_simulated[power_col_label_sim] + df_simulated[col_label]
        i = i + 1

    # convert simulated energy consumption (Wh) for every minute to power (W)
    df_simulated[power_col_label_sim] = convert_values(df_simulated[power_col_label_sim], "Wh/min", "W")

    # add average, inlet, and outlet temperature details (ex. visibility, color, etc.) to variables dictionary
    add_temperature_details(variables)
    
    plot = dimes.TimeSeriesPlot(
        df_measured[variables["X-Variables"]["Time"]["Column Names"]["Measured"]]
    )
    
    for row, variable in enumerate(variables["Y-Variables"].keys()):
        for variable_type in variables["Y-Variables"][variable]["Column Names"].keys():
            for value in range(
                    len(variables["Y-Variables"][variable]["Column Names"][variable_type])
            ):
                plot_graphs(plot, df_measured, df_simulated, variable_type, variable, variables, value, row + 1)

    plot.finalize_plot()
    fig = plot.figure
    plot_html = fig.to_html(full_html=True)
    #print(plot_html)
    
    #plot.write_html_plot(output_path)
   
    # return energy string
    result = {}
    result['plot_html'] = plot_html
    result['measuredE_Wh'] = df_measured[power_col_label_meas].sum()/60
    result['simulatedE_Wh'] = df_simulated[power_col_label_sim].sum()/60
    return result

#  main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1
    if n_args == 3:
        measured_path = Path(sys.argv[1])
        simulated_path = Path(sys.argv[2])
        plot(measured_path, simulated_path)
    else:
        sys.exit(
            "Incorrect number of arguments. Must be two: Measured Path, Simulated Path"
        )
    