#include "rpcvalue.h"
#include "cponwriter.h"

#include "cponreader.h"
#include "exception.h"
#include "utils.h"

#include <necrolog.h>

#include <cassert>
#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <sstream>
#include <iostream>
//#include <utility>

namespace {
#if defined _WIN32 || defined LIBC_NEWLIB
// see http://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap04.html#tag_04_15
int is_leap(unsigned y)
{
	y += 1900;
	return (y % 4) == 0 && ((y % 100) != 0 || (y % 400) == 0);
}

time_t timegm(struct tm *tm)
{
	static const unsigned ndays[2][12] = {
		{31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31},
		{31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}
	};
	time_t res = 0;
	int i;

	for (i = 70; i < tm->tm_year; ++i) {
		res += is_leap(i) ? 366 : 365;
	}

	for (i = 0; i < tm->tm_mon; ++i) {
		res += ndays[is_leap(tm->tm_year)][i];
	}
	res += tm->tm_mday - 1;
	res *= 24;
	res += tm->tm_hour;
	res *= 60;
	res += tm->tm_min;
	res *= 60;
	res += tm->tm_sec;
	return res;
}
#endif
} // namespace

namespace shv {
namespace chainpack {

/*
using std::string;
*/
class RpcValue::AbstractValueData
{
public:
	virtual ~AbstractValueData() {}

	virtual RpcValue::Type type() const {return RpcValue::Type::Invalid;}
	virtual RpcValue::Type arrayType() const {return RpcValue::Type::Invalid;}

	virtual const RpcValue::MetaData &metaData() const = 0;
	virtual void setMetaData(RpcValue::MetaData &&meta_data) = 0;
	virtual void setMetaValue(RpcValue::UInt key, const RpcValue &val) = 0;
	virtual void setMetaValue(RpcValue::String key, const RpcValue &val) = 0;

	virtual bool equals(const AbstractValueData * other) const = 0;
	//virtual bool less(const Data * other) const = 0;

	virtual bool isNull() const {return false;}
	virtual double toDouble() const {return 0;}
	virtual RpcValue::Decimal toDecimal() const { return RpcValue::Decimal{}; }
	virtual RpcValue::Int toInt() const {return 0;}
	virtual RpcValue::UInt toUInt() const {return 0;}
	virtual bool toBool() const {return false;}
	virtual RpcValue::DateTime toDateTime() const { return RpcValue::DateTime{}; }
	virtual const std::string &toString() const;
	virtual const RpcValue::Blob &toBlob() const;
	virtual const RpcValue::List &toList() const;
	virtual const RpcValue::Array &toArray() const;
	virtual const RpcValue::Map &toMap() const;
	virtual const RpcValue::IMap &toIMap() const;
	virtual size_t count() const {return 0;}

	virtual RpcValue at(RpcValue::UInt i) const;
	virtual RpcValue at(const RpcValue::String &key) const;
	virtual void set(RpcValue::UInt ix, const RpcValue &val);
	virtual void set(const RpcValue::String &key, const RpcValue &val);
	virtual void append(const RpcValue &);
};

/* * * * * * * * * * * * * * * * * * * *
 * Value wrappers
 */

template <RpcValue::Type tag, typename T>
class ValueData : public RpcValue::AbstractValueData
{
protected:
	explicit ValueData(const T &value) : m_value(value) {}
	explicit ValueData(T &&value) : m_value(std::move(value)) {}
	// disable copy (because of m_metaData)
	ValueData(const ValueData &o) = delete;
	ValueData& operator=(const ValueData &o) = delete;
	//ValueData(ValueData &&o) = delete;
	//ValueData& operator=(ValueData &&o) = delete;
	virtual ~ValueData() override
	{
		if(m_metaData)
			delete m_metaData;
	}

	RpcValue::Type type() const override { return tag; }

	const RpcValue::MetaData &metaData() const override
	{
		static RpcValue::MetaData md;
		if(!m_metaData)
			return md;
		return *m_metaData;
	}

	void setMetaData(RpcValue::MetaData &&d) override
	{
		if(m_metaData)
			(*m_metaData) = std::move(d);
		else
			m_metaData = new RpcValue::MetaData(std::move(d));
	}

