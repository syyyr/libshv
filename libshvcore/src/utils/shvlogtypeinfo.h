#ifndef SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
#define SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H

#include "../shvcoreglobal.h"

#include <shv/chainpack/datachange.h>
//#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <variant>

namespace shv {
namespace core {
namespace utils {

class SHVCORE_DECL_EXPORT ShvLogDescrBase
{
public:
	ShvLogDescrBase() {}
	ShvLogDescrBase(const chainpack::RpcValue &v) : m_data(v) {}

	bool isValid() const { return m_data.isMap(); }
protected:
	chainpack::RpcValue dataValue(const std::string &key, const chainpack::RpcValue &default_val = {}) const;
	void setDataValue(const std::string &key, const chainpack::RpcValue &val);
	//const chainpack::RpcValue::Map& asMap() const { return m_data.asMap(); }
protected:
	chainpack::RpcValue m_data;
};

class SHVCORE_DECL_EXPORT ShvLogTypeDescrField : public ShvLogDescrBase
{
	using Super = ShvLogDescrBase;
public:
	ShvLogTypeDescrField() {}
	//ShvLogTypeDescrField(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS
	//ShvLogTypeDescrField(const std::string &n)
	//	: ShvLogTypeDescrField(n, std::string(), chainpack::RpcValue(), std::string()) {}
	//ShvLogTypeDescrField(const std::string &name, int bit_pos)
	//	: ShvLogTypeDescrField(n, std::string(), bit_pos, d) {}
	//ShvLogTypeDescrField(const std::string &n, const std::string &type_name)
	//	: ShvLogTypeDescrField(n, type_name, chainpack::RpcValue(), std::string()) {}
	ShvLogTypeDescrField(const std::string &n, const std::string &type_name = std::string(), const chainpack::RpcValue &v = chainpack::RpcValue());

	std::string name() const;
	std::string typeName() const;
	std::string label() const;
	std::string description() const;
	chainpack::RpcValue value() const;
	std::string alarm() const;
	int alarmLevel() const;

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescrField fromRpcValue(const chainpack::RpcValue &v);
};

class SHVCORE_DECL_EXPORT ShvLogTypeDescr : public ShvLogDescrBase
{
	using Super = ShvLogDescrBase;
public:
	enum class Type {
		Invalid,
		BitField,
		Enum,
		Bool,
		UInt,
		Int,
		Decimal,
		Double,
		String,
		DateTime,
		List,
		Map,
		IMap,
	};
	enum class SampleType {Invalid = 0, Continuous , Discrete};

	ShvLogTypeDescr() {}
	//ShvLogTypeDescr(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS
	//ShvLogTypeDescr(const std::string &type_name) : type(typeFromString(type_name)) { }
	ShvLogTypeDescr(Type t, SampleType st = SampleType::Continuous)
		: ShvLogTypeDescr(t, std::vector<ShvLogTypeDescrField>(), st) {}
	ShvLogTypeDescr(Type t, std::vector<ShvLogTypeDescrField> &&flds, SampleType st = SampleType::Continuous);

	Type type() const;
	ShvLogTypeDescr& setType(Type t);
	std::vector<ShvLogTypeDescrField> fields() const;
	SampleType sampleType() const; // = SampleType::Continuous;

	//ShvLogTypeDescr& withTags(const chainpack::RpcValue::Map &tags) { this->tags = tags; return *this; }
	ShvLogTypeDescr& withType(Type t);

	bool isValid() const { return type() != Type::Invalid; }

	static const std::string typeToString(Type t);
	static Type typeFromString(const std::string &s);

	static const std::string sampleTypeToString(SampleType t);
	static SampleType sampleTypeFromString(const std::string &s);

	//void applyTags(const chainpack::RpcValue::Map &t);
	//chainpack::RpcValue defaultRpcValue() const;

	//std::string unit() const;
	std::string visualStyleName() const;
	std::string alarm() const;
	int alarmLevel() const;
	int decimalPlaces() const;
	ShvLogTypeDescr& setDecimalPlaces(int n);

	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeDescr fromRpcValue(const chainpack::RpcValue &v);
};

class SHVCORE_DECL_EXPORT ShvLogPathDescr : public ShvLogDescrBase
{
	using Super = ShvLogDescrBase;
public:
	ShvLogPathDescr() {}
	//ShvLogPathDescr(const chainpack::RpcValue &v) : Super(v) {} DANGEROUS

	std::string typeName() const;
	std::string label() const;
	std::string description() const;
	std::string unit() const;
	chainpack::RpcValue tags() const;

	chainpack::RpcValue toRpcValue() const;
	static ShvLogPathDescr fromRpcValue(const chainpack::RpcValue &v);
	//static ShvLogPathDescr fromTags(const chainpack::RpcValue::Map &tags);
};

class SHVCORE_DECL_EXPORT ShvLogTypeInfo
{
public:
	ShvLogTypeInfo() {}
	ShvLogTypeInfo(std::map<std::string, ShvLogTypeDescr> &&types, std::map<std::string, ShvLogPathDescr> &&paths)
		: m_types(std::move(types))
		, m_propertyPaths(std::move(paths))
	{}

	bool isEmpty() const { return m_types.size() == 0 && m_propertyPaths.size() == 0; }
	//const std::map<std::string, ShvLogTypeDescr>& types() const { return m_types; }

	ShvLogTypeInfo& addDevicePath(const std::string &device_path, const std::string &device_type);
	ShvLogTypeInfo& addPathDescription(const ShvLogPathDescr &path_descr, const std::string &property_path, const std::string &device_type = {} );
	ShvLogTypeInfo& addTypeDescription(const ShvLogTypeDescr &type_descr, const std::string &type_name);
	ShvLogPathDescr pathDescriptionForPath(const std::string &shv_path) const;
	ShvLogTypeDescr typeDescriptionForPath(const std::string &shv_path) const;
	ShvLogTypeDescr typeDescriptionForName(const std::string &type_name) const;
	std::string findSystemPath(const std::string &shv_path) const;

	chainpack::RpcValue typesAsRpcValue() const;
	chainpack::RpcValue toRpcValue() const;
	static ShvLogTypeInfo fromRpcValue(const chainpack::RpcValue &v);

	shv::chainpack::RpcValue applyType(const shv::chainpack::RpcValue &val, const std::string &type_name) const;
private:
	std::map<std::string, ShvLogTypeDescr> m_types; // type_name -> type_description
	std::map<std::string, std::string> m_devicePaths; // path -> device
	std::map<std::string, ShvLogPathDescr> m_propertyPaths; // device_property_path -> path_descr
	std::map<std::string, std::string> m_systemPathsRoots;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGTYPEDESCRIPTION_H
