# uv run convert_models.py ../../../test/models_json/models.json ../../../build ../../../test/models_json
#
# calls `hpwh convert' for each model in models_list_file json

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

    for model,id in json_data.items():  
      #convert_list = [app_cmd, 'convert', '-n', str(model["number"]), '-d', output_dir, '-f', model["name"]]
      convert_list = [app_cmd, 'convert', '-s', 'Legacy', '-n', str(id), '-d', output_dir, '-f', model]
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

