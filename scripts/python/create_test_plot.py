import sys
from pathlib import Path

import pandas as pd
import plotly.graph_objects as go
from plotly.subplots import make_subplots
from koozie import convert

if len(sys.argv) != 4:
    sys.exit("Incorrect number of arguments. Must be two: Measured Path, Simulated Path, Output Path")

measured_path = Path(sys.argv[1])
simulated_path = Path(sys.argv[2])
output_path = Path(sys.argv[3])

pd.set_option("display.max_rows", 1500)

DEGREE_SIGN = "\N{DEGREE SIGN}"
GRID_LINE_WIDTH = 1.5
GRID_LINES_COLOR = "rgba(128,128,128,0.3)"
RED_BLUE_DIVERGING_PALLETTE = [
    "#003a6d",
    "#0072c3",
    "#33b1ff",
    "#ff8389",
    "#da1e28",
    "#750e13",
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


def calculate_energy_consumption():
    global variables, df_measured, df_simulated
    column_energy_name_measured = variables["Y-Variables"]["Power Input"][
        "Column Names"
    ]["Measured"]
    column_energy_name_simulated = variables["Y-Variables"]["Power Input"][
        "Column Names"
    ]["Simulated"]

    # set parasitic load to 0
    df_measured_no_parasitic_load = df_measured[column_energy_name_measured].where(
        df_measured[column_energy_name_measured] > 10, 0
    )

    print(
        f"Measured Energy Consumption:  {df_measured[column_energy_name_measured].sum().values[0] / 60:.0f} Wh"
    )
    # print(f"Measured Energy (no parasitic load) {df_measured_no_parasitic_load[column_energy_name_measured].sum().values[0]/60:.0f} Wh")
    print(
        f"Simulated Energy Consumption: {df_simulated[column_energy_name_simulated].sum().values[0] / 60:.0f} Wh"
    )


def set_inlet_temperature_to_nan_at_no_draw(df, inlet_temperature_column, draw_column):
    for index in range(len(df)):
        if df.loc[index, draw_column] == 0:
            df.loc[index, inlet_temperature_column] = None

    return df


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
        "Line Visibility": [True, True, "legendonly", "legendonly"],
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
# convert simulated energy consumption (Wh) for every minute to power (W)
# (Wh / min) * (60 min / 1 h)
df_simulated["h_src1In (Wh)"] = convert_values(
    df_simulated["h_src1In (Wh)"], "Wh/min", "W"
)

fig = go.Figure()
fig = make_subplots(rows=3, shared_xaxes=True, vertical_spacing=0.05)

# link column names in measured and simulated dataframes to variable name
# link variable name to units and graph color
variables = {
    "Y-Variables": {
        "Power Input": {
            "Column Names": {
                "Measured": ["Power_kW"],
                "Simulated": ["h_src1In (Wh)"],
            },
            "Labels": ["Power Input"],
            "Units": "W",
            "Colors": ["red"],
            "Line Mode": ["lines"],
            "Line Visibility": [True],
        },
        "Flow Rate": {
            "Column Names": {"Measured": ["flow_out_gpm"], "Simulated": ["draw"]},
            "Labels": ["Flow Rate"],
            "Units": "Gal/min",
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
                    for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
                ],
            },
            "Labels": [
                f"Storage Tank Temperature {number}"
                for number in range(1, NUMBER_OF_THERMOCOUPLES + 1)
            ],
            "Units": f"{DEGREE_SIGN}F",
            "Colors": list(RED_BLUE_DIVERGING_PALLETTE),
            "Line Mode": ["lines"] * NUMBER_OF_THERMOCOUPLES,
            "Line Visibility": ["legendonly"] * NUMBER_OF_THERMOCOUPLES,
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

# print measured and simulated energy consumption
calculate_energy_consumption()


def retrieve_dataframe(variable_type):
    if variable_type == "Measured":
        return df_measured
    elif variable_type == "Simulated":
        return df_simulated


def retrieve_line_type(variable_type, value):
    if variable_type == "Measured":
        return dict(color=variables["Y-Variables"][variable]["Colors"][value])
    elif variable_type == "Simulated":
        return dict(
            color=variables["Y-Variables"][variable]["Colors"][value], dash="dot"
        )


def plot_graphs(variable_type, variable, value, row):
    df = retrieve_dataframe(variable_type)
    LINE_TYPE = retrieve_line_type(variable_type, value)

    if (value in [1, 2]) and (variable_type == "Measured"):
        fillcolor = "white"
        marker = dict(
            size=7,
            line=dict(
                color=variables["Y-Variables"][variable]["Colors"][value], width=2
            ),
        )
    elif (value in [1, 2]) and (variable_type == "Simulated"):
        fillcolor = None
        marker = dict(
            size=7,
            color="white",
            line=dict(
                color=variables["Y-Variables"][variable]["Colors"][value], width=2
            ),
        )
    else:
        fillcolor = None
        marker = None

    fig.add_trace(
        go.Scatter(
            x=df[variables["X-Variables"]["Time"]["Column Names"][variable_type]],
            y=df[
                variables["Y-Variables"][variable]["Column Names"][variable_type][value]
            ],
            name=f"{variables['Y-Variables'][variable]['Labels'][value]} - {variable_type}",
            mode="lines+markers"
            if (value == 2) and (variable_type == "Measured")
            else variables["Y-Variables"][variable]["Line Mode"][value],
            marker=marker,
            fillcolor=fillcolor,
            line=LINE_TYPE,
            visible=variables["Y-Variables"][variable]["Line Visibility"][value],
        ),
        row=row,
        col=1,
    )


def retrieve_y_axis_range(y_max, y_min, row, variable_type):
    df = retrieve_dataframe(variable_type)
    y_max[row].append(
        max(
            df[variables["Y-Variables"][variable]["Column Names"][variable_type][value]]
        )
    )
    y_min[row].append(
        min(
            df[variables["Y-Variables"][variable]["Column Names"][variable_type][value]]
        )
    )
    return y_max, y_min


y_max = [[], [], []]
y_min = [[], [], []]

for row, variable in enumerate(variables["Y-Variables"].keys()):
    for variable_type in variables["Y-Variables"][variable]["Column Names"].keys():
        for value in range(
                len(variables["Y-Variables"][variable]["Column Names"][variable_type])
        ):
            plot_graphs(variable_type, variable, value, row + 1)
            y_max, y_min = retrieve_y_axis_range(y_max, y_min, row, variable_type)

BLANK_SPACE = 0.1
y_range = [[], [], []]

for row in range(3):
    y_max_row = max(y_max[row])
    y_min_row = min(y_min[row])
    y_range_row = y_max_row - y_min_row
    y_range[row].append(y_min_row - y_range_row * BLANK_SPACE)
    y_range[row].append(y_max_row + y_range_row * BLANK_SPACE)

fig.update_layout(
    xaxis_rangeslider_visible=False,
    xaxis2_rangeslider_visible=False,
    xaxis3_rangeslider_visible=False,
    yaxis_title="Power Input<br>(W)",
    yaxis2_title="Flow Rate<br>(gpm)<br>",
    yaxis3_title=f"Temperature<br>({DEGREE_SIGN}F)",
    yaxis=dict(range=y_range[0]),
    yaxis2=dict(range=y_range[1]),
    yaxis3=dict(range=y_range[2]),
    xaxis3_title="Time (min)",
    title="HPWH Data",
    title_x=0.5,
    plot_bgcolor="white",
    font_color="black",
    margin=dict(l=0, r=0, t=30, b=40),
)
fig.update_xaxes(
    linecolor="black",
    linewidth=GRID_LINE_WIDTH,
    mirror=True,
    ticks="outside",
    tickson="boundaries",
    tickwidth=GRID_LINE_WIDTH,
    tickcolor="black",
    minallowed=1,
    maxallowed=1440,
    rangeslider=dict(bgcolor="grey", thickness=0.05),
)
fig.update_yaxes(
    showline=True,
    linecolor="black",
    linewidth=GRID_LINE_WIDTH,
    mirror=True,
    showgrid=True,
    gridcolor=GRID_LINES_COLOR,
    gridwidth=GRID_LINE_WIDTH,
    fixedrange=False,
    zeroline=True,
    zerolinecolor=GRID_LINES_COLOR,
)

fig.write_html(output_path)