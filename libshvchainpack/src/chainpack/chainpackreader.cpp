#include "chainpackreader.h"

namespace shv {
namespace chainpack {

namespace {
template<typename T>
T readData_UInt(std::istream &data, int *pbitlen = nullptr)
{
	T num = 0;
	int bitlen = 0;
	do {
		if(data.eof())
			break;
		uint8_t head = data.get();

		int bytes_to_read_cnt;
		if     ((head & 128) == 0) {bytes_to_read_cnt = 0; num = head & 127; bitlen = 7;}
		else if((head &  64) == 0) {bytes_to_read_cnt = 1; num = head & 63; bitlen = 6 + 8;}
		else if((head &  32) == 0) {bytes_to_read_cnt = 2; num = head & 31; bitlen = 5 + 2*8;}
		else if((head &  16) == 0) {bytes_to_read_cnt = 3; num = head & 15; bitlen = 4 + 3*8;}
		else {
			bytes_to_read_cnt = (head & 0xf) + 4;
			bitlen = bytes_to_read_cnt * 8;
		}

		for (int i = 0; i < bytes_to_read_cnt; ++i) {
			if(data.eof()) {
				bitlen = 0;
				break;
			}
			uint8_t r = data.get();
			num = (num << 8) + r;
		};
	} while(false);
	if(pbitlen)
		*pbitlen = bitlen;
	return num;
}

template<typename T>
T readData_Int(std::istream &data)
{
	int bitlen;
	using UT = typename std::make_unsigned<T>::type;
	UT num = readData_UInt<UT>(data, &bitlen);
	UT sign_bit_mask = UT{1} << (bitlen - 1);
	bool neg = num & sign_bit_mask;
	T snum = num;
	if(neg) {
		snum &= ~sign_bit_mask;
		snum = -snum;
	}
	return snum;
}

double readData_Double(std::istream &data)
{
	union U {uint64_t n; double d;} u;
	u.n = 0;
	int shift = 0;
	for (size_t i = 0; i < sizeof(u.n); ++i) {
		uint8_t r = data.get();
		uint64_t n1 = r;
		n1 <<= shift;
		shift += 8;
		u.n += n1;
	}
	return u.d;
}

RpcValue::Decimal readData_Decimal(std::istream &data)
{
	int64_t mant = readData_Int<int64_t>(data);
	int prec = readData_Int<int>(data);
	return RpcValue::Decimal(mant, prec);
}

template<typename T>
T readData_Blob(std::istream &data)
{
	using S = typename T::size_type;
	unsigned int len = readData_UInt<unsigned int>(data);
	T ret;
	//ret.reserve(len);
	for (S i = 0; i < len; ++i)
		ret += data.get();
	return ret;
}

RpcValue::String readData_CString(std::istream &data)
{
	RpcValue::String ret;
	bool is_esc = false;
	while (true) {
		int i = data.get();
		if(is_esc) {
			switch (i) {
			case '\\': ret += '\\'; break;
			case '0' : ret +=  '\0'; break;
			default:
				ret += (char)i;
				break;
			}
			is_esc = false;
		}
		else {
			if(i)
				ret += (char)i;
			else
				break;
		}
	}
	return ret;
}
/*
RpcValue::DateTime readData_DateTimeEpoch(std::istream &data)
{
	RpcValue::DateTime dt = RpcValue::DateTime::fromMSecsSinceEpoch(readData_Int<int64_t>(data));
	return dt;
}
*/
RpcValue::DateTime readData_DateTime(std::istream &data)
{
	int64_t d = readData_Int<int64_t>(data);
	int8_t offset = 0;
	bool has_tz_offset = d & 1;
	bool has_not_msec = d & 2;
	d >>= 2;
	if(has_tz_offset) {
		offset = d & 0b01111111;
		offset <<= 1;
		offset >>= 1;
		d >>= 7;
	}
	if(has_not_msec)
		d *= 1000;
	d += ChainPack::SHV_EPOCH_MSEC;
	RpcValue::DateTime dt = RpcValue::DateTime::fromMSecsSinceEpoch(d, offset * (int)15);
	return dt;
}

} // namespace

void ChainPackReader::read(RpcValue::MetaData &meta_data)
{
	RpcValue::IMap imap;
	RpcValue::Map smap;
	uint8_t type_info = m_in.peek();
	if(type_info == ChainPack::PackingSchema::MetaMap) {
		m_in.get();
		while(true) {
			int b = m_in.peek();
			if(b == ChainPack::PackingSchema::TERM) {
				m_in.get();
				break;
			}
			else if(b == ChainPack::STRING_META_KEY_PREFIX) {
				m_in.get();
				RpcValue::String key = readData_Blob<RpcValue::String>(m_in);
				RpcValue cp = read();
				smap[key] = cp;
			}
			else {
				RpcValue::UInt key = readData_UInt<RpcValue::UInt>(m_in);
				RpcValue cp = read();
				imap[key] = cp;
			}
		}
	}
	meta_data = RpcValue::MetaData(std::move(imap), std::move(smap));
}

void ChainPackReader::read(RpcValue &val)
{
	RpcValue::MetaData meta_data;
	read(meta_data);
	uint8_t type = m_in.get();
	if(type < 128) {
		if(type & 64) {
			// tiny Int
			int n = type & 63;
			val = RpcValue(n);
		}
		else {
			// tiny UInt
			RpcValue::UInt n = type & 63;
			val = RpcValue(n);
		}
	}
	else if(type == ChainPack::PackingSchema::FALSE) {
		val = RpcValue(false);
	}
	else if(type == ChainPack::PackingSchema::TRUE) {
		val = RpcValue(true);
	}
	if(!val.isValid()) {
		bool is_array = type & ChainPack::ARRAY_FLAG_MASK;
		type = type & ~ChainPack::ARRAY_FLAG_MASK;
		val = readData((ChainPack::PackingSchema::Enum)type, is_array);
	}
	if(!meta_data.isEmpty())
		val.setMetaData(std::move(meta_data));
}

uint64_t ChainPackReader::readUIntData(std::istream &data, bool *ok)
{
	int bitlen;
	uint64_t ret = readData_UInt<uint64_t>(data, &bitlen);
	if(ok)
		*ok = (bitlen > 0);
	return ret;
}

RpcValue ChainPackReader::readData(ChainPack::PackingSchema::Enum type_info, bool is_array)
{
	RpcValue ret;
	if(is_array) {
		RpcValue::Array val = readData_Array(type_info);
		ret = RpcValue(val);
	}
	else {
		switch (type_info) {
		case ChainPack::PackingSchema::Null: { ret = RpcValue(nullptr); break; }
		case ChainPack::PackingSchema::UInt: { uint64_t u = readData_UInt<uint64_t>(m_in); ret = RpcValue(u); break; }
		case ChainPack::PackingSchema::Int: { int64_t i = readData_Int<int64_t>(m_in); ret = RpcValue(i); break; }
		case ChainPack::PackingSchema::Double: { double d = readData_Double(m_in); ret = RpcValue(d); break; }
		case ChainPack::PackingSchema::Decimal: { RpcValue::Decimal d = readData_Decimal(m_in); ret = RpcValue(d); break; }
		case ChainPack::PackingSchema::TRUE: { bool b = true; ret = RpcValue(b); break; }
		case ChainPack::PackingSchema::FALSE: { bool b = false; ret = RpcValue(b); break; }
		//case ChainPack::TypeInfo::DateTimeEpoch: { RpcValue::DateTime val = readData_DateTimeEpoch(m_in); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::DateTime: { RpcValue::DateTime val = readData_DateTime(m_in); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::String: { RpcValue::String val = readData_Blob<RpcValue::String>(m_in); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::CString: { RpcValue::String val = readData_CString(m_in); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::Blob_depr: { RpcValue::String val = readData_Blob<RpcValue::String>(m_in); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::List: { RpcValue::List val = readData_List(); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::Map: { RpcValue::Map val = readData_Map(); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::IMap: { RpcValue::IMap val = readData_IMap(); ret = RpcValue(val); break; }
		case ChainPack::PackingSchema::Bool: { uint8_t t = m_in.get(); ret = RpcValue(t != 0); break; }
		default:
			SHVCHP_EXCEPTION("Internal error: attempt to read helper type directly. type: " + Utils::toString(type_info) + " " + ChainPack::PackingSchema::name(type_info));
		}
	}
	return ret;
}

RpcValue::List ChainPackReader::readData_List()
{
	RpcValue::List lst;
	while(true) {
		int b = m_in.peek();
		if(b < 0)
			SHVCHP_EXCEPTION("Unexpected EOF!");
		if(b == ChainPack::PackingSchema::TERM) {
			m_in.get();
			break;
		}
		RpcValue cp = read();
		lst.push_back(cp);
	}
	return lst;
}

RpcValue::Map ChainPackReader::readData_Map()
{
	RpcValue::Map ret;
	while(true) {
		int b = m_in.peek();
		if(b < 0)
			SHVCHP_EXCEPTION("Unexpected EOF!");
		if(b == ChainPack::PackingSchema::TERM) {
			m_in.get();
			break;
		}
		RpcValue::String key = readData_Blob<RpcValue::String>(m_in);
		RpcValue cp = read();
		ret[key] = cp;
	}
	return ret;
}

RpcValue::IMap ChainPackReader::readData_IMap()
{
	RpcValue::IMap ret;
	while(true) {
		int b = m_in.peek();
		if(b == ChainPack::PackingSchema::TERM) {
			m_in.get();
			break;
		}
		RpcValue::UInt key = readData_UInt<RpcValue::UInt>(m_in);
		RpcValue cp = read();
		ret[key] = cp;
	}
	return ret;
}

RpcValue::Array ChainPackReader::readData_Array(ChainPack::PackingSchema::Enum array_type_info)
{
	RpcValue::Type type = ChainPack::typeInfoToArrayType(array_type_info);
	RpcValue::Array ret(type);
	RpcValue::UInt size = readData_UInt<RpcValue::UInt>(m_in);
	ret.reserve(size);
	for (unsigned i = 0; i < size; ++i) {
		RpcValue cp = readData(array_type_info, false);
		ret.push_back(RpcValue::Array::makeElement(cp));
	}
	return ret;
}

} // namespace chainpack
} // namespace shv
