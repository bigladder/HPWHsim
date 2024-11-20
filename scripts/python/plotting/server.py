# Launches a simple tcp server for dispatching commands initiated from a web page.
# Two command strings are supported currently: 'test' and 'measure'.
# Launch the server with "poetry run python server.py".
# Exit the server with "ctrl-c".
# Open the local file "index.html" in a browser to access the server functions.

import http.server
import socketserver
import urllib.parse as urlparse
import main
from json import dumps

PORT = 8000

def do_measure(model_spec, model_name, build_dir):
    main.call_measure(model_spec, model_name, build_dir)
    return 'do_measure done'

class MyHandler(http.server.SimpleHTTPRequestHandler):
	def do_GET(self):
			if self.path.startswith('/test'):
					query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)

					model_spec = query_components.get('model_spec', [None])[0]
					model_name = query_components.get('model_name', [None])[0]
					test_dir = query_components.get('test_dir', [None])[0]
					build_dir = query_components.get('build_dir', [None])[0]

					response = main.call_test(model_spec, model_name, test_dir, build_dir)

					print('test done')	
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
				build_dir= query_components.get('build_dir', [None])[0]

				do_measure(model_spec, model_name, build_dir)

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