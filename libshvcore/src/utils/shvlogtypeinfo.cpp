#include "shvlogtypeinfo.h"
#include "../string.h"
#include "../log.h"

using namespace std;
using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

static const char* KEY_LABEL = "label";
static const char* KEY_DESCRIPTION = "description";
static const char* KEY_UNIT = "unit";
static const char* KEY_NAME = "name";
static const char* KEY_TYPE_NAME = "typeName";
static const char* KEY_TYPE = "type";
static const char* KEY_VALUE = "value";
static const char* KEY_FIELDS = "fields";
static const char* KEY_SAMPLE_TYPE = "sampleType";
static const char* KEY_TAGS = "tags";
//static const char* KEY_MIN_VAL = "minVal";
//static const char* KEY_MAX_VAL = "maxVal";
static const char* KEY_DEC_PLACES = "decPlaces";
static const char* KEY_VISUAL_STYLE = "visualStyle";
static const char* KEY_ALARM = "alarm";
static const char* KEY_ALARM_LEVEL = "alarmLevel";

//=====================================================================
// ShvLogDescrBase
//=====================================================================
RpcValue ShvLogDescrBase::dataValue(const std::string &key, const chainpack::RpcValue &default_val) const
{
	return m_data.asMap().value(key, default_val);
}

void ShvLogDescrBase::setDataValue(const std::string &key, const chainpack::RpcValue &val)
{
	if(!m_data.isMap()) {
		m_data = RpcValue::Map();
	}
	m_data.set(key, val);
}

void ShvLogDescrBase::setData(const chainpack::RpcValue &data)
{
	if(data.isMap()) {
		RpcValue::Map map = data.asMap();
		mergeTags(map);
		m_data = map;
	}
}

void ShvLogDescrBase::mergeTags(chainpack::RpcValue::Map &map)
{
	auto nh = map.extract("tags");
	if(!nh.empty()) {
		RpcValue::Map tags = nh.mapped().asMap();
		map.merge(tags);
	}
}
//=====================================================================
// ShvLogTypeDescrField
//=====================================================================
ShvLogTypeDescrField::ShvLogTypeDescrField(const std::string &name, const std::string &type_name, const chainpack::RpcValue &value)
{
	setDataValue(KEY_NAME, name);
	setDataValue(KEY_TYPE_NAME, type_name);
	setDataValue(KEY_VALUE, value);
}

string ShvLogTypeDescrField::name() const
{
	return dataValue(KEY_NAME).asString();
}

string ShvLogTypeDescrField::typeName() const
{
	return dataValue(KEY_TYPE_NAME).asString();
}

string ShvLogTypeDescrField::label() const
{
	return dataValue(KEY_LABEL).asString();
}

string ShvLogTypeDescrField::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

RpcValue ShvLogTypeDescrField::value() const
{
	return dataValue(KEY_VALUE);
}

string ShvLogTypeDescrField::alarm() const
{
	return dataValue(KEY_ALARM).asString();
}

int ShvLogTypeDescrField::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

RpcValue ShvLogTypeDescrField::toRpcValue() const
{
	RpcValue ret = m_data;
	if(description().empty())
		ret.set(KEY_DESCRIPTION, {});
	return ret;
}

ShvLogTypeDescrField ShvLogTypeDescrField::fromRpcValue(const RpcValue &v)
{
	ShvLogTypeDescrField ret;
	ret.setData(v);
	return ret;
}

//=====================================================================
// ShvLogTypeDescr
//=====================================================================
ShvLogTypeDescr::Type ShvLogTypeDescr::type() const
{
	return static_cast<Type>(dataValue(KEY_TYPE).toInt());
}

ShvLogTypeDescr &ShvLogTypeDescr::setType(Type t)
{
	setDataValue(KEY_TYPE, (int)t);
	return *this;
}

std::vector<ShvLogTypeDescrField> ShvLogTypeDescr::fields() const
{
	std::vector<ShvLogTypeDescrField> ret;
	for(const RpcValue &rv : dataValue(KEY_FIELDS).asList())
		ret.push_back(ShvLogTypeDescrField::fromRpcValue(rv));
	return ret;
}

ShvLogTypeDescr::SampleType ShvLogTypeDescr::sampleType() const
{
	return static_cast<SampleType>(dataValue(KEY_SAMPLE_TYPE).toInt());
}

