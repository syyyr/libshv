#include "serviceproviderpath.h"
#include "shvpath.h"

using namespace std;

namespace shv {
namespace core {
namespace utils {

ServiceProviderPath::ServiceProviderPath(const std::string &shv_path)
	: m_shvPath(shv_path)
{
	auto ix = serviceProviderMarkIndex(shv_path);
	if(ix > 0) {
		auto type_mark = shv_path[ix];
		if(type_mark == ABSOLUTE_MARK)
			m_type = Type::Absolute;
		else if(type_mark == RELATIVE_MARK)
			m_type = Type::Relative;
		m_service = StringView(shv_path, 0, ix);
		ssize_t bid_ix = m_service.lastIndexOf('@');
		if(bid_ix > 0) {
			m_brokerId = m_service.mid(bid_ix);
			m_service = m_service.mid(0, bid_ix);
		}
		m_pathRest = StringView(shv_path, ix + typeMark(m_type).size() + 1);
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
	case Type::Relative: return std::string(1, RELATIVE_MARK) + END_MARK;
	case Type::Absolute: return std::string(1, ABSOLUTE_MARK) + END_MARK;
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
	string rest = prefix.empty()? pathRest().toString(): prefix.toString() + ShvPath::SHV_PATH_DELIM + pathRest().toString();
	return makePath(type(), service(), brokerId(), rest);
}

std::string ServiceProviderPath::makePath(ServiceProviderPath::Type type, const StringView &service, const StringView &server_id, const StringView &path_rest)
{
	if(type == Type::Plain)
		return StringViewList{service, path_rest}.join(ShvPath::SHV_PATH_DELIM);
	string srv = service.toString();
	if(!server_id.empty())
		srv += server_id.toString();
	srv += typeMark(type);
	return StringViewList{srv, path_rest}.join(ShvPath::SHV_PATH_DELIM);
}

size_t ServiceProviderPath::serviceProviderMarkIndex(const std::string &path)
{
	for (size_t ix = 1; ix + 2 < path.size(); ++ix) {
		if(path[ix + 1] == END_MARK && path[ix + 2] == ShvPath::SHV_PATH_DELIM) {
			if(path[ix] == RELATIVE_MARK || path[ix] == ABSOLUTE_MARK)
				return ix;
		}
	}
	return 0;
}

} // namespace utils
} // namespace core
} // namespace shv
