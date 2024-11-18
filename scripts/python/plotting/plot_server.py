import os
import sys
from pathlib import Path
from tkinter import Tk
from tkinter.filedialog import askdirectory

import dimes  # type: ignore
from dimes import LineProperties
import pandas as pd  # type: ignore
from koozie import convert  # type: ignore
import json
from dash import Dash, dcc, html, Input, Output, State, callback, set_props
import plotly.express as px
from simulate import simulate
from measure import measure

from plot import Plotter

#
app = Dash(__name__)
app.layout = [
		html.Label("build directory", htmlFor='build-dir'),
		dcc.Input(id="build-dir", type="text", value="../../../build", style={'width': '400px', "margin-left": "5px"}),
		html.Br(), html.Br(),
						
		html.Div([
			html.Label("model spec", htmlFor='model-spec'),
	    dcc.Dropdown(
		    id='model-spec',
		    options=["Preset", "File"],
		    value="File",
		    clearable=False, style={'width': '200px', "margin-left": "5px"})
			], style=dict(display='flex')),
		html.Br(),
		

		html.Label("model name", htmlFor='model-name'),
		dcc.Input(id='model-name', type="text", value="BradfordWhiteAeroThermRE2H65", style={'width': '400px', "margin-left": "5px"}),
		html.Br(), html.Br(),
		
		html.Label("test directory", htmlFor='test-dir'),	
		dcc.Input(id="test-dir", type="text", value="BradfordWhite/AeroThermRE2H65/RE2H65_UEF50", style={'width': '400px', "margin-left": "5px"}),
		dcc.Upload(html.Button('Select'), id='test-dir-upload'),

		html.Br(), html.Br(),
		
		html.Div([
			html.Label("Draw profile", htmlFor='draw-profile'),
	    dcc.Dropdown(
		    id='draw-profile',
		    options=["auto", "High", "Medium", "Low", "Very Small"],
		    value="auto",
		    clearable=False, style={'width': '200px', "margin-left": "5px"})
			], style=dict(display='flex')),
			
		html.Button("Test", id='test-btn', n_clicks=0),
		
		html.Div(id='test-div', hidden=True, children=[
			dcc.Graph(id='test-graph', style ={'width': '1200px', 'height': '800px', 'display': 'block'} ),
			html.Br(),
			
			html.Div("", id='results', style={'whiteSpace': 'pre'}),
			html.Br()]
		)
]

@callback(
	Output('test-dir', 'value'),
			
	Input('test-dir-upload', 'filename'),
	prevent_initial_call=True
)
def select_test_dir(filename):
	orig_dir = str(Path.cwd())
	os.chdir("../../../test")
	
	print(filename)
	
	os.chdir(orig_dir)
	return filename
	
@callback(
		Output('test-graph', 'figure'),
		Output('results', 'children'),

		Input('test-btn', "n_clicks"),
		State('build-dir', "value"),
		State('model-spec', "value"),
		State('model-name', "value"),
		State('draw-profile', "value"),
		running=[(Output('test-btn', "disabled"), True, False), (Output('test-div', "hidden"), True, False)],
		prevent_initial_call=True,	
)
def update_graph(n_clicks, build_dir, test_dir, model_spec, model_name, draw_profile):

	simulate(model_spec, model_name, test_dir, build_dir)
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	os.chdir("../../../test")
	abs_test_dir = str(Path.cwd())
	os.chdir(orig_dir)

	abs_output_dir = os.path.join(abs_build_dir, "test", "output") 
	if not os.path.exists(abs_output_dir):
		os.mkdir(abs_output_dir)
	    
	measured_path = os.path.join(abs_test_dir, test_dir, "measured.csv")    
	test_name = Path(test_dir).name

	simulated_path = os.path.join(abs_output_dir, test_name + "_" + model_spec + "_" + model_name + ".csv")

	plotter = Plotter()
	plotter.read(measured_path, simulated_path)
		
	measure(model_spec, model_name, build_dir, draw_profile)
	results_path = os.path.join(abs_output_dir, "results.txt")
	f = open(results_path, "r")
	results = f.read()
	f.close()
	#print(results)
	return plotter.plot().fig, results
#plotter.plot().fig, {'width': '1200px', 'height': '800px', 'display': 'block'}, results, {'width': '800px', 'width': '800px', 'display': 'block'}

#  main
if __name__ == "__main__":
    app.run(debug=True)