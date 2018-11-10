#pragma once

#include "abstractstreamreader.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponReaderOptions
{
};

class SHVCHAINPACK_DECL_EXPORT CponReader : public AbstractStreamReader
{
	using Super = AbstractStreamReader;
public:
	CponReader(std::istream &in) : Super(in) {}

	CponReader& operator >>(RpcValue &value);
	CponReader& operator >>(RpcValue::MetaData &meta_data);

	using Super::read;
	void read(RpcValue::MetaData &meta_data) override;
	void read(RpcValue &val) override;
	void read(RpcValue &val, std::string &err);
	void read(RpcValue &val, std::string *err);

	//RpcValue::DateTime readDateTime();
private:
	void unpackNext();

	void parseList(RpcValue &val);
	void parseMetaData(RpcValue::MetaData &meta_data);
	void parseMap(RpcValue &val);
	void parseIMap(RpcValue &val);
private:
	//int m_depth = 0;
};

} // namespace chainpack
} // namespace shv

