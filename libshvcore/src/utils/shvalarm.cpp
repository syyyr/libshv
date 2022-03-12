#include "shvalarm.h"

#include <shv/chainpack/rpcvalue.h>
#include <necrolog.h>

namespace shv {
namespace core {
namespace utils {

ShvAlarm::ShvAlarm()
	: m_path("")
	, m_isActive(false)
	, m_description("")
	, m_severity(Severity::Invalid)
{}

ShvAlarm::ShvAlarm(const std::string &path,  bool is_active, Severity severity, int level, const std::string &description)
	: m_path(path)
	, m_isActive(is_active)
	, m_description(description)
	, m_level(level)
	, m_severity(severity)
{}

ShvAlarm::Severity ShvAlarm::severityFromString(const std::string &lvl)
{
	return NecroLog::stringToLevel(lvl.c_str());
}

const char* ShvAlarm::severityName() const
{
	return NecroLog::levelToString(m_severity);
}

bool ShvAlarm::isLessSevere(const ShvAlarm &a) const
{
	if (m_severity == a.m_severity)
		return m_level < a.m_level;
	if(m_severity == Severity::Invalid)
		return true;
	return (m_severity > a.m_severity);
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
	if(isValid()) {
		return shv::chainpack::RpcValue::Map{
			{"path", path()},
			{"isActive", isActive()},
			{"severity", static_cast<int>(severity())},
			{"severityName", severityName()},
			{"alarmLevel", level()},
			{"description", description()}
		};
	}
	return nullptr;
}

ShvAlarm ShvAlarm::fromRpcValue(const chainpack::RpcValue &rv)
{
	const chainpack::RpcValue::Map &m = rv.asMap();
	ShvAlarm a {
		m.value("path").asString(),
		m.value("isActive").toBool(),
		static_cast<Severity>(m.value("severity").toInt()),
		m.value("alarmLevel").toInt(),
		m.value("description").asString(),
	};
	return a;
}


} // namespace utils
} // namespace core
} // namespace shv
