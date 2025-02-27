# From containing folder launch with "poetry run python wstest/ws.py".

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
			print(f"client: {client}")
			msg = await client.recv()
		
			print(f"received by ws: {msg}\n")
			await client.send(msg)
				
				#print(summary)
		except ConnectionClosedOK:
			print("connection closed.")
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
	time.sleep(1)

	launch_ws.proc.start()
	time.sleep(2)
	print("launched websocket")

launch_ws.proc = -1

if __name__ == "__main__":
    launch_ws()