	void setMetaValue(RpcValue::UInt key, const RpcValue &val) override
	{
		if(!m_metaData)
			m_metaData = new RpcValue::MetaData();
		m_metaData->setValue(key, val);
	}
	void setMetaValue(RpcValue::String key, const RpcValue &val) override
	{
		if(!m_metaData)
			m_metaData = new RpcValue::MetaData();
		m_metaData->setValue(key, val);
	}
protected:
	T m_value;
	RpcValue::MetaData *m_metaData = nullptr;
};

class ChainPackDouble final : public ValueData<RpcValue::Type::Double, double>
{
	double toDouble() const override { return m_value; }
	RpcValue::Int toInt() const override { return static_cast<int>(m_value); }
	bool equals(const RpcValue::AbstractValueData * other) const override {
		return m_value == other->toDouble();
	}
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackDouble(double value) : ValueData(value) {}
};

class ChainPackDecimal final : public ValueData<RpcValue::Type::Decimal, RpcValue::Decimal>
{
	double toDouble() const override { return m_value.toDouble(); }
	RpcValue::Int toInt() const override { return static_cast<int>(m_value.toDouble()); }
	RpcValue::Decimal toDecimal() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return toDouble() == other->toDouble(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackDecimal(RpcValue::Decimal &&value) : ValueData(std::move(value)) {}
};

class ChainPackInt final : public ValueData<RpcValue::Type::Int, RpcValue::Int>
{
	double toDouble() const override { return m_value; }
	RpcValue::Int toInt() const override { return m_value; }
	RpcValue::UInt toUInt() const override { return (unsigned int)m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toInt(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackInt(RpcValue::Int value) : ValueData(value) {}
};

class ChainPackUInt : public ValueData<RpcValue::Type::UInt, RpcValue::UInt>
{
protected:
	double toDouble() const override { return m_value; }
	RpcValue::Int toInt() const override { return m_value; }
	RpcValue::UInt toUInt() const override { return m_value; }
protected:
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toUInt(); }
	//bool less(const Data * other) const override { return m_value < other->toDouble(); }
public:
	explicit ChainPackUInt(RpcValue::UInt value) : ValueData(value) {}
};

class ChainPackBoolean final : public ValueData<RpcValue::Type::Bool, bool>
{
	bool toBool() const override { return m_value; }
	RpcValue::Int toInt() const override { return m_value? true: false; }
	RpcValue::UInt toUInt() const override { return toInt(); }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toBool(); }
public:
	explicit ChainPackBoolean(bool value) : ValueData(value) {}
};

class ChainPackDateTime final : public ValueData<RpcValue::Type::DateTime, RpcValue::DateTime>
{
	bool toBool() const override { return m_value.msecsSinceEpoch() != 0; }
	RpcValue::Int toInt() const override { return m_value.msecsSinceEpoch(); }
	RpcValue::UInt toUInt() const override { return m_value.msecsSinceEpoch(); }
	RpcValue::DateTime toDateTime() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value.msecsSinceEpoch() == other->toDateTime().msecsSinceEpoch(); }
public:
	explicit ChainPackDateTime(RpcValue::DateTime value) : ValueData(value) {}
};

class ChainPackString : public ValueData<RpcValue::Type::String, RpcValue::String>
{
	const std::string &toString() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toString(); }
public:
	explicit ChainPackString(const RpcValue::String &value) : ValueData(value) {}
	explicit ChainPackString(RpcValue::String &&value) : ValueData(std::move(value)) {}
};

class ChainPackBlob final : public ValueData<RpcValue::Type::Blob, RpcValue::Blob>
{
	const RpcValue::Blob &toBlob() const override { return m_value; }
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toBlob(); }
public:
	explicit ChainPackBlob(const RpcValue::Blob &value) : ValueData(value) {}
	explicit ChainPackBlob(RpcValue::Blob &&value) : ValueData(std::move(value)) {}
	explicit ChainPackBlob(const uint8_t *bytes, size_t size) : ValueData(RpcValue::Blob()) {
		m_value.reserve(size);
		for (size_t i = 0; i < size; ++i) {
			m_value[i] = bytes[i];
		}
	}
};

class ChainPackList final : public ValueData<RpcValue::Type::List, RpcValue::List>
{
	size_t count() const override {return m_value.size();}
	RpcValue at(RpcValue::UInt i) const override;
	void set(RpcValue::UInt key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toList(); }
public:
	explicit ChainPackList(const RpcValue::List &value) : ValueData(value) {}
	explicit ChainPackList(RpcValue::List &&value) : ValueData(move(value)) {}

	const RpcValue::List &toList() const override { return m_value; }
};

class ChainPackArray final : public ValueData<RpcValue::Type::Array, RpcValue::Array>
{
	size_t count() const override {return m_value.size();}
	RpcValue at(RpcValue::UInt i) const override;
	void set(RpcValue::UInt key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override
	{
		const RpcValue::Array other_array = other->toArray();
		if(m_value.size() != other_array.size())
			return false;
		for (size_t i = 0; i < m_value.size(); ++i) {
			const RpcValue v1 = m_value.valueAt(i);
			const RpcValue v2 = other_array.valueAt(i);
			if(v1 == v2)
				continue;
			return false;
		}
		return true;
	}
public:
	explicit ChainPackArray(const RpcValue::Array &value) : ValueData(value) {}
	explicit ChainPackArray(RpcValue::Array &&value) noexcept : ValueData(std::move(value)) {}

	RpcValue::Type arrayType() const override {return m_value.type();}
	const RpcValue::Array &toArray() const override { return m_value; }
};

class ChainPackMap final : public ValueData<RpcValue::Type::Map, RpcValue::Map>
{
	size_t count() const override {return m_value.size();}
	RpcValue at(const RpcValue::String &key) const override;
	void set(const RpcValue::String &key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toMap(); }
public:
	explicit ChainPackMap(const RpcValue::Map &value) : ValueData(value) {}
	explicit ChainPackMap(RpcValue::Map &&value) : ValueData(move(value)) {}

