"use strict"

function RpcMessage(rpcval)
{
	this.rpcval = rpcval;
	if(this.rpcval) {
		if(!this.rpcval.meta)
			this.rpcval.meta = {}
		if(!this.rpcval.value)
			this.rpcval.value = {}
			this.rpcval.type = RpcValue.Type.IMap
	}
}

RpcMessage.TagRequestId = "8";
RpcMessage.TagShvPath = "9";
RpcMessage.TagMethod = "10";

RpcMessage.KeyParams = "1";
RpcMessage.KeyResult = "2";
RpcMessage.KeyError = "3";

RpcMessage.prototype.isValid = function() {return this.rpcval? true: false; }
RpcMessage.prototype.isRequest = function() {return this.requestId() && this.method(); }
RpcMessage.prototype.isResponse = function() {return this.requestId() && !this.method(); }
RpcMessage.prototype.isSignal = function() {return !this.requestId() && this.method(); }

RpcMessage.prototype.requestId = function() {return this.isValid()? this.rpcval.meta[RpcMessage.TagRequestId]: 0; }
RpcMessage.prototype.setRequestId = function(id) {return this.rpcval.meta[RpcMessage.TagRequestId] = id; }

RpcMessage.prototype.shvPath = function() {return this.isValid()? this.rpcval.meta[RpcMessage.TagShvPath]: null; }
RpcMessage.prototype.setShvPath = function(val) {return this.rpcval.meta[RpcMessage.TagShvPath] = val; }

RpcMessage.prototype.method = function() {return this.isValid()? this.rpcval.value[RpcMessage.KeyMethod]: null; }
RpcMessage.prototype.setMethod = function(val) {return this.rpcval.value[RpcMessage.KeyMethod] = val; }

RpcMessage.prototype.params = function() {return this.isValid()? this.rpcval.value[RpcMessage.KeyParams]: null; }
RpcMessage.prototype.setParams = function(params) {return this.rpcval.value[RpcMessage.KeyParams] = params; }

RpcMessage.prototype.result = function() {return this.isValid()? this.rpcval.value[RpcMessage.KeyResult]: null; }
RpcMessage.prototype.setResult = function(result) {return this.rpcval.value[RpcMessage.KeyResult] = result; }

RpcMessage.prototype.error = function() {return this.isValid()? this.rpcval.value[RpcMessage.KeyError]: null; }
RpcMessage.prototype.setError = function(err) {return this.rpcval.value[RpcMessage.KeyError] = err; }
