#ifndef CPONWRITER_H
#define CPONWRITER_H

#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponWriterOptions
{
	bool m_translateIds = false;
	bool m_indent = 0;
public:
	//CponWriterOptions() {}

	bool setTranslateIds() const {return m_translateIds;}
	CponWriterOptions& setTranslateIds(bool b) {m_translateIds = b; return *this;}

	bool isIndent() const {return m_indent;}
	CponWriterOptions& setIndent(bool i) {m_indent = i; return *this;}
};

class SHVCHAINPACK_DECL_EXPORT CponWriter : public AbstractStreamWriter
{
	using Super = AbstractStreamWriter;
public:
	enum class Begin {Map, IMap, List, Meta, Array};
	enum class End {Map, IMap, List, Meta, Array};

	class ListElement : public RpcValue
	{
		friend class CponWriter;
		const RpcValue &val;
	public:
		ListElement(const RpcValue &v) : val(v) {}
	};
	class MapElement
	{
		friend class CponWriter;
		const std::string &key;
		const RpcValue &val;
	public:
		MapElement(const std::string &k, const RpcValue &v) : key(k), val(v) {}
	};
	class IMapElement
	{
		friend class CponWriter;
		const RpcValue::UInt key;
		const RpcValue &val;
	public:
		IMapElement(RpcValue::UInt k, const RpcValue &v) : key(k), val(v) {}
	};
public:
	CponWriter(std::ostream &out) : m_out(out) {}
	CponWriter(std::ostream &out, const CponWriterOptions &opts) : CponWriter(out) {m_opts = opts;}

	CponWriter& operator <<(const RpcValue &value);
	CponWriter& operator <<(const RpcValue::MetaData &value);

	CponWriter& operator <<(Begin manip);
	CponWriter& operator <<(End manip);
	CponWriter& operator <<(const ListElement &el);
	CponWriter& operator <<(const MapElement &el);
	CponWriter& operator <<(const IMapElement &el);

	CponWriter& operator <<(std::nullptr_t);
	CponWriter& operator <<(bool value);
	CponWriter& operator <<(RpcValue::Int value);
	CponWriter& operator <<(RpcValue::UInt value);
	CponWriter& operator <<(double value);
	CponWriter& operator <<(RpcValue::Decimal value);
	CponWriter& operator <<(RpcValue::DateTime value);
	CponWriter& operator <<(const std::string &value);
	CponWriter& operator <<(const RpcValue::Blob &value);
	CponWriter& operator <<(const RpcValue::List &values);
	CponWriter& operator <<(const RpcValue::Array &values);
	CponWriter& operator <<(const RpcValue::Map &values);
	CponWriter& operator <<(const RpcValue::IMap &values);
private:
	void writeIMap(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data = nullptr);
	void writeIMapContent(const RpcValue::IMap &values, const RpcValue::MetaData *meta_data = nullptr);
	void writeMapContent(const RpcValue::Map &values);
	void startBlock();
	void endBlock();
	void indentElement();
	void separateElement();
private:
	std::ostream &m_out;
	CponWriterOptions m_opts;
	int m_currentIndent = 0;
};

} // namespace chainpack
} // namespace shv

#endif // CPONWRITER_H
