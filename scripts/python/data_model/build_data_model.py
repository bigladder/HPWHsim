# 'poetry update lattice', if needed
# 'poetry run python build_data_model.py'

# Generates hpwh-data-model source code based on schema in 'data_model_dir'
# into 'src_out_dir'.

from lattice import Lattice
from pathlib import Path
import os
import subprocess
import shutil
import sys
import time

def generate(repo_dir):
	
	data_model_dir = os.path.join(repo_dir, "build", "hpwh_data_model")
	working_dir = "."
	gen_out_dir = os.path.join(repo_dir, "vendor", "hpwh_data_model")

	orig_dir = str(Path.cwd())
	
	# remove hpwh-data-model repo dir, then recreate
	if os.path.exists(data_model_dir):
		shutil.rmtree(data_model_dir)
	
	try:
		os.mkdir(data_model_dir)
	except FileExistsError:
		print(f"Directory '{data_model_dir} already exists.")
	except FileNotFoundError:
		print(f"Cannot create repo directory {data_model_dir}")
		return

	# clone hpwh-data-model repo
	cmd = ["git", "clone", "https://github.com/bigladder/hpwh-data-model.git", data_model_dir]
	result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
	print(result.stdout)

	# change to repo dir and checkout branch
	os.chdir(data_model_dir)
	cmd = ["git", "fetch"]
	result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
	print(result.stdout)
 
	cmd = ["git", "checkout", "add-product-info"]
	result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True)
	print(result.stdout)
  
	# create generated-code dir    
	os.chdir(orig_dir)
	try:
		os.mkdir(gen_out_dir)
	except FileExistsError:
		print(f"Directory '{gen_out_dir}' already exists.")
	except FileNotFoundError:
		print(f"Cannot create code-generation directory {gen_out_dir}")
		return
	
	# generate code
	try:		
		lat = Lattice(data_model_dir, working_dir, gen_out_dir, False)
		lat.generate_cpp_project()
	except:
		print(f"Code generation failed")
		

# main
if __name__ == "__main__":
		repo_dir = "../../../"
		data_model_dir = ""
		gen_out_dir = ""
		n_args = len(sys.argv) - 1
		if n_args == 31:
			repo_dir = sys.argv[1]

		generate(repo_dir)

