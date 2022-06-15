#include "shvalarm.h"
#include "shvlogtypeinfo.h"

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

shv::chainpack::RpcValue ShvAlarm::toRpcValue() const
{
	shv::chainpack::RpcValue::Map ret;
	ret["path"] = path();
	ret["isActive"] = isActive();
	if(severity() != Severity::Invalid) {
		ret["severity"] = static_cast<int>(severity());
		ret["severityName"] = severityName();
	}
	if(level() > 0)
		ret["alarmLevel"] = level();
	if(!description().empty())
		ret["description"] = description();
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

static vector<ShvAlarm> checkAlarm_helper(const ShvLogTypeInfo &type_info, const ShvLogTypeDescr &type_descr, const std::string &shv_path, const chainpack::RpcValue &value)
{
	if (type_descr.type() == ShvLogTypeDescr::Type::BitField) {
		vector<ShvAlarm> alarms;
		auto flds = type_descr.fields();
		for (size_t i = 0; i < flds.size(); ++i) {
			const ShvLogTypeDescrField &fld_descr = flds[i];
			if(string alarm = fld_descr.alarm(); !alarm.empty()) {
				bool is_alarm = ShvLogTypeInfo::fieldValue(value, fld_descr).toBool();
				if(is_alarm) {
					alarms.push_back(ShvAlarm(shv_path + '/' + fld_descr.name(),
						true,
						ShvAlarm::severityFromString(alarm),
						fld_descr.alarmLevel(),
						fld_descr.description()
					));
				}
				else {
					alarms.push_back(ShvAlarm{shv_path + '/' + fld_descr.name(), false});
				}
			}
			else {
				if(auto type_descr2 = type_info.typeDescriptionForName(fld_descr.typeName()); type_descr2.isValid()) {
					auto alarms2 = checkAlarm_helper(type_info, type_descr2, shv_path + '/' + fld_descr.name(), ShvLogTypeInfo::fieldValue(value, fld_descr));
					alarms.insert(alarms.end(), alarms2.begin(), alarms2.end());
				}
			}
		}
		return alarms;
	}
	else if (type_descr.type() == ShvLogTypeDescr::Type::Enum) {
		auto flds = type_descr.fields();
		bool has_alarm = false;
		size_t active_alarm_ix = flds.size();
		for (size_t i = 0; i < flds.size(); ++i) {
			const ShvLogTypeDescrField &fld_descr = flds[i];
			if(string alarm = fld_descr.alarm(); !alarm.empty()) {
				has_alarm = true;
				if(value == fld_descr.value())
					active_alarm_ix = i;
			}
		}
		if(has_alarm) {
			if(active_alarm_ix < flds.size()) {
				const ShvLogTypeDescrField &fld_descr = flds[active_alarm_ix];
				return {ShvAlarm(shv_path,
						true,
						ShvAlarm::severityFromString(fld_descr.alarm()),
						fld_descr.alarmLevel(),
						fld_descr.description()
					)};
			}
			else {
				return {ShvAlarm(shv_path, false )};
			}
		}
	}
	return {};
}

vector<ShvAlarm> ShvAlarm::checkAlarms(const ShvLogTypeInfo &type_info, const std::string &shv_path, const chainpack::RpcValue &value)
{
	ShvLogNodeDescr node_descr = type_info.nodeDescriptionForPath(shv_path);
	if(!node_descr.isValid())
		return {};
	if(string alarm = node_descr.alarm(); !alarm.empty()) {
		if(value.toBool()) {
			return {ShvAlarm(shv_path,
				true,
				severityFromString(alarm),
				node_descr.alarmLevel(),
				node_descr.description()
			)};
		}
		else {
			return {ShvAlarm{shv_path, false}};
		}
	}
	if(ShvLogTypeDescr type_descr = type_info.typeDescriptionForName(node_descr.typeName()); type_descr.isValid()) {
		return checkAlarm_helper(type_info, type_descr, shv_path, value);
	}
	return {};
}


} // namespace utils
} // namespace core
} // namespace shv
