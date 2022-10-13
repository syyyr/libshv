#include "shvtypeinfo.h"
#include "shvpath.h"
#include "../string.h"
#include "../utils.h"
#include "../log.h"

#include<shv/chainpack/metamethod.h>

#include <cassert>

using namespace std;
using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

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
//static const char* KEY_BLACKLIST = "blacklist";
static const char* KEY_DEC_PLACES = "decPlaces";
static const char* KEY_VISUAL_STYLE = "visualStyle";
static const char* KEY_ALARM = "alarm";
static const char* KEY_ALARM_LEVEL = "alarmLevel";

//=====================================================================
// ShvTypeDescrBase
//=====================================================================
RpcValue ShvTypeDescrBase::dataValue(const std::string &key, const chainpack::RpcValue &default_val) const
{
	return m_data.asMap().value(key, default_val);
}

void ShvTypeDescrBase::setDataValue(const std::string &key, const chainpack::RpcValue &val)
{
	if(!m_data.isMap()) {
		m_data = RpcValue::Map();
	}
	m_data.set(key, val);
}

void ShvTypeDescrBase::setData(const chainpack::RpcValue &data)
{
	if(data.isMap()) {
		RpcValue::Map map = data.asMap();
		mergeTags(map);
		m_data = map;
	}
}

void ShvTypeDescrBase::mergeTags(chainpack::RpcValue::Map &map)
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
		result = static_cast<bool>(uval != 0);
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
	setDataValue(KEY_TYPE, (int)t);
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
	setDataValue(KEY_SAMPLE_TYPE, (int)st);
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

const std::string ShvTypeDescr::typeToString(ShvTypeDescr::Type t)
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

const std::string ShvTypeDescr::sampleTypeToString(ShvTypeDescr::SampleType t)
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
			ret.setDataValue(KEY_TYPE, (int)typeFromString(rv.asString()));
	}
	{
		auto rv = ret.dataValue(KEY_SAMPLE_TYPE);
		if(rv.isString())
			ret.setDataValue(KEY_SAMPLE_TYPE, (int)sampleTypeFromString(rv.asString()));
	}
	return ret;
}

//=====================================================================
// ShvLogNodeDescr
//=====================================================================
string ShvLogNodeDescr::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

ShvLogNodeDescr &ShvLogNodeDescr::setTypeName(const string &type_name)
{
	setDataValue(KEY_TYPE_NAME, type_name);
	return *this;
}

string ShvLogNodeDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

ShvLogNodeDescr &ShvLogNodeDescr::setLabel(const string &label)
{
	setDataValue(KEY_LABEL, label);
	return *this;
}

string ShvLogNodeDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

ShvLogNodeDescr &ShvLogNodeDescr::setDescription(const string &description)
{
	setDataValue(KEY_DESCRIPTION, description);
	return *this;
}

string ShvLogNodeDescr::unit() const
{
	return dataValue(KEY_UNIT).asString();
}

ShvLogNodeDescr &ShvLogNodeDescr::setUnit(const string &unit)
{
	setDataValue(KEY_UNIT, unit);
	return *this;
}

string ShvLogNodeDescr::visualStyleName() const
{
	return dataValue(KEY_VISUAL_STYLE).asString();
}

ShvLogNodeDescr &ShvLogNodeDescr::setVisualStyleName(const string &visual_style_name)
{
	setDataValue(KEY_VISUAL_STYLE, visual_style_name);
	return *this;
}
/*
RpcValue ShvNodeDescr::blacklist() const
{
	return dataValue(KEY_BLACKLIST);
}

ShvNodeDescr &ShvNodeDescr::setBlacklist(chainpack::RpcValue::Map &&black_list)
{
	RpcValue rv = black_list.empty()? RpcValue(): RpcValue(move(black_list));
	setDataValue(KEY_BLACKLIST, rv);
	return *this;
}
*/
string ShvLogNodeDescr::alarm() const
{
	return dataValue(KEY_ALARM, std::string()).asString();
}

ShvLogNodeDescr &ShvLogNodeDescr::setAlarm(const string &alarm)
{
	setDataValue(KEY_ALARM, alarm);
	return *this;
}

int ShvLogNodeDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

string ShvLogNodeDescr::alarmDescription() const
{
	return description();
}

std::vector<ShvLogMethodDescr> ShvLogNodeDescr::methods() const
{
	std::vector<ShvLogMethodDescr> ret;
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		ret.push_back(ShvLogMethodDescr::fromRpcValue(m));
	}
	return ret;
}

ShvLogMethodDescr ShvLogNodeDescr::method(const std::string &name) const
{
	RpcValue rv = dataValue(KEY_METHODS);
	for(const auto &m : rv.asList()) {
		auto mm = ShvLogMethodDescr::fromRpcValue(m);
		if(mm.name() == name)
			return mm;
	}
	return {};
}

RpcValue ShvLogNodeDescr::toRpcValue() const
{
	//RpcValue::Map m;
	//m[KEY_TYPE_NAME] = typeName;
	//if(!description.empty())
	//	m[KEY_DESCRIPTION] = description;
	//if(!tags.empty())
	//	m[KEY_TAGS] = tags;
	return m_data;
}

ShvLogNodeDescr ShvLogNodeDescr::fromRpcValue(const RpcValue &v, RpcValue::Map *extra_tags)
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
	ShvLogNodeDescr ret;
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
			else {
				prefix = prefix.mid(0, ix);
			}
		}
		else {
			return it;
		}
	}
	return map.cend();
}

bool ShvTypeInfo::isPathBlacklisted(const std::string &shv_path) const
{
	auto pi = pathInfo(shv_path);
	if(pi.devicePath.empty()) {
		return false;
	}
	if(auto it = m_extraTags.find(pi.devicePath); it == m_extraTags.cend()) {
		return false;
	}
	else {
		auto prop_path = shv::core::Utils::joinPath(pi.propertyPath, pi.fieldPath);
		auto black_list = it->second.asMap().value("blacklist");
		if(black_list.isList()) {
			for(const auto &path : black_list.asList()) {
				if(shv::core::utils::ShvPath::startsWithPath(prop_path, path.asString())) {
					return true;
				}
			}
		}
		else if(black_list.isMap()) {
			for(const auto &[path, val] : black_list.asMap()) {
				if(shv::core::utils::ShvPath::startsWithPath(prop_path, path)) {
					return true;
				}
			}
		}
	}
	return false;
}

