
import json

def read_file(filename):
	try:
			with open(filename, 'r') as json_file:
				return json.load(json_file)
	except:
			print(f"failed to load {filename}")
			return
	
def write_file(filename, json_data):
	try:
			with open(filename, 'w') as json_file:
				json.dump(json_data, json_file, indent=2)			
	except:
			print(f"failed to write {filename}")
			return