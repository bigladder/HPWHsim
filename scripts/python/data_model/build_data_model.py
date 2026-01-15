# uv run build_data_model.py
# generates source code based on hpwh_data_model schema.
# schema repo is cloned in build directory

import os
import shutil
import stat
import subprocess
import sys
from pathlib import Path

from lattice import Lattice


def generate(repo_dir, build_dir):
    data_model_dir = Path(build_dir) / "hpwh_data_model"
    working_dir = Path(__file__).parent
    gen_out_dir = Path(repo_dir) / "vendor" / "hpwh_data_model"

    orig_dir = Path.cwd()

    # remove hpwh-data-model repo dir, then recreate
    if data_model_dir.exists():
        shutil.rmtree(data_model_dir, onerror=onerror)

    try:
        data_model_dir.mkdir()
    except FileExistsError:
        print(f"Directory '{data_model_dir} already exists.")
    except FileNotFoundError:
        print(f"Cannot create repo directory {data_model_dir}")
        return

    # clone hpwh-data-model repo
    cmd = ["git", "clone", "https://github.com/bigladder/hpwh-data-model.git", data_model_dir]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True, check=False)
    print(result.stdout)

    # change to repo dir and checkout branch
    os.chdir(data_model_dir)
    cmd = ["git", "fetch"]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True, check=False)
    print(result.stdout)

    cmd = ["git", "checkout", "use-lattice-forge-generation"]
    result = subprocess.run(cmd, stdout=subprocess.PIPE, text=True, check=False)
    print(result.stdout)

    # create generated-code dir
    os.chdir(orig_dir)
    try:
        gen_out_dir.mkdir()
    except FileExistsError:
        print(f"Directory '{gen_out_dir}' already exists.")
    except FileNotFoundError:
        print(f"Cannot create code-generation directory {gen_out_dir}")
        return

    # generate code
    try:
        lat = Lattice(data_model_dir, working_dir, gen_out_dir, False)
        lat.generate_cpp_project(
            False, False, "Big Ladder Software", "info@bigladdersoftware.com", "2026", "BSD-3-Clause"
        )
    except Exception as e:
        print("Code generation failed")
        print(e)


# Source - https://stackoverflow.com/questions/2656322/shutil-rmtree-fails-on-windows-with-access-is-denied
# Posted by Justin Peel, modified by community. See post 'Timeline' for change history
# Retrieved 2026-01-15, License - CC BY-SA 4.0


def onerror(func, path, exc_info):
    """
    Error handler for ``shutil.rmtree``.

    If the error is due to an access error (read only file)
    it attempts to add write permission and then retries.

    If the error is for another reason it re-raises the error.

    Usage : ``shutil.rmtree(path, onerror=onerror)``
    """

    # Is the error an access error?
    if not os.access(path, os.W_OK):
        os.chmod(path, stat.S_IWUSR)
        func(path)
    else:
        raise PermissionError(f"{path} cannot be deleted/cleaned.")


# main
if __name__ == "__main__":
    repo_dir = "../../../"
    build_dir = Path(repo_dir) / "build"
    n_args = len(sys.argv) - 1
    if n_args == 2:  # noqa: PLR2004
        repo_dir = sys.argv[1]
        build_dir = sys.argv[2]
    generate(repo_dir, build_dir)
