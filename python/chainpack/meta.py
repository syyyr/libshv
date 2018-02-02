import enum

class Tag(enum.IntFlag):
	Invalid = -1,
	MetaTypeId = 1,
	MetaTypeNameSpaceId = 2,
	USER = 8

class RpcMessage():
	ID = 1

	class Key(enum.IntFlag):
		Method = 1,
		Params = 2,
		Result = 3,
		Error = 4,
		ErrorCode = 5,
		ErrorMessage = 6,
		MAX = 7

	class Tag(enum.IntFlag):
		RequestId = Tag.USER,
		RpcCallType = Tag.USER + 1,
		DeviceId = Tag.USER + 2,
		MAX = Tag.USER + 3

	class RpcCallType(enum.IntFlag):
		Undefined = 0,
		Request = 1,
		Response = 2,
		Notify = 3
