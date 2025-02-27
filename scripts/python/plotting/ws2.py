from quart import websocket, Quart

app = Quart(__name__)

@app.websocket("/ws")
async def ws():
    await websocket.accept()
    while True:
        msg = await websocket.receive()
        print(msg)
        await websocket.send(msg)

if __name__ == "__main__":
    app.run(port=5000)