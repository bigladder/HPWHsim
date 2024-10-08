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
    
    test_dir = os.path.join("../../test/", test_name)
    output_dir = os.path.join(build_dir, "test", "output")
  
    measured_path = "measured.csv"
    os.path.join(test_dir, measured_path)
      
    simulated_path = test_name + "_" + model_spec + "_" + model_name + ".csv"
    os.path.join(output_dir, simulated_path)
                 
    return plot(measured_path, simulated_path, build_dir, "plot.html")


def call_measure(model_spec, model_name, build_dir):
    #
    measure(model_spec, model_name, build_dir)
    return 'success'