ShvTypeInfo &ShvTypeInfo::setDevicePath(const std::string &device_path, const std::string &device_type)
{
	m_devicePaths[device_path] = device_type;
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setNodeDescription(const ShvLogNodeDescr &node_descr, const std::string &node_path, const std::string &device_type)
{
	string path = node_path;
	if(!device_type.empty()) {
		path = path.empty()? device_type: device_type + '/' + path;
	}
	m_nodeDescriptions[path] = node_descr;
	return *this;
}

ShvTypeInfo &ShvTypeInfo::setExtraTags(const std::string &node_path, const chainpack::RpcValue &tags)
{
	if(tags.isMap())
		m_extraTags[node_path] = tags;
	else
		m_extraTags.erase(node_path);
	return *this;
}

RpcValue ShvTypeInfo::extraTags(const std::string &node_path) const
{
	if(auto it = m_extraTags.find(node_path); it == m_extraTags.cend()) {
		return {};
	}
	else {
		return it->second;
	}
}

ShvTypeInfo &ShvTypeInfo::setTypeDescription(const ShvTypeDescr &type_descr, const std::string &type_name)
{
	if(type_descr.isValid())
		m_types[type_name] = type_descr;
	else
		m_types.erase(type_name);
	return *this;
}

ShvTypeInfo::PathInfo ShvTypeInfo::pathInfo(const std::string &shv_path) const
{
	auto cut_field_path = [](const string &property_path, const string &field_path) {
		if(!field_path.empty()) {
			auto property_path_len = property_path.size() - field_path.size();
			if(property_path_len > 0)
				property_path_len--; // remove also slash
			return property_path.substr(0, property_path_len);
		}
		return property_path;
	};
	PathInfo ret;
	ret.nodeDescription = findNodeDescription(shv_path, &ret.fieldPath);
	if(ret.nodeDescription.isValid()) {
		// path found in nodes directly
		ret.deviceType = findDeviceType(shv_path, &ret.propertyPath);
		if(ret.deviceType.empty()) {
			// not device node
			ret.propertyPath = cut_field_path(shv_path, ret.fieldPath);
		}
		else {
			// device path override
			ret.devicePath = cut_field_path(shv_path, ret.propertyPath);
			ret.propertyPath = cut_field_path(ret.propertyPath, ret.fieldPath);
		}
	}
	else {
		// try find tath in devices
		ret.deviceType = findDeviceType(shv_path, &ret.propertyPath);
		if(ret.deviceType.empty()) {
			// not device node
			ret = {};
		}
		else {
			// device path found
			ret.devicePath = cut_field_path(shv_path, ret.propertyPath);
			auto nodes_path = shv::core::Utils::joinPath(ret.deviceType, ret.propertyPath);
			ret.nodeDescription = findNodeDescription(nodes_path, &ret.fieldPath);
			if(ret.nodeDescription.isValid()) {
				ret.propertyPath = cut_field_path(ret.propertyPath, ret.fieldPath);
			}
		}
	}
	return ret;
}
/*
ShvLogNodeDescr ShvTypeInfo::nodeDescriptionForDevice(const std::string &device_type, const string &property_path, string *p_field_name) const
{
	string property = property_path.empty()? device_type: device_type + '/' + property_path;
	if(auto it = find_longest_prefix(m_nodeDescriptions, property); it == m_nodeDescriptions.cend()) {
		return {};
	}
	else {
		string prefix = it->first;
		const ShvLogNodeDescr &node_descr = it->second;
		if(p_field_name)
			*p_field_name = String(property).mid(prefix.size() + 1);
		return node_descr;
	}
}
*/
ShvLogNodeDescr ShvTypeInfo::nodeDescriptionForPath(const std::string &shv_path, string *p_field_name) const
{
	auto info = pathInfo(shv_path);
	if(p_field_name)
		*p_field_name = info.fieldPath;
	return info.nodeDescription;
}

string ShvTypeInfo::findDeviceType(const std::string &shv_path, std::string *p_property_path) const
{
	if(auto it = find_longest_prefix(m_devicePaths, shv_path); it == m_devicePaths.cend()) {
		return {};
	}
	else {
		string prefix = it->first;
		string device_type = it->second;
		if(p_property_path)
			*p_property_path = String(shv_path).mid(prefix.size() + 1);
		return device_type;
	}
}

ShvTypeDescr ShvTypeInfo::typeDescriptionForName(const std::string &type_name) const
{
	if(auto it = m_types.find(type_name); it == m_types.end())
		return ShvTypeDescr(type_name);
	else
		return it->second;
}

ShvTypeDescr ShvTypeInfo::typeDescriptionForPath(const std::string &shv_path) const
{
	return typeDescriptionForName(nodeDescriptionForPath(shv_path).typeName());
}

RpcValue ShvTypeInfo::extraTagsForPath(const std::string &shv_path) const
{
	if(auto it = m_extraTags.find(shv_path); it == m_extraTags.end())
		return {};
	else
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
static const char *TYPES = "types";
static const char *DEVICES = "devices";
static const char *NODES = "nodes";
static const char *PATHS = "paths";
static const char *EXTRA_TAGS = "extraTags";
static const char *SYSTEM_PATHS_ROOTS = "systemPathsRoots";

RpcValue ShvTypeInfo::toRpcValue() const
{
	RpcValue::Map map;
	map[TYPES] = typesAsRpcValue();
	{
		RpcValue::Map m;
		for(const auto &kv : m_devicePaths) {
			m[kv.first] = kv.second;
		}
		map[DEVICES] = std::move(m);
	}
	{
		RpcValue::Map m;
		for(const auto &kv : m_nodeDescriptions) {
			m[kv.first] = kv.second.toRpcValue();
		}
		map[NODES] = std::move(m);
	}
	map[EXTRA_TAGS] = m_extraTags;
	{
		RpcValue::Map m;
		for(const auto &[key, val] : m_systemPathsRoots) {
			m[key] = val;
		}
		map[SYSTEM_PATHS_ROOTS] = std::move(m);
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
			const RpcValue::Map &m = map.value(DEVICES).asMap();
			for(const auto &kv : m) {
				ret.m_devicePaths[kv.first] = kv.second.asString();
			}
		}
		{
			const RpcValue::Map &m = map.value(NODES).asMap();
			for(const auto &kv : m) {
				ret.m_nodeDescriptions[kv.first] = ShvLogNodeDescr::fromRpcValue(kv.second);
			}
		}
		ret.m_extraTags = map.value(EXTRA_TAGS).asMap();
		{
			const RpcValue::Map &m = map.value(SYSTEM_PATHS_ROOTS).asMap();
			for(const auto &[key, val] : m) {
				ret.m_systemPathsRoots[key] = val.asString();
			}
		}
		return ret;
	}
	else if(map.hasKey(PATHS) && map.hasKey(TYPES)) {
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
				auto nd = ShvLogNodeDescr::fromRpcValue(val);
				nd.setTypeName(val.asMap().value("type").asString());
				ret.m_nodeDescriptions[path] = nd;
			}
		}
		return ret;
	}
	else {
		return fromNodesTree(v);
	}
}

