# 'poetry update lattice', if needed
# 'poetry run python build_data_model.py'

# Generates hpwh-data-model source code based on schema in 'data_model'dir'
# into 'src_out_dir'.

from lattice import Lattice
from pathlib import Path
import os
import sys

def generate(repo_dir, data_model_dir, gen_out_dir):

	if data_model_dir == '':
		data_model_dir = os.path.join(repo_dir, "vendor", "hpwh_data_model")
            
	build_dir = "."
	if gen_out_dir == '':    
		gen_out_dir = os.path.join(repo_dir, "src", "hpwh_data_model")
		
	lat = Lattice(data_model_dir, build_dir, gen_out_dir, False)
	lat.generate_cpp_project([])

# main
if __name__ == "__main__":

		repo_dir = "../../../"
		data_model_dir = ""
		gen_out_dir = ""
		n_args = len(sys.argv) - 1
		if n_args == 3:
			repo_dir = sys.argv[1]
			data_model_dir = sys.argv[2]
			gen_out_dir = sys.argv[3]

		generate(repo_dir, data_model_dir, gen_out_dir)

