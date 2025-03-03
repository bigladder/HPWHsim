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
			#print(f"sent by client: {client}")
			new_client = True
			for c in handler.clients:
				if c == client:
					new_client = False
			
			if new_client:
				if client != -1:
					handler.clients.append(client)
				
			if client != -1:
				msg = await client.recv()	
				#print(f"received by ws: {msg}\n")
			
			for c in handler.clients:
				#print(f"send to client: {c}")
				await c.send(msg)
				
				#print(summary)
		except ConnectionClosedOK:
			print("connection closed.")
			break
	
handler.clients = []
		
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
		time.sleep(1)

	launch_ws.proc = mp.Process(target=run_main, args=(), name='ws')
	time.sleep(1)

	launch_ws.proc.start()
	time.sleep(2)
	print("launched websocket")

launch_ws.proc = -1

if __name__ == "__main__":
    launch_ws()