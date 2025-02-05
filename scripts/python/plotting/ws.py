# Launch the server with "poetry run python ws.py".

import socketserver
import urllib.parse as urlparse

import json
from json import dumps
import websockets
import asyncio
from websockets.exceptions import ConnectionClosedOK
import time
import multiprocessing as mp
	
async def handler(websocket):
	while True:
		try:
			msg = await websocket.recv()
			print(msg)
			data = json.loads(msg)
			if "source" in data:
				
				if data['source'] == "perf-proc":
					handler.perf_proc_client = websocket
			if not handler.perf_proc_client == -1:
				await handler.perf_proc_client.send(msg)
				
			if data['source'] == "test-proc":
					handler.test_proc_client = websocket
			if not handler.test_proc_client == -1:
				await handler.test_proc_client.send(msg)
				
			await websocket.recv()
		except ConnectionClosedOK:
			break
	
handler.perf_proc_client = -1
handler.test_proc_client = -1
		
async def main():
	async with websockets.serve(handler, "localhost", 8600):
		await asyncio.Future()	

def run_main():        
	asyncio.run(main())


# Runs a simulation and generates plot
def launch_ws():

	if launch_ws.proc != -1:
		print("killing current websocket...")
		launch_ws.proc.kill()
		handler.dash_perf_client = -1
		time.sleep(1)

	launch_ws.proc = mp.Process(target=run_main, args=(), name='ws')
	print("launching websocket...")
	time.sleep(1)

	launch_ws.proc.start()
	time.sleep(2)

launch_ws.proc = -1