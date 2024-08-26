import os
import sys
import subprocess
from pathlib import Path
from simulate import simulate
from plot import plot
from measure import measure


def call_test_and_plot(model_spec, model_name, test_name, plot_name, measurements_name):
    #
    print(Path.cwd())
    print("model spec is " + model_spec)
    orig_dir = str(Path.cwd())
    os.chdir('../..')
    repo_path = str(Path.cwd())
    print("path is " + repo_path)

    # measurements_name = 'measurements_test.csv'
    output_name = test_name + '_' + model_spec + '_' + model_name + ".csv"

    test_dir = os.path.join(repo_path, "test")
    output_dir = os.path.join(repo_path, "build", "test", "output")
    simulate(repo_path, model_spec, model_name, test_name, output_dir)

    #
    measured_path = os.path.join(test_dir, test_name, measurements_name)
    simulated_path = os.path.join(output_dir, output_name)
    plot_path = os.path.join(output_dir, plot_name)
    energy_path = os.path.join(output_dir, "energy.txt")

    plot(measured_path, simulated_path, plot_path, energy_path)
    
    os.chdir(orig_dir)

    return 'success'


def call_measure(model_spec, model_name):
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
    measure(repo_path, model_spec, model_name, results_file)

    os.chdir(orig_dir)

    return 'success'