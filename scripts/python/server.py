import http.server
import socketserver
import run_plot

# in scripts/python
# poetry run python server.py
# open "index.html" in browser

PORT = 8000

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        print(self.path)
        if self.path == '/run_function':
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
           
            result = my_python_function() 
            print(result)
            #self.wfile.write(result)
        else:
            super().do_GET()

def my_python_function():
    return run_plot.main_fnc()

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()
