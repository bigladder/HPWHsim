import asyncio
import json
import random
from quart import websocket, Quart

app = Quart(__name__)

@app.websocket("/perf")
async def perf():
    while True:
        msg = await websocket.receive()
        print(msg)
        print(websocket)
        await asyncio.sleep(1)
        rsp = await websocket.send("x")
        print(rsp)
        await asyncio.sleep(1)

if __name__ == "__main__":
    app.run(port=8500)