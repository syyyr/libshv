#include "shvtypeinfo.h"
#include "shvpath.h"
#include "../string.h"
#include "../utils.h"
#include "../log.h"

#include<shv/chainpack/metamethod.h>

#include <cassert>

using namespace std;
using namespace shv::chainpack;

namespace shv::core::utils {

static const char* KEY_DEVICE_TYPE = "deviceType";
static const char* KEY_TYPE_NAME = "typeName";
static const char* KEY_LABEL = "label";
static const char* KEY_DESCRIPTION = "description";
static const char* KEY_UNIT = "unit";
static const char* KEY_NAME = "name";
static const char* KEY_TYPE = "type";
static const char* KEY_VALUE = "value";
static const char* KEY_FIELDS = "fields";
static const char* KEY_SAMPLE_TYPE = "sampleType";
static const char* KEY_TAGS = "tags";
static const char* KEY_METHODS = "methods";
static const char* KEY_BLACKLIST = "blacklist";
static const char* KEY_DEC_PLACES = "decPlaces";
static const char* KEY_VISUAL_STYLE = "visualStyle";
static const char* KEY_ALARM = "alarm";
static const char* KEY_ALARM_LEVEL = "alarmLevel";

//=====================================================================
// ShvTypeDescrBase
//=====================================================================
RpcValue ShvDescriptionBase::dataValue(const std::string &key, const chainpack::RpcValue &default_val) const
{
	return m_data.asMap().value(key, default_val);
}

void ShvDescriptionBase::setDataValue(const std::string &key, const chainpack::RpcValue &val)
{
	if(!m_data.isMap()) {
		m_data = RpcValue::Map();
	}
	m_data.set(key, val);
}

void ShvDescriptionBase::setData(const chainpack::RpcValue &data)
{
	if(data.isMap()) {
		RpcValue::Map map = data.asMap();
		mergeTags(map);
		m_data = map;
	}
}

void ShvDescriptionBase::mergeTags(chainpack::RpcValue::Map &map)
{
	auto nh = map.extract(KEY_TAGS);
	if(!nh.empty()) {
		RpcValue::Map tags = nh.mapped().asMap();
		map.merge(tags);
	}
}
//=====================================================================
// ShvTypeDescrField
//=====================================================================
ShvFieldDescr::ShvFieldDescr(const std::string &name, const std::string &type_name, const chainpack::RpcValue &value, chainpack::RpcValue::Map &&tags)
{
	setData(std::move(tags));
	setDataValue(KEY_NAME, name);
	setDataValue(KEY_TYPE_NAME, type_name);
	setDataValue(KEY_VALUE, value);
}

string ShvFieldDescr::name() const
{
	return dataValue(KEY_NAME).asString();
}

string ShvFieldDescr::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

string ShvFieldDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

string ShvFieldDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

RpcValue ShvFieldDescr::value() const
{
	return dataValue(KEY_VALUE);
}

string ShvFieldDescr::visualStyleName() const
{
	return dataValue(KEY_VISUAL_STYLE).asString();
}

string ShvFieldDescr::alarm() const
{
	return dataValue(KEY_ALARM).asString();
}

int ShvFieldDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

RpcValue ShvFieldDescr::toRpcValue() const
{
	RpcValue ret = m_data;
	if(description().empty())
		ret.set(KEY_DESCRIPTION, {});
	return ret;
}

ShvFieldDescr ShvFieldDescr::fromRpcValue(const RpcValue &v)
{
	ShvFieldDescr ret;
	ret.setData(v);
	return ret;
}

std::pair<unsigned, unsigned> ShvFieldDescr::bitRange() const
{
	unsigned bit_no1 = 0;
	unsigned bit_no2 = 0;
	if(value().isList()) {
		const auto &lst = value().asList();
		bit_no1 = lst.value(0).toUInt();
		bit_no2 = lst.value(1).toUInt();
		if(bit_no2 < bit_no1) {
			shvError() << "Invalid bit specification:" << value().toCpon();
			bit_no2 = bit_no1;
		}
	}
	else {
		bit_no1 = bit_no2 = value().toUInt();
	}
	return std::pair<unsigned, unsigned>(bit_no1, bit_no2);
}

RpcValue ShvFieldDescr::bitfieldValue(uint64_t uval) const
{
	if(!isValid())
		return {};
	auto [bit_no1, bit_no2] = bitRange();
	uint64_t mask = ~((~static_cast<uint64_t>(0)) << (bit_no2 + 1));
	shvDebug() << "bits:" << bit_no1 << bit_no2 << (bit_no1 == bit_no2) << "uval:" << uval << "mask:" << mask;
	uval = (uval & mask) >> bit_no1;
	shvDebug() << "uval masked and rotated right by:" << bit_no1 << "bits, uval:" << uval;
	RpcValue result;
	if(bit_no1 == bit_no2)
		result = uval != 0;
	else
		result = uval;
	return result;
}

uint64_t ShvFieldDescr::setBitfieldValue(uint64_t bitfield, uint64_t uval) const
{
	if(!isValid())
		return uval;
	auto [bit_no1, bit_no2] = bitRange();
	uint64_t mask = ~((~static_cast<uint64_t>(0)) << (bit_no2 - bit_no1 + 1));
	uval &= mask;
	mask <<= bit_no1;
	uval <<= bit_no1;
	bitfield &= ~mask;
	bitfield |= uval;
	return bitfield;
}

//=====================================================================
// ShvTypeDescr
//=====================================================================
ShvTypeDescr::ShvTypeDescr(Type t, std::vector<ShvFieldDescr> &&flds, SampleType st, chainpack::RpcValue::Map &&tags)
{
	m_data = RpcValue(move(tags));
	setType(t);
	setFields(flds);
	setSampleType(st);
}

ShvTypeDescr::Type ShvTypeDescr::type() const
{
	return static_cast<Type>(dataValue(KEY_TYPE).toInt());
}

ShvTypeDescr &ShvTypeDescr::setType(Type t)
{
	setDataValue(KEY_TYPE, static_cast<int>(t));
	return *this;
}

std::vector<ShvFieldDescr> ShvTypeDescr::fields() const
{
	std::vector<ShvFieldDescr> ret;
	auto flds = dataValue(KEY_FIELDS);
	for(const RpcValue &rv : flds.asList())
		ret.push_back(ShvFieldDescr::fromRpcValue(rv));
	return ret;
}

ShvTypeDescr &ShvTypeDescr::setFields(const std::vector<ShvFieldDescr> &fields)
{
	RpcValue::List cp_fields;
	for (const ShvFieldDescr &field : fields) {
		cp_fields.push_back(field.toRpcValue());
	}
	setDataValue(KEY_FIELDS, cp_fields);
	return *this;
}

ShvTypeDescr::SampleType ShvTypeDescr::sampleType() const
{
	return static_cast<SampleType>(dataValue(KEY_SAMPLE_TYPE).toInt());
}

ShvTypeDescr &ShvTypeDescr::setSampleType(ShvTypeDescr::SampleType st)
{
	setDataValue(KEY_SAMPLE_TYPE, static_cast<int>(st));
	return *this;
}

ShvFieldDescr ShvTypeDescr::field(const std::string &field_name) const
{
	auto flds = dataValue(KEY_FIELDS);
	for(const RpcValue &rv : flds.asList()) {
		auto fld_descr = ShvFieldDescr::fromRpcValue(rv);
		if(fld_descr.name() == field_name) {
			return fld_descr;
		}
	}
	return {};
}

RpcValue ShvTypeDescr::fieldValue(const chainpack::RpcValue &val, const std::string &field_name) const
{
	switch (type()) {
	case Type::BitField:
		return field(field_name).bitfieldValue(val.toUInt64());
	case Type::Map:
		return val.asMap().value(field_name);
	case Type::Enum:
		return field(field_name).value().toInt();
	default:
		break;
	}
	return {};
}

std::string ShvTypeDescr::typeToString(ShvTypeDescr::Type t)
{
	switch (t) {
	case Type::Invalid: break;
	case Type::BitField: return "BitField";
	case Type::Enum: return "Enum";
	case Type::Bool: return "Bool";
	case Type::UInt: return "UInt";
	case Type::Int: return "Int";
	case Type::Decimal: return "Decimal";
	case Type::Double: return "Double";
	case Type::String: return "String";
	case Type::DateTime: return "DateTime";
	case Type::List: return "List";
	case Type::Map: return "Map";
	case Type::IMap: return "IMap";
	}
	return "Invalid";
}

ShvTypeDescr::Type ShvTypeDescr::typeFromString(const std::string &s)
{
	if(s == "BitField") return Type::BitField;
	if(s == "Enum") return Type::Enum;
	if(s == "Bool") return Type::Bool;
	if(s == "UInt") return Type::UInt;
	if(s == "Int") return Type::Int;
	if(s == "Decimal") return Type::Decimal;
	if(s == "Double") return Type::Double;
	if(s == "String") return Type::String;
	if(s == "DateTime") return Type::DateTime;
	if(s == "List") return Type::List;
	if(s == "Map") return Type::Map;
	if(s == "IMap") return Type::IMap;
	return Type::Invalid;
}

std::string ShvTypeDescr::sampleTypeToString(ShvTypeDescr::SampleType t)
{
	switch (t) {
	case SampleType::Discrete: return "Discrete";
	case SampleType::Continuous: return "Continuous";
	case SampleType::Invalid: return "Invalid";
	}
	return std::string();
}

ShvTypeDescr::SampleType ShvTypeDescr::sampleTypeFromString(const std::string &s)
{
	if(s == "2" || s == "D" || s == "Discrete" || s == "discrete")
		return SampleType::Discrete;
	if(s.empty() || s == "C" || s == "Continuous" || s == "continuous")
		return SampleType::Continuous;
	return SampleType::Invalid;
}

RpcValue ShvTypeDescr::defaultRpcValue() const
{
	using namespace chainpack;

	switch (type()) {
	case Type::Invalid: return RpcValue::fromType(RpcValue::Type::Invalid); break;
	case Type::BitField: return RpcValue::fromType(RpcValue::Type::UInt); break;
	case Type::Enum: return RpcValue::fromType(RpcValue::Type::Int); break;
	case Type::Bool: return RpcValue::fromType(RpcValue::Type::Bool); break;
	case Type::UInt: return RpcValue::fromType(RpcValue::Type::UInt); break;
	case Type::Int: return RpcValue::fromType(RpcValue::Type::Int); break;
	case Type::Decimal: return RpcValue::fromType(RpcValue::Type::Decimal); break;
	case Type::Double: return RpcValue::fromType(RpcValue::Type::Double); break;
	case Type::String: return RpcValue::fromType(RpcValue::Type::String); break;
	case Type::DateTime: return RpcValue::fromType(RpcValue::Type::DateTime); break;
	case Type::List: return RpcValue::fromType(RpcValue::Type::List); break;
	case Type::Map: return RpcValue::fromType(RpcValue::Type::Map); break;
	case Type::IMap: return RpcValue::fromType(RpcValue::Type::IMap); break;
	}

	return RpcValue::fromType(RpcValue::Type::Null);
}

int ShvTypeDescr::decimalPlaces() const
{
	return dataValue(KEY_DEC_PLACES).toInt();
}

ShvTypeDescr &ShvTypeDescr::setDecimalPlaces(int n)
{
	setDataValue(KEY_DEC_PLACES, n);
	return *this;
}

RpcValue ShvTypeDescr::toRpcValue() const
{
	RpcValue::Map map = m_data.asMap();
	map.setValue(KEY_TYPE, typeToString(type()));
	if(sampleType() != ShvTypeDescr::SampleType::Invalid)
		map.setValue(KEY_SAMPLE_TYPE, sampleTypeToString(sampleType()));
	else
		map.setValue(KEY_SAMPLE_TYPE, {});
	return map;
}

ShvTypeDescr ShvTypeDescr::fromRpcValue(const RpcValue &v)
{
	ShvTypeDescr ret;
	{
		RpcValue::Map map = v.asMap();
		RpcValue src_fields = map.take("fields");
		RpcValue::List fields;
		for(const auto &rv : src_fields.asList()) {
			RpcValue::Map field = rv.asMap();
			mergeTags(field);
			fields.push_back(move(field));
		}
		mergeTags(map);
		map[KEY_FIELDS] = RpcValue(move(fields));
		ret.m_data = RpcValue(move(map));
	}
	{
		auto rv = ret.dataValue(KEY_TYPE);
		if(rv.isString())
			ret.setDataValue(KEY_TYPE, static_cast<int>(typeFromString(rv.asString())));
	}
	{
		auto rv = ret.dataValue(KEY_SAMPLE_TYPE);
		if(rv.isString())
			ret.setDataValue(KEY_SAMPLE_TYPE, static_cast<int>(sampleTypeFromString(rv.asString())));
	}
	return ret;
}

//=====================================================================
// ShvPropertyDescr
//=====================================================================
string ShvPropertyDescr::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

ShvPropertyDescr &ShvPropertyDescr::setTypeName(const string &type_name)
{
	setDataValue(KEY_TYPE_NAME, type_name);
	return *this;
}

string ShvPropertyDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

ShvPropertyDescr &ShvPropertyDescr::setLabel(const string &label)
{
	setDataValue(KEY_LABEL, label);
	return *this;
}

string ShvPropertyDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

ShvPropertyDescr &ShvPropertyDescr::setDescription(const string &description)
{
	setDataValue(KEY_DESCRIPTION, description);
	return *this;
}

string ShvPropertyDescr::unit() const
{
	return dataValue(KEY_UNIT).asString();
}

ShvPropertyDescr &ShvPropertyDescr::setUnit(const string &unit)
{
	setDataValue(KEY_UNIT, unit);
	return *this;
}

string ShvPropertyDescr::visualStyleName() const
{
	return dataValue(KEY_VISUAL_STYLE).asString();
}

ShvPropertyDescr &ShvPropertyDescr::setVisualStyleName(const string &visual_style_name)
{
	setDataValue(KEY_VISUAL_STYLE, visual_style_name);
	return *this;
}
/*
RpcValue ShvPropertyDescr::blacklist() const
{
	return dataValue(KEY_BLACKLIST);
}

ShvPropertyDescr &ShvPropertyDescr::setBlacklist(chainpack::RpcValue::Map &&black_list)
{
	RpcValue rv = black_list.empty()? RpcValue(): RpcValue(move(black_list));
	setDataValue(KEY_BLACKLIST, rv);
	return *this;
}
*/
string ShvPropertyDescr::alarm() const
{
	return dataValue(KEY_ALARM, std::string()).asString();
}

ShvPropertyDescr &ShvPropertyDescr::setAlarm(const string &alarm)
{
	setDataValue(KEY_ALARM, alarm);
	return *this;
}

int ShvPropertyDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

string ShvPropertyDescr::alarmDescription() const
{
	return description();
}

std::vector<ShvLogMethodDescr> ShvPropertyDescr::methods() const
{
	std::vector<ShvLogMethodDescr> ret;
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		ret.push_back(ShvLogMethodDescr::fromRpcValue(m));
	}
	return ret;
}

