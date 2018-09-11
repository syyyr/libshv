#include "abstractstreamreader1.h"

namespace shv {
namespace chainpack {

AbstractStreamReader1::AbstractStreamReader1(std::istream &in)
	: m_in(in)
{

}

RpcValue AbstractStreamReader1::read()
{
	RpcValue value;
	read(value);
	return value;
}

} // namespace chainpack
} // namespace shv
