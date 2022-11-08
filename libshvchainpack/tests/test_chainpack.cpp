#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <shv/chainpack/chainpack.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/utils.h>

#include <necrolog.h>

#include <doctest/doctest.h>

#include <optional>

using namespace shv::chainpack;
using std::string;

// Check that ChainPack has the properties we want.
#define CHECK_TRAIT(x) static_assert(std::x::value, #x)
CHECK_TRAIT(is_nothrow_constructible<RpcValue>);
CHECK_TRAIT(is_nothrow_default_constructible<RpcValue>);
CHECK_TRAIT(is_copy_constructible<RpcValue>);
CHECK_TRAIT(is_move_constructible<RpcValue>);
CHECK_TRAIT(is_copy_assignable<RpcValue>);
CHECK_TRAIT(is_move_assignable<RpcValue>);
CHECK_TRAIT(is_nothrow_destructible<RpcValue>);

namespace {

template< typename T >
std::string int_to_hex( T i )
{
	std::stringstream stream;
	stream << "0x"
			  //<< std::setfill ('0') << std::setw(sizeof(T)*2)
		   << std::hex << i;
	return stream.str();
}

inline char hex_nibble(char i)
{
	if(i < 10)
		return '0' + i;
	return 'A' + (i - 10);
}

std::string hex_dump(const RpcValue::String &out)
{
	std::string ret;
	for (char i : out) {
		char h = i / 16;
		char l = i % 16;
		ret += hex_nibble(h);
		ret += hex_nibble(l);
	}
	return ret;
}

std::string binary_dump(const RpcValue::String &out)
{
	std::string ret;
	for (size_t i = 0; i < out.size(); ++i) {
		auto u = static_cast<uint8_t>(out[i]);
		//ret += std::to_string(u);
		if(i > 0)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & ((static_cast<uint8_t>(128)) >> j))? '1': '0';
		}
	}
	return ret;
}
/*
std::string binary_dump_rev(const void *data, size_t len)
{
	std::string ret;
	for (size_t i = len-1; ; --i) {
		uint8_t u = (reinterpret_cast<const uint8_t*>(data))[i];
		if(i < len-1)
			ret += '|';
		for (size_t j = 0; j < 8*sizeof(u); ++j) {
			ret += (u & ((static_cast<uint8_t>(128)) >> j))? '1': '0';
		}
		if(i == 0)
			break;
	}
	return ret;
}

void write_double_bits()
{
	for (double d : {
		 0.,
		// 1.,  2., 3., 4., 5., 6., 7.,
		// -1., -2., -3.,
		 static_cast<double>(1 << 0),
		 static_cast<double>((static_cast<uint64_t>(1) << 2) + 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 4) + 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 8) + 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 16) + 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 32) + 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 51) + 0),
		 static_cast<double>((static_cast<uint64_t>(1) << 52) - 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 53) - 1),
		 static_cast<double>((static_cast<uint64_t>(1) << 54) - 1),
		 -static_cast<double>((static_cast<uint64_t>(1) << 51) + 0),
		 -static_cast<double>((static_cast<uint64_t>(1) << 52) - 1),
		 1./3.,
		 0.1,
		 0.123,
		 0.1234,
		 0.12345,
		 1.1,
		 2.123,
		 3.1234,
		 4.12345,
		 std::numeric_limits<double>::quiet_NaN(),
		 std::numeric_limits<double>::infinity(),
		}
		 ) {
		uint64_t *pn = reinterpret_cast<uint64_t*>(&d);
		uint64_t mant_mask = ((static_cast<uint64_t>(1) << 52) - 1);
		uint64_t umant = *pn & mant_mask;
		uint64_t exp_mask = ((static_cast<uint64_t>(1) << 11) - 1) << 52;
		uint uexp = static_cast<uint>((*pn & exp_mask) >> 52);
		uint64_t sgn_mask = ~static_cast<uint64_t>(0x7fffffffffffffff);
		int sgn = (*pn & sgn_mask)? -1: 1;
		int exponent;
		int64_t mantisa;
		if(uexp == 0 && umant == 0) {
			exponent = 0;
			mantisa = 0;
		}
		else if(uexp == 0x7ff) {
			exponent = static_cast<int>(uexp);
			if(umant == 0) {
				// infinity;
				nDebug() << sgn << "* INF";
				mantisa = 0;
			}
			else {
				// NaN
				nDebug() << "NaN";
				mantisa = 1;
			}
		}
		else {
			if(uexp == 0) {
				// subnormal
				mantisa = static_cast<int64_t>(umant);
			}
			else {
				mantisa = static_cast<int64_t>(umant | (static_cast<uint64_t>(1) << 52));
			}
			exponent = static_cast<int>(uexp) - 1023;
			exponent -= 52;
			for (int i=0; i<52; i--) {
				if(mantisa & 1)
					break;
				mantisa >>= 1;
				exponent++;
			}
			mantisa *= sgn;
		}
		nDebug() << d << "----------------------------------";
		nDebug()<< "mantisa:" << mantisa << "exp:" << exponent;
		nDebug()<< "neg:" << sgn << "umant:" << umant << "uexp:" << uexp;
		nDebug() << binary_dump_rev(pn, sizeof (*pn)).c_str();
		//nDebug() << binary_dump_rev((void*)&exp_mask, sizeof (exp_mask)).c_str();
		//nDebug() << binary_dump_rev((void*)&mant_mask, sizeof (mant_mask)).c_str();
		//nDebug() << binary_dump_rev((void*)&uexp, sizeof (uexp)).c_str();
		//nDebug() << binary_dump_rev((void*)&umant, sizeof (umant)).c_str();
	}
}
*/
}

