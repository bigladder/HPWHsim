import pandas as pd
import plotly.graph_objects as go
import plotly.offline as pyo
from plotly.subplots import make_subplots
from koozie import convert

pd.set_option('display.max_rows', 1500)

DEGREE_SIGN = u'\N{DEGREE SIGN}'
GRID_LINE_WIDTH = 1.5
GRID_LINES_COLOR = "rgba(128,128,128,0.3)"
# TODO: reverse colors in list below, and revert reverse in variables dictionary
RED_BLUE_DIVERGING_PALLETTE = ["#750e13","#da1e28","#ff8389",
                               "#33b1ff","#0072c3","#003a6d"]

STORAGE_TANK_AVG_TEMP_LABEL = "Storage Tank Average Temperature"
STORAGE_TANK_AVG_TEMP_COLOR = "black"

NUMBER_OF_THERMOCOUPLES = 6

def call_csv(path,skip_rows):
	data = pd.read_csv(path,skiprows=skip_rows)
	df = pd.DataFrame(data)
	return df

def convert_values(df_column,from_units,to_units):
    converted_column = df_column.apply(lambda x: convert(x,from_units,to_units))
    return converted_column

def filter_dataframe_range(df,variable_type):
    global variables
    column_time_name = variables["X-Variables"]["Time"]["Column Names"][variable_type]
    return df[(df[column_time_name] > 0) & (df[column_time_name] <= 1440)].reset_index()

def calculate_energy_consumption():
    global variables, df_measured, df_simulated
    column_energy_name_measured = variables["Y-Variables"]["Power Input"]["Column Names"]["Measured"]
    column_energy_name_simulated = variables["Y-Variables"]["Power Input"]["Column Names"]["Simulated"]

    # set parasitic load to 0
    df_measured_no_parasitic_load = df_measured[column_energy_name_measured].where(df_measured[column_energy_name_measured] > 10, 0)

    print(f"Measured Energy Consumption:  {df_measured[column_energy_name_measured].sum().values[0]/60:.0f} Wh")
    # print(f"Measured Energy (no parasitic load) {df_measured_no_parasitic_load[column_energy_name_measured].sum().values[0]/60:.0f} Wh")
    print(f"Simulated Energy Consumption: {df_simulated[column_energy_name_simulated].sum().values[0]/60:.0f} Wh")

def calculate_average_tank_temperature(variable_type):
    global df_measured, df_simulated

    if variable_type == "Measured":
        df = df_measured
        AVG_TEMP_UNIT_LABEL = " (C)"
    elif variable_type == "Simulated":
        df = df_simulated
        AVG_TEMP_UNIT_LABEL = ""

    df[f"{STORAGE_TANK_AVG_TEMP_LABEL} (C)"] = df[variables["Y-Variables"]["Temperature"]["Column Names"][variable_type]].mean(axis=1)
    variables["Y-Variables"]["Temperature"]["Column Names"][variable_type].insert(0,f"{STORAGE_TANK_AVG_TEMP_LABEL}{AVG_TEMP_UNIT_LABEL}")

    if variable_type == "Measured":
        [df.rename(columns={temperature_column:f"{temperature_column}{AVG_TEMP_UNIT_LABEL}"}, inplace=True) for temperature_column in variables["Y-Variables"]["Temperature"]["Column Names"]["Measured"]] 
    
    for temperature_column in variables["Y-Variables"]["Temperature"]["Column Names"][variable_type]:
        for index in range(len(df)):
            df.loc[index,temperature_column] = convert(df.loc[index,temperature_column],"degC","degF")

    return df

def add_average_temperature_details():

    AVERAGE_TEMPERATURE_DETAILS = {
            "Labels":f"{STORAGE_TANK_AVG_TEMP_LABEL}",
            "Colors":f"{STORAGE_TANK_AVG_TEMP_COLOR}"
        }
    
    for key in AVERAGE_TEMPERATURE_DETAILS.keys():
        variables["Y-Variables"]["Temperature"][key].insert(0,AVERAGE_TEMPERATURE_DETAILS[key])


df_measured = call_csv("plots/villara_24hr67_Expt_AquaThermAire.csv",0)
df_simulated = call_csv("plots/villara_24hr67_File_AquaThermAire.csv",0)

# convert measured power from kW to W
df_measured["Power_kW"] = convert_values(df_measured["Power_kW"],"kW","W")
# convert simulated energy consumption (Wh) for every minute to power (W)
# (Wh / min) * (60 min / 1 h)
df_simulated["h_src1In (Wh)"] = df_simulated["h_src1In (Wh)"] * 60

fig = go.Figure()
fig = make_subplots(rows=3,shared_xaxes=True, vertical_spacing=0.05)

