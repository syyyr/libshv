#include "serviceproviderpath.h"
#include "shvpath.h"

namespace shv {
namespace core {
namespace utils {

ServiceProviderPath::ServiceProviderPath(const std::string &shv_path)
{
	auto ix = serviceProviderMarkIndex(shv_path);
	if(ix > 0) {
		m_typeMark = shv_path[ix];
		m_service = StringView(shv_path, 0, ix);
		m_pathRest = StringView(shv_path, ix + 2);
		if(shv_path[ix] == ABSOLUTE_MARK)
			m_type = Type::Absolute;
		else if(shv_path[ix] == RELATIVE_MARK)
			m_type = Type::Relative;
	}
}

const char *ServiceProviderPath::typeString() const
{
	switch (type()) {
	case Type::Invalid: return "Invalid";
	case Type::Relative: return "Relative";
	case Type::Absolute: return "Absolute";
	}
	return "???";
}


size_t ServiceProviderPath::serviceProviderMarkIndex(const std::string &path)
{
	for (size_t ix = 1; ix + 1 < path.size(); ++ix) {
		if(path[ix + 1] == ShvPath::SHV_PATH_DELIM) {
			if(path[ix] == RELATIVE_MARK || path[ix] == ABSOLUTE_MARK)
				return ix;
		}
	}
	return 0;
}

} // namespace utils
} // namespace core
} // namespace shv
