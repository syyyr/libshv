#pragma once

#include "abstractstreamreader.h"
#include "chainpack.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackReader1 : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	ChainPackReader1(std::istream &in) : Super(in) {}

	using Super::read;
	void read(RpcValue::MetaData &meta_data) override;
	void read(RpcValue &val) override;

	static uint64_t readUIntData(std::istream &data, bool *ok = nullptr);
private:
	RpcValue readData(ChainPack::PackingSchema::Enum type_info, bool is_array);

	RpcValue::List readData_List();
	RpcValue::List readData_Array(ChainPack::PackingSchema::Enum type_info);
	RpcValue::Map readData_Map();
	RpcValue::IMap readData_IMap();
};

} // namespace chainpack
} // namespace shv