const std::string ShvLogTypeDescr::typeToString(ShvLogTypeDescr::Type t)
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

ShvLogTypeDescr::Type ShvLogTypeDescr::typeFromString(const std::string &s)
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

const std::string ShvLogTypeDescr::sampleTypeToString(ShvLogTypeDescr::SampleType t)
{
	switch (t) {
	case SampleType::Discrete: return "Discrete";
	case SampleType::Continuous: return "Continuous";
	case SampleType::Invalid: return "Invalid";
	}
	return std::string();
}

ShvLogTypeDescr::SampleType ShvLogTypeDescr::sampleTypeFromString(const std::string &s)
{
	if(s == "2" || s == "D" || s == "Discrete" || s == "discrete")
		return SampleType::Discrete;
	if(s.empty() || s == "C" || s == "Continuous" || s == "continuous")
		return SampleType::Continuous;
	return SampleType::Invalid;
}
/*
void ShvLogTypeDescr::applyTags(const RpcValue::Map &t)
{
	if (type == Type::Invalid)
		type = typeFromString(t.value("typeName").toStdString());

	if (t.hasKey("sampleType"))
		sampleType = sampleTypeFromString(t.value("sampleType").asString());

	//if (t.hasKey(description))
	//	description = t.value(KEY_DESCRIPTION).toStdString();

	tags.insert(t.begin(), t.end());
}

RpcValue ShvLogTypeDescr::defaultRpcValue() const
{
	using namespace chainpack;

	switch (type) {
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
*/
std::string ShvLogTypeDescr::visualStyleName() const
{
	return dataValue(KEY_VISUAL_STYLE, std::string()).asString();
}

std::string ShvLogTypeDescr::alarm() const
{
	return dataValue(KEY_ALARM, std::string()).asString();
}

int ShvLogTypeDescr::alarmLevel() const
{
	return dataValue(KEY_ALARM_LEVEL).toInt();
}

int ShvLogTypeDescr::decimalPlaces() const
{
	return dataValue(KEY_DEC_PLACES, 0).toInt();
}

ShvLogTypeDescr &ShvLogTypeDescr::setDecimalPlaces(int n)
{
	setDataValue(KEY_DEC_PLACES, n);
	return *this;
}

RpcValue ShvLogTypeDescr::toRpcValue() const
{
	RpcValue::Map map = m_data.asMap();
	map.setValue(KEY_TYPE, typeToString(type()));
	if(sampleType() != ShvLogTypeDescr::SampleType::Invalid)
		map.setValue(KEY_SAMPLE_TYPE, sampleTypeToString(sampleType()));
	else
		map.setValue(KEY_SAMPLE_TYPE, {});
	return map;
}

