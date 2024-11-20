import os
import sys

import pandas as pd  # type: ignore
import json
from dash import Dash, dcc, html, Input, Output, State, callback, set_props
import threading

def dash_proc(fig):

		app = Dash(__name__)
		app.layout = dcc.Graph(id='test-graph', figure=fig, style ={'width': '1200px', 'height': '800px', 'display': 'block'} )	
		app.run(debug=True, use_reloader=False)