ShvLogMethodDescr ShvPropertyDescr::method(const std::string &name) const
{
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		auto mm = ShvLogMethodDescr::fromRpcValue(m);
		if(mm.name() == name)
			return mm;
	}
	return {};
}

ShvPropertyDescr &ShvPropertyDescr::addMethod(const ShvMethodDescr &method_descr)
{
	RpcValue::List new_methods = dataValue(KEY_METHODS).asList();
	new_methods.push_back(method_descr.toRpcValue());
	setDataValue(KEY_METHODS, new_methods);
	return *this;
}

ShvPropertyDescr &ShvPropertyDescr::setMethod(const ShvMethodDescr &method_descr)
{
	auto name = method_descr.name();
	auto method_list = methods();
	bool method_found = false;
	for(auto &mm : method_list) {
		if(mm.name() == name) {
			mm = method_descr;
			method_found = true;
			break;
		}
	}
	if(!method_found) {
		method_list.push_back(method_descr);
	}
	RpcValue::List new_methods;
	for(const auto &mm : method_list) {
		new_methods.push_back(mm.toRpcValue());
	}
	setDataValue(KEY_METHODS, new_methods);
	return *this;
}

RpcValue ShvPropertyDescr::toRpcValue() const
{
	//RpcValue::Map m;
	//m[KEY_TYPE_NAME] = typeName;
	//if(!description.empty())
	//	m[KEY_DESCRIPTION] = description;
	//if(!tags.empty())
	//	m[KEY_TAGS] = tags;
	return m_data;
}

