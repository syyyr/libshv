#ifndef SHV_CORE_UTILS_SHVURL_H
#define SHV_CORE_UTILS_SHVURL_H

#include "../stringview.h"

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvUrl
{
public:
	enum class Type { Plain, AbsoluteService, MountPointRelativeService, DownTreeService };
public:
	ShvUrl(const std::string &shv_path);

	bool isServicePath() const { return type() != Type::Plain; }
	bool isUpTreeMountPointRelative() const { return type() == Type::MountPointRelativeService; }
	bool isUpTreeAbsolute() const { return type() == Type::AbsoluteService; }
	bool isPlain() const { return type() == Type::Plain; }
	bool isDownTree() const { return type() == Type::DownTreeService; }
	bool isUpTree() const { return isUpTreeAbsolute() || isUpTreeMountPointRelative(); }
	Type type() const { return m_type; }
	const char* typeString() const;
	StringView service() const { return m_service; }
	/// broker ID including '@' separator
	StringView fullBrokerId() const { return m_fullBrokerId; }
	StringView brokerId() const { return m_fullBrokerId.substr(1); }
	StringView pathPart() const { return m_pathPart; }
	std::string toPlainPath(const StringView &path_part_prefix = {}) const;
	std::string toString(const StringView &path_part_prefix = {}) const;
	const std::string& shvPath() const { return m_shvPath;}

	static std::string makeShvUrlString(Type type, const StringView &service, const StringView &full_broker_id, const StringView &path_rest);
private:
	static constexpr char END_MARK = ':';
	static constexpr char RELATIVE_MARK = '~';
	static constexpr char ABSOLUTE_MARK = '|';
	static constexpr char DOWNTREE_MARK = '>';

	static size_t serviceProviderMarkIndex(const std::string &path);
	std::string typeMark() const { return typeMark(type()); }
	static std::string typeMark(Type t);
private:
	const std::string &m_shvPath;
	Type m_type = Type::Plain;
	StringView m_service;
	StringView m_fullBrokerId; // including @, like '@mpk'
	StringView m_pathPart;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVURL_H
