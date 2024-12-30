# 'poetry update lattice', if needed
# 'poetry run python convert_models.py IHPWH_models.json ../../../build ../../../test/models_json'
# or
# 'poetry run python convert_models.py CWHS_models.json ../../../build ../../../test/models_json'
# Generates hpwh-data-model source code based on schema in 'data_model'dir'
# into 'src_out_dir'.

from pathlib import Path
import os
import sys
import json
import subprocess

def convert_models(models_list_file, build_dir, output_dir):
    
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

        convert_models(models_list_file, build_dir, output_dir)

