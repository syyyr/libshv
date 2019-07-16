"use strict"

function RpcMessage(rpc_val)
{
	if(typeof rpc_val === 'undefined')
		this.rpcValue = new RpcValue();
	else if(typeof rpc_val === 'null')
		this.rpcValue = null;
	else if(rpc_val && rpc_val.constructor.name === "RpcValue")
		this.rpcValue = rpc_val;
	else
		throw new TypeError("RpcMessage cannot be constructed with " + typeof rpc_val)

	if(this.rpcValue) {
		if(!this.rpcValue.meta)
			this.rpcValue.meta = {}
		if(!this.rpcValue.value)
			this.rpcValue.value = {}
		this.rpcValue.type = RpcValue.Type.IMap
	}
}

RpcMessage.TagRequestId = "8";
RpcMessage.TagShvPath = "9";
RpcMessage.TagMethod = "10";

RpcMessage.KeyParams = "1";
RpcMessage.KeyResult = "2";
RpcMessage.KeyError = "3";

	def isValid():
		return this.rpcValue? true: false;
	def isRequest():
		return this.requestId() && this.method();
	def isResponse():
		return this.requestId() && !this.method();
	def isSignal():
		return !this.requestId() && this.method();

	def requestId():
		return this.isValid()? this.rpcValue.meta[RpcMessage.TagRequestId]: 0;
	def setRequestId(id):
		return this.rpcValue.meta[RpcMessage.TagRequestId] = id;

	def shvPath():
		return this.isValid()? this.rpcValue.meta[RpcMessage.TagShvPath]: null;
	def setShvPath(val):
		return this.rpcValue.meta[RpcMessage.TagShvPath] = val;

	def method():
		return this.isValid()? this.rpcValue.meta[RpcMessage.TagMethod]: null;
	def setMethod(val):
		return this.rpcValue.meta[RpcMessage.TagMethod] = val;

	def params():
		return this.isValid()? this.rpcValue.value[RpcMessage.KeyParams]: null;
	def setParams(params):
		return this.rpcValue.value[RpcMessage.KeyParams] = params;

	def result():
		return this.isValid()? this.rpcValue.value[RpcMessage.KeyResult]: null;
	def setResult(result):
		return this.rpcValue.value[RpcMessage.KeyResult] = result;

	def error():
		return this.isValid()? this.rpcValue.value[RpcMessage.KeyError]: null;
	def setError(err):
		return this.rpcValue.value[RpcMessage.KeyError] = err;

	def toString():
		return this.isValid()? this.rpcValue.toString(): "";
	def toCpon():
		return this.isValid()? this.rpcValue.toCpon(): "";
	def toChainPack():
		return this.isValid()? this.rpcValue.toChainPack(): "";