RpcValue ShvTypeInfo::applyTypeDescription(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums) const
{
	ShvTypeDescr td = typeDescriptionForName(type_name);
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
		else {
			return ival;
		}
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
	shvError() << "Invalid type:" << (int)type_descr.type();
	return val;
}

void ShvTypeInfo::forEachDeviceProperty(const std::string &device_path, std::function<void (const std::string &, const ShvLogNodeDescr &)> fn) const
{
	if(auto it = m_devicePaths.find(device_path); it == m_devicePaths.end()) {
		// device path invalid
		return;
	}
	else {
		auto device_type = it->second;
		ShvPath::forEachDirAndSubdirs(nodeDescriptions(), device_type, [=](auto it_nd) {
			string property_path = it->first;
			property_path = property_path.substr(device_type.size());
			if(property_path.size() > 0 && property_path[0] == '/')
				property_path = property_path.substr(1);
			string full_path = shv::core::Utils::joinPath(device_path, property_path);
			if(auto it2 = m_nodeDescriptions.find(full_path); it2 == m_nodeDescriptions.end()) {
				// overlay does not exist
				const ShvLogNodeDescr &node_descr = it_nd->second;
				fn(property_path, node_descr);
			}
			else {
				const ShvLogNodeDescr &node_descr = it2->second;
				fn(property_path, node_descr);
			}
		});
	}
}

void ShvTypeInfo::forEachNode(std::function<void (const std::string &shv_path, const ShvLogNodeDescr &node_descr)> fn) const
{
	map<string, vector<string>> device_type_to_path;
	for(const auto& [path, device_id] : m_devicePaths)
		device_type_to_path[device_id].push_back(path);
	for(const auto& [path, node_descr] : m_nodeDescriptions) {
		string device_type;
		if(auto ix = path.find('/'); ix != string::npos) {
			device_type = path.substr(0, ix);
			string property_id = path.substr(ix + 1);
			if(auto it = device_type_to_path.find(device_type); it == device_type_to_path.end()) {
				device_type = {};
			}
			else {
				// device paths
				for(const auto &device_path : it->second) {
					string shv_path = shv::core::Utils::joinPath(device_path, property_id);
					fn(shv_path, node_descr);
				}
			}
		}
		if(device_type.empty()) {
			// direct node path
			fn(path, node_descr);
		}
	}
}

ShvNodeDescr ShvTypeInfo::findNodeDescription(const std::string &path, std::string *p_field_name) const
{
	//string property = property_path.empty()? device_type: device_type + '/' + property_path;
	if(auto it = find_longest_prefix(m_nodeDescriptions, path); it == m_nodeDescriptions.cend()) {
		return {};
	}
	else {
		string prefix = it->first;
		const ShvLogNodeDescr &node_descr = it->second;
		if(p_field_name)
			*p_field_name = String(path).mid(prefix.size() + 1);
		return node_descr;
	}
}

