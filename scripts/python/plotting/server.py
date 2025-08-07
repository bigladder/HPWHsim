# Launches a simple tcp server for dispatching commands initiated from a web page.
# Two command strings are supported currently: 'test' and 'measure'.
# Launch the server with "uv run server.py".
# Exit the server with "ctrl-c".
# Open the local file "index.html" in a browser to access the server functions.

import http.server
import socketserver
import urllib.parse as urlparse
from simulate import simulate
from measure import measure
from test_proc import test_proc
from perf_proc import perf_proc
from fit_proc import fit_proc
from ws import launch_ws
import json
from json import dumps
import os, shutil

PORT = 8000
		
class MyHandler(http.server.SimpleHTTPRequestHandler):
	def log_message(self, format, *args):
		pass
	
	def do_GET(self):
			
			if self.path.startswith('/file'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				filename = query_components.get('filename', [None])[0]
				cmd = query_components.get('cmd', [None])[0]
				if cmd == 'read':
					with open(filename, "r") as json_file:
						json_data = json.load(json_file)
						json_file.close()
						self.send_response(200)
						self.send_header("Content-type", "application/json")
						self.send_header("Content-Length", str(len(dumps(json_data))))
						self.send_header("Access-Control-Allow-Origin", "*")
						self.end_headers()
						self.wfile.write(dumps(json_data).encode('utf-8'))
				elif cmd == 'write':
					json_str = query_components.get('json_data', [None])[0]
					json_data = json.loads(json_str)
					with open(filename, "w") as json_file:
						json.dump(json_data, json_file, indent=2)
						json_file.close()
						self.send_response(200)
						self.send_header("Content-type", "text/html")
						self.send_header("Access-Control-Allow-Origin", "*")
						self.end_headers()
				elif cmd == 'delete':
					if os.path.exists(filename):
						os.remove(filename)
					self.send_response(200)
					self.send_header("Content-type", "text/html")
					self.send_header("Access-Control-Allow-Origin", "*")
					self.end_headers()
				elif cmd == 'copy':
					if os.path.exists(filename):
						new_filename = query_components.get('new_filename', [None])[0]
						shutil.copy(filename, new_filename)
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
					
			elif self.path.startswith('/perf_proc'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				data_str = query_components.get('data', [None])[0]	
				data = json.loads(data_str)
				if data['cmd'] == 'start':
					response = perf_proc.start(data)
				elif data['cmd'] == 'stop':
					response = perf_proc.stop()
				self.send_response(200)
				self.send_header("Content-type", "application/json")
				self.send_header("Content-Length", str(len(dumps(response))))
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				self.wfile.write(dumps(response).encode('utf-8'))
				return
										
			elif self.path.startswith('/test_proc'):
				query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
				data_str = query_components.get('data', [None])[0]	
				data = json.loads(data_str)
				if data['cmd'] == 'start':
					response = test_proc.start(data)
				elif data['cmd'] == 'stop':
					response = test_proc.stop()
				self.send_response(200)
				self.send_header("Content-type", "application/json")
				self.send_header("Content-Length", str(len(dumps(response))))
				#self.send_header("Content-type", "text/html")
				self.send_header("Access-Control-Allow-Origin", "*")
				self.end_headers()
				self.wfile.write(dumps(response).encode('utf-8'))
				return
			
			elif self.path.startswith('/fit_proc'):
					query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
					data_str = query_components.get('data', [None])[0]	
					print(data_str)
					data = json.loads(data_str)
					if data['cmd'] == 'start':
						response = fit_proc.start(data)
					elif data['cmd'] == 'stop':
						response = fit_proc.stop()
					self.send_response(200)
					self.send_header("Content-type", "application/json")
					self.send_header("Content-Length", str(len(dumps(response))))
					#self.send_header("Content-type", "text/html")
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