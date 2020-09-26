#ifndef SHV_CORE_UTILS_SERVICEPROVIDERPATH_H
#define SHV_CORE_UTILS_SERVICEPROVIDERPATH_H

#include "../stringview.h"

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ServiceProviderPath
{
public:
	enum class Type { Invalid, Absolute, Relative };
public:
	ServiceProviderPath(const std::string &shv_path);

	bool isValid() const { return type() != Type::Invalid; }
	Type type() const { return m_type; }
	const char* typeString() const;
	char typeMark() const { return m_typeMark; }
	StringView service() const { return m_service; }
	StringView pathRest() const { return m_pathRest; }
private:
	static constexpr char RELATIVE_MARK = ':';
	static constexpr char ABSOLUTE_MARK = '|';

	static size_t serviceProviderMarkIndex(const std::string &path);
private:
	Type m_type = Type::Invalid;
	StringView m_service;
	StringView m_pathRest;
	char m_typeMark = 0;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SERVICEPROVIDERPATH_H
