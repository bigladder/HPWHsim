# 'poetry run python incorp_models.py IHPWH_models.json ../../../build ../../../src/models'
# or
# 'poetry run python incorp_models.py CWHS_models.json ../../../build ../../../src/models'
# calls `hpwh convert' for each model in models_list_file json,
# then creates a readable C++ file containing that data.

from pathlib import Path
import os
import sys
import json
import subprocess

def incorp_models(models_list_file, build_dir, output_dir):
    
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	with open(models_list_file) as json_file:
		json_data = json.load(json_file)
		json_file.close()

		app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
		if output_dir == "":
			output_dir = os.path.join(abs_build_dir , "test", "output")

		if not os.path.exists(output_dir):
			os.mkdir(output_dir)
			
		for model in json_data:  
			convert_list = [app_cmd, 'convert', '-m', str(model["number"]), '-n', '-d', output_dir, '-f', model["name"]]
		  
			model_json_path = os.path.join(output_dir, model["name"] + ".json")
		  
			try:
				with open(model_json_path) as json_data:
						data = json.load(json_data)
						model_text_path = os.path.join("../../../src/models", model["name"] + ".h"), 
			except:
				continue
			


		  print(convert_list)
		  result = subprocess.run(convert_list, stdout=subprocess.PIPE, text=True)



  os.chdir(orig_dir)
  
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 3:
        models_list_file = sys.argv[1]
        build_dir = sys.argv[2]
        output_dir = sys.argv[3]

        incorp_models(models_list_file, build_dir, output_dir)

