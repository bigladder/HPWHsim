# 'poetry run python incorp_presets.py ../../../build IHPWH_models.json CWHS_models.json'

# calls `hpwh convert' for each model in models_list_file json,
# then creates a C++ header file containing that data.

from pathlib import Path
import os
import sys
import json
import subprocess

def incorp_presets(presets_list_files, build_dir):
    
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	presets_header_text = ""

	for preset_list_file in presets_list_files:
		with open(preset_list_file) as json_file:
			json_data = json.load(json_file)
			json_file.close()

			app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
			output_dir = os.path.join(abs_build_dir , "test", "output")

			if not os.path.exists(output_dir):
				os.mkdir(output_dir)
				
			for preset in json_data:  
				convert_list = [app_cmd, 'convert', '-m', str(preset["number"]), '-n', '-d', output_dir, '-f', preset["name"]]
				result = subprocess.run(convert_list, stdout=subprocess.PIPE, text=True)	
					
				preset_json_path = os.path.join(output_dir, preset["name"] + ".json")
			  
				data = {}
				try:
					with open(preset_json_path) as json_data:
						data = json.load(json_data)
						json_data.close()
				except:
					continue	

				guard_name = preset["name"].upper() + "_H"
				
				preset_text = "#ifndef " + guard_name + "\n"
				preset_text += "#define " + guard_name + "\n\n"

				preset_text += "namespace hpwh_presets {\n"
				preset_text += "constexpr char *" + preset["name"] + "= R\"config("
				preset_text += json.dumps(data)
				preset_text += ")config\";" + "\n\n"
				
				preset_text += "constexpr int index_" + preset["name"] + " = " + str(preset["number"]) + "\n\n"
				
				preset_text += "}\n"		
				preset_text += "#endif\n"
				
				try:	
					preset_text_path = os.path.join("../../../src/presets/include", preset["name"] + ".h")
					with open(preset_text_path, "w") as preset_text_file:
						preset_text_file.write(preset_text)
						preset_text_file.close()
				except:
					print("Failed to create file")

				presets_header_text += "#include \"" + preset["name"] + ".h\"\n"

		presets_header =  "#ifndef PRESETS_H\n"
		presets_header += "#define PRESETS_H\n\n"
		
		presets_header += presets_header_text + "\n"
		

		presets_header += "namespace hpwh_presets {\n\n"
		
		presets_header += "std::unordered_map<int, constexpr char *> presets_map\n"

		presets_header += "}\n\n"

		presets_header += "#endif\n"				
		try:	
			with open("../../../src/presets/presets.h", "w") as presets_header_file:
				presets_header_file.write(presets_header)
				presets_header_file.close()
		except:
			print("Failed to create presets.h")

	os.chdir(orig_dir)
  
# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1

	if n_args > 1:
		build_dir = sys.argv[1]
		presets_list_files = []
		for i in range(1, n_args):
			presets_list_files.append(sys.argv[i + 1])

		incorp_presets(presets_list_files, build_dir)

