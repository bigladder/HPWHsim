# 'poetry run python params.py IHPWH_models.json ../../../build ../build/test/results'
# Generates hpwh-data-model source code based on schema in 'data_model'dir'
# into 'src_out_dir'.

from pathlib import Path
import os
import sys
import json
import subprocess

def list_params(models_list_file, build_dir):
    
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)


	with open(models_list_file) as json_file:
		json_data = json.load(json_file)
		json_file.close()  

		app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
		output_dir = os.path.join(abs_build_dir , "test", "results")

		if not os.path.exists(output_dir):
			os.mkdir(output_dir)

		os.chdir("../../../test")
		for model in json_data:  
			outfilename =  "results_" + model["name"]
			measure_list = [app_cmd, 'measure', '-s', 'JSON', '-m', model["name"], '-d', output_dir, '-r', outfilename, '-n']
			try:
				result = subprocess.run(measure_list, stdout=subprocess.PIPE, text=True)
			except:
				print("Unable to measure {}".format(model["name"]))

		os.chdir(output_dir)
		summary_filename = 'summary.csv'
		with open(summary_filename, 'w') as summary_file:
			summary_file.write("model, COP, UA (kJ/hC), UEF\n")
			for model in json_data:
				model_results_filename = "results_" + model["name"] + ".json"
				with open(model_results_filename) as model_results_file:
					try:
						res = json.load(model_results_file) 
						model_results_file.close()
						summary_file.write("{}, {}, {}, {}\n".format(model["name"], res["recovery_efficiency"], res["standby_loss_coefficient_kJperhC"], res["UEF"]) )
					except:
						print("Unable to read results for {}".format(model["name"]))
			summary_file.close()

	os.chdir(orig_dir)
  
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 2:
        models_list_file = sys.argv[1]
        build_dir = sys.argv[2]

        list_params(models_list_file, build_dir)

