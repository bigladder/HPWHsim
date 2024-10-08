import os
import sys
import subprocess
from pathlib import Path
from simulate import simulate
from plot import plot
from measure import measure


def call_test(model_spec, model_name, test_name, build_dir):
    #
    simulate(model_spec, model_name, test_name, build_dir)
    
    orig_dir = str(Path.cwd())
    os.chdir(build_dir)
    abs_build_dir = str(Path.cwd())
    os.chdir(orig_dir)
    
    os.chdir("../../test")
    abs_test_dir = str(Path.cwd())
    os.chdir(orig_dir)
    
    output_dir = os.path.join(abs_build_dir, "test", "output") 
    measured_path = os.path.join(abs_test_dir, test_name, "measured.csv")   
    simulated_path = os.path.join(output_dir, test_name + "_" + model_spec + "_" + model_name + ".csv")
    plot_path =  os.path.join(output_dir, "plot.html")         
    return plot(measured_path, simulated_path, plot_path)


def call_measure(model_spec, model_name, build_dir):
    #
    measure(model_spec, model_name, build_dir)
    return 'success'