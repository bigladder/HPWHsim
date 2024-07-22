import http.server
import socketserver
import urllib.parse as urlparse
import main

# in scripts/python
# poetry run python server.py
# open "index.html" in browser

PORT = 8000

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/run_python_function'):
            print('here1')
            query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
            print( f"{query_components=}" )
            print('here2')
            model_spec = query_components.get('arg1', [None])[0]
            model_name = query_components.get('arg2', [None])[0]
            test_name = query_components.get('arg3', [None])[0]
            plot_name = query_components.get('arg4', [None])[0]
            measurements_name = query_components.get('arg5', [None])[0]
          
            print(model_spec)
            print(model_name)
            result = python_function(model_spec, model_name, test_name, plot_name, measurements_name) 

            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
          
            print(result)
            #self.wfile.write(result)
        else:
            super().do_GET()

def python_function(model_spec, model_name, test_name, plot_name, measurements_name):
    return main.main(model_spec, model_name, test_name, plot_name, measurements_name)

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()
