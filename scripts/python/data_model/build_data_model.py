# /// script
# dependencies = [ "pyyaml", "lattice" ]
# [tool.uv.sources]
# lattice = { git = "https://github.com/bigladder/lattice", branch = "add-back-support-headers" }
# ///

"""
Use: e.g. 'uv run scripts/python/data_model/build_data_model.py . ./build' [from HPWHSim project dir]
This script generates source code based on hpwh_data_model schema. The schema repo is cloned
in the project's main build directory; only the source schema are strictly necessary.
"""

import logging
import logging.config
import os
import shutil
import stat
import subprocess
import sys
from pathlib import Path

import yaml
from lattice import Lattice


def generate(project_directory: Path, build_directory: Path) -> None:
    """Generate source code from data model schema.

    Args:
        project_directory (Path): Home of the project that will host the lattice-built source code
        build_directory (Path): Usually project_directory / "build"
    """
    data_model_dir = Path(build_directory) / "hpwh_data_model"
    output_directory = Path(project_directory) / "vendor" / "hpwh_data_model"

    # remove hpwh-data-model repo dir, then recreate
    if data_model_dir.exists():
        shutil.rmtree(data_model_dir, onerror=onerror)

    try:
        data_model_dir.mkdir(parents=True)
    except FileExistsError:
        print(f"Directory '{data_model_dir} already exists.")
    except FileNotFoundError:
        print(f"Cannot create repo directory {data_model_dir}")
        return

    # clone hpwh-data-model repo
    cmd = ["git", "clone", "https://github.com/bigladder/hpwh-data-model.git", data_model_dir]
    subprocess.run(cmd, stdout=subprocess.PIPE, text=True, check=False)

    # change to repo dir and checkout branch
    cmd = ["git", "fetch"]
    subprocess.run(cmd, stdout=subprocess.PIPE, cwd=data_model_dir, text=True, check=False)

    cmd = ["git", "checkout", "use-lattice-forge-generation"]
    subprocess.run(cmd, stdout=subprocess.PIPE, cwd=data_model_dir, text=True, check=False)

    # create generated-code dir
    try:
        output_directory.mkdir(parents=True)
    except FileExistsError:
        print(f"Directory '{output_directory}' exists. Files will be updated according to existing configuration.")
    except FileNotFoundError:
        print(f"Cannot create code-generation directory {output_directory}")
        return

    with open(Path(__file__).with_name("logger.yaml"), "r", encoding="utf-8") as stream:
        config = yaml.safe_load(stream)
        logging.config.dictConfig(config)

    # generate code
    try:
        working_dir = Path(__file__).parent
        lat = Lattice(data_model_dir, working_dir, output_directory, False)  # version from pyproject.toml
        lat.generate_cpp_project(
            False, False, False, "Big Ladder Software", "info@bigladdersoftware.com", "2026", "BSD-3-Clause"
        )
    except Exception as e:
        print("Code generation failed:", e)


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
    project_dir = Path(__file__).parent.parent.parent.parent
    build_dir = Path(project_dir) / "build"
    n_args = len(sys.argv) - 1
    if n_args == 2:  # noqa: PLR2004
        project_dir = Path(sys.argv[1]).resolve()
        build_dir = Path(sys.argv[2]).resolve()
    generate(project_dir, build_dir)
