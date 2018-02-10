#pragma once

#include "../shvchainpackglobal.h"
#include "exception.h"
#include "metatypes.h"

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <initializer_list>

#ifndef CHAINPACK_UINT
	#define CHAINPACK_UINT unsigned
#endif
#ifndef CHAINPACK_INT
	#define CHAINPACK_INT int
#endif

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT RpcValue
{
public:
	class AbstractValueData;

	enum class Type {
		Invalid,
		Null,
		UInt,
		Int,
		Double,
		Bool,
		Blob,
		String,
		DateTime,
		List,
		Array,
		Map,
		IMap,
		Decimal,
		//MetaData,
	};
	static const char* typeToName(Type t);

	using Int = CHAINPACK_INT;
	using UInt = CHAINPACK_UINT;
	class SHVCHAINPACK_DECL_EXPORT Decimal
	{
		Int m_mantisa = 0;
		int16_t m_precision = 0;
	public:
		Decimal() : m_precision(-1) {}
		Decimal(Int mantisa, int precision) : m_mantisa(mantisa), m_precision(precision) {}
		static Decimal fromDouble(double n, int precision)
		{
			if(precision > 0) {
				//m_mantisa = (int)(n * std::pow(10, precision) + 0.5);
				for(; precision > 0; precision--) n *= 10;
			}
			return Decimal((Int)(n + 0.5), precision);
		}
		Int mantisa() const {return m_mantisa;}
		int precision() const {return m_precision;}
		double toDouble() const
		{
			double ret = mantisa();
			int prec = precision();
			if(prec > 0)
				for(; prec > 0; prec--) ret /= 10;
			else
				for(; prec < 0; prec++) ret *= 10;
			return ret;
		}
		bool isValid() const {return !(mantisa() == 0 && precision() != 0);}
		std::string toString() const;
	};
	class SHVCHAINPACK_DECL_EXPORT DateTime
	{
	public:
		// UTC msec since 2.2. 2018 folowed by signed UTC offset in 1/4 hour
		// Fri Feb 02 2018 00:00:00 == 1517529600 EPOCH
		static constexpr int64_t SHV_EPOCH_MSEC = 1517529600000;
	public:
		DateTime() {}
		int64_t msecsSinceEpoch() const { return m_dtm.msec; }
		int offsetFromUtc() const { return m_dtm.tz * 15; }
		//bool isTZSet() const { return m_dtm.tz == TZ_INVALID; }

		static DateTime fromLocalString(const std::string &local_date_time_str);
		static DateTime fromUtcString(const std::string &utc_date_time_str);
		static DateTime fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min = 0);

		std::string toLocalString() const;
		std::string toUtcString() const;

		bool operator ==(const DateTime &o) const {return m_dtm.msec == o.m_dtm.msec && m_dtm.tz == o.m_dtm.tz;}
	private:
		//static constexpr int8_t TZ_INVALID = -128;
		struct MsTz {
			int64_t tz: 7, msec: 57;
		};
		MsTz m_dtm = {0, 0};
	};
	using String = std::string;
	struct SHVCHAINPACK_DECL_EXPORT Blob : public std::basic_string<char>
	{
	private:
		using Super = std::basic_string<char>;
	public:
		using Super::Super; // expose base class constructors
		Blob() : Super() {}
		Blob(const Super &str) : Super(str) {}
		Blob(Super &&str) : Super(std::move(str)) {}
		//const std::string& toString() const {return *this;}
	};
	using List = std::vector<RpcValue>;
	class Map : public std::map<String, RpcValue>
	{
		using Super = std::map<String, RpcValue>;
		using Super::Super; // expose base class constructors
	public:
		RpcValue value(const std::string &key, const RpcValue &default_val = RpcValue()) const
		{
			auto it = find(key);
			if(it == end())
				return default_val;
			return it->second;
		}
		bool hasKey(const std::string &key) const
		{
			auto it = find(key);
			return !(it == end());
		}
	};
	using IMap = std::map<RpcValue::UInt, RpcValue>;
	union ArrayElement
	{
		Int int_value;
		UInt uint_value;
		double double_value;
		bool bool_value;
		std::nullptr_t null_value;

		ArrayElement() {}
		ArrayElement(std::nullptr_t) : null_value(nullptr) {}
		ArrayElement(int16_t i) : int_value(i) {}
		ArrayElement(int32_t i) : int_value(i) {}
		ArrayElement(int64_t i) : int_value(i) {}
		ArrayElement(uint16_t i) : uint_value(i) {}
		ArrayElement(uint32_t i) : uint_value(i) {}
		ArrayElement(uint64_t i) : uint_value(i) {}
		ArrayElement(double d) : double_value(d) {}
		ArrayElement(bool b) : bool_value(b) {}
	};
	class SHVCHAINPACK_DECL_EXPORT Array : public std::vector<ArrayElement>
	{
		using Super = std::vector<ArrayElement>;
	public:
		Array() {}
		Array(Type type) : m_type(type) {}
		template<typename T, size_t sz>
		Array(const T(&arr)[sz])
		{
			reserve(sz);
			m_type = RpcValue::guessType<T>();
			for (size_t i = 0; i < sz; ++i) {
				ArrayElement el(arr[i]);
				push_back(std::move(el));
			}
		}
		Type type() const {return m_type;}
		RpcValue valueAt(size_t ix) const
		{
			switch(type()) {
			case RpcValue::Type::Null: return RpcValue(nullptr);
			case RpcValue::Type::Int: return RpcValue(Super::at(ix).int_value);
			case RpcValue::Type::UInt: return RpcValue(Super::at(ix).uint_value);
			case RpcValue::Type::Double: return RpcValue(Super::at(ix).double_value);
			case RpcValue::Type::Bool: return RpcValue(Super::at(ix).bool_value);
			default: SHVCHP_EXCEPTION("Unsupported array type");
			}
		}
		static ArrayElement makeElement(const RpcValue &val)
		{
			ArrayElement el;
			switch(val.type()) {
			case RpcValue::Type::Null: el.null_value = nullptr; break;
			case RpcValue::Type::Int: el.int_value = val.toInt(); break;
			case RpcValue::Type::UInt: el.uint_value = val.toUInt(); break;
			case RpcValue::Type::Double: el.double_value = val.toDouble(); break;
			case RpcValue::Type::Bool: el.bool_value = val.toBool(); break;
			default: SHVCHP_EXCEPTION("Unsupported array type");
			}
			return el;
		}
	private:
		Type m_type = Type::Invalid;
	};
	class SHVCHAINPACK_DECL_EXPORT MetaData
	{
	public:
		MetaData() {}
		MetaData(MetaData &&o);
		MetaData(RpcValue::IMap &&imap);
		MetaData(RpcValue::Map &&smap);
		MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap);
		~MetaData();

		MetaData& operator =(MetaData &&o) {swap(o); return *this;}

		int metaTypeId() const {return value(meta::Tag::MetaTypeId).toInt();}
		void setMetaTypeId(RpcValue::Int id) {setValue(meta::Tag::MetaTypeId, id);}
		int metaTypeNameSpaceId() const {return value(meta::Tag::MetaTypeNameSpaceId).toInt();}
		void setMetaTypeNameSpaceId(RpcValue::Int id) {setValue(meta::Tag::MetaTypeNameSpaceId, id);}
		std::vector<RpcValue::UInt> iKeys() const;
		std::vector<RpcValue::String> sKeys() const;
		RpcValue value(RpcValue::UInt key) const;
		RpcValue value(RpcValue::String key) const;
		void setValue(RpcValue::UInt key, const RpcValue &val);
		void setValue(RpcValue::String key, const RpcValue &val);
		bool isEmpty() const;
		bool operator==(const MetaData &o) const;
		const RpcValue::IMap& iValues() const;
		const RpcValue::Map& sValues() const;
		std::string toStdString() const;
	private:
		MetaData(const MetaData &o);
		MetaData& operator =(const MetaData &o);
		void swap(MetaData &o);
	private:
		RpcValue::IMap *m_imap = nullptr;
		RpcValue::Map *m_smap = nullptr;
	};

	// Constructors for the various types of JSON value.
	RpcValue() noexcept;                // Null
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue(const RpcValue &other) noexcept : m_ptr(other.m_ptr) {}
	RpcValue(RpcValue &&other) noexcept : RpcValue() { swap(other); }