ShvPropertyDescr ShvPropertyDescr::fromRpcValue(const RpcValue &v, RpcValue::Map *extra_tags)
{
	static const vector<string> known_tags{
		KEY_DEVICE_TYPE,
		KEY_TYPE_NAME,
		KEY_LABEL,
		KEY_DESCRIPTION,
		KEY_UNIT,
		//KEY_BLACKLIST,
		KEY_METHODS,
		"autoload",
		"autorefresh",
		"monitored",
		"monitorOptions",
		"sampleType",
		KEY_ALARM,
		KEY_ALARM_LEVEL,
	};
	RpcValue::Map m = v.asMap();
	RpcValue::Map node_map;
	for(const auto &key : known_tags)
		if(auto it = m.find(key); it != m.cend()) {
			auto nh = m.extract(key);
			node_map.insert(move(nh));
		}
	if(extra_tags)
		*extra_tags = move(m);
	ShvPropertyDescr ret;
	ret.setData(node_map);
	return ret;
}

//=====================================================================
// ShvLogTypeInfo
//=====================================================================
template<typename T>
typename map<string, T>::const_iterator find_longest_prefix(const map<string, T> &map, const string &path)
{
	shv::core::String prefix = path;
	while(true) {
		auto it = map.find(prefix);
		if(it == map.end()) {
			// if not found, cut last path part
			size_t ix = prefix.lastIndexOf('/');
			if(ix == string::npos) {
				return map.cend();
			}

			prefix = prefix.mid(0, ix);
		}
		else {
			return it;
		}
	}
	return map.cend();
}

