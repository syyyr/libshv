#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

AbstractStreamWriter::AbstractStreamWriter(std::ostream &out)
	: m_out(out)
{

}

} // namespace chainpack
} // namespace shv
