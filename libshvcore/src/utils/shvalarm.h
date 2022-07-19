#ifndef SHV_CORE_UTILS_SHVALARM_H
#define SHV_CORE_UTILS_SHVALARM_H

#include "../shvcoreglobal.h"

#include <necrologlevel.h>

#include <string>
#include <vector>

namespace shv { namespace chainpack { class RpcValue; }}

namespace shv {
namespace core {
namespace utils {

class ShvTypeInfo;

class SHVCORE_DECL_EXPORT ShvAlarm {
public:
	using Severity = NecroLogLevel;
public:
	ShvAlarm();
	ShvAlarm(const std::string &path, bool is_active = false, Severity severity = Severity::Invalid, int level = 0, const std::string &description = {});

public:
	static Severity severityFromString(const std::string &lvl);
	const char *severityName() const;
	bool isLessSevere(const ShvAlarm &a) const;
	bool operator==(const ShvAlarm &a) const;

	Severity severity() const { return m_severity; }
	const std::string& path() const { return m_path; }
	const std::string& description() const { return m_description; }
	Severity severityFromString() const { return m_severity; }
	int level() const { return m_level; }
	bool isValid() const { return !path().empty(); }
	bool isActive() const { return m_isActive; }

	shv::chainpack::RpcValue toRpcValue(bool all_fields_if_not_active = false) const;
	static ShvAlarm fromRpcValue(const shv::chainpack::RpcValue &rv);
	static std::vector<ShvAlarm> checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const shv::chainpack::RpcValue &value);
	static std::vector<ShvAlarm> checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const std::string &type_name, const shv::chainpack::RpcValue &value);
protected:
	std::string m_path;
	bool m_isActive = false;
	std::string m_description;
	int m_level = 0;
	Severity m_severity = Severity::Invalid;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVALARM_H
