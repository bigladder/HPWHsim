import http.server
import socketserver
import urllib.parse as urlparse
import main

# in scripts/python
# poetry run python server.py
# open "index.html" in browser

# quit the server with ctrl-c

PORT = 8000

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        if self.path.startswith('/test_and_plot'):
            query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
            model_spec = query_components.get('arg1', [None])[0]
            model_name = query_components.get('arg2', [None])[0]
            test_name = query_components.get('arg3', [None])[0]
            plot_name = query_components.get('arg4', [None])[0]
            measurements_name = query_components.get('arg5', [None])[0]

            do_test_and_plot(model_spec, model_name, test_name, plot_name, measurements_name)

            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()

        elif self.path.startswith('/measure'):
            query_components = urlparse.parse_qs(urlparse.urlparse(self.path).query)
            model_spec = query_components.get('arg1', [None])[0]
            model_name = query_components.get('arg2', [None])[0]

            do_measure(model_spec, model_name)
            
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.send_header("Access-Control-Allow-Origin", "*")
            self.end_headers()
 
        else:
            super().do_GET()

def do_test_and_plot(model_spec, model_name, test_name, plot_name, measurements_name):
    main.call_test_and_plot(model_spec, model_name, test_name, plot_name, measurements_name)
    return 'done'

def do_measure(model_spec, model_name):
    main.call_measure(model_spec, model_name)
    return 'done'

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()
