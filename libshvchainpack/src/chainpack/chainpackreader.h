#pragma once

#include "abstractstreamreader.h"
#include "chainpacktokenizer.h"

//#include "rpcvalue.h"
//#include "chainpack.h"
//#include "abstractstreamreader.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackReader : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	ChainPackReader(std::istream &in) : Super(), m_tokenizer(in) {}

	ChainPackReader& operator >>(RpcValue &value);
	ChainPackReader& operator >>(RpcValue::MetaData &meta_data);

	//static uint64_t readUIntData(std::istream &in, bool *ok = nullptr);
private:
	//uint64_t readUIntData(bool *ok = nullptr);
	/*
	ChainPack::TypeInfo::Enum readTypeInfo(RpcValue &meta, int &tiny_uint);
	RpcValue readData(ChainPack::TypeInfo::Enum tid, bool is_array);

	void readValue(RpcValue &val);
	void readMetaData(RpcValue::MetaData &meta_data);

	RpcValue::List readData_List();
	RpcValue::Array readData_Array(ChainPack::TypeInfo::Enum type_info);
	RpcValue::Map readData_Map();
	RpcValue::IMap readData_IMap();
	*/
private:
	ChainPackTokenizer m_tokenizer;
};


} // namespace chainpack
} // namespace shv