	const RpcValue::Map &toMap() const override { return m_value; }
};

class ChainPackIMap final : public ValueData<RpcValue::Type::IMap, RpcValue::IMap>
{
	//const ChainPack::Map &toMap() const override { return m_value; }
	size_t count() const override {return m_value.size();}
	RpcValue at(RpcValue::UInt key) const override;
	void set(RpcValue::UInt key, const RpcValue &val) override;
	bool equals(const RpcValue::AbstractValueData * other) const override { return m_value == other->toIMap(); }
public:
	explicit ChainPackIMap(const RpcValue::IMap &value) : ValueData(value) {}
	explicit ChainPackIMap(RpcValue::IMap &&value) : ValueData(std::move(value)) {}

	const RpcValue::IMap &toIMap() const override { return m_value; }
};

class ChainPackNull final : public ValueData<RpcValue::Type::Null, std::nullptr_t>
{
	bool isNull() const override {return true;}
	bool equals(const RpcValue::AbstractValueData * other) const override { return other->isNull(); }
public:
	ChainPackNull() : ValueData({}) {}
};

/* * * * * * * * * * * * * * * * * * * *
 * Static globals - static-init-safe
 */
struct Statics
{
	const std::shared_ptr<RpcValue::AbstractValueData> null = std::make_shared<ChainPackNull>();
	const std::shared_ptr<RpcValue::AbstractValueData> t = std::make_shared<ChainPackBoolean>(true);
	const std::shared_ptr<RpcValue::AbstractValueData> f = std::make_shared<ChainPackBoolean>(false);
	const RpcValue::String empty_string;
	const RpcValue::Blob empty_blob;
	//const std::vector<ChainPack> empty_vector;
	//const std::map<ChainPack::String, ChainPack> empty_map;
	Statics() {}
};

static const Statics & statics()
{
	static const Statics s {};
	return s;
}

static const RpcValue::String & static_empty_string() { return statics().empty_string; }
static const RpcValue::Blob & static_empty_blob() { return statics().empty_blob; }

static const RpcValue & static_chain_pack_invalid() { static const RpcValue s{}; return s; }
//static const ChainPack & static_chain_pack_null() { static const ChainPack s{statics().null}; return s; }
static const RpcValue::List & static_empty_list() { static const RpcValue::List s{}; return s; }
static const RpcValue::Array & static_empty_array() { static const RpcValue::Array s{}; return s; }
static const RpcValue::Map & static_empty_map() { static const RpcValue::Map s{}; return s; }
static const RpcValue::IMap & static_empty_imap() { static const RpcValue::IMap s{}; return s; }

/* * * * * * * * * * * * * * * * * * * *
 * Constructors
 */

RpcValue::RpcValue() noexcept {}
RpcValue::RpcValue(std::nullptr_t) noexcept : m_ptr(statics().null) {}
RpcValue::RpcValue(double value) : m_ptr(std::make_shared<ChainPackDouble>(value)) {}
RpcValue::RpcValue(RpcValue::Decimal value) : m_ptr(std::make_shared<ChainPackDecimal>(std::move(value))) {}
RpcValue::RpcValue(Int value) : m_ptr(std::make_shared<ChainPackInt>(value)) {}
RpcValue::RpcValue(UInt value) : m_ptr(std::make_shared<ChainPackUInt>(value)) {}
RpcValue::RpcValue(bool value) : m_ptr(value ? statics().t : statics().f) {}
RpcValue::RpcValue(const DateTime &value) : m_ptr(std::make_shared<ChainPackDateTime>(value)) {}

RpcValue::RpcValue(const RpcValue::Blob &value) : m_ptr(std::make_shared<ChainPackBlob>(value)) {}
RpcValue::RpcValue(RpcValue::Blob &&value) : m_ptr(std::make_shared<ChainPackBlob>(std::move(value))) {}
RpcValue::RpcValue(const uint8_t * value, size_t size) : m_ptr(std::make_shared<ChainPackBlob>(value, size)) {}
RpcValue::RpcValue(const std::string &value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
RpcValue::RpcValue(std::string &&value) : m_ptr(std::make_shared<ChainPackString>(std::move(value))) {}
RpcValue::RpcValue(const char * value) : m_ptr(std::make_shared<ChainPackString>(value)) {}
RpcValue::RpcValue(const RpcValue::List &values) : m_ptr(std::make_shared<ChainPackList>(values)) {}
RpcValue::RpcValue(RpcValue::List &&values) : m_ptr(std::make_shared<ChainPackList>(std::move(values))) {}

RpcValue::RpcValue(const Array &values) : m_ptr(std::make_shared<ChainPackArray>(values)) {}
RpcValue::RpcValue(RpcValue::Array &&values) : m_ptr(std::make_shared<ChainPackArray>(std::move(values))) {}

RpcValue::RpcValue(const RpcValue::Map &values) : m_ptr(std::make_shared<ChainPackMap>(values)) {}
RpcValue::RpcValue(RpcValue::Map &&values) : m_ptr(std::make_shared<ChainPackMap>(std::move(values))) {}

RpcValue::RpcValue(const RpcValue::IMap &values) : m_ptr(std::make_shared<ChainPackIMap>(values)) {}
RpcValue::RpcValue(RpcValue::IMap &&values) : m_ptr(std::make_shared<ChainPackIMap>(std::move(values))) {}

RpcValue::~RpcValue()
{
	//std::cerr << __FUNCTION__ << " >>>>>>>>>>>>> " << m_ptr.get() << " ref cnt: " << m_ptr.use_count() << " val: " << toStdString() << std::endl;
}
#ifdef RPCVALUE_COPY_AND_SWAP
void RpcValue::swap(RpcValue& other) noexcept
{
	/*
	std::cerr << __FUNCTION__ << " xxxxxxxxxx "
			  << m_ptr.get() << " ref cnt: " << m_ptr.use_count() << " val: " << toStdString()
			  << " X "
			  << other.m_ptr.get() << " ref cnt: " << other.m_ptr.use_count() << " val: " << other.toStdString()
			  << std::endl;
	*/
	std::swap(m_ptr, other.m_ptr);
}
#endif
//Value::Value(const Value::MetaTypeId &value) : m_ptr(std::make_shared<ChainPackMetaTypeId>(value)) {}
//Value::Value(const Value::MetaTypeNameSpaceId &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceId>(value)) {}
//Value::Value(const Value::MetaTypeName &value) : m_ptr(std::make_shared<ChainPackMetaTypeName>(value)) {}
//Value::Value(const Value::MetaTypeNameSpaceName &value) : m_ptr(std::make_shared<ChainPackMetaTypeNameSpaceName>(value)) {}

/* * * * * * * * * * * * * * * * * * * *
 * Accessors
 */

RpcValue::Type RpcValue::type() const
{
	return m_ptr? m_ptr->type(): Type::Invalid;
}

RpcValue::Type RpcValue::arrayType() const
{
	return m_ptr? m_ptr->arrayType(): Type::Invalid;
}

const RpcValue::MetaData &RpcValue::metaData() const
{
	static MetaData md;
	if(m_ptr)
		return m_ptr->metaData();
	return md;
}

RpcValue RpcValue::metaValue(RpcValue::UInt key) const
{
	const MetaData &md = metaData();
	RpcValue ret = md.value(key);
	return ret;
}

RpcValue RpcValue::metaValue(const RpcValue::String &key) const
{
	const MetaData &md = metaData();
	RpcValue ret = md.value(key);
	return ret;
}

void RpcValue::setMetaData(RpcValue::MetaData &&meta_data)
{
	if(!m_ptr && !meta_data.isEmpty())
		SHVCHP_EXCEPTION("Cannot set valid meta data to invalid ChainPack value!");
	if(m_ptr)
		m_ptr->setMetaData(std::move(meta_data));
}

void RpcValue::setMetaValue(RpcValue::UInt key, const RpcValue &val)
{
	if(!m_ptr && val.isValid())
		SHVCHP_EXCEPTION("Cannot set valid meta value to invalid ChainPack value!");
	if(m_ptr)
		m_ptr->setMetaValue(key, val);
}

void RpcValue::setMetaValue(const RpcValue::String &key, const RpcValue &val)
{
	if(!m_ptr && val.isValid())
		SHVCHP_EXCEPTION("Cannot set valid meta value to invalid ChainPack value!");
	if(m_ptr)
		m_ptr->setMetaValue(key, val);
}

bool RpcValue::isValid() const
{
	return !!m_ptr;
}

double RpcValue::toDouble() const { return m_ptr? m_ptr->toDouble(): 0; }
RpcValue::Decimal RpcValue::toDecimal() const { return m_ptr? m_ptr->toDecimal(): Decimal(); }
RpcValue::Int RpcValue::toInt() const { return m_ptr? m_ptr->toInt(): 0; }
RpcValue::UInt RpcValue::toUInt() const { return m_ptr? m_ptr->toUInt(): 0; }
bool RpcValue::toBool() const { return m_ptr? m_ptr->toBool(): false; }
RpcValue::DateTime RpcValue::toDateTime() const { return m_ptr? m_ptr->toDateTime(): RpcValue::DateTime{}; }

const std::string & RpcValue::toString() const { return m_ptr? m_ptr->toString(): static_empty_string(); }
const RpcValue::Blob &RpcValue::toBlob() const { return m_ptr? m_ptr->toBlob(): static_empty_blob(); }

size_t RpcValue::count() const { return m_ptr? m_ptr->count(): 0; }
const RpcValue::List & RpcValue::toList() const { return m_ptr? m_ptr->toList(): static_empty_list(); }
const RpcValue::Array & RpcValue::toArray() const { return m_ptr? m_ptr->toArray(): static_empty_array(); }
const RpcValue::Map & RpcValue::toMap() const { return m_ptr? m_ptr->toMap(): static_empty_map(); }
const RpcValue::IMap &RpcValue::toIMap() const { return m_ptr? m_ptr->toIMap(): static_empty_imap(); }
RpcValue RpcValue::at (RpcValue::UInt i) const { return m_ptr? m_ptr->at(i): RpcValue(); }
RpcValue RpcValue::at (const RpcValue::String &key) const { return m_ptr? m_ptr->at(key): RpcValue(); }

void RpcValue::set(RpcValue::UInt ix, const RpcValue &val)
{
	if(m_ptr)
		m_ptr->set(ix, val);
	else
		nError() << " Cannot set value to invalid ChainPack value! Index: " << ix;
}

void RpcValue::set(const RpcValue::String &key, const RpcValue &val)
{
	if(m_ptr)
		m_ptr->set(key, val);
	else
		nError() << " Cannot set value to invalid ChainPack value! Key: " << key;
}

void RpcValue::append(const RpcValue &val)
{
	if(m_ptr)
		m_ptr->append(val);
	else
		nError() << "Cannot append to invalid ChainPack value!";
}

std::string RpcValue::toPrettyString(const std::string &indent) const
{
	std::ostringstream out;
	{
		CponWriterOptions opts;
		opts.setTranslateIds(true).setIndent(indent);
		CponWriter wr(out, opts);
		wr << *this;
	}
	return out.str();
}

std::string RpcValue::toCpon() const
{
	std::ostringstream out;
	{
		CponWriterOptions opts;
		opts.setTranslateIds(false);
		CponWriter wr(out, opts);
		wr << *this;
	}
	return out.str();
}

const std::string & RpcValue::AbstractValueData::toString() const { return static_empty_string(); }
const RpcValue::Blob & RpcValue::AbstractValueData::toBlob() const { return static_empty_blob(); }
const RpcValue::List & RpcValue::AbstractValueData::toList() const { return static_empty_list(); }
const RpcValue::Array &RpcValue::AbstractValueData::toArray() const { return static_empty_array(); }
const RpcValue::Map & RpcValue::AbstractValueData::toMap() const { return static_empty_map(); }
const RpcValue::IMap & RpcValue::AbstractValueData::toIMap() const { return static_empty_imap(); }

RpcValue RpcValue::AbstractValueData::at(RpcValue::UInt) const { return RpcValue(); }
RpcValue RpcValue::AbstractValueData::at(const RpcValue::String &) const { return RpcValue(); }

void RpcValue::AbstractValueData::set(RpcValue::UInt ix, const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::set: trivial implementation called! Key: " << ix;
}

void RpcValue::AbstractValueData::set(const RpcValue::String &key, const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::set: trivial implementation called! Key: " << key;
}

void RpcValue::AbstractValueData::append(const RpcValue &)
{
	nError() << "RpcValue::AbstractValueData::append: trivial implementation called!";
}


RpcValue ChainPackList::at(RpcValue::UInt i) const
{
	if (i >= m_value.size())
		return static_chain_pack_invalid();
	else
		return m_value[i];
}

void ChainPackList::set(RpcValue::UInt key, const RpcValue &val)
{
	if(key >= m_value.size())
		m_value.resize(key + 1);
	m_value[key] = val;
}

RpcValue ChainPackArray::at(RpcValue::UInt i) const
{
	if (i >= m_value.size())
		return static_chain_pack_invalid();
	else
		return m_value.valueAt(i);
}

void ChainPackArray::set(RpcValue::UInt key, const RpcValue &val)
{
	if(key >= m_value.size())
		m_value.resize(key + 1);
	m_value[key] = RpcValue::Array::makeElement(val);
}

/* * * * * * * * * * * * * * * * * * * *
 * Comparison
 */
bool RpcValue::operator== (const RpcValue &other) const
{
	if(isValid() && other.isValid()) {
		if (m_ptr->type() != other.m_ptr->type())
			return false;
		return m_ptr->equals(other.m_ptr.get());
	}
	return (!isValid() && !other.isValid());
}
/*
bool ChainPack::operator< (const ChainPack &other) const
{
	if(isValid() && other.isValid()) {
		if (m_ptr->type() != other.m_ptr->type())
			return m_ptr->type() < other.m_ptr->type();
		return m_ptr->less(other.m_ptr.get());
	}
	return (!isValid() && other.isValid());
}
*/

RpcValue RpcValue::parseCpon(const std::string &str, std::string *err)
{
	RpcValue ret;
	std::istringstream in(str);
	CponReader rd(in);
	if(err) {
		err->clear();
		try {
			rd >> ret;
		}
		catch(CponReader::ParseException &e) {
			*err = e.mesage();
		}
	}
	else {
		rd >> ret;
	}
	return ret;
}

const char *RpcValue::typeToName(RpcValue::Type t)
{
	switch (t) {
	case Type::Invalid: return "INVALID";
	case Type::Null: return "Null";
	case Type::UInt: return "UInt";
	case Type::Int: return "Int";
	case Type::Double: return "Double";
	case Type::Bool: return "Bool";
	case Type::Blob: return "Blob";
	case Type::String: return "String";
	case Type::List: return "List";
	case Type::Array: return "Array";
	case Type::Map: return "Map";
	case Type::IMap: return "IMap";
	case Type::DateTime: return "DateTime";
	//case Type::MetaIMap: return "MetaIMap";
	case Type::Decimal: return "Decimal";
	}
	return "UNKNOWN"; // just to remove mingw warning
}

RpcValue ChainPackMap::at(const RpcValue::String &key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? static_chain_pack_invalid() : iter->second;
}

void ChainPackMap::set(const RpcValue::String &key, const RpcValue &val)
{
	if(val.isValid())
		m_value[key] = val;
	else
		m_value.erase(key);
}

RpcValue ChainPackIMap::at(RpcValue::UInt key) const
{
	auto iter = m_value.find(key);
	return (iter == m_value.end()) ? static_chain_pack_invalid() : iter->second;
}

void ChainPackIMap::set(RpcValue::UInt key, const RpcValue &val)
{
	if(val.isValid())
		m_value[key] = val;
	else
		m_value.erase(key);
}
#if 0
RpcValue::DateTimeEpoch RpcValue::DateTimeEpoch::fromLocalString(const std::string &local_date_time_str)
{
	std::tm tm;
	unsigned msec;
	int utc_offset;
	DateTimeEpoch ret;
	if(!RpcValue::DateTime::parseISODateTime(local_date_time_str, tm, msec, utc_offset))
		return ret;

	std::time_t tim = std::mktime(&tm);
	if(tim == -1) {
		nError() << "Invalid date time string:" << local_date_time_str;
		return ret;
	}
	ret.m_msecs = tim * 1000 + msec;

	return ret;
}

RpcValue::DateTimeEpoch RpcValue::DateTimeEpoch::fromUtcString(const std::string &utc_date_time_str)
{
	std::tm tm;
	unsigned msec;
	int utc_offset;
	DateTimeEpoch ret;
	if(!RpcValue::DateTime::parseISODateTime(local_date_time_str, tm, msec, utc_offset))
		return ret;

	std::time_t tim = timegm(&tm);
	if(tim == -1) {
		nError() << "Invalid date time string:" << local_date_time_str;
		return ret;
	}
	ret.m_msecs = tim * 1000 + msec;

	return ret;
}

RpcValue::DateTimeEpoch RpcValue::DateTimeEpoch::fromMSecsSinceEpoch(int64_t msecs)
{
	DateTimeEpoch ret;
	ret.m_msecs = msecs;
	return ret;
}

std::string RpcValue::DateTimeEpoch::toLocalString() const
{
	std::time_t tim = m_msecs / 1000;
	std::tm *tm = std::localtime(&tim);
	if(tm == nullptr) {
		std::cerr << "Invalid date time: " << m_msecs;
	}
	else {
		char buffer[80];
		std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
		std::string ret(buffer);
		ret += '.' + Utils::toString(m_msecs % 1000);
		return ret;
	}
	return std::string();
}

std::string RpcValue::DateTimeEpoch::toUtcString() const
{
	std::time_t tim = m_msecs / 1000;
	std::tm *tm = std::gmtime(&tim);
	if(tm == nullptr) {
		std::cerr << "Invalid date time: " << m_msecs;
	}
	else {
		char buffer[80];
		std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
		std::string ret(buffer);
		ret += '.' + Utils::toString(m_msecs % 1000);
		return ret;
	}
	return std::string();
}
#endif

static bool parse_ISO_DateTime(const std::string &s, std::tm &tm, unsigned &msec, int8_t &utc_offset)
{
	std::istringstream iss(s);
	char sep;

	tm.tm_year = 0;
	tm.tm_mon = 0;
	tm.tm_mday = 0;
	tm.tm_hour = 0;
	tm.tm_min = 0;
	tm.tm_sec = 0;
	tm.tm_isdst = -1;

	msec = 0;
	utc_offset = 0;

	if (iss >> tm.tm_year >> sep >> tm.tm_mon >> sep >> tm.tm_mday) {
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
	}
	else {
		nError() << "Invalid date string:" << s << "too short";
		return false;
	}
	iss.get();
	if (iss >> tm.tm_hour >> sep >> tm.tm_min >> sep >> tm.tm_sec) {
	}
	else {
		nError() << "Invalid time string:" << s << "too short";
		return false;
	}

	while(!iss.eof()) {
		iss >> sep;
		if(sep == '.') {
			if(!(iss >> msec)) {
				nError() << "Invalid date time string:" << s << "msecs missing";
				return false;
			}
		}
		else if(sep == '+' || sep == '-') {
			int off;
			if(!(iss >> off)) {
				nError() << "Invalid date time string:" << s << "UTC offset missing";
				return false;
			}
			if(off < 99)
				off *= 100;
			off = off / 100 * 60 + (off % 100);
			off /= 15;
			utc_offset = (sep == '-')? -off: off;
		}
		else if(sep == 'Z') {
		}
		else {
			nError() << "Invalid date time string:" << s << "illegal UTC separator:" << sep;
			return false;
		}
	}

	return true;
}

RpcValue::DateTime RpcValue::DateTime::fromLocalString(const std::string &local_date_time_str)
{
	std::tm tm;
	unsigned msec;
	int8_t utc_offset;
	DateTime ret;
	if(!parse_ISO_DateTime(local_date_time_str, tm, msec, utc_offset))
		return ret;

	std::time_t tim = std::mktime(&tm);
	if(tim == -1) {
		nError() << "Invalid date time string:" << local_date_time_str;
		return ret;
	}
	ret.m_dtm.msec = (tim - utc_offset * 15 * 60) * 1000 + msec;
	ret.m_dtm.tz = utc_offset;

	return ret;
}

RpcValue::DateTime RpcValue::DateTime::fromUtcString(const std::string &utc_date_time_str)
{
	std::tm tm;
	unsigned msec;
	int8_t utc_offset;
	DateTime ret;
	if(!parse_ISO_DateTime(utc_date_time_str, tm, msec, utc_offset))
		return ret;

	std::time_t tim = timegm(&tm);
	if(tim == -1) {
		nError() << "Invalid date time string:" << utc_date_time_str;
		return ret;
	}
	ret.m_dtm.msec = (tim - utc_offset * 15 * 60) * 1000 + msec;
	ret.m_dtm.tz = utc_offset;

	return ret;
}

RpcValue::DateTime RpcValue::DateTime::fromMSecsSinceEpoch(int64_t msecs, int utc_offset_min)
{
	DateTime ret;
	ret.m_dtm.msec = msecs;
	ret.m_dtm.tz = utc_offset_min / 15;
	return ret;
}
/*
RpcValue::DateTime RpcValue::DateTime::fromMSecs20180202(int64_t msecs, int utc_offset_min)
{
	DateTime ret;
	ret.m_dtm.msec = msecs + SHV_EPOCH_MSEC;
	ret.m_dtm.tz = utc_offset_min / 15;
	return ret;
}
*/
std::string RpcValue::DateTime::toLocalString() const
{
	std::time_t tim = m_dtm.msec / 1000 + m_dtm.tz * 15 * 60;
	std::tm *tm = std::localtime(&tim);
	if(tm == nullptr) {
		nError() << "Invalid date time: " << m_dtm.msec;
		return std::string();
	}
	char buffer[80];
	std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
	std::string ret(buffer);
	int msecs = m_dtm.msec % 1000;
	if(msecs > 0)
		ret += '.' + Utils::toString(msecs % 1000);
	return ret;
}

std::string RpcValue::DateTime::toUtcString() const
{
	std::time_t tim = m_dtm.msec / 1000 + m_dtm.tz * 15 * 60;
	std::tm *tm = std::gmtime(&tim);
	if(tm == nullptr) {
		nError() << "Invalid date time: " << m_dtm.msec;
		return std::string();
	}
	char buffer[80];
	std::strftime(buffer, sizeof(buffer),"%Y-%m-%dT%H:%M:%S",tm);
	std::string ret(buffer);
	int msecs = m_dtm.msec % 1000;
	if(msecs > 0)
		ret += '.' + Utils::toString(msecs % 1000, 3);
	if(m_dtm.tz == 0) {
		ret += 'Z';
	}
	else {
		int min = offsetFromUtc();
		if(min < 0)
			min = -min;
		int hour = min / 60;
		min = min % 60;
		std::string tz = Utils::toString(hour, 2);
		if(min != 0)
			tz += Utils::toString(min, 2);
		ret += ((m_dtm.tz < 0)? '-': '+') + tz;
	}
	return ret;
}

RpcValue::MetaData::MetaData(RpcValue::MetaData &&o)
	: MetaData()
{
	swap(o);
}

RpcValue::MetaData::MetaData(RpcValue::IMap &&imap)
{
	if(!imap.empty())
		m_imap = new RpcValue::IMap(std::move(imap));
}

RpcValue::MetaData::MetaData(RpcValue::Map &&smap)
{
	if(!smap.empty())
		m_smap = new RpcValue::Map(std::move(smap));
}

RpcValue::MetaData::MetaData(RpcValue::IMap &&imap, RpcValue::Map &&smap)
{
	if(!imap.empty())
		m_imap = new RpcValue::IMap(std::move(imap));
	if(!smap.empty())
		m_smap = new RpcValue::Map(std::move(smap));
}

RpcValue::MetaData::~MetaData()
{
	if(m_imap)
		delete m_imap;
	if(m_smap)
		delete m_smap;
}

std::vector<RpcValue::UInt> RpcValue::MetaData::iKeys() const
{
	std::vector<RpcValue::UInt> ret;
	for(const auto &it : iValues())
		ret.push_back(it.first);
	return ret;
}

std::vector<RpcValue::String> RpcValue::MetaData::sKeys() const
{
	std::vector<RpcValue::String> ret;
	for(const auto &it : sValues())
		ret.push_back(it.first);
	return ret;
}

RpcValue RpcValue::MetaData::value(RpcValue::UInt key) const
{
	const IMap &m = iValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	return RpcValue();
}

RpcValue RpcValue::MetaData::value(RpcValue::String key) const
{
	const Map &m = sValues();
	auto it = m.find(key);
	if(it != m.end())
		return it->second;
	return RpcValue();
}

void RpcValue::MetaData::setValue(RpcValue::UInt key, const RpcValue &val)
{
	if(val.isValid()) {
		if(!m_imap)
			m_imap = new RpcValue::IMap();
		(*m_imap)[key] = val;
	}
	else {
		if(m_imap)
			m_imap->erase(key);
	}
}

void RpcValue::MetaData::setValue(RpcValue::String key, const RpcValue &val)
{
	if(val.isValid()) {
		if(!m_smap)
			m_smap = new RpcValue::Map();
		(*m_smap)[key] = val;
	}
	else {
		if(m_smap)
			m_smap->erase(key);
	}
}

bool RpcValue::MetaData::isEmpty() const
{
	return (!m_imap || m_imap->empty()) && (!m_smap || m_smap->empty());
}

bool RpcValue::MetaData::operator==(const RpcValue::MetaData &o) const
{
	/*
	std::cerr << "this" << std::endl;
	for(const auto &it : m_imap)
		std::cerr << '\t' << it.first << ": " << it.second.dumpText() << std::endl;
	std::cerr << "other" << std::endl;
	for(const auto &it : o.m_imap)
		std::cerr << '\t' << it.first << ": " << it.second.dumpText() << std::endl;
	*/
	return iValues() == o.iValues() && sValues() == o.sValues();
}

const RpcValue::IMap &RpcValue::MetaData::iValues() const
{
	static RpcValue::IMap m;
	return m_imap? *m_imap: m;
}

const RpcValue::Map &RpcValue::MetaData::sValues() const
{
	static RpcValue::Map m;
	return m_smap? *m_smap: m;
}

std::string RpcValue::MetaData::toStdString() const
{
	std::ostringstream out;
	{
		CponWriterOptions opts;
		opts.setTranslateIds(true);
		CponWriter wr(out, opts);
		wr << *this;
	}
	return out.str();
}

void RpcValue::MetaData::swap(RpcValue::MetaData &o)
{
	std::swap(m_imap, o.m_imap);
	std::swap(m_smap, o.m_smap);
}

std::string RpcValue::Decimal::toString() const
{
	std::string ret = Utils::toString(mantisa());
	int prec = precision();
	if(prec >= 0) {
		int len = (int)ret.length();
		if(prec > len)
			ret.insert(0, prec - len, '0'); // insert '0' after dec point
		ret.insert(ret.length() - prec, 1, '.');
		if(prec >= len)
			ret.insert(0, 1, '0'); // insert '0' before dec point
	}
	else {
		ret.insert(ret.length(), -prec, '0');
		ret.push_back('.');
	}
	return ret;
}

}}
