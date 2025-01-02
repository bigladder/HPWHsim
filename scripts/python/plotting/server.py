# Launches a simple tcp server for dispatching commands initiated from a web page.
# Two command strings are supported currently: 'test' and 'measure'.
# Launch the server with "poetry run python server.py".
# Exit the server with "ctrl-c".
# Open the local file "index.html" in a browser to access the server functions.

import http.server
import socketserver
import urllib.parse as urlparse
from simulate import simulate
from plot import plot
from measure import measure
import json
from json import dumps
from main import dash_plot

PORT = 8000

dash_plot.proc = -1
class MyHandler(http.server.SimpleHTTPRequestHandler):
	def do_GET(self):
			
			if self.path.startswith('/read_json'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				filename = query_components.get('filename', [None])[0]
				content = {}
				with open(filename, "r") as json_file:
					content = json.load(json_file)
					json_file.close()

				self.send_response(200)
				self.send_header("Content-type", "application/json")
				self.send_header("Content-Length", str(len(dumps(content))))
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				
				self.wfile.write(dumps(content).encode('utf-8'))
				return

			elif self.path.startswith('/write_json'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				filename = query_components.get('filename', [None])[0]
				json_str = query_components.get('json_data', [None])[0]
				print(json_str)
				json_data = json.loads(json_str)
				with open(filename, "w") as json_file:
					#json_file.write(json_data)
					json.dump(json_data, json_file)
					json_file.close()

				self.send_response(200)
				self.send_header("Content-type", "text/html")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				return
			
			elif self.path.startswith('/simulate'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)

				model_spec = query_components.get('model_spec', [None])[0]
				model_name = query_components.get('model_name', [None])[0]
				test_dir = query_components.get('test_dir', [None])[0]
				build_dir = query_components.get('build_dir', [None])[0]

				response = simulate(model_spec, model_name, test_dir, build_dir)

				self.send_response(200)
				self.send_header("Content-type", "application/json")
				self.send_header("Content-Length", str(len(dumps(response))))
				#self.send_header("Content-type", "text/html")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()

				self.wfile.write(dumps(response).encode('utf-8'))
				return

			elif self.path.startswith('/measure'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				model_spec = query_components.get('model_spec', [None])[0]
				model_name = query_components.get('model_name', [None])[0]
				build_dir = query_components.get('build_dir', [None])[0]
				draw_profile = query_components.get('draw_profile', [None])[0]

				measure(model_spec, model_name, build_dir, draw_profile)

				self.send_response(200)
				self.send_header("Content-type", "text/html")
				self.send_header("Content-Length", "")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				return

							
			elif self.path.startswith('/plot'):
					query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
					test_dir = query_components.get('test_dir', [None])[0]
					build_dir = query_components.get('build_dir', [None])[0]
					show_types  = int(query_components.get('show_types', [None])[0])
					simulated_filename  = query_components.get('simulated_filename', [None])[0]
					measured_filename= query_components.get('measured_filename', [None])[0]
					response = dash_plot(test_dir, build_dir, show_types, measured_filename, simulated_filename)

					self.send_response(200)
					self.send_header("Content-type", "application/json")
					self.send_header("Content-Length", str(len(dumps(response))))
					#self.send_header("Content-type", "text/html")
					self.send_header("Access-Control-Allow-Origin", "*")
					self.end_headers()

					self.wfile.write(dumps(response).encode('utf-8'))
					return


			else:
				super().do_GET()

def run():
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()

if __name__ == "__main__":
    run()