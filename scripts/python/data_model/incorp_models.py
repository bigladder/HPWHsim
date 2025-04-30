# 'poetry run python incorp_models.py IHPWH_models.json ../../../build'
# or
# 'poetry run python incorp_models.py CWHS_models.json ../../../build'
# calls `hpwh convert' for each model in models_list_file json,
# then creates a readable C++ file containing that data.

from pathlib import Path
import os
import sys
import json
import subprocess

def incorp_models(models_list_file, build_dir):
    
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	with open(models_list_file) as json_file:
		json_data = json.load(json_file)
		json_file.close()

		app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
		output_dir = os.path.join(abs_build_dir , "test", "output")

		if not os.path.exists(output_dir):
			os.mkdir(output_dir)
			
		combined_header =  "#ifndef MODELS_H\n"
		combined_header += "#define MODELS_H\n\n"
		
		for model in json_data:  
			convert_list = [app_cmd, 'convert', '-m', str(model["number"]), '-n', '-d', output_dir, '-f', model["name"]]
			result = subprocess.run(convert_list, stdout=subprocess.PIPE, text=True)	
				
			model_json_path = os.path.join(output_dir, model["name"] + ".json")
		  
			data = {}
			try:
				with open(model_json_path) as json_data:
					data = json.load(json_data)
					json_data.close()
			except:
				continue	
			model_text_path = os.path.join("../../../src/presets/include", model["name"] + ".h")

			guard_name = model["name"].upper() + "_H"
			combined_header += "#include <" + model["name"] + ".h>\n"
			
			model_text = "#ifndef " + guard_name + "\n"
			model_text += "#define " + guard_name + "\n\n"

			model_text += "constexpr char *j_" + model["name"] + "= R\"config("
			model_text += json.dumps(data)
			model_text += ")config\";" + "\n\n"
			
			model_text += "#endif\n"
			
			try:	
				with open(model_text_path, "w") as model_text_file:
					model_text_file.write(model_text)
					model_text_file.close()
			except:
				print("Failed to create file")

		combined_header += "#endif\n\n"				
		try:	
			with open("../../../src/presets/models.h", "w") as model_header:
				model_header.write(combined_header)
				model_header.close()
		except:
			print("Failed to combined header")

	os.chdir(orig_dir)
  
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 2:
        models_list_file = sys.argv[1]
        build_dir = sys.argv[2]

        incorp_models(models_list_file, build_dir)

