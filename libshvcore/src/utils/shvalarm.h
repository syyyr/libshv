#ifndef SHV_CORE_UTILS_SHVALARM_H
#define SHV_CORE_UTILS_SHVALARM_H

#include "../shvcoreglobal.h"

#include <necrolog.h>

#include <string>

namespace shv { namespace chainpack { class RpcValue; }}

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvAlarm {
public:
	typedef NecroLog::Level Severity;
public:
	ShvAlarm();
	ShvAlarm(const std::string &path, bool is_active = false, Severity severity = Severity::Invalid, int level = 0, const std::string &description = {});

public:
	static Severity severityFromString(const std::string &lvl);
	const char *severityName() const;
	bool operator<(const ShvAlarm &a) const;
	bool operator==(const ShvAlarm &a) const;

	Severity severity() const { return m_severity; }
	const std::string& path() const { return m_path; }
	const std::string& description() const { return m_description; }
	Severity severityFromString() const { return m_severity; }
	int level() const { return m_level; }
	bool isActive() const { return m_isActive; }

	shv::chainpack::RpcValue toRpcValue() const;
	static ShvAlarm fromRpcValue(const shv::chainpack::RpcValue &rv);
protected:
	std::string m_path = "";
	bool m_isActive = false;
	std::string m_description = "";
	int m_level = 0;
	Severity m_severity = Severity::Invalid;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVALARM_H