bool ShvTypeInfo::isPathBlacklisted(const std::string &shv_path) const
{
	return std::any_of(m_blacklistedPaths.begin(), m_blacklistedPaths.end(), [&shv_path] (const auto& path) {
		return shv::core::utils::ShvPath::startsWithPath(shv_path, path.first);
	});
}

void ShvTypeInfo::setBlacklist(const std::string &shv_path, const chainpack::RpcValue &blacklist)
{
	if(blacklist.isList()) {
		for(const auto &path : blacklist.asList()) {
			m_blacklistedPaths[shv::core::Utils::joinPath(shv_path, path.asString())];
		}
	}
	else if(blacklist.isMap()) {
		for(const auto &[path, val] : blacklist.asMap()) {
			m_blacklistedPaths[shv::core::Utils::joinPath(shv_path, path)] = val;
		}
	}
}

void ShvTypeInfo::setPropertyDeviation(const std::string &shv_path, const ShvPropertyDescr &property_descr)
{
	m_propertyDeviations[shv_path] = property_descr;
}

ShvTypeInfo ShvTypeInfo::fromVersion2(std::map<std::string, ShvTypeDescr> &&types, std::map<std::string, ShvPropertyDescr> &&node_descriptions)
{
	ShvTypeInfo ret;
	ret.m_types = std::move(types);
	ret.m_devicePaths[""] = "";
	ret.m_deviceProperties[""] = std::move(node_descriptions);
	return ret;
}

const std::map<string, ShvPropertyDescr> &ShvTypeInfo::devicePropertyDescriptions(const std::string &device_type) const
{
	auto it = m_deviceProperties.find(device_type);
	if( it == m_deviceProperties.end()) {
		static DeviceProperties empty;
		return empty;
	}

	return it->second;
}

