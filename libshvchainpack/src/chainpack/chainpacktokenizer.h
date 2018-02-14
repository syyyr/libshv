#pragma once

#include "rpcvalue.h"
#include "abstractstreamtokenizer.h"

namespace shv {
namespace chainpack {

class ChainPackTokenizer : public shv::chainpack::AbstractStreamTokenizer
{
	using Super = AbstractStreamTokenizer;
public:
	using Super::Super;
public:
	TokenType nextToken() override;

	static uint64_t readUIntData(std::istream &in, bool *ok);
};

} // namespace chainpack
} // namespace shv
