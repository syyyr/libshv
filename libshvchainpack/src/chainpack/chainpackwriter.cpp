#include "chainpackwriter.h"
#include "chainpack.h"

#include <cassert>

namespace shv {
namespace chainpack {

namespace {

template<typename T>
int significant_bits_part_length(T n)
{
	constexpr int bitlen = sizeof(T) * 8;
	constexpr T mask = T{1} << (bitlen - 1);
	int len = bitlen;
	for (; n && !(n & mask); --len) {
		n <<= 1;
	}
	return n? len: 0;
}

// number of bytes needed to encode bit_len
int bytes_needed(int bit_len)
{
	int cnt;
	if(bit_len <= 28)
		cnt = (bit_len - 1) / 7 + 1;
	else
		cnt = (bit_len - 1) / 8 + 2;
	return cnt;
}

/* UInt
 0 ...  7 bits  1  byte  |0|x|x|x|x|x|x|x|<-- LSB
 8 ... 14 bits  2  bytes |1|0|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
15 ... 21 bits  3  bytes |1|1|0|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
22 ... 28 bits  4  bytes |1|1|1|0|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                    n ==  0 ->  4 bytes number (32 bit number)
                    n ==  1 ->  5 bytes number
                    n == 14 -> 18 bytes number
                    n == 15 -> for future (number of bytes will be specified in next byte)
*/

template<typename T>
void writeData_int_helper(std::ostream &out, T num, int bit_len)
{
	int byte_cnt = bytes_needed(bit_len);
	uint8_t bytes[byte_cnt];
	for (int i = byte_cnt-1; i >= 0; --i) {
		uint8_t r = num & 255;
		bytes[i] = r;
		num = num >> 8;
	}

	uint8_t &head = bytes[0];
	if(bit_len <= 28) {
		uint8_t mask = 0xf0 << (4 - byte_cnt);
		head = head & ~mask;
		mask <<= 1;
		head = head | mask;
	}
	else {
		head = 0xf0 | (byte_cnt - 5);
	}

	for (int i = 0; i < byte_cnt; ++i) {
		uint8_t r = bytes[i];
		out << r;
	}
}

template<typename T>
void writeData_UInt(std::ostream &out, T num)
{
	constexpr int UINT_BYTES_MAX = 18;
	if(sizeof(num) > UINT_BYTES_MAX)
		SHVCHP_EXCEPTION("writeData_UInt: value too big to pack!");

	int bitlen = significant_bits_part_length(num);
	writeData_int_helper<T>(out, num, bitlen);
}

/*
 0 ...  7 bits  1  byte  |0|s|x|x|x|x|x|x|<-- LSB
 8 ... 14 bits  2  bytes |1|0|s|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
15 ... 21 bits  3  bytes |1|1|0|s|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
22 ... 28 bits  4  bytes |1|1|1|0|s|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x|<-- LSB
29+       bits  5+ bytes |1|1|1|1|n|n|n|n| |s|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| |x|x|x|x|x|x|x|x| ... <-- LSB
                    n ==  0 ->  4 bytes number (32 bit number)
                    n ==  1 ->  5 bytes number
                    n == 14 -> 18 bytes number
                    n == 15 -> for future (number of bytes will be specified in next byte)
*/

// return max bit length >= bit_len, which can be encoded by same number of bytes
int expand_bit_len(int bit_len)
{
	int ret;
	int byte_cnt = bytes_needed(bit_len);
	if(bit_len <= 28) {
		ret = byte_cnt * (8 - 1) - 1;
	}
	else {
		ret = (byte_cnt - 1) * 8 - 1;
	}
	return ret;
}

template<typename T>
void writeData_Int(std::ostream &out, T snum)
{
	using UT = typename std::make_unsigned<T>::type;
	UT num = snum < 0? -snum: snum;
	bool neg = snum < 0? true: false;

	int bitlen = significant_bits_part_length(num);
	bitlen++; // add sign bit
	if(neg) {
		int sign_pos = expand_bit_len(bitlen);
		UT sign_bit_mask = UT{1} << sign_pos;
		num |= sign_bit_mask;
	}
	writeData_int_helper(out, num, bitlen);
}

void writeData_Double(std::ostream &out, double d)
{
	union U {uint64_t n; double d;} u;
	assert(sizeof(u.n) == sizeof(u.d));
	u.d = d;
	for (size_t i = 0; i < sizeof(u.n); ++i) {
		uint8_t r = u.n & 255;
		u.n = u.n >> 8;
		out << r;
	}
}

void writeData_Decimal(std::ostream &out, const RpcValue::Decimal &d)
{
	writeData_Int(out, d.mantisa());
	writeData_Int(out, d.precision());
}

template<typename T>
void writeData_Blob(std::ostream &out, const T &blob)
{
	using S = typename T::size_type;
	S l = blob.length();
	writeData_UInt<S>(out, l);
	//out.reserve(out.size() + l);
	for (S i = 0; i < l; ++i)
		out << (uint8_t)blob[i];
}

void writeData_DateTime(std::ostream &out, const RpcValue::DateTime &dt)
{
	int64_t msecs = dt.msecsSinceEpoch() - ChainPack::SHV_EPOCH_MSEC;
	int offset = (dt.minutesFromUtc() / 15) & 0b01111111;
	int ms = msecs % 1000;
	if(ms == 0)
		msecs /= 1000;
	if(offset != 0) {
		msecs <<= 7;
		msecs |= offset;
	}
	msecs <<= 2;
	if(offset != 0)
		msecs |= 1;
	if(ms == 0)
		msecs |= 2;
	writeData_Int(out, msecs);
}

} // namespace

size_t ChainPackWriter::write(const RpcValue &val)
{
	size_t len = m_out.tellp();
	if(!val.isValid()) {
		if(WRITE_INVALID_AS_NULL)
			write(RpcValue(nullptr));
		else
			SHVCHP_EXCEPTION("Cannot serialize invalid ChainPack.");
	}
	else {
		write(val.metaData());
		if(!writeTypeInfo(val))
			writeData(val);
	}
	return (size_t)m_out.tellp() - len;
}

size_t ChainPackWriter::write(const RpcValue::MetaData &meta_data)
{
	size_t len = m_out.tellp();
	if(!meta_data.isEmpty()) {
		const RpcValue::IMap &cim = meta_data.iValues();
		const RpcValue::Map &csm = meta_data.sValues();
		if(!cim.empty() || !csm.empty()) {
			m_out << (uint8_t)ChainPack::TypeInfo::MetaMap;
			for (const auto &kv : cim) {
				writeMapElement(kv.first, kv.second);
			}
			for (const auto &kv : csm) {
				/// we don't expect more tha 17 bytes long uint keys
				/// so 0xFE can be used as string key prefix
				m_out << ChainPack::STRING_META_KEY_PREFIX;
				writeMapElement(kv.first, kv.second);
			}
			writeContainerEnd(RpcValue::Type::Map);
		}
	}
	return (size_t)m_out.tellp() - len;
}

void ChainPackWriter::writeUIntData(uint64_t n)
{
	writeUIntData(m_out, n);
}

void ChainPackWriter::writeUIntData(std::ostream &os, uint64_t n)
{
	writeData_UInt(os, n);
}

static ChainPack::TypeInfo::Enum typeToTypeInfo(RpcValue::Type type)
{
	switch (type) {
	case RpcValue::Type::Invalid:
		SHVCHP_EXCEPTION("There is no type info for type Invalid");
	case RpcValue::Type::Array:
		SHVCHP_EXCEPTION("There is no type info for type Array");
	case RpcValue::Type::Null: return ChainPack::TypeInfo::Null;
	case RpcValue::Type::UInt: return ChainPack::TypeInfo::UInt;
	case RpcValue::Type::Int: return ChainPack::TypeInfo::Int;
	case RpcValue::Type::Double: return ChainPack::TypeInfo::Double;
	case RpcValue::Type::Bool: return ChainPack::TypeInfo::Bool;
	//case RpcValue::Type::Blob: return ChainPack::TypeInfo::Blob;
	case RpcValue::Type::String: return ChainPack::TypeInfo::String;
	case RpcValue::Type::List: return ChainPack::TypeInfo::List;
	case RpcValue::Type::Map: return ChainPack::TypeInfo::Map;
	case RpcValue::Type::IMap: return ChainPack::TypeInfo::IMap;
	//case RpcValue::Type::DateTimeEpoch: return TypeInfo::DateTimeEpoch;
	case RpcValue::Type::DateTime: return ChainPack::TypeInfo::DateTime;
	case RpcValue::Type::Decimal: return ChainPack::TypeInfo::Decimal;
	}
	SHVCHP_EXCEPTION("Unknown RpcValue::Type!");
	return ChainPack::TypeInfo::INVALID; // just to remove mingw warning
}

bool ChainPackWriter::writeTypeInfo(const RpcValue &pack)
{
	if(!pack.isValid())
		SHVCHP_EXCEPTION("Cannot serialize invalid ChainPack.");
	bool ret = false;
	RpcValue::Type type = pack.type();
	int t = ChainPack::TypeInfo::INVALID;
	if(type == RpcValue::Type::Bool) {
		t = pack.toBool()? ChainPack::TypeInfo::TRUE: ChainPack::TypeInfo::FALSE;
		ret = true;
	}
	else if(type == RpcValue::Type::UInt) {
		auto n = pack.toUInt();
		if(n < 64) {
			/// TinyUInt
			t = n;
			ret = true;
		}
	}
	else if(type == RpcValue::Type::Int) {
		auto n = pack.toInt();
		if(n >= 0 && n < 64) {
			/// TinyInt
			t = 64 + n;
			ret = true;
		}
	}
	else if(type == RpcValue::Type::Array) {
		t = typeToTypeInfo(pack.arrayType());
		t |= ChainPack::ARRAY_FLAG_MASK;
	}
	if(t == ChainPack::TypeInfo::INVALID) {
		t = typeToTypeInfo(pack.type());
	}
	m_out << (uint8_t)t;
	return ret;
}

void ChainPackWriter::writeData(const RpcValue &val)
{
	RpcValue::Type type = val.type();
	switch (type) {
	case RpcValue::Type::Null: break;
	case RpcValue::Type::Bool: m_out << (uint8_t)(val.toBool() ? 1 : 0); break;
	case RpcValue::Type::UInt: { uint64_t u = val.toUInt64(); writeData_UInt(m_out, u); break; }
	case RpcValue::Type::Int: { int64_t n = val.toInt64(); writeData_Int(m_out, n); break; }
	case RpcValue::Type::Double: writeData_Double(m_out, val.toDouble()); break;
	case RpcValue::Type::Decimal: writeData_Decimal(m_out, val.toDecimal()); break;
	case RpcValue::Type::DateTime: writeData_DateTime(m_out, val.toDateTime()); break;
	case RpcValue::Type::String: writeData_Blob(m_out, val.toString()); break;
	//case RpcValue::Type::Blob: writeData_Blob(m_out, val.toBlob()); break;
	case RpcValue::Type::List: writeData_List(val.toList()); break;
	case RpcValue::Type::Array: writeData_Array(val.toArray()); break;
	case RpcValue::Type::Map: writeData_Map(val.toMap()); break;
	case RpcValue::Type::IMap: writeData_IMap(val.toIMap()); break;
	case RpcValue::Type::Invalid:
	//case RpcValue::Type::MetaIMap:
		SHVCHP_EXCEPTION("Internal error: attempt to write helper type directly. type: " + std::string(RpcValue::typeToName(type)));
	}
}

void ChainPackWriter::writeData_Map(const RpcValue::Map &map)
{
	for (const auto &kv : map) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd(RpcValue::Type::Map);
}

void ChainPackWriter::writeData_IMap(const RpcValue::IMap &map)
{
	for (const auto &kv : map) {
		writeMapElement(kv.first, kv.second);
	}
	writeContainerEnd(RpcValue::Type::IMap);
}

void ChainPackWriter::writeData_List(const RpcValue::List &list)
{
	unsigned size = list.size();
	//write_UIntData(out, size);
	for (unsigned i = 0; i < size; ++i) {
		const RpcValue &cp = list.at(i);
		write(cp);
	}
	writeContainerEnd(RpcValue::Type::List);
}

void ChainPackWriter::writeData_Array(const RpcValue::Array &array)
{
	unsigned size = array.size();
	writeUIntData(size);
	for (unsigned i = 0; i < size; ++i) {
		const RpcValue &cp = array.valueAt(i);
		writeData(cp);
	}
}

void ChainPackWriter::writeContainerBegin(RpcValue::Type container_type)
{
	ChainPack::TypeInfo::Enum t = typeToTypeInfo(container_type);
	m_out << (uint8_t)t;
}

void ChainPackWriter::writeContainerEnd(RpcValue::Type container_type)
{
	(void)container_type;
	m_out << (uint8_t)ChainPack::TypeInfo::TERM;
}

void ChainPackWriter::writeListElement(const RpcValue &val)
{
	write(val);
}

void ChainPackWriter::writeMapElement(const std::string &key, const RpcValue &val)
{
	writeData_Blob(m_out, key);
	write(val);
}

void ChainPackWriter::writeMapElement(RpcValue::UInt key, const RpcValue &val)
{
	writeData_UInt(m_out, key);
	write(val);
}

void ChainPackWriter::writeArrayBegin(RpcValue::Type array_type, size_t array_size)
{
	uint8_t t = (uint8_t)typeToTypeInfo(array_type);
	t |= ChainPack::ARRAY_FLAG_MASK;
	m_out << t;
	writeUIntData(array_size);
}

void ChainPackWriter::writeArrayElement(const RpcValue &val)
{
	writeData(val);
}

} // namespace chainpack
} // namespace shv