# link column names in measured and simulated dataframes to variable name
# link variable name to units and graph color
variables = {"Y-Variables":{
                "Power Input":
                    {"Column Names":{"Measured":["Power_kW"],
							 "Simulated":["h_src1In (Wh)"]},
                    "Labels":["Power Input"],
					"Units":"W",
                    "Colors":["red"]},
                "Flow Rate":
                    {"Column Names":{"Measured":["flow_out_gpm"],
							 "Simulated":["draw"]},
					"Labels":["Flow Rate"],
                    "Units":"Gal/min",
                    "Colors":["green"]},
                "Temperature":
                    {"Column Names":{"Measured":[f"T_Tank_{str(number)}" for number in range(1,NUMBER_OF_THERMOCOUPLES+1)],
                             "Simulated":[f"tcouple{number} (C)" for number in reversed(range(1,NUMBER_OF_THERMOCOUPLES+1))]},
                    "Labels":[f"Storage Tank Temperature {number}" for number in range(1,NUMBER_OF_THERMOCOUPLES+1)],
                    "Units":f"{DEGREE_SIGN}F",
                    "Colors":list(reversed(RED_BLUE_DIVERGING_PALLETTE))}
                },
            "X-Variables":{"Time":
                    {"Column Names":{"Measured":"minutes",
							 "Simulated":"minutes"},
					"Units":"Min",
                    }
            }}

# remove rows from dataframes outside of inclusive range [1,1440] 
df_measured = filter_dataframe_range(df_measured,"Measured")
df_simulated = filter_dataframe_range(df_simulated,"Simulated")

df_measured = calculate_average_tank_temperature("Measured")
df_simulated = calculate_average_tank_temperature("Simulated")
add_average_temperature_details()

# print measured and simulated energy consumption
calculate_energy_consumption()

def retrieve_dataframe(variable_type):
    if variable_type == "Measured":
         return df_measured
    elif variable_type == "Simulated":
         return df_simulated

def retrieve_line_type(variable_type): 
    if variable_type == "Measured":
         return dict(color=variables["Y-Variables"][variable]["Colors"][value])
    elif variable_type == "Simulated":
         return dict(color=variables["Y-Variables"][variable]["Colors"][value],dash='dot')

def plot_graphs(variable_type,variable,value,row):
    df = retrieve_dataframe(variable_type)
    LINE_TYPE = retrieve_line_type(variable_type)
    if value == 0:
        visible = True
    else:
        visible = 'legendonly'
    fig.add_trace(go.Scatter(
                        x=df[variables["X-Variables"]["Time"]["Column Names"][variable_type]],
                        y=df[variables["Y-Variables"][variable]["Column Names"][variable_type][value]],
                        name=f"{variables['Y-Variables'][variable]['Labels'][value]} - {variable_type}",
                        mode="lines",
                        line=LINE_TYPE,
                        visible=visible,
                        ),
                row=row,
                col=1
                    )

def retrieve_y_axis_range(y_max,y_min,row,variable_type):
    df = retrieve_dataframe(variable_type)
    y_max[row].append(max(df[variables["Y-Variables"][variable]["Column Names"][variable_type][value]]))
    y_min[row].append(min(df[variables["Y-Variables"][variable]["Column Names"][variable_type][value]]))
    return y_max,y_min

y_max = [[],[],[]]
y_min = [[],[],[]]

for row, variable in enumerate(variables["Y-Variables"].keys()):
    for variable_type in variables["Y-Variables"][variable]["Column Names"].keys():
        for value in range(len(variables["Y-Variables"][variable]["Column Names"][variable_type])):
            plot_graphs(variable_type,variable,value,row+1)
            y_max,y_min = retrieve_y_axis_range(y_max,y_min,row,variable_type)

BLANK_SPACE = 0.1
y_range = [[],[],[]]

for row in range(3):
    y_max_row = max(y_max[row])
    y_min_row = min(y_min[row])
    y_range_row = y_max_row - y_min_row
    y_range[row].append(y_min_row - y_range_row*BLANK_SPACE)
    y_range[row].append(y_max_row + y_range_row*BLANK_SPACE)
    
fig.update_layout(
    xaxis_rangeslider_visible=False,
    xaxis2_rangeslider_visible=False,
    xaxis3_rangeslider_visible=False,
    yaxis_title="Power Input<br>(W)",
    yaxis2_title="Flow Rate<br>(gpm)<br>",
    yaxis3_title=f"Temperature<br>({DEGREE_SIGN}F)",
    yaxis = dict(range=y_range[0]),
    yaxis2 = dict(range=y_range[1]),
    yaxis3 = dict(range=y_range[2]),
    xaxis = dict(range=[0,1440]),
    xaxis3_title="Time (min)",
    title="HPWH Data",
    title_x=0.5,
    plot_bgcolor='white',
    font_color='black',
    margin=dict(l=0, r=0, t=30, b=40),

    # updatemenus =[dict(type='buttons',
    #                 buttons=[button_degF, button_degC],
    #                 x=1.05,
    #                 xanchor="left",
    #                 y=1,
    #                 yanchor="top")])

    )
fig.update_xaxes(
    linecolor='black',
    linewidth=GRID_LINE_WIDTH,
    mirror=True,
    ticks='outside',
    tickson='boundaries',
    tickwidth=GRID_LINE_WIDTH,
    tickcolor='black',
    rangeslider=dict(
         bgcolor='grey',
         thickness=0.05
    )
    )
fig.update_yaxes(
    showline=True,
    linecolor='black',
    linewidth=GRID_LINE_WIDTH,
    mirror=True,
    showgrid=True,
    gridcolor=GRID_LINES_COLOR,
    gridwidth=GRID_LINE_WIDTH,
    fixedrange=False,
    zeroline = True,
    zerolinecolor = GRID_LINES_COLOR,
    )

file_path = "hpwh_plots.html"
fig.write_html(file_path)