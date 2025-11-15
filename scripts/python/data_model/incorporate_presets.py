# uv run incorporate_presets.py JSON ../../../build ../../../test/models_json/models.json

# calls `hpwh convert' for each model in models_list_file json,
# then creates a C++ header file containing that data.

import json
import os
import sys
from pathlib import Path

import cbor2
from jinja2 import Environment, FileSystemLoader, select_autoescape


def incorporate_presets(presets_list_file: Path, build_dir: Path):
    """Using a json-formatted list of numbered presets, convert the preset's model representation
    into binary-encoded text for compilation into the HPWHsim library.

    Args:
        presets_list_file (Path): JSON-formatted file containing list of presets (names, numbers)
        build_dir (Path): Output directory for generated files
    """

    template_dir = Path(__file__).parent.parent / "templates"

    env = Environment(
        loader=FileSystemLoader(template_dir),
        autoescape=select_autoescape(["html", "xml"]),
        trim_blocks=True,
        lstrip_blocks=True,
        comment_start_string="{##",
        comment_end_string="##}",
    )

    with open(presets_list_file, "r", encoding="utf-8") as json_file:
        models_dict = json.load(json_file)
        models_list = [{"name": name, "number": str(number)} for name, number in models_dict.items()]

        presets_dir = build_dir / "presets"
        presets_dir.mkdir(exist_ok=True)

        # create library header file
        presets_h = env.get_template("presets.h.j2")
        presets_header = presets_h.render(models=models_list)
        with open(os.path.join(presets_dir, "presets.h"), "w") as presets_header_file:
            presets_header_file.write(presets_header)

        # create library implementation file
        presets_cpp = env.get_template("presets.cpp.j2")
        presets_implementation = presets_cpp.render(models=models_list)
        with open(os.path.join(presets_dir, "presets.cpp"), "w") as presets_implementation_file:
            presets_implementation_file.write(presets_implementation)

        preset_model_h = env.get_template("preset_model.h.j2")

        for name in models_dict:
            test_json_dir = Path(presets_list_file).parent
            preset_json_path = (test_json_dir / name).with_suffix(".json")

            try:
                with open(preset_json_path, "r", encoding="utf-8") as f:
                    json_data = json.load(f)
                    nbytes, cbor_text = get_cbor_as_text(json_data)

                    presets_include_dir = presets_dir / "include"
                    presets_include_dir.mkdir(exist_ok=True)

                    preset_model_header = preset_model_h.render(
                        name=name, size=nbytes, cbor=cbor_text, guard_name=name.upper() + "_H"
                    )
                    preset_model_header_path = (presets_include_dir / name).with_suffix(".h")
                    with open(preset_model_header_path, "w") as preset_model_header_file:
                        preset_model_header_file.write(preset_model_header)
            except FileNotFoundError:
                raise


def get_cbor_as_text(json_data):
    cbor_data = cbor2.dumps(json_data)
    nbytes = len(cbor_data)
    cbor_text = ""
    for i, entry in enumerate(cbor_data):
        cbor_text += hex(entry) + ", "
        if (i % 40 == 0) and (i != 0):
            cbor_text += "\n"
    return nbytes, cbor_text


if __name__ == "__main__":
    if len(sys.argv) == 3:  # noqa: PLR2004
        build_dir = sys.argv[1]
        presets_list_files = sys.argv[2]

        incorporate_presets(Path(presets_list_files), Path(build_dir))
