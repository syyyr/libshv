"use strict"

function UnpackContext(uint8_array)
{
	this.data = uint8_array
	this.index = 0;
}

UnpackContext.prototype.getByte = function()
{
	if(this.index >= this.data.length)
		throw new RangeError("unexpected end of data")
	return this.data[this.index++]
}

UnpackContext.prototype.peekByte = function()
{
	if(this.index >= this.data.length)
		return -1
	return this.data[this.index]
}

UnpackContext.prototype.getBytes = function(str)
{
	for (var i = 0; i < str.length; i++) {
		if(str.charCodeAt(i) != this.getByte())
			throw new TypeError("'" + str + "'' expected");
	}
}

function PackContext()
{
	//this.buffer = new ArrayBuffer(PackContext.CHUNK_LEN)
	this.data = new Uint8Array(0)
	this.length = 0;
}

PackContext.CHUNK_LEN = 1024;

PackContext.transfer = function(source, length)
{
	if (!(source instanceof ArrayBuffer))
		throw new TypeError('Source must be an instance of ArrayBuffer');
	if (length <= source.byteLength)
		return source.slice(0, length);
	var source_view = new Uint8Array(source),
		dest_view = new Uint8Array(new ArrayBuffer(length));
	dest_view.set(source_view);
	return dest_view.buffer;
}

PackContext.prototype.putByte = function(b)
{
	if(this.length >= this.data.length) {
		let buffer = PackContext.transfer(this.data.buffer, this.data.length + PackContext.CHUNK_LEN)
		this.data = new Uint8Array(buffer);
	}
	this.data[this.length++] = b;
}

PackContext.prototype.putBytes = function(bytes)
{
	if(typeof bytes === "string") {
		for (let i = 0; i < bytes.length; i++) {
			let b = bytes.charCodeAt(i) % 256;
			this.putByte(b);
		}
	}
}

PackContext.prototype.bytes = function()
{
	return this.data.subarray(0, this.length)
}
