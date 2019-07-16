from enum import Enum
import datetime

from .cpcontext import UnpackContext

class RpcValue:
	class Type(Enum):
		Undefined = 0
		Null = 1
		Bool = 2
		Int = 3
		UInt = 4
		Double = 5
		Decimal = 6
		String = 7
		DateTime = 8
		List = 9
		Map = 10
		IMap = 11
		# Meta = 12,

	class DateTime:
		def __init__(self, msec, utc_offset):
			self.epochMsec = msec
			self.utcOffsetMin = utc_offset

	class Decimal:
		def __init__(self, mantisa, exponent):
			self.mantisa = mantisa
			self.exponent = exponent

	def __init__(self, value=None, meta=None, value_type: Type = Type.Undefined):
		self.value = value
		self.meta = meta
		if value_type == RpcValue.Type.Undefined:
			if value is None:
				self.type = RpcValue.Type.Null
			elif isinstance(value, bool):
				self.type = RpcValue.Type.Bool
			elif isinstance(value, str):
				self.type = RpcValue.Type.String
				self.value = self.value.encode('utf8')
			elif isinstance(value, (bytes, bytearray)):
				self.type = RpcValue.Type.String
			elif isinstance(value, datetime.datetime):
				self.type = RpcValue.Type.DateTime
				self.value = RpcValue.DateTime(int(value.timestamp() * 1000), (-int(value.utcoffset()) if value.utcoffset else 0))
			elif isinstance(value, int):
				self.type = RpcValue.Type.Int
			elif isinstance(value, float):
				self.type = RpcValue.Type.Double
			elif isinstance(value, list):
				self.type = RpcValue.Type.List
				lst = []
				for v in value:
					lst.append(RpcValue(v))
				self.value = lst
			elif isinstance(value, dict):
				self.type = RpcValue.Type.List
				all_keys_int = False
				for k in value:
					if not isinstance(k, int):
						all_keys_int = False
						break
				if all_keys_int:
					self.type = RpcValue.Type.IMap
				else:
					new_val = {}
					for k, v in value:
						new_val[str(k)] = v
					self.value = new_val
					self.type = RpcValue.Type.Map
			else:
				raise TypeError("Unsupported init value " + repr(value))
		else:
			self.type = value_type

	def is_valid(self):
		return self.type != RpcValue.Type.Undefined

	# def from_cpon(cpon):
	# 	unpack_context = None
	# 	if isinstance(cpon, str):
	# 		unpack_context = UnpackContext(cpon.encode('utf8'))
	# 	elif isinstance(cpon, (bytes, bytearray)):
	# 		unpack_context = UnpackContext(cpon)
	#
	# 	if unpack_context is None:
	# 		raise TypeError("Invalid input data type: " + type(cpon))
	# 	rd = CponReader(unpack_context)
	# 	return rd.read()
	#
	# def from_chainpack(sels, data):
	# 	unpack_context = UnpackContext(data)
	# 	rd = ChainPackReader(unpack_context)
	# 	return rd.read()

	# def to_cpon(self):
	# 	wr = CponWriter()
	# 	wr.write(this)
	# 	return wr.data()
	#
	# def to_chainpack(self):
	# 	wr = ChainPackWriter()
	# 	wr.write(this)
	# 	return wr.data()

