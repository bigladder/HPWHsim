import os
import sys

import pandas as pd  # type: ignore
import json
from dash import Dash, dcc, html, Input, Output, State, callback, no_update, dash_table
import plotly.express as px
import plotly.graph_objects as go

import plotly.io as io
from scipy.optimize import least_squares
import numpy as np
import math
import time
from test_plot import TestPlotter, plot
import multiprocessing as mp
from pathlib import Path
from dash_extensions import WebSocket

from common import read_file, write_file, get_tank_volume

energy_kJ_format = {'specifier': ".2f"}

class TestProc:
	
	def __init__(self):
		self.ws = None
		self.i_send = 0
		self.changed = 0
		self.prefs = {}
		self.plotter = {}
		self.port_num = 8050
		self.running = False
			
		self.selected_t_minV = []
		self.selected_tank_TV = []
		self.selected_ambient_TV = []
		self.variable_type = ""
		self.i_send = 0
		self.prev_show = 0
		self.ef_val = 0
		self.plotter = {}
		self.process = {}
	
	#
	def start(self, data):
		if not self.running:
			self.process = mp.Process(target=self.proc, args=(data, ), name='test-proc')
			time.sleep(1)
			print("launching dash for plotting tests...")
			self.process.start()
			time.sleep(2)	
			self.running = True   
		return {'port_num': self.port_num}

	#
	def stop(self):
		if self.running:
			print("killing current dash for plotting tests...")
			self.process.kill()
			time.sleep(1)
			self.running = False
		return {}
				
	def sync_prefs(self):
		prefs = read_file("prefs.json")
		if 'tests' in self.prefs:
			if 'plots' in self.prefs['tests']:
				prefs['tests']['plots'] = self.prefs['tests']['plots']
		self.prefs = prefs
		write_file("prefs.json", prefs)	
		
	def init_plot(self, data):
		self.sync_prefs()
		data['model_id'] = self.prefs['model_id']
		data['test_id'] = self.prefs['tests']['id']
				
		fit_list = read_file("fit_list.json")
		if 'metrics' in fit_list:
			metrics = fit_list['metrics']
		else:
			metrics = []
		data['test_points'] = []
		
		for metric in metrics:
			if metric['type'] == 'test_point':
				res = metric['model_id'] == self.prefs['model_id']
				res = res and metric['test_id'] == self.prefs['tests']['id']						
				if res:
					data['test_points'].append(metric)
		
		self.plotter = plot(data)
		if self.plotter.have_fig:
			self.plotter.plot.figure.update_layout(clickmode='event+select')
				
			#show checks					
			show_option_list = []
			show_value_list = []
			hide_show_div= True
			if self.plotter.measured.have_data:
				show_option_list.append({'label': 'measured', 'value': 'measured'})
				show_value_list.append('measured')
				self.prev_show |= 1
				hide_show_div = False
			if self.plotter.simulated.have_data:
				show_option_list.append({'label': 'simulated', 'value': 'simulated'})
				show_value_list.append('simulated')
				self.prev_show |= 2
				hide_show_div = False
			
			#summary table	
			summary_data_list = []
			for data_set in [self.plotter.measured, self.plotter.simulated]:
				if data_set.have_data:
					self.plotter.analyze(data_set)

				for summary in ['first-recovery_period', 'standby_period', '24-hr-test']:
					for item in data_set.test_summary[summary]:
						summary_data_list.append([item])
						
			for summary_data in summary_data_list:
				for summary_data[0] in ['first-recovery_period', 'standby_period', '24-hr-test']:
					for data_set in [self.plotter.measured, self.plotter.simulated]:
						if data_set.have_data:
							if item in data_set.test_summary[summary]:
								summary_data.append(data_set.test_summary[summary][item])
							else:
								summary_data.append("")
						else:
							summary_data.append("")
			
			summary_table_df = pd.DataFrame(
				columns = ['Quantity', 'Measured', 'Simulated'],	
				data = summary_data_list
			)
			summary_table_data = summary_table_df.to_dict('records')			
			if self.plotter.simulated.have_data:
				self.prev_show |= 2
				hide_show_div = False

			return self.plotter.plot.figure, summary_table_data, hide_show_div, no_update, no_update
	
		return tuple([no_update] * 5)
	
	def update_plot(self, fig):
		#self.plotter.plot.figure.update_layout(autosize = False)
		self.plotter.reread_simulated()		
		self.plotter.update_simulated()
		#self.plotter.plot.figure.update_layout(autosize = True)
			#self.plotter.plot.figure.update_layout(fig['relayoutData'])
		for item in fig['layout']:
			if "axis" in item:		
				self.plotter.plot.figure['layout'][item] = fig['layout'][item]
		#if 'range' in fig:
			#self.plotter.plot.figure.update_layout(range = fig['range'])
		return tuple([self.plotter.plot.figure] + [no_update] * 4)
	
	def proc(self, data):	
		external_stylesheets = ['https://codepen.io/chriddyp/pen/bWLwgP.css']
		app = Dash(__name__, external_stylesheets=external_stylesheets)
		styles = {
			'pre': {
				'border': 'thin lightgrey solid',
				'overflowX': 'scroll'
			}
		}

		PARAMETERS = (
		    "tank volume (L)",
		)
		
		app.layout = [
			html.Div([
	   		WebSocket(url="ws://localhost:8600", id="ws"),
				html.Div(html.Button("send", id='send-btn', n_clicks=0), hidden=True),
				
				html.Div(dcc.Checklist(id="show-check", inline=True), id='show-div', hidden=True),

				dcc.Graph(id='test-graph', figure={}, style ={'width': '1200px', 'height': '800px', 'display': 'block'}),
		
				html.Div([
						dash_table.DataTable(
							columns=(
								{'id': "Quantity", 'name': "Quantity" }, 
								{'id': "Value", 'name': "Value", 'type': "numeric", 'format': energy_kJ_format}),
							data=[],
							style_table = {'width': '400px'},
					    style_cell_conditional=[
				        {
				            'if': {'column_id': 'Quantity'},
				            'width': '200px',
										'textAlign': 'left'
				        },
								{
				            'if': {'column_id': 'Value'},
				            'width': '150px',
										'textAlign': 'right'
				        }
					    ],
							id='selected-table'),

						html.P("selected:", id='selected-text', style={'fontSize': '12px', 'margin': '4px', 'display': 'block'}),
						html.P("match:", style={'fontSize': '12px', 'margin': '4px', 'display': 'inline-block'}),
						html.Button("+", id='match-selected', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),				
						html.Button("-", id='ignore-selected', n_clicks=0, style={'fontSize': '12px', 'margin': '1px', 'display': 'inline-block'}),				
						],
						id='select-div',
						hidden = True
					),
			
					html.Div([
							dash_table.DataTable(
								columns=(
									{'id': "Quantity", 'name': "Quantity" },
									{'id': "Measured", 'name': "Measured", 'type': "numeric", 'format': energy_kJ_format}, 
									{'id': "Simulated", 'name': "Simulated", 'type': "numeric", 'format': energy_kJ_format}),
								data=[],
								style_table = {'width': '400px'},
						    style_cell_conditional=[
					        {
					            'if': {'column_id': 'Quantity'},
					            'width': '200px',
											'textAlign': 'left'
					        },
									{
					            'if': {'column_id': 'Measured'},
					            'width': '150px',
											'textAlign': 'right'
					        },
									{
					            'if': {'column_id': 'Simulated'},
					            'width': '150px',
											'textAlign': 'right'
					        }
						    ],
								id='summary-table')
							],
						id='summary-table-div',
						hidden = True
					),
				],
			id='main-div', hidden=True)			
		]
		
		@app.callback(
			Output("ws", "send"),
			Input("send-btn", "n_clicks")
		)
		def send_message(n_clicks):
			self.i_send = self.i_send + 1
			message = json.dumps({"source": "test-proc", "dest": "index", "cmd": "init", 'port_num': self.port_num, "index": self.i_send})
			return message

		@app.callback(
					Output('ws', 'send', allow_duplicate=True),
					Output('test-graph', 'figure', allow_duplicate=True),
					Output('summary-table', 'data'),
					Output('show-div', 'hidden', allow_duplicate=True),
					Output('show-check', 'options'),
					Output('show-check', 'value'),

					[Input("ws", "message")],
					State('test-graph', 'figure'),
					prevent_initial_call=True
				)
		def message(msg, fig):
			if 'data' in msg:
				data = json.loads(msg['data'])
				if 'dest' in data and data['dest'] == 'test-proc':
					print(f"received by test-proc:\n{data}")
					if 'cmd' in data:
						msg = {"source": "test-proc", "dest": "index", "cmd": "refresh", 'port_num': self.port_num, "index": self.i_send}
						self.i_send = self.i_send + 1
						if data['cmd'] == 'plot':
							return tuple([json.dumps(msg)] + list(self.init_plot(data)))
						if data['cmd'] == 'update':
							return tuple([json.dumps(msg)] + list(self.update_plot(fig)))							
			return tuple([no_update] * 7)
		
		@callback(
			Output('test-graph', 'figure', allow_duplicate=True),
			Output('show-check', 'value', allow_duplicate=True),
			Input('show-check', 'value'),
			prevent_initial_call=True
		)
		def change_show(value):	
			data = {'show': 0}
			if 'measured' in value:
				self.prefs['tests']['plots']['show_measured'] = True
				data['show'] |= 1
			if 'simulated' in value:
				self.prefs['tests']['plots']['show_simulated'] = True
				data['show'] |= 2
			
			if data['show'] == 0:
				data['show'] = self.prev_show
			
			self.prev_show = data['show']
			self.plotter.draw(data)
			
			value_list = []
			if data['show'] & 1 == 1:
				value_list.append('measured')
			if data['show'] & 2 == 2:
				value_list.append('simulated')	
				
			return self.plotter.plot.figure, value_list
				
		@callback(
			Output('test-graph', 'figure', allow_duplicate=True),
			Output('select-div', 'hidden', allow_duplicate=True),
			Output('selected-table', 'data'),
			Output('selected-text', 'children', allow_duplicate=True),
			Input('test-graph', 'selectedData'),
			State('test-graph', 'figure'),
			prevent_initial_call=True
		)
		def select_data(selectedData, fig):
			if not selectedData:
				return no_update, True, [], ""
								
			#selectDiv['hidden'] = False
			table_df = pd.DataFrame(
				columns = ['Quantity', 'Value'],
				data = self.plotter.select_data(selectedData)
				)
			table_data = table_df.to_dict('records')
			hidden = len(table_df.index) == 0
			return fig,hidden, table_data, ""
				
		@callback(
			Output('ws', 'send', allow_duplicate=True),
			Output('test-graph', 'figure', allow_duplicate=True),
			Input('test-graph', 'clickData'),
			State('test-graph', 'figure'),
			prevent_initial_call=True
		)
		def click_data(clickData, fig):
			if not clickData:
				return no_update, no_update
			
			if not "points" in clickData:
				return no_update, no_update
					
			self.plotter.click_data(clickData)	
			self.plotter.update_selected()
			fit_list = read_file("fit_list.json")
			if 'metrics' in fit_list:
				metrics = fit_list['metrics']
			else:
				metrics = []		

			for test_point in self.plotter.test_points:
				exists = False
				for metric in metrics:
					if metric['type'] == 'test_point':						
						res = metric['variable'] == test_point['variable']
						res = res and metric['model_id'] == test_point['model_id']
						res = res and metric['test_id'] == test_point['test_id']
						res = res and metric['variable'] == test_point['variable']
						res = res and metric['t_min'] == test_point['t_min']
						exists = exists or res
						if exists:
								break
				if not(exists):
					metric = test_point
					metric['type'] = 'test_point' 
					metrics.append(metric)
			
			fit_list['metrics'] = metrics			
			write_file("fit_list.json", fit_list)
			self.i_send = self.i_send + 1
			msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": self.i_send}
			return json.dumps(msg), self.plotter.plot.figure
		
		@app.callback(
				Output('ws', 'send', allow_duplicate=True),
				Input('match-selected', 'n_clicks'),
				prevent_initial_call=True
		)
		def match_selected(nclicks):
			fit_list = read_file("fit_list.json")
			if 'metrics' in fit_list:
				metrics = fit_list['metrics']
			else:
				metrics = {}		
			
			if 'test_points' in metrics:
				test_points = metrics['test_points']
			else:
				test_points = []
			for test_point in self.plotter.test_points:
				test_points.append(test_point)
			
			if test_points:
				metrics['test_points'] = test_points
			
			if metrics:
				fit_list['metrics'] = metrics			
			write_file("fit_list.json", fit_list)
			self.i_send = self.i_send + 1
			msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": self.i_send}
			return json.dumps(msg)

		@app.callback(
				Output('ws', 'send', allow_duplicate=True),
				Input('ignore-selected', 'n_clicks'),
				prevent_initial_call=True
		)
		def ignore_selected(nclicks):
			fit_list = read_file("fit_list.json")
			if 'metrics' in fit_list:
				fit_metrics = fit_list['metrics']
			else:
				fit_metrics = []		
		
			new_metrics = fit_metrics
			for metric in self.plotter.metrics:
				for index, fit_metric in reversed(list(enumerate(fit_metrics))):
					if 'type' not in metric or metric['type'] != fit_metric['type']:
						continue
					if 'variable' not in metric or metric['variable'] != fit_metric['variable']:
						continue
					if 'model_id' not in metric or metric['model_id'] != fit_metric['model_id']:
						continue				
					if 't_min' not in metric or metric['t_min'] != fit_metric['t_min']:
						continue
									
					del new_metrics[index]
			
			fit_list['metrics'] = new_metrics			
			write_file("fit_list.json", fit_list)
			self.i_send = self.i_send + 1
			msg = {"source": "test-proc", "dest": "index", "cmd": "refresh-fit", "index": self.i_send}
			return json.dumps(msg)
		
		app.run(debug=True, use_reloader=False, port = self.port_num)


test_proc = TestProc()