namespace shv::chainpack {

doctest::String toString(const RpcValue& value) {
	return value.toCpon().c_str();
}
/*
doctest::String toString(const RpcValue::List& value) {
	return RpcValue(value).toCpon().c_str();
}

doctest::String toString(const RpcValue::Map& value) {
	return RpcValue(value).toCpon().c_str();
}
*/
}

DOCTEST_TEST_CASE("ChainPack")
{
	DOCTEST_SUBCASE("Dump packing schema")
	{
		nDebug() << "============= chainpack binary test ============";
		for (int i = ChainPack::PackingSchema::Null; i <= ChainPack::PackingSchema::CString; ++i) {
			RpcValue::String out;
			out += static_cast<char>(i);
			auto e = static_cast<ChainPack::PackingSchema::Enum>(i);
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPack::PackingSchema::name(e);
			nInfo() << str.str();
		}
		for (int i = ChainPack::PackingSchema::FALSE; i <= ChainPack::PackingSchema::TERM; ++i) {
			RpcValue::String out;
			out += static_cast<char>(i);
			auto e = static_cast<ChainPack::PackingSchema::Enum>(i);
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPack::PackingSchema::name(e);
			nInfo() << str.str();
		}
	}
	DOCTEST_SUBCASE("NULL")
	{
		RpcValue cp1{nullptr};
		std::stringstream out;
		{ ChainPackWriter wr(out);  wr.write(cp1); }
		REQUIRE(out.str().size() == 1);
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
		REQUIRE(cp1.type() == cp2.type());
	}
	DOCTEST_SUBCASE("tiny uint")
	{
		for (RpcValue::UInt n = 0; n < 64; ++n) {
			RpcValue cp1{n};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			if(n < 10)
				nDebug() << n << " " << cp1.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(out.str().size() == 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toUInt() == cp2.toUInt());
		}
	}
	DOCTEST_SUBCASE("uint")
	{
		for (unsigned i = 0; i < sizeof(RpcValue::UInt); ++i) {
			for (unsigned j = 0; j < 3; ++j) {
				RpcValue::UInt n = RpcValue::UInt{1} << (i*8 + j*3+1);
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				//REQUIRE(len > 1);
				ChainPackReader rd(out);
				RpcValue cp2 = rd.read();
				//if(n < 100*step)
				nDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toUInt() == cp2.toUInt());
			}
		}
	}
	DOCTEST_SUBCASE("tiny int")
	for (RpcValue::Int n = 0; n < 64; ++n) {
		RpcValue cp1{n};
		std::stringstream out;
		{ ChainPackWriter wr(out);  wr.write(cp1); }
		if(out.str().size() < 10)
			nDebug() << n << " " << cp1.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
		REQUIRE(out.str().size() == 1);
		ChainPackReader rd(out); RpcValue cp2 = rd.read();
		REQUIRE(cp1.type() == cp2.type());
		REQUIRE(cp1.toInt() == cp2.toInt());
	}
	DOCTEST_SUBCASE("int")
	{
		for (int sig = 1; sig >= -1; sig-=2) {
			for (unsigned i = 0; i < sizeof(RpcValue::Int); ++i) {
				for (unsigned j = 0; j < 3; ++j) {
					RpcValue::Int n = sig * (RpcValue::Int{1} << (i*8 + j*2+2));
					//nDebug() << sig << i << j << (i*8 + j*3+1) << n;
					RpcValue cp1{n};
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					//REQUIRE(len > 1);
					ChainPackReader rd(out); RpcValue cp2 = rd.read();
					//if(n < 100*step)
					nDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					REQUIRE(cp1.type() == cp2.type());
					REQUIRE(cp1.toUInt() == cp2.toUInt());
				}
			}
		}
	}
	DOCTEST_SUBCASE("double")
	{
		{
			auto n_max = 1000000.;
			auto n_min = -1000000.;
			auto step = (n_max - n_min) / 100;
			for (auto n = n_min; n < n_max; n += step) {
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				REQUIRE(out.str().size() > 1);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				if(n > -3*step && n < 3*step)
					nDebug() << n << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toDouble() == cp2.toDouble());
			}
		}
		{
			double n_max = std::numeric_limits<double>::max();
			double n_min = std::numeric_limits<double>::min();
			double step = -1.23456789e10;
			//nDebug() << n_min << " - " << n_max << ": " << step << " === " << (n_max / step / 10);
			for (double n = n_min; n < n_max / -step / 10; n *= step) {
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				REQUIRE(out.str().size() > 1);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				if(n > -100 && n < 100)
					nDebug() << n << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toDouble() == cp2.toDouble());
			}
		}
	}
	DOCTEST_SUBCASE("Decimal")
	{
		RpcValue::Int mant = 123456789;
		int prec_max = 16;
		int prec_min = -16;
		int step = 1;
		for (int prec = prec_min; prec <= prec_max; prec += step) {
			RpcValue cp1{RpcValue::Decimal(mant, prec)};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			REQUIRE(out.str().size() > 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << mant << prec << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1 == cp2);
		}
	}
	DOCTEST_SUBCASE("bool")
	{
		for(bool b : {false, true}) {
			RpcValue cp1{b};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			REQUIRE(out.str().size() == 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << b << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toBool() == cp2.toBool());
		}
	}
	DOCTEST_SUBCASE("Blob")
	{
		std::string s = "blob containing zero character";
		RpcValue::Blob blob{s.begin(), s.end()};
		blob[blob.size() - 9] = 0;
		RpcValue cp1{blob};
		std::stringstream out;
		{
			ChainPackWriter wr(out);
			wr.write(cp1);
		}
		ChainPackReader rd(out);
		RpcValue cp2 = rd.read();
		//nDebug() << blob << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
		REQUIRE(cp1.type() == cp2.type());
		REQUIRE(cp1.asBlob() == cp2.asBlob());
	}
	DOCTEST_SUBCASE("string")
	{
		{
			RpcValue::String str{"string containing zero character"};
			str[str.size() - 10] = 0;
			RpcValue cp1{str};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << str << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.asString() == cp2.asString());
		}
		{
			// long string
			RpcValue::String str;
			for (int i = 0; i < 1000; ++i)
				str += std::to_string(i % 10);
			RpcValue cp1{str};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			//nDebug() << str << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp2.asString().size() == str.size());
			REQUIRE(cp1 == cp2);
		}
	}
	DOCTEST_SUBCASE("DateTime")
	{
		REQUIRE(RpcValue::DateTime::fromUtcString("") == RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime() == RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(0) == RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(1) == RpcValue::DateTime::fromMSecsSinceEpoch(1, 2));
		REQUIRE(!(RpcValue::DateTime() < RpcValue::DateTime()));
		//REQUIRE(RpcValue::DateTime() < RpcValue::DateTime::fromMSecsSinceEpoch(0));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(1) < RpcValue::DateTime::fromMSecsSinceEpoch(2));
		REQUIRE(RpcValue::DateTime::fromMSecsSinceEpoch(0) == RpcValue::DateTime::fromUtcString("1970-01-01T00:00:00"));
		//RpcValue::DateTime ts;// = RpcValue::DateTime::now();
		//nDebug() << "~~~~~~~~~~~~~~~~~~~~~~~~~ " << RpcValue(ts).toCpon();
		for(std::string str : {
			"2018-02-02 0:00:00.001",
			"2018-02-02 01:00:00.001+01",
			"2018-12-02 0:00:00",
			"2018-01-01 0:00:00",
			"2019-01-01 0:00:00",
			"2020-01-01 0:00:00",
			"2021-01-01 0:00:00",
			"2031-01-01 0:00:00",
			"2041-01-01 0:00:00",
			"2041-03-04 0:00:00-1015",
			"2041-03-04 0:00:00.123-1015",
			"1970-01-01 0:00:00",
			"2017-05-03 5:52:03",
			"2017-05-03T15:52:03.923Z",
			"2017-05-03T15:52:31.123+10",
			"2017-05-03T15:52:03Z",
			"2017-05-03T15:52:03.000-0130",
			"2017-05-03T15:52:03.923+00",
		}) {
			RpcValue::DateTime dt = RpcValue::DateTime::fromUtcString(str);
			RpcValue cp1{dt};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			std::string pack = out.str();
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << str << " " << dt.toIsoString().c_str() << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(pack);
			//nDebug() << cp1.toDateTime().msecsSinceEpoch() << cp1.toDateTime().offsetFromUtc();
			//nDebug() << cp2.toDateTime().msecsSinceEpoch() << cp2.toDateTime().offsetFromUtc();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toDateTime() == cp2.toDateTime());
		}
	}
	DOCTEST_SUBCASE("List")
	{
		{
			for(const std::string s : {"[]", "[[]]", R"(["a",123,true,[1,2,3],null])"}) {
				string err;
				RpcValue cp1 = RpcValue::fromCpon(s, &err);
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				nDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				REQUIRE(cp1.type() == cp2.type());
				REQUIRE(cp1.toList() == cp2.toList());
			}
		}
		{
			RpcValue cp1{RpcValue::List{1,2,3}};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toList() == cp2.toList());
		}
		{
			static constexpr size_t N = 10;
			std::stringstream out;
			{
				ChainPackWriter wr(out);
				wr.writeContainerBegin(RpcValue::Type::List);
				std::string s("foo-bar");
				for (size_t i = 0; i < N; ++i) {
					wr.writeListElement(RpcValue(s + shv::chainpack::Utils::toString(i)));
				}
				wr.writeContainerEnd();
			}
			//out.exceptions(std::iostream::eofbit);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp2.toCpon() << " dump: " << binary_dump(out.str()).c_str();
			const RpcValue::List list = cp2.toList();
			REQUIRE(list.size() == N);
			for (size_t i = 0; i < list.size(); ++i) {
				std::string s("foo-bar");
				REQUIRE(RpcValue(s + shv::chainpack::Utils::toString(i)) == list.at(i));
			}
		}
	}
	DOCTEST_SUBCASE("Map")
	{
		{
			RpcValue cp1{{
					{"foo", RpcValue::List{11,12,13}},
					{"bar", 2},
					{"baz", 3},
						 }};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toMap() == cp2.toMap());
		}
		{
			RpcValue::Map m{
					{"foo", RpcValue::List{11,12,13}},
					{"bar", 2},
					{"baz", 3},
						 };
			std::stringstream out;
			{
				ChainPackWriter wr(out);
				wr.writeContainerBegin(RpcValue::Type::Map);
				for(auto it : m)
					wr.writeMapElement(it.first, it.second);
				wr.writeContainerEnd();
			}
			ChainPackReader rd(out);
			RpcValue::Map m2 = rd.read().toMap();
			for(auto it : m2) {
				REQUIRE(it.second == m[it.first]);
			}
		}
		for(const std::string s : {"{}", R"({"a":{}})", R"({"foo":{"bar":"baz"}})"}) {
			string err;
			RpcValue cp1 = RpcValue::fromCpon(s, &err);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1 == cp2);
		}
	}
	DOCTEST_SUBCASE("IMap")
	{
		{
			RpcValue::IMap map {
				{1, "foo"},
				{2, "bar"},
				{333, 15U},
			};
			RpcValue cp1{map};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toIMap() == cp2.toIMap());
		}
		{
			RpcValue cp1{{
					{127, RpcValue::List{11,12,13}},
					{128, 2},
					{129, 3},
						 }};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.toIMap() == cp2.toIMap());
		}
	}
	DOCTEST_SUBCASE("Meta")
	{
		{
			nDebug() << "------------- Meta1";
			RpcValue cp1{RpcValue::String{"META1"}};
			cp1.setMetaValue(meta::Tag::MetaTypeId, 11);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			std::ostream::pos_type consumed = out.tellg();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			nDebug() << "hex:" << hex_dump(out.str());
			REQUIRE(out.str().size() == static_cast<size_t>(consumed));
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.metaData() == cp2.metaData());
		}
		{
			nDebug() << "------------- Meta2";
			RpcValue cp1{RpcValue::List{"META2", 17, 18, 19}};
			cp1.setMetaValue(meta::Tag::MetaTypeNameSpaceId, 12);
			cp1.setMetaValue(meta::Tag::MetaTypeId, 2);
			cp1.setMetaValue(meta::Tag::USER, "foo");
			cp1.setMetaValue(meta::Tag::USER+1, RpcValue::List{1,2,3});
			cp1.setMetaValue("bar", 3);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			std::ostream::pos_type consumed = out.tellg();
			nDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			nDebug() << "hex:" << hex_dump(out.str());
			REQUIRE(out.str().size() == static_cast<size_t>(consumed));
			REQUIRE(cp1.type() == cp2.type());
			REQUIRE(cp1.metaData() == cp2.metaData());
		}
	}

}
