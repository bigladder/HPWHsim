# 'poetry run python save_graphs.py ../IHPWH_models.json ../../../build/test/perf_plots'
# or
# 'poetry run python save_graphs.py ../CWHS_models.json ../../../build/test/perf_plots'
# Generates performance map plot for each model in list

from pathlib import Path
import os
import sys
import json
import subprocess
from perf_plot import write_plot
from common import read_file, write_file, get_perf_map

def save_graphs(models_list_file, output_dir):
 
	with open(models_list_file) as json_file:
		json_data = json.load(json_file)
		json_file.close()

		if not os.path.exists(output_dir):
			os.mkdir(output_dir)

		prefs = {}
		prefs["contour_coloring"] = 1
		prefs["interpolate"] = 0
		prefs["show_points"] = 1
		
		model_dir = "../../../test/models_json"
		for model in json_data: 
			model_data_filepath = os.path.join(model_dir, model['name'] + ".json")
			model_data = read_file(model_data_filepath)
			perf_map = get_perf_map(model_data)
			prefs["model_id"] = model["name"]
			
			model_output_path = os.path.join(output_dir, model['name'])
			if not os.path.exists(model_output_path):
				os.mkdir(model_output_path)
			
			is_central = "central_system" in model_data						
			contour_variable = 0
			for variable in ["Pin", "Pout", "COP"]:
				prefs["contour_variable"] = contour_variable
				plot_filepath = os.path.join(model_output_path, variable)
				
				if is_central:
					grid_vars = perf_map["grid_variables"]
					nT3s = len(grid_vars["condenser_leaving_temperature"])
					for iT3 in range(nT3s):
						ext_plot_filepath = plot_filepath + "_T" + str(iT3)
						prefs["iT3"] = iT3
						ext_plot_filepath = ext_plot_filepath + ".html" 
						write_plot(prefs, model_data, ext_plot_filepath)
						print(ext_plot_filepath)
				else:
						plot_filepath = plot_filepath + ".html" 
						print(plot_filepath)
						write_plot(prefs, model_data, plot_filepath)
				contour_variable += 1
  
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 2:
        models_list_file = sys.argv[1]
        output_dir = sys.argv[2]

        save_graphs(models_list_file, output_dir)

