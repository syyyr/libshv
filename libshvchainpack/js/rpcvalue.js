"use strict"

function RpcValue(value, meta)
{
	if(value)
		this.value = value;
	if(meta)
		this.meta = meta;
}

RpcValue.Type = Object.freeze({
	"Null": 1,
	"Bool": 2,
	"Int": 3,
	"UInt": 4,
	"Double": 5,
	"Decimal": 6,
	"String": 7,
	"DateTime": 8,
	"List": 9,
	"Map": 10,
	"IMap": 11,
	//"Meta": 11,
})

RpcValue.fromCpon = function(cpon)
{
	let unpack_context = null;
	if(typeof cpon === 'string') {
		unpack_context = new UnpackContext(Cpon.stringToUtf8(cpon));
	}
	if(unpack_context === null)
		throw new TypeError("Invalid input data type")
	let rc = new CponReader(unpack_context);
	return rc.read();
}

RpcValue.prototype.toInt = function()
{
	switch(this.type) {
		case RpcValue.Type.Int:
		case RpcValue.Type.UInt:
			return this.value;
		case RpcValue.Type.Decimal:
			return this.value.mantisa * (10 ** this.value.exponent);
		case RpcValue.Type.Double:
			return (this.value >> 0);
	}
	return 0;
}

RpcValue.prototype.toString = function()
{
	let ba = this.toCpon();
	return Cpon.utf8ToString(ba);
}

RpcValue.prototype.toCpon = function()
{
	let wr = new CponWriter();
	wr.write(this);
	return wr.ctx.bytes();
}

RpcValue.prototype.isValid = function()
{
	return this.value && this.type;
}
