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
# Launch the server with "poetry run python ws.py".
	
clients =[]
	
async def handler(websocket):
	while True:
		try:
			msg = await websocket.recv()
			print(msg)
			if not websocket in clients:
				clients.append(websocket)

			print("clients:")
			for client in clients:
				print(client)
				await client.send(msg)

			await websocket.recv()
		except ConnectionClosedOK:
			break

		
async def main():
	async with websockets.serve(handler, "localhost", 8600):
		await asyncio.Future()	
        
asyncio.run(main())