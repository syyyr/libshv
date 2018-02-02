#!/usr/bin/python3.6
# -*- coding: utf-8 -*-


import socket
import logging
from rpcdriver import RpcDriver
from rpcvalue import RpcValue
from rpcmessage import RpcMessage


logger=logging.getLogger()
from utils import print_to_string
def log(*vargs):
	logger.debug(print_to_string(*vargs))
def info(*vargs):
	logger.info(print_to_string(*vargs))


class Client(RpcDriver):
	def __init__(s, host = "127.0.0.1", port = 6016):
		super().__init__()
		s.socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
		print ("connecting..")
		s.socket.connect((host, port))
		print("connected!")

	def recv(s):
		s.done = False
		while True:
			s.bytesRead(s.socket.recv(240000))
			if s.done:
				return s.result

	def writeBytes(s, b):
		print ("writeBytes:", b)
		s.socket.send(b)

	def onMessageReceived(s, msg: RpcMessage):
		s.result = msg
		print("message received:", s.result._value.toPython())
		s.done = True

	def callMethodSync(s, method: str, msg: RpcValue):
		s.sendRequest(method, msg)
		return s.recv()


if __name__ == "__main__":
	client = Client()
	client.sendRequest("echo", RpcValue("hello"))
	client.recv()