ShvTypeInfo &ShvTypeInfo::setDevicePath(const std::string &device_path, const std::string &device_type)
{
	m_devicePaths[device_path] = device_type;
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setDevicePropertyDescription(const std::string &device_path, const std::string &device_type, const std::string &property_path, const ShvPropertyDescr &property_descr)
{
	setDevicePath(device_path, device_type);
	return setDevicePropertyDescription(device_type, property_path, property_descr);
}

ShvTypeInfo &ShvTypeInfo::setDevicePropertyDescription(const std::string &device_type, const std::string &property_path, const ShvPropertyDescr &property_descr)
{
	if(property_descr.isValid())
		m_deviceProperties[device_type][property_path] = property_descr;
	else
		m_deviceProperties[device_type].erase(property_path);
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setExtraTags(const std::string &node_path, const chainpack::RpcValue &tags)
{
	if(tags.isValid())
		m_extraTags[node_path] = tags;
	else
		m_extraTags.erase(node_path);
	return *this;
}

RpcValue ShvTypeInfo::extraTags(const std::string &node_path) const
{
	auto it = m_extraTags.find(node_path);
	if( it == m_extraTags.cend()) {
		return {};
	}

	return it->second;
}

ShvTypeInfo &ShvTypeInfo::setTypeDescription(const std::string &type_name, const ShvTypeDescr &type_descr)
{
	if(type_descr.isValid())
		m_types[type_name] = type_descr;
	else
		m_types.erase(type_name);
	return *this;
}

namespace {
string cut_prefix(const string &path, const string &prefix) {
	if(prefix.size() == 0) {
		return path;
	}
	if(prefix.size() < path.size()) {
		return path.substr(prefix.size() + 1);
	}
	return string();
};
string cut_postfix(const string &path, const string &postfix) {
	if(postfix.size() == 0) {
		return path;
	}
	if(postfix.size() > 0 && postfix.size() < path.size()) {
		return path.substr(0, path.size() - postfix.size() - 1);
	}
	return string();
};
}

ShvTypeInfo::PathInfo ShvTypeInfo::pathInfo(const std::string &shv_path) const
{
	PathInfo ret;
	bool deviation_found = false;
	if(auto it3 = find_longest_prefix(m_propertyDeviations, shv_path); it3 != m_propertyDeviations.cend()) {
		deviation_found = true;
		const string own_property_path = it3->first;
		ret.fieldPath = cut_prefix(shv_path, own_property_path);
		ret.propertyDescription = it3->second;
	}
	const auto &[device_path, device_type, property_path] = findDeviceType(shv_path);
	if(!device_type.empty()) {
		ret.devicePath = device_path;
		ret.deviceType = device_type;
		if(deviation_found) {
			ret.propertyPath = cut_postfix(cut_prefix(shv_path, ret.devicePath), ret.fieldPath);
		}
		else {
			const auto &[own_property_path, field_path, property_descr] = findPropertyDescription(device_type, property_path);
			if(property_descr.isValid()) {
				ret.propertyPath = own_property_path;
				ret.fieldPath = field_path;
				ret.propertyDescription = property_descr;
			}
		}
	}
	return ret;
}

ShvPropertyDescr ShvTypeInfo::propertyDescriptionForPath(const std::string &shv_path, string *p_field_name) const
{
	auto info = pathInfo(shv_path);
	if(p_field_name)
		*p_field_name = info.fieldPath;
	return info.propertyDescription;
}

std::tuple<string, string, string> ShvTypeInfo::findDeviceType(const std::string &shv_path) const
{
	auto it = find_longest_prefix(m_devicePaths, shv_path);
	if( it == m_devicePaths.cend()) {
		return {};
	}

	const string prefix = it->first;
	const string device_type = it->second;
	const string property_path = String(shv_path).mid(prefix.size() + 1);
	return make_tuple(prefix, device_type, property_path);

}

std::tuple<string, string, ShvPropertyDescr> ShvTypeInfo::findPropertyDescription(const std::string &device_type, const std::string &property_path) const
{
	if(auto it2 = m_deviceProperties.find(device_type); it2 != m_deviceProperties.end()) {
		const DeviceProperties &dev_prop = it2->second;
		if(auto it3 = find_longest_prefix(dev_prop, property_path); it3 != dev_prop.cend()) {
			const string own_property_path = it3->first;
			const string field_path = cut_prefix(property_path, own_property_path);
			return make_tuple(own_property_path, field_path, it3->second);
		}
	}
	return {};
}

ShvTypeDescr ShvTypeInfo::findTypeDescription(const std::string &type_name) const
{
	auto it = m_types.find(type_name);
	if( it == m_types.end())
		return ShvTypeDescr(type_name);

	return it->second;
}

ShvTypeDescr ShvTypeInfo::typeDescriptionForPath(const std::string &shv_path) const
{
	return findTypeDescription(propertyDescriptionForPath(shv_path).typeName());
}

RpcValue ShvTypeInfo::extraTagsForPath(const std::string &shv_path) const
{
	auto it = m_extraTags.find(shv_path);
	if( it == m_extraTags.end())
		return {};

	return it->second;
}

std::string ShvTypeInfo::findSystemPath(const std::string &shv_path) const
{
	string current_root;
	string ret;
	for(const auto& [shv_root_path, system_path] : m_systemPathsRoots) {
		if(String::startsWith(shv_path, shv_root_path)) {
			if(shv_root_path.empty() || shv_path.size() == shv_root_path.size() || shv_path[shv_root_path.size()] == '/') {
				if(current_root.empty() || shv_root_path.size() > current_root.size()) {
					// fing longest match
					current_root = shv_root_path;
					ret = system_path;
				}
			}
		}
	}
	return ret;
}

RpcValue ShvTypeInfo::typesAsRpcValue() const
{
	RpcValue::Map m;
	for(const auto &kv : m_types) {
		m[kv.first] = kv.second.toRpcValue();
	}
	return m;
}

static const char *VERSION = "version";
static const char *PATHS = "paths"; // SHV2
static const char *TYPES = "types"; // SHV2 + SHV3
static const char *DEVICE_PATHS = "devicePaths";
static const char *DEVICE_PROPERTIES = "deviceProperties";
static const char *PROPERTY_DEVIATIONS = "propertyDeviations";
static const char *EXTRA_TAGS = "extraTags";
static const char *SYSTEM_PATHS_ROOTS = "systemPathsRoots";
static const char *BLACKLISTED_PATHS = "blacklistedPaths";

RpcValue ShvTypeInfo::toRpcValue() const
{
	RpcValue::Map map;
	map[TYPES] = typesAsRpcValue();
	{
		RpcValue::Map m;
		for(const auto &kv : m_devicePaths) {
			m[kv.first] = kv.second;
		}
		map[DEVICE_PATHS] = std::move(m);
	}
	{
		RpcValue::Map m;
		for(const auto &[device_type, property_map] : m_deviceProperties) {
			RpcValue::Map dev_properties;
			for(const auto &[property_path, node_descr] : property_map) {
				dev_properties[property_path] = node_descr.toRpcValue();
			}
			m[device_type] = dev_properties;
		}
		map[DEVICE_PROPERTIES] = std::move(m);
	}
	map[EXTRA_TAGS] = m_extraTags;
	{
		RpcValue::Map m;
		for(const auto &[key, val] : m_systemPathsRoots) {
			m[key] = val;
		}
		map[SYSTEM_PATHS_ROOTS] = std::move(m);
	}
	map[BLACKLISTED_PATHS] = m_blacklistedPaths;
	if(!m_propertyDeviations.empty()) {
		RpcValue::Map m;
		for(const auto &[key, val] : m_propertyDeviations) {
			m[key] = val.toRpcValue();
		}
		map[PROPERTY_DEVIATIONS] = std::move(m);
	}
	RpcValue ret = map;
	ret.setMetaValue(VERSION, 3);
	return ret;
}

ShvTypeInfo ShvTypeInfo::fromRpcValue(const RpcValue &v)
{
	int version = v.metaValue(VERSION).toInt();
	const RpcValue::Map &map = v.asMap();
	if(version == 3) {
		ShvTypeInfo ret;
		{
			const RpcValue::Map &m = map.value(TYPES).asMap();
			for(const auto &kv : m) {
				ret.m_types[kv.first] = ShvTypeDescr::fromRpcValue(kv.second);
			}
		}
		{
			const RpcValue::Map &m = map.value(DEVICE_PATHS).asMap();
			for(const auto &kv : m) {
				ret.m_devicePaths[kv.first] = kv.second.asString();
			}
		}
		{
			const RpcValue::Map &m = map.value(DEVICE_PROPERTIES).asMap();
			for(const auto &[device_type, prop_map] : m) {
				const RpcValue::Map &pm = prop_map.asMap();
				for(const auto &[property_path, node_descr] : pm) {
					ret.m_deviceProperties[device_type][property_path] = ShvPropertyDescr::fromRpcValue(node_descr);
				}
			}
		}
		ret.m_extraTags = map.value(EXTRA_TAGS).asMap();
		{
			const RpcValue::Map &m = map.value(SYSTEM_PATHS_ROOTS).asMap();
			for(const auto &[key, val] : m) {
				ret.m_systemPathsRoots[key] = val.asString();
			}
		}
		ret.m_blacklistedPaths = map.value(BLACKLISTED_PATHS).asMap();
		{
			const RpcValue::Map &m = map.value(PROPERTY_DEVIATIONS).asMap();
			for(const auto &[shv_path, node_descr] : m) {
				ret.m_propertyDeviations[shv_path] = ShvPropertyDescr::fromRpcValue(node_descr);
			}
		}
		return ret;
	}
	if(map.hasKey(PATHS) && map.hasKey(TYPES)) {
		// version 2
		ShvTypeInfo ret;
		{
			const RpcValue::Map &m = map.value(TYPES).asMap();
			for(const auto &kv : m) {
				ret.m_types[kv.first] = ShvTypeDescr::fromRpcValue(kv.second);
			}
		}
		{
			const RpcValue::Map &m = map.value(PATHS).asMap();
			for(const auto &[path, val] : m) {
				auto nd = ShvPropertyDescr::fromRpcValue(val);
				nd.setTypeName(val.asMap().value("type").asString());
				ret.m_deviceProperties[""][path] = nd;
			}
		}
		return ret;
	}

	return fromNodesTree(v);
}

RpcValue ShvTypeInfo::applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums) const
{
	ShvTypeDescr td = findTypeDescription(type_name);
	return applyTypeDescription(val, td, translate_enums);
}

RpcValue ShvTypeInfo::applyTypeDescription(const chainpack::RpcValue &val, const ShvTypeDescr &type_descr, bool translate_enums) const
{
	//shvWarning() << type_name << "--->" << td.toRpcValue().toCpon();
	switch(type_descr.type()) {
	case ShvTypeDescr::Type::Invalid:
		return val;
	case ShvTypeDescr::Type::BitField: {
		RpcValue::Map map;
		for(const ShvFieldDescr &fld : type_descr.fields()) {
			RpcValue result = fld.bitfieldValue(val.toUInt64());
			result = applyTypeDescription(result, fld.typeName());
			map[fld.name()] = result;
		}
		return map;
	}
	case ShvTypeDescr::Type::Enum: {
		int ival = val.toInt();
		if(translate_enums) {
			for(const ShvFieldDescr &fld : type_descr.fields()) {
				if(fld.value().toInt() == ival)
					return fld.name();
			}
			return "UNKNOWN_" + val.toCpon();
		}

		return ival;
	}
	case ShvTypeDescr::Type::Bool:
		return val.toBool();
	case ShvTypeDescr::Type::UInt:
		return val.toUInt();
	case ShvTypeDescr::Type::Int:
		return val.toInt();
	case ShvTypeDescr::Type::Decimal:
		if(val.isDecimal())
			return val;
		return RpcValue::Decimal::fromDouble(val.toDouble(), type_descr.decimalPlaces());
	case ShvTypeDescr::Type::Double:
		return val.toDouble();
	case ShvTypeDescr::Type::String:
		if(val.isString())
			return val;
		return val.asString();
	case ShvTypeDescr::Type::DateTime:
		return val.toDateTime();
	case ShvTypeDescr::Type::List:
		if(val.isList())
			return val;
		return val.asList();
	case ShvTypeDescr::Type::Map:
		if(val.isMap())
			return val;
		return val.asMap();
	case ShvTypeDescr::Type::IMap:
		if(val.isIMap())
			return val;
		return val.asIMap();
	}
	shvError() << "Invalid type:" << static_cast<int>(type_descr.type());
	return val;
}

void ShvTypeInfo::forEachDeviceProperty(const std::string &device_type, std::function<void (const std::string &, const ShvPropertyDescr &)> fn) const
{
	auto it = m_deviceProperties.find(device_type);
	if( it == m_deviceProperties.end()) {
		// device path invalid
		return;
	}

	for(const auto &[property_path, node_descr] : it->second) {
		fn(property_path, node_descr);
	};
}

void ShvTypeInfo::forEachProperty(std::function<void (const std::string &shv_path, const ShvPropertyDescr &node_descr)> fn) const
{
	for(const auto& [device_path, device_type] : m_devicePaths) {
		if(auto it = m_deviceProperties.find(device_type); it != m_deviceProperties.end()) {
			for(const auto& [property_path, node_descr] : it->second) {
				const auto shv_path = shv::core::Utils::joinPath(device_path, property_path);
				fn(shv_path, node_descr);
			}
		}
	}
}

/*
ShvPropertyDescr ShvTypeInfo::findNodeDescription(const std::string &shv_path, std::string *p_field_name) const
{
	auto pi = pathInfo(shv_path);
	if(p_field_name)
		*p_field_name = pi.fieldPath;
	return pi.nodeDescription;
}
*/
namespace {
ShvPropertyDescr take(ShvTypeInfo::DeviceProperties &map, const string &key)
{
	auto it = map.find(key);
	if(it == map.end())
		return {};
	auto ret = it->second;
	map.erase(it);
	return ret;
}
}
void ShvTypeInfo::fromNodesTree_helper(const RpcValue::Map &node_types,
										const shv::chainpack::RpcValue &node,
										const std::string &device_type,
										const std::string &device_path,
										const std::string &property_path,
										DeviceProperties *device_properties)
{
	using namespace shv::core::utils;
	using namespace shv::core;
	if(!node.isValid())
		return;

	if(auto ref = node.metaValue("nodeTypeRef").asString(); !ref.empty()) {
		fromNodesTree_helper(node_types, node_types.value(ref), device_type, device_path, property_path, device_properties);
		return;
	}

	string current_device_type = device_type;
	string current_device_path = device_path;
	string current_property_path = property_path;
	DeviceProperties new_device_properties;
	DeviceProperties *current_device_properties = device_properties? device_properties: &new_device_properties;
	RpcValue::Map property_descr_map;
	RpcValue::List property_methods;
	bool new_device_type_entered = device_properties == nullptr;
	static const string CREATE_FROM_TYPE_NAME = "createFromTypeName";
	//static const string STATUS = "status";
	static const string SYSTEM_PATH = "systemPath";
	//shvInfo() << "id:" << node->nodeId() << "node path:" << node_shv_path << "shv path:" << shv_path;
	//StringViewList node_shv_dir_list = ShvPath::splitPath(node_shv_path);
	for(const RpcValue &rv : node.metaValue("methods").asList()) {
		const auto mm = MetaMethod::fromRpcValue(rv);
		const string &method_name = mm.name();
		if(method_name == Rpc::METH_LS || method_name == Rpc::METH_DIR)
			continue;
		property_methods.push_back(mm.toRpcValue());
		if(!mm.tags().empty()) {
			string key = shv::core::Utils::joinPath(property_path, "method"s);
			key = shv::core::Utils::joinPath(key, mm.name());
			setExtraTags(key, mm.tags());
		}
	}
	const RpcValue::Map &node_tags = node.metaValue(KEY_TAGS).asMap();
	if(!node_tags.empty()) {
		RpcValue::Map tags_map = node_tags;
		const string &dtype = tags_map.value(KEY_DEVICE_TYPE).asString();
		if(!dtype.empty()) {
			current_device_type = dtype;
			current_device_path = Utils::joinPath(device_path, property_path);
			current_property_path = string();
			current_device_properties = &new_device_properties;
			new_device_type_entered = true;
		}
		property_descr_map.merge(tags_map);
	}
	property_descr_map.erase(CREATE_FROM_TYPE_NAME); // erase shvgate obsolete tag
	if(!property_methods.empty())
		property_descr_map[KEY_METHODS] = property_methods;
	if(!property_descr_map.empty()) {
		RpcValue::Map extra_tags;
		auto property_descr = shv::core::utils::ShvPropertyDescr::fromRpcValue(property_descr_map, &extra_tags);
		if(property_descr.isValid()) {
			(*current_device_properties)[current_property_path] = property_descr;
		}
		auto system_path = extra_tags.take(SYSTEM_PATH).asString();
		if(!system_path.empty()) {
			auto shv_path = Utils::joinPath(current_device_path, current_property_path);
			m_systemPathsRoots[shv_path] = system_path;
		}
		const auto blacklist = extra_tags.take(KEY_BLACKLIST);
		if(blacklist.isValid()) {
			auto shv_path = Utils::joinPath(current_device_path, current_property_path);
			setBlacklist(shv_path, blacklist);
		}
		if(!extra_tags.empty()) {
			auto shv_path = Utils::joinPath(current_device_path, current_property_path);
			setExtraTags(shv_path, extra_tags);
		}
	}
	for (const auto& [child_name, child_node] : node.asMap()) {
		if(child_name.empty())
			continue;
		// skip children of nodes created from typeName
		if(node_tags.hasKey(CREATE_FROM_TYPE_NAME))
			continue;
		ShvPath child_property_path = shv::core::Utils::joinPath(current_property_path, child_name);
		fromNodesTree_helper(node_types, child_node, current_device_type, current_device_path, child_property_path, current_device_properties);
	}
	if(new_device_type_entered) {
		setDevicePath(current_device_path, current_device_type);
		if(auto it = m_deviceProperties.find(current_device_type); it == m_deviceProperties.end()) {
			// device type defined first time
			m_deviceProperties[current_device_type] = move(new_device_properties);
		}
		else {
			// check deviations
			for(const auto &[existing_property_path, existing_property_descr] : it->second) {
				auto new_property_descr = take(new_device_properties, existing_property_path);
				if(new_property_descr.isValid()) {
					// defined in both definitions, compare them
					if(!(new_property_descr == existing_property_descr)) {
						// they are not the same, create deviation
						auto shv_path = Utils::joinPath(current_device_path, existing_property_path);
						setPropertyDeviation(shv_path, new_property_descr);
					}
				}
				else {
					// property defined only in previous definition, add it as empty to deviations
					auto shv_path = Utils::joinPath(current_device_path, existing_property_path);
					setPropertyDeviation(shv_path, {});
				}
			}
			// check remaining property paths in new definition
			for(const auto &[existing_property_path, new_property_descr] : new_device_properties) {
				auto shv_path = Utils::joinPath(current_device_path, existing_property_path);
				setPropertyDeviation(shv_path, new_property_descr);
			}
		}
	}
}

ShvTypeInfo ShvTypeInfo::fromNodesTree(const chainpack::RpcValue &v)
{
	ShvTypeInfo ret;
	{
		const RpcValue types = v.metaValue("typeInfo").asMap().value("types");
		const RpcValue::Map &m = types.asMap();
		for(const auto &kv : m) {
			ret.m_types[kv.first] = ShvTypeDescr::fromRpcValue(kv.second);
		}
	}
	const RpcValue node_types = v.metaValue("nodeTypes");
	ret.fromNodesTree_helper(node_types.asMap(), v, {}, {}, {}, nullptr);
	return ret;
}

} // namespace shv