#endif
	RpcValue(std::nullptr_t) noexcept;  // Null
	RpcValue(bool value);               // Bool
	RpcValue(Int value);                // Int
	RpcValue(UInt value);                // UInt
	//RpcValue(int value) : RpcValue((Int)value) {}
	RpcValue(uint16_t value) : RpcValue((UInt)value) {}
	//RpcValue(unsigned int value) : RpcValue((UInt)value) {}
	RpcValue(double value);             // Double
	RpcValue(Decimal value);             // Decimal
	RpcValue(const DateTime &value);
	RpcValue(const Blob &value); // Blob
	RpcValue(Blob &&value);
	RpcValue(const uint8_t *value, size_t size);
	RpcValue(const std::string &value); // String
	RpcValue(std::string &&value);      // String
	RpcValue(const char *value);       // String
	RpcValue(const List &values);      // List
	RpcValue(List &&values);           // List
	RpcValue(const Array &values);
	RpcValue(Array &&values);
	RpcValue(const Map &values);     // Map
	RpcValue(Map &&values);          // Map
	RpcValue(const IMap &values);     // IMap
	RpcValue(IMap &&values);          // IMap

	// Implicit constructor: anything with a to_json() function.
	template <class T, class = decltype(&T::to_json)>
	RpcValue(const T & t) : RpcValue(t.to_json()) {}

	// Implicit constructor: map-like objects (std::map, std::unordered_map, etc)
	template <class M, typename std::enable_if<
				  std::is_constructible<RpcValue::String, typename M::key_type>::value
				  && std::is_constructible<RpcValue, typename M::mapped_type>::value,
				  int>::type = 0>
	RpcValue(const M & m) : RpcValue(Map(m.begin(), m.end())) {}

	// Implicit constructor: vector-like objects (std::list, std::vector, std::set, etc)
	template <class V, typename std::enable_if<
				  std::is_constructible<RpcValue, typename V::value_type>::value,
				  int>::type = 0>
	RpcValue(const V & v) : RpcValue(List(v.begin(), v.end())) {}

	// This prevents ChainPack(some_pointer) from accidentally producing a bool. Use
	// ChainPack(bool(some_pointer)) if that behavior is desired.
	RpcValue(void *) = delete;

	~RpcValue();

	Type type() const;
	Type arrayType() const;

	const MetaData &metaData() const;
	RpcValue metaValue(RpcValue::UInt key) const;
	RpcValue metaValue(const RpcValue::String &key) const;
	void setMetaData(MetaData &&meta_data);
	void setMetaValue(UInt key, const RpcValue &val);
	void setMetaValue(const String &key, const RpcValue &val);

	template<typename T> static Type guessType();

	bool isValid() const;
	bool isNull() const { return type() == Type::Null; }
	bool isInt() const { return type() == Type::Int; }
	bool isDateTime() const { return type() == Type::DateTime; }
	bool isUInt() const { return type() == Type::UInt; }
	bool isDouble() const { return type() == Type::Double; }
	bool isBool() const { return type() == Type::Bool; }
	bool isString() const { return type() == Type::String; }
	bool isList() const { return type() == Type::List; }
	bool isArray() const { return type() == Type::Array; }
	bool isMap() const { return type() == Type::Map; }
	bool isIMap() const { return type() == Type::IMap; }

	double toDouble() const;
	Decimal toDecimal() const;
	Int toInt() const;
	UInt toUInt() const;
	bool toBool() const;
	DateTime toDateTime() const;
	const RpcValue::String &toString() const;
	const Blob &toBlob() const;
	const List &toList() const;
	const Array &toArray() const;
	const Map &toMap() const;
	const IMap &toIMap() const;

	size_t count() const;
	RpcValue at(UInt i) const;
	RpcValue at(const RpcValue::String &key) const;
	RpcValue operator[](UInt i) const {return at(i);}
	RpcValue operator[](const RpcValue::String &key) const {return at(key);}
	void set(UInt ix, const RpcValue &val);
	void set(const RpcValue::String &key, const RpcValue &val);

	std::string toStdString() const;
	std::string toCpon() const;
	static RpcValue parseCpon(const std::string & str, std::string *err = nullptr);

	bool operator== (const RpcValue &rhs) const;
#ifdef RPCVALUE_COPY_AND_SWAP
	RpcValue& operator= (RpcValue rhs) noexcept
	{
		swap(rhs);
		return *this;
	}
	void swap(RpcValue& other) noexcept;
#endif
	/*
	bool operator<  (const ChainPack &rhs) const;
	bool operator!= (const ChainPack &rhs) const { return !(*this == rhs); }
	bool operator<= (const ChainPack &rhs) const { return !(rhs < *this); }
	bool operator>  (const ChainPack &rhs) const { return  (rhs < *this); }
	bool operator>= (const ChainPack &rhs) const { return !(*this < rhs); }
	*/
private:
	std::shared_ptr<AbstractValueData> m_ptr;
};

template<typename T> RpcValue::Type guessType() { throw std::runtime_error("guessing of this type is not implemented"); }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::Int>() { return Type::Int; }
template<> inline RpcValue::Type RpcValue::guessType<RpcValue::UInt>() { return Type::UInt; }
template<> inline RpcValue::Type RpcValue::guessType<uint16_t>() { return Type::UInt; }

}}
