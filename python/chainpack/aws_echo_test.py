import socket_client

client = socket_client.Client('35.167.83.38', 3755)

def round_trip(value):
	return client.callMethodSync("echo", value).result()

import test_rpcvalue
test_rpcvalue.round_trip = round_trip

test_rpcvalue.test_decode_inverts_encode()