#pragma once

#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackWriter : public shv::chainpack::AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	ChainPackWriter(std::ostream &out) : Super(out) {}

	void write(const RpcValue &val) override;
	void write(const RpcValue::MetaData &meta_data) override;

	void writeUIntData(uint64_t n);

	void writeContainerBegin(RpcValue::Type container_type) override;
	void writeContainerEnd(RpcValue::Type container_type) override;
	void writeListElement(const RpcValue &val) override;
	void writeMapElement(const std::string &key, const RpcValue &val) override;
	void writeMapElement(RpcValue::UInt key, const RpcValue &val) override;
	void writeArrayBegin(RpcValue::Type array_type, size_t array_size) override;
	void writeArrayElement(const RpcValue &val) override;
private:
	bool writeTypeInfo(const RpcValue &pack);
	void writeData(const RpcValue &val);

	void writeData_Map(const RpcValue::Map &map);
	void writeData_IMap(const RpcValue::IMap &map);
	void writeData_List(const RpcValue::List &list);
	void writeData_Array(const RpcValue::Array &array);
};

} // namespace chainpack
} // namespace shv
