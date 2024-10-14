# 'poetry update lattice', if needed
# 'poetry run python build_data_model.py'

# Generates hpwh-data-model source code based on schema in 'data_model'dir'
# into 'src_out_dir'.

from lattice import Lattice
from pathlib import Path
import os
import sys

def generate(data_model_dir, src_out_dir):
	orig_dir = str(Path.cwd())
	os.chdir("../../../")
	repo_dir = str(Path.cwd())
	os.chdir(orig_dir)

	if data_model_dir == '':
		data_model_dir = os.path.join(repo_dir, "vendor", "hpwh-data-model", "data_model")
            
	build_dir = "."
	if src_out_dir == '':    
		src_out_dir = os.path.join(repo_dir, "src", "data_model")

	lat = Lattice(data_model_dir, build_dir,  src_out_dir, False)
	lat.generate_cpp_project([])

# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 2:
        data_model_dir = sys.argv[1]
        src_out_dir = sys.argv[2]

        generate(data_model_dir, src_out_dir)

