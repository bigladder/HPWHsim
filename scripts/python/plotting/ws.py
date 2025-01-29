import http.server
import socketserver
import urllib.parse as urlparse
from simulate import simulate
from measure import measure
from test_proc import launch_test_plot
from perf_proc import launch_perf_plot
import json
from json import dumps
import websockets
import asyncio
from websockets.exceptions import ConnectionClosedOK
		
async def handler(websocket):
	while True:
		try:
			msg = await websocket.recv()
			print(msg)
			await websocket.send("server msg")
		except ConnectionClosedOK:
			break

		
async def main():
	async with websockets.serve(handler, "127.0.0.1", 8765):
		await asyncio.Future()	
        
asyncio.run(main())