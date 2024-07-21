import http.server
import socketserver
import main

# in scripts/python
# poetry run python server.py
# open "index.html" in browser

PORT = 8000

class MyHandler(http.server.SimpleHTTPRequestHandler):
    def do_GET(self):
        print(self.path)
        if self.path == '/run_python_function':
            self.send_response(200)
            self.send_header("Content-type", "text/html")
            self.end_headers()
           
            result = python_function() 
            print(result)
            #self.wfile.write(result)
        else:
            super().do_GET()

def python_function():
    return main.main()

if __name__ == "__main__":
    with socketserver.TCPServer(("", PORT), MyHandler) as httpd:
        print(f"Serving on port {PORT}")
        httpd.serve_forever()
