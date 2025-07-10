# uv run incorporate_presets.py JSON ../../../build ../../../test/models_json/models.json

# calls `hpwh convert' for each model in models_list_file json,
# then creates a C++ header file containing that data.

from pathlib import Path
import os
import sys
import json
import subprocess
import cbor2

from jinja2 import Environment, FileSystemLoader, select_autoescape

def incorporate_presets(presets_list_files, build_dir, spec_type):
    
	orig_dir = str(Path.cwd())
	os.chdir(build_dir)
	abs_build_dir = str(Path.cwd())
	os.chdir(orig_dir)

	presets_headers_text = ""
	presets_identifiers_text = ""
	presets_models_text = ""
	presets_vector_text = ""
	
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

	template_dir = '../templates'
	env = Environment(
	    loader=FileSystemLoader(template_dir),
	    autoescape=select_autoescape(["html", "xml"]),
	    trim_blocks=True,
	    lstrip_blocks=True,
	    comment_start_string="{##",
	    comment_end_string="##}",
    )
		
	preset_model_h = env.get_template("preset_model.h.j2")											 

	models_out = []
	first = True
	for preset_list_file in presets_list_files:
		with open(preset_list_file) as json_file:
			models_dict= json.load(json_file)
			json_file.close()
			
			for name in models_dict:  
				preset_json_path = os.path.join(test_json_dir, name + ".json")
			  
				json_data = {}
				try:
					with open(preset_json_path, 'r', encoding='utf-8') as f:
						json_data = json.load(f)
						f.close()
				except:
					continue
			
				cbor_data = cbor2.dumps(json_data)
					
				guard_name = name.upper() + "_H"
				
				nbytes = len(cbor_data)
				cbor_text = ""
				for i, entry  in enumerate(cbor_data):
					cbor_text += hex(entry) + ", "
					if (i % 40 == 0) and (i != 0): 
						cbor_text += "\n";
								
				try:
					preset_model_header = preset_model_h.render(name = name, size = nbytes, cbor = cbor_text, guard_name = guard_name)
					preset_model_header_path = os.path.join(presets_include_dir, name + ".h")
					with open(preset_model_header_path, "w") as preset_model_header_file:
						preset_model_header_file.write(preset_model_header )
						preset_model_header_file.close()
				except:
					print("Failed to create file")
			
				if not first:
					presets_vector_text += ",\n"
					presets_models_text += ",\n"
					
				models_out.append({'name': name, 'number': str(models_dict[name]), 'size': str(nbytes)})
				#presets_headers_text += "#include \"" + name + ".h\"\n"
				presets_models_text += "\t" + name + " = " + str(models_dict[name])
				presets_vector_text += "\t{ MODELS::" + name + ", \"" + name +"\", cbor_" + name + ".data(), sizeof(cbor_" + name + ")}"
				first = False

		# create library header
		presets_h = env.get_template("presets.h.j2")											 
		presets_header =  presets_h.render(models = models_out)

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

		incorporate_presets(presets_list_files, build_dir, spec_type)

