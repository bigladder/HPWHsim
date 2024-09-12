import http.server
import socketserver
import urllib.parse as urlparse
import main
from json import dumps
# in scripts/python
# poetry run python server.py
# open "index.html" in browser

# quit the server with ctrl-c

PORT = 8000


def do_test_and_plot(model_spec, model_name, test_name, measurements_name):
    return main.call_test_and_plot(model_spec, model_name, test_name, measurements_name)

def do_measure(model_spec, model_name):
    main.call_measure(model_spec, model_name)
    return 'do_measure done'

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/test_and_plot'):
            query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
            model_spec = query_components.get('arg1', [None])[0]
            model_name = query_components.get('arg2', [None])[0]
            test_name = query_components.get('arg3', [None])[0]
            measurements_name = query_components.get('arg4', [None])[0]

            response = {"result": do_test_and_plot(model_spec, model_name, test_name, measurements_name)}
            
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
            model_spec = query_components.get('arg1', [None])[0]
            model_name = query_components.get('arg2', [None])[0]

            do_measure(model_spec, model_name)
            
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