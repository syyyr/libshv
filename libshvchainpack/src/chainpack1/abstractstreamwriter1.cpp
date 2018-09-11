#include "abstractstreamwriter1.h"

namespace shv {
namespace chainpack1 {

AbstractStreamWriter::AbstractStreamWriter(std::ostream &out)
	: m_out(out)
{

}

AbstractStreamWriter::~AbstractStreamWriter()
{
}

} // namespace chainpack
} // namespace shv
