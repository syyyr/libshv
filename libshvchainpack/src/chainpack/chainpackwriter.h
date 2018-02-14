#pragma once

#include "abstractstreamwriter.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	class BeginArray
	{
		RpcValue::Type m_type;
		size_t m_N;
	public:
		BeginArray(RpcValue::Type type, size_t N) : m_type(type), m_N(N) {}

		RpcValue::Type type() const {return m_type;}
		size_t N() const {return m_N;}
	};

public:
	ChainPackWriter(std::ostream &out) : m_out(out) {}

	ChainPackWriter& operator <<(const RpcValue &value) {write(value); return *this;}
	ChainPackWriter& operator <<(const RpcValue::MetaData &md) {writeMetaData(md); return *this;}

	ChainPackWriter& operator <<(Begin manip);
	ChainPackWriter& operator <<(End manip);
	ChainPackWriter& operator <<(const ListElement &el);
	ChainPackWriter& operator <<(const MapElement &el);
	ChainPackWriter& operator <<(const IMapElement &el);

	ChainPackWriter& operator <<(std::nullptr_t);
	ChainPackWriter& operator <<(bool value);
	ChainPackWriter& operator <<(RpcValue::Int value);
	ChainPackWriter& operator <<(RpcValue::UInt value);
	ChainPackWriter& operator <<(double value);
	ChainPackWriter& operator <<(RpcValue::Decimal value);
	ChainPackWriter& operator <<(RpcValue::DateTime value);
	ChainPackWriter& operator <<(const std::string &value);
	ChainPackWriter& operator <<(const RpcValue::Blob &value);
	ChainPackWriter& operator <<(const RpcValue::List &values);
	ChainPackWriter& operator <<(const RpcValue::Array &values);
	ChainPackWriter& operator <<(const RpcValue::Map &values);
	ChainPackWriter& operator <<(const RpcValue::IMap &values);

	ChainPackWriter& writeUIntData(uint64_t n);
private:
	void writeMetaData(const RpcValue::MetaData &meta_data);
	int write(const RpcValue &val);

	bool writeTypeInfo(const RpcValue &pack);
	void writeData(const RpcValue &val);

	void writeData_Array(const RpcValue::Array &array);
	void writeData_List(const RpcValue::List &list);
	void writeData_Map(const RpcValue::Map &map);
	void writeData_IMap(const RpcValue::IMap &map);

	void writeContainerBegin(const RpcValue::Type &container_type);
	void writeContainerEnd();
	void writeListElement(const RpcValue &val);
	void writeMapElement(const std::string &key, const RpcValue &val);
	void writeMapElement(const RpcValue::UInt &key, const RpcValue &val);
	void writeArrayBegin(const RpcValue::Type &array_type, size_t array_size);
	void writeArrayElement(const RpcValue &val) {	writeData(val); }

private:
	static constexpr bool WRITE_INVALID_AS_NULL = true;
private:
	std::ostream &m_out;
};

} // namespace chainpack
} // namespace shv
