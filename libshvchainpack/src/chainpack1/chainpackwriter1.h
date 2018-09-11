#pragma once

#include "abstractstreamwriter1.h"

namespace shv {
namespace chainpack1 {

class SHVCHAINPACK_DECL_EXPORT ChainPackWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	ChainPackWriter(std::ostream &out) : Super(out) {}

	ChainPackWriter& operator <<(const RpcValue &value) {write(value); return *this;}
	ChainPackWriter& operator <<(const RpcValue::MetaData &meta_data) {write(meta_data); return *this;}

	size_t write(const RpcValue &val) override;
	size_t write(const RpcValue::MetaData &meta_data) override;

	void writeUIntData(uint64_t n);
	static void writeUIntData(std::ostream &os, uint64_t n);

	void writeIMapKey(RpcValue::UInt key) override {writeUIntData(key);}
	void writeContainerBegin(RpcValue::Type container_type) override;
	/// ChainPack doesn't need to know container type to close it
	void writeContainerEnd(RpcValue::Type container_type = RpcValue::Type::Invalid) override;
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
