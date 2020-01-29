#include "shvlogheader.h"

#include "../stringview.h"
#include "../exception.h"

namespace shv {
namespace core {
namespace utils {

ShvLogHeader ShvLogHeader::fromMetaData(const chainpack::RpcValue::MetaData &md)
{
	ShvLogHeader ret;
	const chainpack::RpcValue::Map &device = md.value("device").toMap();
	ret.setDeviceId(device.value("id").toString());
	ret.setDeviceType(device.value("type").toString());
	ret.setLogVersion(md.value("logVersion").toInt());
	ShvJournalGetLogParams params = ShvJournalGetLogParams::fromRpcValue(md.value("logParams"));
	ret.setLogParams(std::move(params));
	ret.setRecordCount(md.value("recordCount").toInt());
	ret.setRecordCountLimit(md.value("recordCountLimit").toInt());
	ret.setWithUptime(md.value("withUptime").toBool());
	ret.setWithSnapShot(md.value("withSnapShot").toBool());
	ret.setFields(md.value("fields"));
	ret.setPathDict(md.value("pathDict"));
	ret.m_typeInfos = md.value("typeInfos").toMap();
	auto ti = md.value("typeInfo");
	if(ti.isMap())
		ret.m_typeInfos["."] = ti;
	ret.setDateTime(md.value("dateTime"));
	ret.setSince(md.value("since"));
	ret.setUntil(md.value("until"));
	return ret;
}

chainpack::RpcValue::MetaData ShvLogHeader::toMetaData() const
{
	chainpack::RpcValue::MetaData md;
	chainpack::RpcValue::Map device;
	if(!deviceId().empty())
		device["id"] = deviceId();
	if(!deviceType().empty())
		device["type"] = deviceType();
	if(!device.empty())
		md.setValue("device", device);
	md.setValue("logVersion", logVersion());
	md.setValue("logParams", logParams().toRpcValue());
	md.setValue("recordCount", recordCount());
	md.setValue("recordCountLimit", recordCountLimit());
	md.setValue("withUptime", withUptime());
	md.setValue("withSnapShot", withSnapShot());
	const chainpack::RpcValue::List &flds = fields().toList();
	if(!flds.empty())
		md.setValue("fields", fields());
	const chainpack::RpcValue::IMap &dict = pathDict().toIMap();
	if(!dict.empty())
		md.setValue("pathDict", pathDict());
	if(!m_typeInfos.empty()) {
		if(m_typeInfos.size() == 1 && m_typeInfos.count(".") == 1)
			md.setValue("typeInfo", m_typeInfos.value("."));
		else
			md.setValue("typeInfos", m_typeInfos);
	}
	md.setValue("dateTime", dateTime());
	md.setValue("since", since());
	md.setValue("until", until());
	return md;
}

void ShvLogHeader::setTypeInfo(const std::string &path_prefix, const chainpack::RpcValue &i)
{
	if(path_prefix.empty())
		m_typeInfos["."] = i;
	else
		m_typeInfos[path_prefix] = i;
}

#if 0
chainpack::RpcValue ShvLogHeader::valueOnPath(const std::string &path) const
{
	return valueOnPath(StringView(path).split('/'));
}

chainpack::RpcValue ShvLogHeader::valueOnPath(const StringViewList &path) const
{
	chainpack::RpcValue ret;
	if(path.empty())
		return ret;
	ret = value(path.value(0).toString());
	return valueOnPath_helper(ret, path);
}

chainpack::RpcValue ShvLogHeader::valueOnPath_helper(const chainpack::RpcValue &parent, const StringViewList &path) const
{
	chainpack::RpcValue ret = parent;
	for (size_t i = 1; i < path.size(); ++i) {
		const auto &sv = path[i];
		ret = ret.at(sv.toString());
	}
	return ret;
}

void ShvLogHeader::setValueOnPath(const std::string &path, const chainpack::RpcValue &val)
{
	setValueOnPath(StringView(path).split('/'), val);
}

void ShvLogHeader::setValueOnPath(const StringViewList &path, const chainpack::RpcValue &val)
{
	if(path.empty())
		SHV_EXCEPTION("Empty path");
	std::string path0 = path.value(0).toString();
	if(path.size() == 1) {
		setValue(path0, val);
		return;
	}
	shv::chainpack::RpcValue v = value(path0);
	if(!v.isMap()) {
		v = chainpack::RpcValue::Map();
		setValue(path0, v);
	}
	setValueOnPath_helper(v, path, val);
}

void ShvLogHeader::setValueOnPath_helper(const chainpack::RpcValue &parent, const StringViewList &path, const chainpack::RpcValue &val)
{
	chainpack::RpcValue v = parent;
	for (size_t i = 1; i < path.size()-1; ++i) {
		const std::string dir = path.at(i).toString();
		if(!v.isMap())
			SHV_EXCEPTION("Internal error");
		shv::chainpack::RpcValue v1 = v.at(dir);
		if(!v1.isMap()) {
			v1 = chainpack::RpcValue::Map();
			v.set(dir, v1);
		}
		v = v1;
	}
	v.set(path.at(path.size() - 1).toString(), val);
}

std::string ShvLogHeader::deviceId() const
{
	return valueOnPath("device/id").toString();
}

void ShvLogHeader::setDeviceId(const std::string &device_id)
{
	setValueOnPath("device/id", device_id);
}

std::string ShvLogHeader::deviceType() const
{
	return valueOnPath("device/type").toString();
}

void ShvLogHeader::setDeviceType(const std::string &device_type)
{
	setValueOnPath("device/type", device_type);
}
#endif



} // namespace utils
} // namespace core
} // namespace shv
