#include "shvalarm.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace core {
namespace utils {

ShvAlarm::ShvAlarm()
	: m_path("")
	, m_isActive(false)
	, m_description("")
	, m_severity(Severity::NoAlarm)
{}

ShvAlarm::ShvAlarm(const std::string &path,  bool is_active, Severity severity, int level, const std::string &description)
	: m_path(path)
	, m_isActive(is_active)
	, m_description(description)
	, m_level(level)
	, m_severity(severity)
{}

ShvAlarm::Severity ShvAlarm::severityToString(std::string lvl)
{
	if (lvl == "info") {
		return Severity::Info;
	}
	else if (lvl == "warning") {
		return Severity::Warning;
	}
	else if (lvl == "error") {
		return Severity::Error;
	}
	return Severity::NoAlarm;
}

std::string ShvAlarm::severityAsString() const
{
	switch (m_severity) {
	case Severity::NoAlarm: return "";
	case Severity::Info: return "Info";
	case Severity::Warning: return "Warning";
	case Severity::Error: return "Error";
	}
	return "";
}

bool ShvAlarm::operator<(const ShvAlarm &a) const
{
	if (m_severity < a.m_severity) return true;
	if (m_severity == a.m_severity && m_level < a.m_level) return true;
	return false;
}

bool ShvAlarm::operator==(const ShvAlarm &a) const
{
	return m_severity == a.m_severity
			&& m_isActive == a.m_isActive
			&& m_level == a.m_level
			&& m_path == a.m_path;
}

shv::chainpack::RpcValue ShvAlarm::toRpcValue() const
{
	return shv::chainpack::RpcValue::Map{
		{"path", path()},
		{"isActive", isActive()},
		{"severity", static_cast<int>(severity())},
		{"severityName", severityAsString()},
		{"alarmLevel", level()},
		{"description", description()}
	};
}


} // namespace utils
} // namespace core
} // namespace shv
