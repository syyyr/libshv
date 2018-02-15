#include "abstractstreamreader.h"

namespace shv {
namespace chainpack {

AbstractStreamReader::AbstractStreamReader(std::istream &in)
	: m_in(in)
{

}

RpcValue AbstractStreamReader::read()
{
	RpcValue value;
	read(value);
	return value;
}

} // namespace chainpack
} // namespace shv
