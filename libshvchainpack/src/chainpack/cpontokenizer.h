#pragma once

#include "abstractstreamtokenizer.h"
#include "rpcvalue.h"

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT CponTokenizer : public AbstractStreamTokenizer
{
	using Super = AbstractStreamTokenizer;
public:
	using Super::Super;
public:
	TokenType nextToken() override;
private:
	int getChar();
	char getValidChar();
	std::string getString(size_t n);

	void parseStringHelper(std::string &val);
	uint64_t parseInteger(int &cnt);

	void parseNull(RpcValue &val);
	void parseBool(RpcValue &val);
	void parseString(RpcValue &val);
	void parseBlob(RpcValue &val);
	void parseNumber(RpcValue &val);
	void parseDateTime(RpcValue &val);
private:
	//int m_depth = 0;
};


} // namespace chainpack
} // namespace shv
