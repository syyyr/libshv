#include "abstractstreamwriter.h"

namespace shv {
namespace chainpack {

AbstractStreamWriter::AbstractStreamWriter(std::ostream &out)
	: m_out(out)
{

}

AbstractStreamWriter::~AbstractStreamWriter()
{
}

} // namespace chainpack
} // namespace shv
