#include "abstractstreamreader1.h"

namespace shv {
namespace chainpack1 {

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
