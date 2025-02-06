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
	
async def handler(client):
	while True:
		try:
			msg = await client.recv()
			data = json.loads(msg)
			summary = {}
			if "source" in data:
				
				if data['source'] == "perf-proc":
					handler.perf_proc_client = client			
				elif data['source'] == "test-proc":
						handler.test_proc_client = client
				summary['source'] = data['source']
				
			if "dest" in data:
				if data['dest'] == "perf-proc":
					if handler.perf_proc_client != -1:
							await handler.perf_proc_client.send(msg)
				elif data['dest'] == "test-proc":
					if handler.test_proc_client != -1:
							await handler.test_proc_client.send(msg)
				summary['dest'] = data['dest']
				
				#print(summary)
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
		handler.perf_proc_client = -1
		handler.test_proc_client = -1
		time.sleep(1)

	launch_ws.proc = mp.Process(target=run_main, args=(), name='ws')
	print("launching websocket...")
	time.sleep(1)

	launch_ws.proc.start()
	time.sleep(2)

launch_ws.proc = -1