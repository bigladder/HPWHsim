# From enclosing folder launch with "poetry run python wstest/dash_ws.py".

from dash_extensions.enrich import DashProxy, html, dcc, Input, Output
from dash_extensions import WebSocket

# Create example app.
app = DashProxy(prevent_initial_callbacks=True)
app.layout = html.Div([
    dcc.Input(id="input", autoComplete="off"), html.Div(id="message"),
    WebSocket(url="ws://localhost:8600", id="ws")
])

@app.callback(Output("ws", "send"), [Input("input", "value")])
def send(value):
    print("sent from dash app")
    return value

@app.callback(Output("message", "children"), [Input("ws", "message")])
def message(e):
    print("received by dash app")
    return f"Response from websocket: {e['data']}"

if __name__ == '__main__':
    app.run_server()