import enum

class Tag(enum.IntFlag):
	Invalid = -1,
	MetaTypeId = 1,
	MetaTypeNameSpaceId = 2,
	USER = 8

class RpcMessage():
	ID = 1

	class Key(enum.IntFlag):
		Params = 1,
		Result = 2,
		Error = 3,
		ErrorCode = 4,
		ErrorMessage = 5,
		MAX = 6

	class Tag(enum.IntFlag):
		RequestId = Tag.USER,
		CallerId = Tag.USER + 1,
		Method = Tag.USER + 2,
		ShvPath = Tag.USER + 3,
		ProtocolVersion = Tag.USER + 4,
		MAX = Tag.USER + 5

	class RpcCallType(enum.IntFlag):
		Undefined = 0,
		Request = 1,
		Response = 2,
		Notify = 3
