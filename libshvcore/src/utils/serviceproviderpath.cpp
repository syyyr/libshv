#include "serviceproviderpath.h"
#include "shvpath.h"

namespace shv {
namespace core {
namespace utils {

ServiceProviderPath::ServiceProviderPath(const std::string &shv_path)
	: m_shvPath(shv_path)
{
	auto ix = serviceProviderMarkIndex(shv_path);
	if(ix > 0) {
		m_service = StringView(shv_path, 0, ix);
		m_pathRest = StringView(shv_path, ix + 2);
		auto type_mark = shv_path[ix];
		if(type_mark == ABSOLUTE_MARK)
			m_type = Type::Absolute;
		else if(type_mark == RELATIVE_MARK)
			m_type = Type::Relative;
	}
}

const char *ServiceProviderPath::typeString() const
{
	switch (type()) {
	case Type::Plain: return "Plain";
	case Type::Relative: return "Relative";
	case Type::Absolute: return "Absolute";
	}
	return "???";
}

std::string ServiceProviderPath::typeMark(Type t)
{
	switch (t) {
	case Type::Plain: return std::string();
	case Type::Relative: return std::string(1, RELATIVE_MARK);
	case Type::Absolute: return std::string(1, ABSOLUTE_MARK);
	}
	return std::string();
}

std::string ServiceProviderPath::makePlainPath(const StringView &prefix) const
{
	std::string ret = service().toString();
	if(!prefix.empty())
		ret += ShvPath::SHV_PATH_DELIM + prefix.toString();
	ret += ShvPath::SHV_PATH_DELIM + pathRest().toString();
	return ret;
}

std::string ServiceProviderPath::makeServicePath(const StringView &prefix) const
{
	std::string ret = service().toString() + typeMark();
	if(!prefix.empty())
		ret += ShvPath::SHV_PATH_DELIM + prefix.toString();
	ret += ShvPath::SHV_PATH_DELIM + pathRest().toString();
	return ret;
}

std::string ServiceProviderPath::makePath(ServiceProviderPath::Type type, const StringView &service, const StringView &path_rest)
{
	switch(type) {
	case Type::Plain:
		return ShvPath::join({service, path_rest});
	default:
		return ShvPath::join({service, typeMark(type), path_rest});
	}
}
/*
std::string ServiceProviderPath::makeShvPath() const
{
	std::string ret = service().toString();
	if(isServicePath())
		ret += typeMark();
	ret += ShvPath::SHV_PATH_DELIM + pathRest().toString();
	return ret;
}
*/
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
