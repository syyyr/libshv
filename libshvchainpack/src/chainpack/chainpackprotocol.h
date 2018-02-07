#pragma once

#include "rpcvalue.h"

#include <string>
#include <streambuf>

namespace shv {
namespace chainpack {

class SHVCHAINPACK_DECL_EXPORT ChainPackProtocol
{
	static constexpr uint8_t ARRAY_FLAG_MASK = 64;
public: // public just because of testing, normal user do not need to use this enum
	struct SHVCHAINPACK_DECL_EXPORT TypeInfo {
		enum Enum {
			INVALID = -1,
			/// types
			Null = 128,
			UInt,
			Int,
			Double,
			Bool,
			Blob,
			String,
			DateTimeEpoch, // deprecated
			List,
			Map,
			IMap,
			MetaIMap,
			Decimal,
			DateTime,
			MetaSMap,
			/// arrays
			/*
			Null_Array = Null | ARRAY_FLAG_MASK, // if bit 6 is set, then packed value is an Array of corresponding values
			UInt_Array = UInt | ARRAY_FLAG_MASK,
			Int_Array = Int | ARRAY_FLAG_MASK,
			Double_Array = Double | ARRAY_FLAG_MASK,
			Bool_Array = Bool | ARRAY_FLAG_MASK,
			Blob_Array = Blob | ARRAY_FLAG_MASK,
			String_Array = String | ARRAY_FLAG_MASK,
			DateTimeEpoch_Array = DateTimeEpoch | ARRAY_FLAG_MASK, // not used
			List_Array = List | ARRAY_FLAG_MASK,
			Map_Array = Map | ARRAY_FLAG_MASK,
			IMap_Array = IMap | ARRAY_FLAG_MASK,
			MetaIMap_Array = MetaIMap | ARRAY_FLAG_MASK,
			Decimal_Array = Decimal | ARRAY_FLAG_MASK,
			DateTimeUtc_Array = DateTimeUtc | ARRAY_FLAG_MASK, // not used, DateTimeTZ_Array is used allways for DateTime arrays
			DateTimeTZ_Array = DateTimeTZ | ARRAY_FLAG_MASK,
			*/
			FALSE = 253,
			TRUE = 254,
			TERM = 255,
		};
		static const char* name(Enum e);
	};
public:
	static uint64_t readUIntData(std::istream &data, bool *ok = nullptr);
	static RpcValue read(std::istream &data);
	static RpcValue::MetaData readMetaData(std::istream &data);

	static void writeUIntData(std::ostream &out, uint64_t n);
	static void writeMetaData(std::ostream &out, const RpcValue::MetaData &meta_data);

	static int write(std::ostream &out, const RpcValue &val);

	static void writeContainerBegin(std::ostream &out, const RpcValue::Type &container_type);
	static void writeContainerEnd(std::ostream &out);
	static void writeListElement(std::ostream &out, const RpcValue &val);
	static void writeMapElement(std::ostream &out, const std::string &key, const RpcValue &val);
	static void writeMapElement(std::ostream &out, const RpcValue::UInt &key, const RpcValue &val);
	static void writeArrayBegin(std::ostream &out, const RpcValue::Type &array_type, size_t array_size);
	static void writeArrayElement(std::ostream &out, const RpcValue &val) {	writeData(out, val); }
private:
	static TypeInfo::Enum typeToTypeInfo(RpcValue::Type tid);
	static RpcValue::Type typeInfoToType(TypeInfo::Enum type_info);

	static bool writeTypeInfo(std::ostream &out, const RpcValue &pack);
	static void writeData(std::ostream &out, const RpcValue &val);
	static TypeInfo::Enum readTypeInfo(std::istream &data, RpcValue &meta, int &tiny_uint);
	static RpcValue readData(TypeInfo::Enum tid, bool is_array, std::istream &data);
private:
	static void writeData_Array(std::ostream &out, const RpcValue::Array &array);
	static void writeData_List(std::ostream &out, const RpcValue::List &list);
	static void writeData_Map(std::ostream &out, const RpcValue::Map &map);
	static void writeData_IMap(std::ostream &out, const RpcValue::IMap &map);

	static RpcValue::List readData_List(std::istream &data);
	static RpcValue::Array readData_Array(TypeInfo::Enum type_info, std::istream &data);
	static RpcValue::Map readData_Map(std::istream &data);
	static RpcValue::IMap readData_IMap(std::istream &data);
private:
	static constexpr bool WRITE_INVALID_AS_NULL = true;
};

}}

