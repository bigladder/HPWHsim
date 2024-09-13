import os
import sys
import subprocess
from pathlib import Path
from simulate import simulate
from plot import plot
from measure import measure


def call_test_and_plot(model_spec, model_name, test_name, measured_filename, plot_filename):
    #
    print(Path.cwd())
    print("model spec is " + model_spec)
    orig_dir = str(Path.cwd())
    os.chdir('../..')
    repo_path = str(Path.cwd())
    print("path is " + repo_path)

    output_name = test_name + '_' + model_spec + '_' + model_name + ".csv"

    test_dir = os.path.join(repo_path, "test")
    output_dir = os.path.join(repo_path, "build", "test", "output")
    simulate(repo_path, model_spec, model_name, test_name, output_dir)

    #
    measured_path = os.path.join(test_dir, test_name, measured_filename)
    simulated_path = os.path.join(output_dir, output_name)
 
    os.chdir(orig_dir)
    
    result = plot(measured_path, simulated_path, plot_filename)
    return result


def call_measure(model_spec, model_name, draw_profile):
    #
    print(Path.cwd())
    print("model spec is " + model_spec)
    orig_dir = str(Path.cwd())
    os.chdir('../..')
    repo_path = str(Path.cwd())
    print("path is " + repo_path)

    test_dir = os.path.join(repo_path, "test")
    output_dir = os.path.join(repo_path, "build", "test", "output")
    
    results_file = os.path.join(output_dir, "results.txt")
    measure(repo_path, model_spec, model_name, results_file, draw_profile)

    os.chdir(orig_dir)

    return 'success'