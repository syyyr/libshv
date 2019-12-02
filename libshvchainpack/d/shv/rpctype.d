module shv.rpctype;

enum Tag {
	Invalid = -1,
	MetaTypeId = 1,
	MetaTypeNameSpaceId,
	USER = 8
}

enum NameSpaceID
{
	Global = 0,
	Elesys,
	Eyas,
}

struct GlobalNS
{
	enum MetaTypeID
	{
		ChainPackRpcMessage = 1,
		RpcConnectionParams,
		TunnelCtl,
		AccessGrantLogin,
		ValueChange,
	}
}

