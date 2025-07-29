# uv run ws.py

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
		except ConnectionClosedOK:
			print("connection closed.")
			break
		
		#print(f"\nsent by client: {client}")
		#print(f"received by ws: {msg}")			
		new_client = True
		for c in handler.clients:
			if c:
				if c == client:
					new_client = False
		
		if new_client:
			print(f"adding client: {client}")
			handler.clients.append(client)
		
		new_clients = []
		for c in handler.clients:
			try:
				#print(f"send to client: {c}")
				await c.send(msg)
				new_clients.append(c)
				await asyncio.sleep(0.1)
			except:
				print(f"removing client: {c}")
				
		handler.clients = new_clients
	
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
	handler.clients = []
	launch_ws.proc = mp.Process(target=run_main, args=(), name='ws')
	time.sleep(2)

	launch_ws.proc.start()
	time.sleep(2)
	print("launched websocket")

launch_ws.proc = -1

if __name__ == "__main__":
    launch_ws()