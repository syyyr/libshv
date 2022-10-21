#include "shvalarm.h"
#include "shvtypeinfo.h"

#include <shv/chainpack/rpcvalue.h>
#include <necrolog.h>

using namespace std;

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

shv::chainpack::RpcValue ShvAlarm::toRpcValue(bool all_fields_if_not_active) const
{
	shv::chainpack::RpcValue::Map ret;
	ret["path"] = path();
	ret["isActive"] = isActive();
	if(all_fields_if_not_active || isActive()) {
		if(severity() != Severity::Invalid) {
			ret["severity"] = static_cast<int>(severity());
			ret["severityName"] = severityName();
		}
		if(level() > 0)
			ret["alarmLevel"] = level();
		if(!description().empty())
			ret["description"] = description();
	}
	return ret;
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

vector<ShvAlarm> ShvAlarm::checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const chainpack::RpcValue &value)
{
	if(ShvNodeDescr node_descr = type_info.nodeDescriptionForPath(shv_path); node_descr.isValid()) {
		nDebug() << shv_path << node_descr.toRpcValue().toCpon();
		if(string alarm = node_descr.alarm(); !alarm.empty()) {
			return {ShvAlarm(shv_path,
					value.toBool(), // is active
					ShvAlarm::severityFromString(alarm),
					node_descr.alarmLevel(),
					node_descr.alarmDescription()
				)};
		}
		else {
			return checkAlarms(type_info, shv_path, node_descr.typeName(), value);
		}
	}
	return {};
}

std::vector<ShvAlarm> ShvAlarm::checkAlarms(const ShvTypeInfo &type_info, const std::string &shv_path, const std::string &type_name, const chainpack::RpcValue &value)
{
	if(ShvTypeDescr type_descr = type_info.findTypeDescription(type_name); type_descr.isValid()) {
		if (type_descr.type() == ShvTypeDescr::Type::BitField) {
			vector<ShvAlarm> alarms;
			auto flds = type_descr.fields();
			for (size_t i = 0; i < flds.size(); ++i) {
				const ShvFieldDescr &fld_descr = flds[i];
				if(string alarm = fld_descr.alarm(); !alarm.empty()) {
					bool is_alarm = fld_descr.bitfieldValue(value.toUInt64()).toBool();
					alarms.push_back(ShvAlarm(shv_path + '/' + fld_descr.name(),
						is_alarm,
						ShvAlarm::severityFromString(alarm),
						fld_descr.alarmLevel(),
						fld_descr.alarmDescription()
					));
				}
				else {
					auto alarms2 = checkAlarms(type_info, fld_descr.typeName(), shv_path + '/' + fld_descr.name(), fld_descr.bitfieldValue(value.toUInt64()));
					alarms.insert(alarms.end(), alarms2.begin(), alarms2.end());
				}
			}
			return alarms;
		}
		else if (type_descr.type() == ShvTypeDescr::Type::Enum) {
			auto flds = type_descr.fields();
			size_t active_alarm_ix = flds.size();
			for (size_t i = 0; i < flds.size(); ++i) {
				const ShvFieldDescr &fld_descr = flds[i];
				if(string alarm = fld_descr.alarm(); !alarm.empty()) {
					if(value == fld_descr.value())
						active_alarm_ix = i;
				}
			}
			if(active_alarm_ix < flds.size()) {
				const ShvFieldDescr &fld_descr = flds[active_alarm_ix];
				return {ShvAlarm(shv_path,
						active_alarm_ix < flds.size(),
						ShvAlarm::severityFromString(fld_descr.alarm()),
						fld_descr.alarmLevel(),
						fld_descr.alarmDescription()
					)};
			}
		}
	}
	return {};
}


} // namespace utils
} // namespace core
} // namespace shv