ShvLogTypeDescr ShvLogTypeDescr::fromRpcValue(const RpcValue &v)
{
	ShvLogTypeDescr ret;
	{
		RpcValue::Map map = v.asMap();
		auto nd = map.extract("fields");
		RpcValue::List fields;
		for(const auto &rv : nd.mapped().asList()) {
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

string ShvLogNodeDescr::label() const
{
	return dataValue(KEY_LABEL).asString();
}

string ShvLogNodeDescr::description() const
{
	return dataValue(KEY_DESCRIPTION).asString();
}

string ShvLogNodeDescr::unit() const
{
	return dataValue(KEY_UNIT).asString();
}

RpcValue ShvLogNodeDescr::tags() const
{
	return dataValue(KEY_TAGS);
}
/*
ShvLogNodeDescr ShvLogNodeDescr::fromTags(const RpcValue::Map &tags)
{
	ShvLogNodeDescr ret;
	ret.m_data = tags;
	//ret.typeName = ret.tags.take(KEY_TYPE_NAME).asString();
	//ret.description = ret.tags.take(KEY_DESCRIPTION).asString();
	return ret;
}
*/
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

ShvLogNodeDescr ShvLogNodeDescr::fromRpcValue(const RpcValue &v)
{
	ShvLogNodeDescr ret;
	ret.setData(v);
	return ret;
}

//=====================================================================
// ShvLogTypeInfo
//=====================================================================
map<string, string>::const_iterator find_longest_prefix(const map<string, string> &map, const string &path)
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

ShvLogTypeInfo &ShvLogTypeInfo::addDevicePath(const std::string &device_path, const std::string &device_type)
{
	m_devicePaths[device_path] = device_type;
	return *this;
}

ShvLogTypeInfo &ShvLogTypeInfo::addNodeDescription(const ShvLogNodeDescr &node_descr, const std::string &node_path, const std::string &device_type)
{
	string path = node_path;
	if(!device_type.empty()) {
		path = path.empty()? device_type: device_type + '/' + path;
	}
	m_nodeDescriptions[path] = node_descr;
	return *this;
}

ShvLogTypeInfo &ShvLogTypeInfo::addTypeDescription(const ShvLogTypeDescr &type_descr, const std::string &type_name)
{
	if(type_descr.isValid())
		m_types[type_name] = type_descr;
	else
		m_types.erase(type_name);
	return *this;
}

ShvLogNodeDescr ShvLogTypeInfo::nodeDescriptionForPath(const std::string &shv_path) const
{
	if (auto it_path_descr = m_nodeDescriptions.find(shv_path); it_path_descr == m_nodeDescriptions.cend()) {
		if(auto it = find_longest_prefix(m_devicePaths, shv_path); it == m_devicePaths.cend()) {
			return {};
		}
		else {
			string prefix = it->first;
			string device = it->second;
			string property = String(shv_path).mid(prefix.size() + 1);
			string property_path = property.empty()? device: device + '/' + property;
			if(it_path_descr = m_nodeDescriptions.find(property_path); it_path_descr == m_nodeDescriptions.cend()) {
				return {};
			}
			else {
				return it_path_descr->second;
			}
		}
	}
	else {
		return it_path_descr->second;
	}
}

ShvLogTypeDescr ShvLogTypeInfo::typeDescriptionForPath(const std::string &shv_path) const
{
	ShvLogTypeDescr ret;
	auto it_path_descr = m_nodeDescriptions.find(shv_path);
	if (it_path_descr == m_nodeDescriptions.end()) {
		auto it = find_longest_prefix(m_devicePaths, shv_path);
		if(it == m_devicePaths.end()) {
			return ret;
		}
		else {
			string prefix = it->first;
			string device = it->second;
			String property = shv_path;
			property = property.mid(prefix.size() + 1);
			it_path_descr = m_nodeDescriptions.find(device + '/' + property);
		}
	}

	auto it_type_descr = m_types.find(it_path_descr->second.typeName());
	if (it_type_descr != m_types.end())
		ret = it_type_descr->second;

	//ret.applyTags(it_path_descr->second.data);
	return ret;
}

ShvLogTypeDescr ShvLogTypeInfo::typeDescriptionForName(const std::string &type_name) const
{
	if(auto it = m_types.find(type_name); it == m_types.end())
		return {};
	else
		return it->second;
}

std::string ShvLogTypeInfo::findSystemPath(const std::string &shv_path) const
{
	string current_root;
	string ret;
	for(const auto &kv : m_systemPathsRoots) {
		if(String::startsWith(shv_path, kv.first)) {
			if(shv_path.size() == kv.first.size() || shv_path[kv.first.size()] == '/') {
				if(kv.first.size() > current_root.size()) {
					// fing longest match
					current_root = kv.first;
					ret = kv.second;
				}
			}
		}
	}
	return ret;
}

RpcValue ShvLogTypeInfo::typesAsRpcValue() const
{
	RpcValue::Map m;
	for(const auto &kv : m_types) {
		m[kv.first] = kv.second.toRpcValue();
	}
	return m;
}

RpcValue ShvLogTypeInfo::toRpcValue() const
{
	RpcValue::Map map;
	map["types"] = typesAsRpcValue();
	{
		RpcValue::Map m;
		for(const auto &kv : m_devicePaths) {
			m[kv.first] = kv.second;
		}
		map["devices"] = std::move(m);
	}
	{
		RpcValue::Map m;
		for(const auto &kv : m_nodeDescriptions) {
			m[kv.first] = kv.second.toRpcValue();
		}
		map["nodes"] = std::move(m);
	}
	RpcValue ret = map;
	ret.setMetaValue("version", 3);
	return ret;
}

ShvLogTypeInfo ShvLogTypeInfo::fromRpcValue(const RpcValue &v)
{
	int version = v.metaValue("version").toInt();
	if(version != 3) {
		shvError() << "Type info version:" << version << "is not supported.";
		return {};
	}
	ShvLogTypeInfo ret;
	const RpcValue::Map &map = v.asMap();
	{
		const RpcValue::Map &m = map.value("types").asMap();
		for(const auto &kv : m) {
			ret.m_types[kv.first] = ShvLogTypeDescr::fromRpcValue(kv.second);
		}
	}
	{
		const RpcValue::Map &m = map.value("devices").asMap();
		for(const auto &kv : m) {
			ret.m_devicePaths[kv.first] = kv.second.asString();
		}
	}
	{
		const RpcValue::Map &m = map.value("nodes").asMap();
		for(const auto &kv : m) {
			ret.m_nodeDescriptions[kv.first] = ShvLogNodeDescr::fromRpcValue(kv.second);
		}
	}
	return ret;
}

RpcValue ShvLogTypeInfo::applyType(const shv::chainpack::RpcValue &val, const std::string &type_name, bool translate_enums) const
{
	ShvLogTypeDescr td = typeDescriptionForName(type_name);
	//shvWarning() << type_name << "--->" << td.toRpcValue().toCpon();
	switch(td.type()) {
	case ShvLogTypeDescr::Type::Invalid:
		return val;
	case ShvLogTypeDescr::Type::BitField: {
		RpcValue::Map map;
		uint64_t uval = val.toUInt64();
		for(const ShvLogTypeDescrField &fld : td.fields()) {
			unsigned bit_no1 = 0;
			unsigned bit_no2 = 0;
			if(fld.value().isList()) {
				const auto &lst = fld.value().asList();
				bit_no1 = lst.value(0).toUInt();
				bit_no2 = lst.value(1).toUInt();
				if(bit_no2 < bit_no1) {
					shvError() << "Invalid bit specification:" << fld.value().toCpon();
					bit_no2 = bit_no1;
				}
			}
			else {
				bit_no1 = bit_no2 = fld.value().toUInt();
			}
			uint64_t mask = ~((~static_cast<uint64_t>(0)) << (bit_no2 + 1));
			shvDebug() << "bits:" << bit_no1 << bit_no2 << (bit_no1 == bit_no2) << "uval:" << uval << "mask:" << mask;
			uval = (uval & mask) >> bit_no1;
			shvDebug() << "uval masked and rotated right by:" << bit_no1 << "bits, uval:" << uval;
			RpcValue result;
			if(bit_no1 == bit_no2)
				result = static_cast<bool>(uval != 0);
			else
				result = uval;
			result = applyType(result, fld.typeName());
			map[fld.name()] = result;
		}
		return map;
	}
	case ShvLogTypeDescr::Type::Enum: {
		int ival = val.toInt();
		if(translate_enums) {
			for(const ShvLogTypeDescrField &fld : td.fields()) {
				if(fld.value().toInt() == ival)
					return fld.name();
			}
			return "UNKNOWN_" + val.toCpon();
		}
		else {
			return ival;
		}
	}
	case ShvLogTypeDescr::Type::Bool:
		return val.toBool();
	case ShvLogTypeDescr::Type::UInt:
		return val.toUInt();
	case ShvLogTypeDescr::Type::Int:
		return val.toInt();
	case ShvLogTypeDescr::Type::Decimal:
		if(val.isDecimal())
			return val;
		return RpcValue::Decimal::fromDouble(val.toDouble(), td.decimalPlaces());
	case ShvLogTypeDescr::Type::Double:
		return val.toDouble();
	case ShvLogTypeDescr::Type::String:
		if(val.isString())
			return val;
		return val.asString();
	case ShvLogTypeDescr::Type::DateTime:
		return val.toDateTime();
	case ShvLogTypeDescr::Type::List:
		if(val.isList())
			return val;
		return val.asList();
	case ShvLogTypeDescr::Type::Map:
		if(val.isMap())
			return val;
		return val.asMap();
	case ShvLogTypeDescr::Type::IMap:
		if(val.isIMap())
			return val;
		return val.asIMap();
	}
	shvError() << "Invalid type:" << (int)td.type();
	return val;
}

} // namespace utils
} // namespace core
} // namespace shv