void ShvTypeInfo::fromNodesTree_helper(const shv::chainpack::RpcValue &node,
						  const std::string &shv_path,
						  const std::string &recent_device_type,
						  const std::string &recent_device_path,
						  const RpcValue::Map &node_types)
{
	using namespace shv::core::utils;
	using namespace shv::core;
	if(!node.isValid())
		return;

	if(auto ref = node.metaValue("nodeTypeRef").asString(); !ref.empty()) {
		fromNodesTree_helper(node_types.value(ref), shv_path, recent_device_type, recent_device_path, node_types);
		return;
	}

	string current_device_type = recent_device_type;
	string current_device_path = recent_device_path;
	RpcValue::Map property_descr;
	RpcValue::List property_methods;
	static const string CREATE_FROM_TYPE_NAME = "createFromTypeName";
	static const string STATUS = "status";
	static const string SYSTEM_PATH = "systemPath";
	//shvInfo() << "id:" << node->nodeId() << "node path:" << node_shv_path << "shv path:" << shv_path;
	//StringViewList node_shv_dir_list = ShvPath::splitPath(node_shv_path);
	for(const RpcValue &rv : node.metaValue("methods").asList()) {
		const auto mm = MetaMethod::fromRpcValue(rv);
		const string &method_name = mm.name();
		if(method_name == Rpc::METH_LS || method_name == Rpc::METH_DIR)
			continue;
		property_methods.push_back(mm.toRpcValue());
	}
	const RpcValue::Map &node_tags = node.metaValue(KEY_TAGS).asMap();
	if(!node_tags.empty()) {
		RpcValue::Map tags_map = node_tags;
		const string &dtype = tags_map.value(KEY_DEVICE_TYPE).asString();
		if(!dtype.empty()) {
			current_device_type = dtype;
			current_device_path = shv_path;
		}
		property_descr.merge(tags_map);
	}
	property_descr.erase(CREATE_FROM_TYPE_NAME); // erase shvgate obsolete tag
	if(!property_methods.empty())
		property_descr[KEY_METHODS] = property_methods;
	if(!property_descr.empty()) {
		RpcValue::Map extra_tags;
		auto node_descr = shv::core::utils::ShvLogNodeDescr::fromRpcValue(property_descr, &extra_tags);
		if(node_descr.isValid()) {
			if(current_device_type.empty()) {
				setNodeDescription(node_descr, shv_path, {});
			}
			else {
				string node_path = String(shv_path).mid(current_device_path.size() + 1);
				string path_rest;
				auto existing_node_descr = findNodeDescription(shv::core::Utils::joinPath(current_device_type, node_path), &path_rest);
				if(existing_node_descr.isValid() && path_rest.empty()) {
					if(existing_node_descr == node_descr) {
						// node descriptions are the same, no action is needed
					}
					else {
						// node descriptions are different, we must create overlay node descr
						setNodeDescription(node_descr, shv_path, {});
					}
				}
				else {
					// node description does not exist, create new one
					setNodeDescription(node_descr, node_path, current_device_type);
				}
			}
		}
		auto system_path = extra_tags.take(SYSTEM_PATH).asString();
		if(!system_path.empty()) {
			m_systemPathsRoots[shv_path] = system_path;
		}
		if(!extra_tags.empty()) {
			setExtraTags(shv_path, extra_tags);
		}
	}
	if(current_device_type != recent_device_type) {
		setDevicePath(current_device_path, current_device_type);
	}
	for (const auto& [child_name, child_node] : node.asMap()) {
		if(child_name.empty())
			continue;
		// skip children of nodes created from typeName
		if(node_tags.hasKey(CREATE_FROM_TYPE_NAME))
			continue;
		ShvPath child_shv_path = shv::core::Utils::joinPath(shv_path, child_name);
		fromNodesTree_helper(child_node, child_shv_path, current_device_type, current_device_path, node_types);
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
	ret.fromNodesTree_helper(v, {}, {}, {}, node_types.asMap());
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
