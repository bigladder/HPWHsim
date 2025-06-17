# uv run --no-project incorp_presets.py JSON ../../../build IHPWH_models.json CWHS_models.json

# calls `hpwh convert' for each model in models_list_file json,
# then creates a C++ header file containing that data.

from pathlib import Path
import os
import sys
import json
import subprocess
import cbor2

def incorp_presets(presets_list_files, build_dir, spec_type):
    
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	presets_header_text = ""
	presets_source_text = ""
	
	presets_dir = os.path.join(abs_build_dir, 'presets')
	if not os.path.exists(presets_dir):
			os.mkdir(presets_dir)
			
	presets_include_dir = os.path.join(presets_dir, 'include')
	if not os.path.exists(presets_include_dir):
			os.mkdir(presets_include_dir)	

	
	if spec_type == "Preset":			
		if not os.path.exists(output_dir):
			os.mkdir(output_dir)	
		app_cmd = os.path.join(abs_build_dir , 'src', 'hpwh', 'hpwh')
		output_dir = os.path.join(abs_build_dir , "test", "output")
	else:	
		test_json_dir = os.path.join("../../..", "test", "models_json")	
														 
	first = True
	for preset_list_file in presets_list_files:
		with open(preset_list_file) as json_file:
			json_list_data = json.load(json_file)
			json_file.close()
			
			for preset in json_list_data:  
				if spec_type == "Preset":
					convert_list = [app_cmd, 'convert', '-m', preset["name"], '-d', output_dir, '-f', preset["name"]]
					result = subprocess.run(convert_list, stdout=subprocess.PIPE, text=True)					
					preset_json_path = os.path.join(output_dir, preset["name"] + ".json")
				else:
					preset_json_path = os.path.join(test_json_dir, preset["name"] + ".json")
			  

				json_data = {}
				try:
					with open(preset_json_path, 'r', encoding='utf-8') as f:
						json_data = json.load(f)
						f.close()
				except:
					continue
			
				cbor_data = cbor2.dumps(json_data)
					
				guard_name = preset["name"].upper() + "_H"
				
				preset_text = "#ifndef " + guard_name + "\n"
				preset_text += "#define " + guard_name + "\n\n"


				preset_text += "#include <array>\n\n"
				
				preset_text += "namespace hpwh_presets {\n\n"
				
				nbytes = len(cbor_data)
				preset_text += "const std::array<uint8_t, " + str(nbytes) + "> cbor_" + preset["name"] + "{\n"
				for i, entry  in enumerate(cbor_data):
					preset_text += hex(entry) + ", "
					if (i % 40 == 0) and (i != 0): 
						preset_text += "\n";
				
				preset_text += " };\n\n"
				
				preset_text += "}\n"
				preset_text += "#endif\n"
				
				try:	
					preset_text_path = os.path.join(presets_include_dir, preset["name"] + ".h")
					with open(preset_text_path, "w") as preset_text_file:
						preset_text_file.write(preset_text)
						preset_text_file.close()
				except:
					print("Failed to create file")

				presets_header_text += "#include \"" + preset["name"] + ".h\"\n"
				
				if not first:
					presets_source_text += ",\n"
					
				presets_source_text += "\t{ " + str(preset["number"]) + ", \"" + preset["name"] + "\", " + str(nbytes) + ", &cbor_" + preset["name"] + "[0]}"
				first = False

		# create library header
		presets_header =  """
#ifndef PRESETS_H
#define PRESETS_H

#include <iostream>
#include <unordered_map>

"""
		# add the includes	
		presets_header += presets_header_text
	
		presets_header += """		
	
namespace hpwh_presets {

struct Identifier
{
\tint id;
\tstd::string name;
\tconst std::size_t size;
\tconst std::uint8_t *cbor_data;
\tIdentifier(const int id_in, const char* name_in, const std::size_t size_in, const std::uint8_t *cbor_data_in):
\t\tid(id_in), name(name_in), size(size_in), cbor_data(cbor_data_in){}
};

inline std::vector<Identifier> models({
"""	
		presets_header  += presets_source_text + "\n"
		presets_header  += "});\n}\n"
		presets_header  += "#endif\n"

		try:	
			with open(os.path.join(presets_dir, "presets.h"), "w") as presets_header_file:
				presets_header_file.write(presets_header)
				presets_header_file.close()
		except:
			print("Failed to create presets.h")

# main
if __name__ == "__main__":
	n_args = len(sys.argv) - 1

	if n_args > 1:
		spec_type = sys.argv[1]
		build_dir = sys.argv[2]

		presets_list_files = []
		for i in range(3, n_args + 1):
			presets_list_files.append(sys.argv[i])

		incorp_presets(presets_list_files, build_dir, spec_type)

