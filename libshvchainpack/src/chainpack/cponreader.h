#pragma once

#include "abstractstreamreader.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponReaderOptions
{
	/*
	bool m_translateIds = false;
	bool m_indent = 0;
public:
	//CponReaderOptions() {}

	bool setTranslateIds() const {return m_translateIds;}
	CponReaderOptions& setTranslateIds(bool b) {m_translateIds = b; return *this;}

	bool isIndent() const {return m_indent;}
	CponReaderOptions& setIndent(bool i) {m_indent = i; return *this;}
	*/
};

class SHVCHAINPACK_DECL_EXPORT CponReader : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	CponReader(std::istream &in) : Super(in) {}

	CponReader& operator >>(RpcValue &value);
	CponReader& operator >>(RpcValue::MetaData &meta_data);

	using Super::read;
	void read(RpcValue::MetaData &meta_data);
	void read(RpcValue &val);
	void read(RpcValue &val, std::string &err);
private:
	int getChar();
	//RpcValue parseAtPos();

	uint64_t parseInteger(int &cnt);
	RpcValue::IMap parseIMapContent(char closing_bracket);
	RpcValue::MetaData parseMetaDataContent(char closing_bracket);
	void parseStringHelper(std::string &val);
	void parseCStringHelper(std::string &val);

	void parseNull(RpcValue &val);
	void parseBool(RpcValue &val);
	void parseString(RpcValue &val);
	//void parseBlob(RpcValue &val, bool hex_blob);
	void parseNumber(RpcValue &val);
	void parseList(RpcValue &val);
	void parseArray(RpcValue &ret_val);
	void parseMap(RpcValue &val);
	void parseIMap(RpcValue &val);
	void parseDateTime(RpcValue &val);

	char getValidChar();
	std::string getString(size_t n);

	void decodeUtf8(long pt, std::string &out);

private:
	int m_depth = 0;
};

} // namespace chainpack
} // namespace shv

