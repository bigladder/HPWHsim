# uv run check_perfmaps.py ../../../test/models_json/models.json ../../../build/out

from pathlib import Path
import os
import sys
import json
import csv

def check_perfmaps(models_list_filename, output_dir):
    
	orig_dir = str(Path.cwd())

	with open(models_list_filename) as json_file:
		models_dict = json.load(json_file)
		json_file.close()

		models_dir = "../../../test/models_json"
		for name in models_dict:  	
			model_filename = os.path.join(models_dir, name + ".json")
			with open(model_filename) as model_json_file:
				model_data = json.load(model_json_file)
				model_json_file.close()
				
				if not "integrated_system" in model_data:
					continue

				if not "performance" in model_data["integrated_system"]:
					continue
			
				performance = model_data["integrated_system"]["performance"]				
				if not "heat_source_configurations" in performance:
						continue
		
				for heatsourceconfig in performance["heat_source_configurations"]:
					if not "heat_source_type" in heatsourceconfig:
						continue
					if not heatsourceconfig["heat_source_type"] == "CONDENSER":
						continue
					if not "heat_source" in heatsourceconfig:
						continue
					if not "performance" in heatsourceconfig["heat_source"]:
						continue
					if not "performance_map" in heatsourceconfig["heat_source"]["performance"]:
						continue
					if not "grid_variables" in heatsourceconfig["heat_source"]["performance"]["performance_map"]:
						continue
					if not "lookup_variables" in heatsourceconfig["heat_source"]["performance"]["performance_map"]:
						continue
					
					grid_variables = heatsourceconfig["heat_source"]["performance"]["performance_map"]["grid_variables"]
					lookup_variables = heatsourceconfig["heat_source"]["performance"]["performance_map"]["lookup_variables"]
						
					if not "evaporator_environment_dry_bulb_temperature" in grid_variables:
						continue
					if not "heat_source_temperature" in grid_variables:
						continue
					if not "heating_capacity" in lookup_variables:
						continue
					if not "input_power" in lookup_variables:
						continue				

					evaporator_environment_dry_bulb_temperatures = grid_variables["evaporator_environment_dry_bulb_temperature"]
					heat_source_temperatures = grid_variables["heat_source_temperature"]
					heating_capacities = lookup_variables["heating_capacity"]
					input_powers = lookup_variables["input_power"]
					
					n_evapT = len(evaporator_environment_dry_bulb_temperatures)
					n_heatsourceT = len(heat_source_temperatures)

					evapT0= evaporator_environment_dry_bulb_temperatures[0] - 273.15
					sourceT0 = heat_source_temperatures[0] - 273.15
					
					evapT1 = evaporator_environment_dry_bulb_temperatures[n_evapT - 1] - 273.15
					sourceT1 = heat_source_temperatures[n_heatsourceT - 1] - 273.15

					inP00 = input_powers[0]
					outP00 = heating_capacities[0]
					cop00 = outP00 / inP00
				
					inP01 = input_powers[n_heatsourceT - 1]
					outP01 = heating_capacities[n_heatsourceT - 1]
					cop01 = outP01 / inP01
					
					inP10 = input_powers[n_evapT * (n_heatsourceT - 1)]
					outP10 = heating_capacities[n_evapT * (n_heatsourceT - 1)]
					cop10 = outP10 / inP10
					
					inP11 = input_powers[n_evapT * n_heatsourceT - 1]
					outP11 = heating_capacities[n_evapT * n_heatsourceT - 1]
					cop11 = outP11 / inP11				
		
					if (cop00 < 1) or (cop10 < 1) or (cop01 < 1) or (cop11 < 1) or (inP00 < 0) or (inP10 < 0) or (inP01 < 0) or (inP11 < 0):

						if not os.path.exists(output_dir):
							os.mkdir(output_dir)
													
						out_file_name = os.path.join(output_dir, name + ".csv")
						with open(out_file_name , "w") as csv_file:

							csv_writer = csv.writer(csv_file)

							input_power_data = [
								['sourceT (C)', "input_power (W)", 	''],
								[sourceT1, 	inP01, 	inP11],
								[sourceT0, 	inP00, 	inP10],
								['', 	evapT0, 	evapT1],
								['', 'evapT (C)', 	'']
							]
							csv_writer.writerows(input_power_data)
							 
							csv_file.write("\n\n")
							heating_capacity_data = [
								['sourceT (C)', "heating_capacity (W)", 	''],
								[sourceT1, 	outP01, 	outP11],
								[sourceT0, 	outP00, 	outP10],
								['', 	evapT0, 	evapT1],
								['' ,'evapT (C)', 	'',]
							]
							csv_writer.writerows(heating_capacity_data)
							
							csv_file.write("\n\n")
							cop_data = [
								['sourceT (C)', "cop", 	''],
								[sourceT1, cop01, 	cop11],
								[sourceT0, 	cop00, 	cop10],
								['', 	evapT0, 	evapT1],
								['', 'evapT (C)', '']
							]
							csv_writer.writerows(cop_data)						
							csv_file.close()

	os.chdir(orig_dir)
  
# main
if __name__ == "__main__":
    n_args = len(sys.argv) - 1

    if n_args == 2:
        models_list_file = sys.argv[1]
        output_dir = sys.argv[2]

        check_perfmaps(models_list_file, output_dir)

