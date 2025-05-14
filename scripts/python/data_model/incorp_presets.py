# 'poetry run python incorp_presets.py JSON ../../../build IHPWH_models.json CWHS_models.json'

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
			
	presets_src_dir = os.path.join(presets_dir, 'src')
	if not os.path.exists(presets_src_dir):
			os.mkdir(presets_src_dir)
			
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
				#print(cbor_data)
				#c2 = cbor2.loads(cbor_data)
				#json_data2 = json.dumps(c2)
				#print(json_data2)
				#sys.exit(0)
					
				guard_name = preset["name"].upper() + "_H"
				
				preset_text = "#ifndef " + guard_name + "\n"
				preset_text += "#define " + guard_name + "\n\n"


				preset_text += "#include <vector>\n\n"
				
				preset_text += "namespace hpwh_presets {\n\n"
				
				nbytes = len(cbor_data)
				#preset_text += "inline constexpr std::size_t name_" + preset["name"] + " = " + preset["name"] + ";\n"
				#preset_text += "inline constexpr std::size_t size_" + preset["name"] + " = " + str(nbytes) + ";\n"
				preset_text += "inline const std::vector<uint8_t> cbor_" + preset["name"] + "{\n"
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
					
				presets_source_text += "\t{ " + str(preset["number"]) + ", {\"" + preset["name"] + "\", " + str(nbytes) + ", &cbor_" + preset["name"] + "[0]}}"
				first = False

		# create library header
		presets_header =  """
#ifndef PRESETS_H
#define PRESETS_H

"""
		# add the includes	
		presets_header += presets_header_text
	
		presets_header += """		
	
namespace hpwh_presets {

struct Identifier
{
  std::string name;
  const std::size_t size;
  const std::uint8_t *cbor_data;
  Identifier(const char* name_in, const std::size_t size_in, const std::uint8_t *cbor_data_in):
      name(name_in), size(size_in), cbor_data(cbor_data_in){}
};

inline std::unordered_map<int, Identifier> index = {
"""
		presets_header += presets_source_text + "};\n}\n#endif\n"

		try:	
			with open(os.path.join(presets_dir, "presets.h"), "w") as presets_header_file:
				presets_header_file.write(presets_header)
				presets_header_file.close()
		except:
			print("Failed to create presets.h")

		# create library source
		presets_source =  "#include <iostream>\n"
		presets_source += "#include <unordered_map>\n"
		presets_source += "#include \"../presets.h\"\n\n"
		presets_source += "namespace hpwh_presets {\n"

		presets_source += "\n}\n"

		try:	
			with open(os.path.join(presets_src_dir, "presets.cpp"), "w") as presets_src_file:
				presets_src_file.write(presets_source)
				presets_src_file.close()
		except:
			print("Failed to create presets.cpp")

		
	cmake_text = """
cmake_minimum_required (VERSION 3.10)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
project(hpwh_presets)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

find_package(Git QUIET)

set(JSON_BuildTests OFF CACHE INTERNAL "")

add_subdirectory(src)
"""

	try:	
		with open(os.path.join(presets_dir, "CMakeLists.txt"), "w") as cmake_file:
			cmake_file.write(cmake_text)
			cmake_file.close()
	except:
		print("Failed to create CMakeLists.txt")
		
	cmake_src_text = """
file(GLOB lib_headers "${PROJECT_SOURCE_DIR}/*.h" "${PROJECT_SOURCE_DIR}/include/*.h")
file(GLOB lib_src "${PROJECT_SOURCE_DIR}/src/*.cpp")

set (lib_files "${lib_headers}"
             "${lib_src}")

add_library(hpwh_presets STATIC ${lib_files})

target_include_directories(hpwh_presets PUBLIC ${PROJECT_SOURCE_DIR} ${PROJECT_SOURCE_DIR}/include)

target_compile_features(hpwh_presets PRIVATE cxx_std_17)
include(GenerateExportHeader)
generate_export_header(${PROJECT_NAME})
"""

	try:	
		with open(os.path.join(presets_src_dir, "CMakeLists.txt"), "w") as cmake_file:
			cmake_file.write(cmake_src_text)
			cmake_file.close()
	except:
		print("Failed to create src/CMakeLists.txt")


	os.chdir(orig_dir)
  
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

