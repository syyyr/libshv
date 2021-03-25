#ifndef SHV_CORE_UTILS_SERVICEPROVIDERPATH_H
#define SHV_CORE_UTILS_SERVICEPROVIDERPATH_H

#include "../stringview.h"

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ServiceProviderPath
{
public:
	enum class Type { Plain, AbsoluteService, MountPointRelativeService };
public:
	ServiceProviderPath(const std::string &shv_path);

	bool isServicePath() const { return type() != Type::Plain; }
	bool isMountPointRelative() const { return type() == Type::MountPointRelativeService; }
	bool isAbsolute() const { return type() == Type::AbsoluteService; }
	bool isPlain() const { return type() == Type::Plain; }
	Type type() const { return m_type; }
	const char* typeString() const;
	StringView service() const { return m_service; }
	/// broker ID including '@' separator
	StringView brokerId() const { return m_brokerId; }
	StringView pathRest() const { return m_pathRest; }
	std::string makePlainPath(const StringView &prefix = StringView()) const;
	std::string makeServicePath(const StringView &prefix = StringView()) const;
	const std::string& shvPath() const { return m_shvPath;}

	static std::string makePath(Type type, const StringView &service, const StringView &server_id, const StringView &path_rest);
	//static std::string joinPath(const StringView &a, const StringView &b);
private:
	static constexpr char END_MARK = ':';
	static constexpr char RELATIVE_MARK = '~';
	static constexpr char ABSOLUTE_MARK = '|';

	static size_t serviceProviderMarkIndex(const std::string &path);
	std::string typeMark() const { return typeMark(type()); }
	static std::string typeMark(Type t);
private:
	const std::string &m_shvPath;
	Type m_type = Type::Plain;
	StringView m_service;
	StringView m_brokerId; // including @, like '@mpk'
	StringView m_pathRest;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SERVICEPROVIDERPATH_H
