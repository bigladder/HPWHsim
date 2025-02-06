# Launches a simple tcp server for dispatching commands initiated from a web page.
# Two command strings are supported currently: 'test' and 'measure'.
# Launch the server with "poetry run python server.py".
# Exit the server with "ctrl-c".
# Open the local file "index.html" in a browser to access the server functions.

import http.server
import socketserver
import urllib.parse as urlparse
from simulate import simulate
from measure import measure
from test_proc import launch_test_proc
from perf_proc import launch_perf_proc
from ws import launch_ws
import json
from json import dumps
import websockets
import asyncio

PORT = 8000
		
launch_test_proc.proc = -1
class MyHandler(http.server.SimpleHTTPRequestHandler):
	def log_message(self, format, *args):
		pass
	
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
				json_data = json.loads(json_str)
				with open(filename, "w") as json_file:
					#json_file.write(json_data)
					json.dump(json_data, json_file, indent=2)
					json_file.close()

				self.send_response(200)
				self.send_header("Content-type", "text/html")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				return
			
			elif self.path.startswith('/simulate'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				data_str = query_components.get('data', [None])[0]
				data = json.loads(data_str)
				response = simulate(data)

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
				data_str = query_components.get('data', [None])[0]
				data = json.loads(data_str)
				measure(data)

				self.send_response(200)
				self.send_header("Content-type", "text/html")
				self.send_header("Content-Length", "")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				return
							
			elif self.path.startswith('/launch_test_proc'):
					query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
					data_str = query_components.get('data', [None])[0]	
					data = json.loads(data_str)
					response = launch_test_proc(data)

					self.send_response(200)
					self.send_header("Content-type", "application/json")
					self.send_header("Content-Length", str(len(dumps(response))))
					#self.send_header("Content-type", "text/html")
					self.send_header("Access-Control-Allow-Origin", "*")
					self.end_headers()

					self.wfile.write(dumps(response).encode('utf-8'))
					return

			elif self.path.startswith('/launch_perf_proc'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				data_str = query_components.get('data', [None])[0]	
				data = json.loads(data_str)
				response = launch_perf_proc(data)
				self.send_response(200)
				self.send_header("Content-type", "application/json")
				self.send_header("Content-Length", str(len(dumps(response))))
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				self.wfile.write(dumps(response).encode('utf-8'))
				return
			
			elif self.path.startswith('/launch_ws'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				launch_ws()
				
				self.send_response(200)
				self.send_header("Content-type", "text/html")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				return
			else:
				super().do_GET()
		
def run():
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()

if __name__ == "__main__":
    run()