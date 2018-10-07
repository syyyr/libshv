#include <shv/chainpack/rpcvalue.h>
#include <shv/chainpack/chainpack.h>
#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/chainpackreader.h>
#include <shv/chainpack/cponreader.h>

#include <QtTest/QtTest>
#include <QDebug>

#include <cassert>
#include <string>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <list>
#include <set>
#include <unordered_map>
#include <algorithm>
#include <type_traits>

#ifdef __linux

#ifdef RUSAGE_USAGE
#include <sys/time.h>
#include <sys/resource.h>

static void dump_memory()
{
	struct rusage ru;
	getrusage(RUSAGE_SELF, &ru);
	qDebug() << "integral shared memory size     :" << ru.ru_ixrss << "kB";
	qDebug() << "integral unshared data size     :" << ru.ru_idrss;
	qDebug() << "integral unshared stack size    :" << ru.ru_isrss;
	qDebug() << "page reclaims (soft page faults):" << ru.ru_minflt;
	qDebug() << "page faults (hard page faults)  :" << ru.ru_majflt;
	qDebug() << "swaps                           :" << ru.ru_nswap;
}
#endif

#include <unistd.h>
#include <fstream>

void process_mem_usage(double& vm_usage, double& resident_set)
{
	vm_usage     = 0.0;
	resident_set = 0.0;

	// the two fields we want
	unsigned long vsize;
	long rss;
	{
		std::string ignore;
		std::ifstream ifs("/proc/self/stat", std::ios_base::in);
		ifs >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
				>> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore >> ignore
				>> ignore >> ignore >> vsize >> rss;
	}

	long page_size_kb = ::sysconf(_SC_PAGE_SIZE) / 1024; // in case x86-64 is configured to use 2MB pages
	vm_usage = vsize / 1024.0;
	resident_set = rss * page_size_kb;
}

static void dump_memory()
{
	double vm, rss;
	process_mem_usage(vm, rss);
	//cout << "VM: " << vm << "; RSS: " << rss << endl;
	qDebug() << "\tVM :" << vm;
	qDebug() << "\tRSS:" << rss;
	//qDebug() << "\tNNN:" << shv::chainpack::RpcValue::AbstractValueData::nn();
}


#endif

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
QDebug operator<<(QDebug debug, const std::string &s)
{
	//QDebugStateSaver saver(debug);
	debug << s.c_str();

	return debug;
}

template< typename T >
std::string int_to_hex( T i )
{
	std::stringstream stream;
	stream << "0x"
			  //<< std::setfill ('0') << std::setw(sizeof(T)*2)
		   << std::hex << i;
	return stream.str();
}

}

class TestRpcValue: public QObject
{
	Q_OBJECT
private:
	void textTest()
	{
		qDebug() << "============= chainpack text test ============";
		qDebug() << "--------------- String";
		{
			string err;
			for(auto test : {
					"\"a0728a532288587d76617976985992eb3830e259577c7a6d2e2d0758215e2b6f3a399b4d34e12cd91a96d288751ce29d1a8306dfa074bdff86f08c6fa15a54404cbce5ccd7aeb19042e3df52a5e8f6cc9f9bd7153b13530c5665ecafc1be7f9ad30a62d93870e26277b1d9c2d9a7aff6f397157e5248e2b97e126b65ec8f887246767e344ce152c34846e649ea0a7a9a2b79dbe40d55ec231c4104ccc0c3b7b48895c4ca7ab7cf89a1302ed7a3bf2a6cf3862f05d200488c37d8e98f0369d844078e63fced87df6195ecb2fb2f9b532fc3880d6b6e6d880bc79c0e3fbaf387ebbc020cd46675537252aff233cb205ca627fa803b8af672d8cb5bdbfbf75526f\"",
				}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
				QVERIFY(cp.toCpon() == test);
			}
		}
		qDebug() << "--------------- Numbers";
		RpcValue v((int64_t)1526303051038ll);
		auto s = v.toCpon();
		qDebug() << s;
		qDebug() << RpcValue::fromCpon("1526303051038").toCpon();
		{
			string err;
			QVERIFY(RpcValue::fromCpon("0", &err) == RpcValue(0) && err.empty());
			QVERIFY(RpcValue::fromCpon("123", &err) == RpcValue(123) && err.empty());
			QVERIFY(RpcValue::fromCpon("1526303051038", &err) == RpcValue((int64_t)1526303051038ll) && err.empty());
			QVERIFY(RpcValue::fromCpon("-123", &err) == RpcValue(-123) && err.empty());
			QVERIFY(RpcValue::fromCpon("3u", &err) == RpcValue((RpcValue::UInt)3) && err.empty());
			QVERIFY(RpcValue::fromCpon("223.", &err) == RpcValue(223.) && err.empty());
			QVERIFY(RpcValue::fromCpon("0.123", &err) == RpcValue(0.123) && err.empty());
			QVERIFY(RpcValue::fromCpon("0.123", &err) == RpcValue(.123) && err.empty());
			QVERIFY(RpcValue::fromCpon("1e23", &err) == RpcValue(1e23) && err.empty());
			QVERIFY(RpcValue::fromCpon("1e-3", &err) == RpcValue(1e-3) && err.empty());
			QVERIFY(RpcValue::fromCpon("12.3e-10", &err) == RpcValue(12.3e-10) && err.empty());
			QVERIFY(RpcValue::fromCpon("0.0123n", &err) == RpcValue(RpcValue::Decimal(123, 4)) && err.empty());
		}
		qDebug() << "--------------- List test";
		{
			string err;
			for(auto test : {"[]", "[ ]", " [ ]", "[ 1, 2,3  , ]"}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			const string test = R"(
								[
									"foo bar",
									123,
									true,
									false,
									null,
									456u,
									0.123,
									123.456n,
									d"2018-01-14T01:17:33.256-1045"
									]
								)";
			const auto cp = RpcValue::fromCpon(test, &err);
			QVERIFY(err.empty());
			qDebug() << "List test:" << test << "==" << cp.toCpon();
			qDebug() << "Cpon writer:" << cp.toPrettyString();
			QVERIFY(cp[0] == "foo bar");
			QVERIFY(cp[1] == 123);
			QVERIFY(cp[2] == true);
			QVERIFY(cp[3] == false);
			QVERIFY(cp[4] == RpcValue(nullptr));
			QVERIFY(cp[5] == RpcValue::UInt(456));
			QVERIFY(cp[6] == 0.123);
			QVERIFY(cp[7] == RpcValue::Decimal(123456, 3));
			RpcValue::DateTime dt = cp[8].toDateTime();
			QVERIFY(dt.msecsSinceEpoch() % 1000 == 256);
			QVERIFY(dt.minutesFromUtc() == -(10*60+45));
		}
		{
			string err;
			for(auto test : {"{}", "{ }", " { }", R"({ "1": 2,"3": 45u  , })"}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			for(auto test : {
					"i{}",
					"i{ }",
					" i{ }",
					"i{ 1: 2,3: 45  , }",
				}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			for(auto test : {" <>[1]", "<>[]", "<1:2>[ ]", "<\"foo\":1>[ 1, 2,3  , ]"}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
			}
		}
		{
			string err;
			for(auto test : {"[[]]", "[[[]]]", "{\"a\":{}}", "i{1:i{}}", "<8:3u>i{2:[[\".broker\",true]]}"}) {
				const RpcValue cp = RpcValue::fromCpon(test, &err);
				qDebug() << test << "--->" << cp.toCpon();
				QVERIFY(err.empty());
				QVERIFY(cp.toCpon() == test);
			}
		}
		{
			string err;
			const string imap_test = R"(
									 <1:1, 2u: "bar", "pi-value":3.14>
									 i{
									   1:"v1",
									   2:42,
									   3:[
									     "a",
									     {"foo":1,"bar":2,"baz":3},
									     123,
									     true,
									     false,
									     null,
									     123, 456u, 0.123, 123.456n
									   ]
									 }
									 )";
			{
				std::istringstream in(imap_test);
				CponReader rd(in);
				RpcValue::MetaData md;
				rd.read(md);
				qDebug().nospace() << "metadata test: \n" << md.toStdString();
				QVERIFY(!md.isEmpty());
			}
			{
				const auto cp = RpcValue::fromCpon(imap_test, &err);
				qDebug().nospace() << "imap_test: \n" << cp.toPrettyString();
				qDebug() << "err: " << err;
				QVERIFY(err.empty());
			}
		}

		qDebug() << "--------------- Map test";
		{
			string err;
			const string test = "{}";
			const auto cp = RpcValue::fromCpon(test, &err);
			QVERIFY(cp == RpcValue::Map());
		}

		const string simple_test = R"(
								   {
								     "k1":"v1",
								     "k2":42,
								     "k3":["a",123,true,false,null],
								   }
								   )";

		string err;
		const auto json = RpcValue::fromCpon(simple_test, &err);

		qDebug() << "k1: " << json["k1"].toCpon().c_str();
		qDebug() << "k3: " << json["k3"].toCpon().c_str();

		RpcValue cp = json;
		//cp.setMeta(json["k1"]);
		qDebug() << "cp: " << cp.toCpon();

		string comment_test = R"({
							  // comment /* with nested comment */
							  "a": 1,
							  // comment
							  // continued
							  "b": "text",
							  /* multi
							  line
							  comment
							  // line-comment-inside-multiline-comment
							  */
							  // and single-line comment
							  // and single-line comment /* multiline inside single line */
							  "c": [1, 2, 3]
							  // and single-line comment at end of object
							  })";

		string err_comment;
		auto json_comment = RpcValue::fromCpon( comment_test, &err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		comment_test = "{\"a\": 1}//trailing line comment";
		json_comment = RpcValue::fromCpon( comment_test, &err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		comment_test = "{\"a\": 1}/*trailing multi-line comment*/";
		json_comment = RpcValue::fromCpon( comment_test, &err_comment);
		QVERIFY(!json_comment.isNull());
		QVERIFY(err_comment.empty());

		string failing_comment_test = "{\n/* unterminated comment\n\"a\": 1,\n}";
		string err_failing_comment;
		RpcValue json_failing_comment = RpcValue::fromCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n/* unterminated trailing comment }";
		json_failing_comment = RpcValue::fromCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{\n/ / bad comment }";
		json_failing_comment = RpcValue::fromCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		failing_comment_test = "{// bad comment }";
		json_failing_comment = RpcValue::fromCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());
		/*
		failing_comment_test = "{\n\"a\": 1\n}/";
		json_failing_comment = RpcValue::parseCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());
		*/
		failing_comment_test = "{/* bad\ncomment *}";
		json_failing_comment = RpcValue::fromCpon( failing_comment_test, &err_failing_comment);
		QVERIFY(!json_failing_comment.isValid());
		QVERIFY(!err_failing_comment.empty());

		{
			std::list<int> l1 { 1, 2, 3 };
			std::vector<int> l2 { 1, 2, 3 };
			std::set<int> l3 { 1, 2, 3 };
			QCOMPARE(RpcValue(l1), RpcValue(l2));
			QCOMPARE(RpcValue(l2), RpcValue(l3));
			std::map<string, string> m1 { { "k1", "v1" }, { "k2", "v2" } };
			std::unordered_map<string, string> m2 { { "k1", "v1" }, { "k2", "v2" } };
			QCOMPARE(RpcValue(m1), RpcValue(m2));
		}
		// ChainPack literals
		const RpcValue obj = RpcValue::Map({
											   { "k1", "v1" },
											   { "k2", 42.0 },
											   { "k3", RpcValue::List({ "a", 123.0, true, false, nullptr }) },
										   });

		qDebug() << "obj: " << obj.toCpon();
		QVERIFY(obj.toCpon() == "{\"k1\":\"v1\",\"k2\":42.,\"k3\":[\"a\",123.,true,false,null]}");

		QCOMPARE(RpcValue("a").toDouble(), 0.);
		QCOMPARE(RpcValue("a").toString().c_str(), "a");
		QCOMPARE(RpcValue().toDouble(), 0.);
		/*
		{
			const string unicode_escape_test =
					R"([ "blah\ud83d\udca9blah\ud83dblah\udca9blah\u0000blah\u1234" ])";

			const char utf8[] = "blah" "\xf0\x9f\x92\xa9" "blah" "\xed\xa0\xbd" "blah"
								"\xed\xb2\xa9" "blah" "\0" "blah" "\xe1\x88\xb4";
			RpcValue uni = RpcValue::fromCpon(unicode_escape_test, &err);
			QVERIFY(uni[0].toString().size() == (sizeof utf8) - 1);
			QVERIFY(std::memcmp(uni[0].toString().data(), utf8, sizeof utf8) == 0);
		}
		{
			const string escape_test = "b\"foo\\\\1\\r\\n2\\t\\b\\\"bar\\x0d\\x0A\"";
			const char test[] = "foo\\1\r\n2\t\b\"bar\x0d\x0A";
			RpcValue::String b = RpcValue::fromCpon(escape_test, &err).toString();
			//if(!err.empty())
			//	qDebug() << "!!!!!!!!! " << err << "---" << escape_test;
			//qDebug() << "%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%% " << b.size() << "    " << b;
			QVERIFY(b.size() == (sizeof test) - 1);
			QVERIFY(std::memcmp(b.data(), test, b.size()) == 0);
		}
		*/
		RpcValue my_json = RpcValue::Map {
			{ "key1", "value1" },
			{ "key2", false },
			{ "key3", RpcValue::List { 1, 2, 3 } },
		};
		std::string json_obj_str = my_json.toCpon();
		qDebug() << "json_obj_str: " << json_obj_str.c_str();
		QCOMPARE(json_obj_str.c_str(), "{\"key1\":\"value1\",\"key2\":false,\"key3\":[1,2,3]}");

		class Point {
		public:
			int x;
			int y;
			Point (int x, int y) : x(x), y(y) {}
			RpcValue to_json() const { return RpcValue::List { x, y }; }
		};

		std::vector<Point> points = { { 1, 2 }, { 10, 20 }, { 100, 200 } };
		std::string points_json = RpcValue(points).toCpon();
		qDebug() << "points_json: " << points_json.c_str();
		QVERIFY(points_json == "[[1,2],[10,20],[100,200]]");
		{
			string err;
			auto rpcval = RpcValue::fromCpon(R"(<1:2,2:12,8:"foo",9:[1,2,3],"bar":"baz",>["META",17,18,19])", &err);
			QVERIFY(rpcval.isValid());
			QVERIFY(err.empty());
			QVERIFY(rpcval.metaValue("bar") == "baz");
			QVERIFY(rpcval.metaValue(1) == 2);
			QVERIFY(rpcval.metaValue(8) == "foo");
			QVERIFY(rpcval.at(3) == 19);
		}
	}

	static std::string binary_dump(const RpcValue::String &out)
	{
		std::string ret;
		for (size_t i = 0; i < out.size(); ++i) {
			uint8_t u = out[i];
			//ret += std::to_string(u);
			if(i > 0)
				ret += '|';
			for (size_t j = 0; j < 8*sizeof(u); ++j) {
				ret += (u & (((uint8_t)128) >> j))? '1': '0';
			}
		}
		return ret;
	}

	static std::string binary_dump_rev(const void *data, size_t len)
	{
		std::string ret;
		for (size_t i = len-1; ; --i) {
			uint8_t u = ((uint8_t*)data)[i];
			if(i < len-1)
				ret += '|';
			for (size_t j = 0; j < 8*sizeof(u); ++j) {
				ret += (u & (((uint8_t)128) >> j))? '1': '0';
			}
			if(i == 0)
				break;
		}
		return ret;
	}

	static inline char hex_nibble(char i)
	{
		if(i < 10)
			return '0' + i;
		return 'A' + (i - 10);
	}

	static std::string hex_dump(const RpcValue::String &out)
	{
		std::string ret;
		for (size_t i = 0; i < out.size(); ++i) {
			char h = out[i] / 16;
			char l = out[i] % 16;
			ret += hex_nibble(h);
			ret += hex_nibble(l);
		}
		return ret;
	}

	void binaryTest()
	{
		qDebug() << "============= chainpack binary test ============";
		for (int i = ChainPack::PackingSchema::Null; i <= ChainPack::PackingSchema::CString; ++i) {
			RpcValue::String out;
			out += i;
			ChainPack::PackingSchema::Enum e = (ChainPack::PackingSchema::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPack::PackingSchema::name(e);
			qDebug() << str.str();
		}
		/*
		for (int i = ChainPackProtocol::TypeInfo::Null_Array; i <= ChainPackProtocol::TypeInfo::DateTimeTZ_Array; ++i) {
			RpcValue::Blob out;
			out += i;
			ChainPackProtocol::TypeInfo::Enum e = (ChainPackProtocol::TypeInfo::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPackProtocol::TypeInfo::name(e);
			qDebug() << str.str();
		}
		*/
		for (int i = ChainPack::PackingSchema::FALSE; i <= ChainPack::PackingSchema::TERM; ++i) {
			RpcValue::String out;
			out += (char)i;
			ChainPack::PackingSchema::Enum e = (ChainPack::PackingSchema::Enum)i;
			std::ostringstream str;
			str << std::setw(3) << i << " " << std::hex << i << " " << binary_dump(out).c_str() << " "  << ChainPack::PackingSchema::name(e);
			qDebug() << str.str();
		}
		{
			qDebug() << "------------- NULL";
			RpcValue cp1{nullptr};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			QVERIFY(out.str().size() == 1);
			ChainPackReader rd(out);
			RpcValue cp2 = rd.read();
			qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
		}
		{
			qDebug() << "------------- tiny uint";
			for (RpcValue::UInt n = 0; n < 64; ++n) {
				RpcValue cp1{n};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				if(n < 10)
					qDebug() << n << " " << cp1.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(out.str().size() == 1);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toUInt() == cp2.toUInt());
			}
		}
		{
			qDebug() << "------------- uint";
			for (unsigned i = 0; i < sizeof(RpcValue::UInt); ++i) {
				for (unsigned j = 0; j < 3; ++j) {
					RpcValue::UInt n = RpcValue::UInt{1} << (i*8 + j*3+1);
					RpcValue cp1{n};
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					//QVERIFY(len > 1);
					ChainPackReader rd(out);
					RpcValue cp2 = rd.read();
					//if(n < 100*step)
					qDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QCOMPARE(cp1.toUInt(), cp2.toUInt());
				}
			}
		}
		qDebug() << "------------- tiny int";
		for (RpcValue::Int n = 0; n < 64; ++n) {
			RpcValue cp1{n};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			if(out.str().size() < 10)
				qDebug() << n << " " << cp1.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(out.str().size() == 1);
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toInt() == cp2.toInt());
		}
		{
			qDebug() << "------------- int";
			{
				for (int sig = 1; sig >= -1; sig-=2) {
					for (unsigned i = 0; i < sizeof(RpcValue::Int); ++i) {
						for (unsigned j = 0; j < 3; ++j) {
							RpcValue::Int n = sig * (RpcValue::Int{1} << (i*8 + j*2+2));
							//qDebug() << sig << i << j << (i*8 + j*3+1) << n;
							RpcValue cp1{n};
							std::stringstream out;
							{ ChainPackWriter wr(out);  wr.write(cp1); }
							//QVERIFY(len > 1);
							ChainPackReader rd(out); RpcValue cp2 = rd.read();
							//if(n < 100*step)
							qDebug() << n << int_to_hex(n) << "..." << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
							QVERIFY(cp1.type() == cp2.type());
							QCOMPARE(cp1.toUInt(), cp2.toUInt());
						}
					}
				}
			}
		}
		{
			qDebug() << "------------- double";
			{
				auto n_max = 1000000.;
				auto n_min = -1000000.;
				auto step = (n_max - n_min) / 100;
				for (auto n = n_min; n < n_max; n += step) {
					RpcValue cp1{n};
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					QVERIFY(out.str().size() > 1);
					ChainPackReader rd(out); RpcValue cp2 = rd.read();
					if(n > -3*step && n < 3*step)
						qDebug() << n << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toDouble() == cp2.toDouble());
				}
			}
			{
				double n_max = std::numeric_limits<double>::max();
				double n_min = std::numeric_limits<double>::min();
				double step = -1.23456789e10;
				//qDebug() << n_min << " - " << n_max << ": " << step << " === " << (n_max / step / 10);
				for (double n = n_min; n < n_max / -step / 10; n *= step) {
					RpcValue cp1{n};
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					QVERIFY(out.str().size() > 1);
					ChainPackReader rd(out); RpcValue cp2 = rd.read();
					if(n > -100 && n < 100)
						qDebug() << n << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toDouble() == cp2.toDouble());
				}
			}
		}
		{
			qDebug() << "------------- Decimal";
			{
				RpcValue::Int mant = 123456789;
				int prec_max = 16;
				int prec_min = -16;
				int step = 1;
				for (int prec = prec_min; prec <= prec_max; prec += step) {
					RpcValue cp1{RpcValue::Decimal(mant, prec)};
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					QVERIFY(out.str().size() > 1);
					ChainPackReader rd(out); RpcValue cp2 = rd.read();
					qDebug() << mant << prec << " - " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1 == cp2);
				}
			}
		}
		{
			qDebug() << "------------- bool";
			for(bool b : {false, true}) {
				RpcValue cp1{b};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				QVERIFY(out.str().size() == 1);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << b << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toBool() == cp2.toBool());
			}
		}
#if 0
		{
			qDebug() << "------------- Blob";
			RpcValue::Blob blob{"blob containing zero character"};
			blob[blob.size() - 9] = 0;
			RpcValue cp1{blob};
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			qDebug() << blob << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.toBlob() == cp2.toBlob());
		}
#endif
		{
			qDebug() << "------------- string";
			{
				RpcValue::String str{"string containing zero character"};
				str[str.size() - 10] = 0;
				RpcValue cp1{str};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << str << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toString() == cp2.toString());
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
				//qDebug() << str << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp2.toString().size() == str.size());
				QVERIFY(cp1 == cp2);
			}
		}
		{
			qDebug() << "------------- DateTime";
			QVERIFY(RpcValue::DateTime().isValid() == false);
			QVERIFY(RpcValue::DateTime::fromUtcString("").isValid() == false);
			QVERIFY(RpcValue::DateTime::fromMSecsSinceEpoch(0).isValid() == true);
			QVERIFY(RpcValue::DateTime() == RpcValue::DateTime());
			QVERIFY(RpcValue::DateTime::fromMSecsSinceEpoch(0) == RpcValue::DateTime::fromMSecsSinceEpoch(0));
			QVERIFY(RpcValue::DateTime::fromMSecsSinceEpoch(1) == RpcValue::DateTime::fromMSecsSinceEpoch(1, 2));
			QVERIFY(!(RpcValue::DateTime() < RpcValue::DateTime()));
			QVERIFY(RpcValue::DateTime() < RpcValue::DateTime::fromMSecsSinceEpoch(0));
			QVERIFY(RpcValue::DateTime::fromMSecsSinceEpoch(1) < RpcValue::DateTime::fromMSecsSinceEpoch(2));
			QVERIFY(RpcValue::DateTime::fromMSecsSinceEpoch(0) == RpcValue::DateTime::fromUtcString("1970-01-01T00:00:00"));
			//RpcValue::DateTime ts;// = RpcValue::DateTime::now();
			//qDebug() << "~~~~~~~~~~~~~~~~~~~~~~~~~ " << RpcValue(ts).toCpon();
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
				qDebug() << str << " " << dt.toIsoString().c_str() << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(pack);
				//qDebug() << cp1.toDateTime().msecsSinceEpoch() << cp1.toDateTime().offsetFromUtc();
				//qDebug() << cp2.toDateTime().msecsSinceEpoch() << cp2.toDateTime().offsetFromUtc();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toDateTime() == cp2.toDateTime());
			}
		}
#ifdef HAS_ARRAY
		{
			qDebug() << "------------- Array";
			{
				qDebug() << "\t of Int";
				RpcValue::Array t{RpcValue::Type::Int};
				t.push_back(RpcValue::ArrayElement(RpcValue::Int(11)));
				t.push_back(RpcValue::Int(12));
				t.push_back(RpcValue::Int(13));
				t.push_back(RpcValue::Int(14));
				RpcValue cp1{t};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toArray() == cp2.toArray());
			}
			{
				static constexpr size_t N = 10;
				uint16_t samples[N];
				for (size_t i = 0; i < N; ++i) {
					samples[i] = i+1;
				}
				RpcValue::Array t{samples};
				RpcValue cp1{t};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toArray() == cp2.toArray());
			}
			{
				qDebug() << "\t of DateTime";
				static constexpr size_t N = 10;
				RpcValue::DateTime samples[N];
				for (size_t i = 0; i < N; ++i) {
					samples[i] = RpcValue::DateTime::fromMSecsSinceEpoch(100000 * i);
				}
				RpcValue::Array t{samples};
				RpcValue cp1{t};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toArray() == cp2.toArray());
			}
			{
				qDebug() << "\t of Decimals";
				static constexpr size_t N = 10;
				RpcValue::Decimal samples[N];
				for (size_t i = 0; i < N; ++i) {
					int rnd = qrand() - RAND_MAX/2;
					int prec = i;
					prec -= N/2;
					samples[i] = RpcValue::Decimal(rnd, prec);
				}
				RpcValue::Array t{samples};
				RpcValue cp1{t};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toArray() == cp2.toArray());
				const RpcValue::Array a1 = cp1.toArray();
				const RpcValue::Array a2 = cp2.toArray();
				for (size_t i = 0; i < a1.size(); ++i) {
					qDebug() << i << a1.valueAt(i).toCpon() << a2.valueAt(i).toCpon();
 				}
			}
			{
				static constexpr size_t N = 10;
				std::stringstream out;
				ChainPackWriter wr(out);
				wr.writeArrayBegin(RpcValue::Type::Bool, N);
				bool b = false;
				for (size_t i = 0; i < N; ++i) {
					wr.writeArrayElement(RpcValue(b));
					b = !b;
				}
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				const RpcValue::Array array = cp2.toArray();
				b = false;
				for (size_t i = 0; i < array.size(); ++i) {
					QVERIFY(RpcValue(b) == array.valueAt(i));
					b = !b;
				}
			}
			{
				static constexpr size_t N = 10;
				std::stringstream out;
				ChainPackWriter wr(out);
				wr.writeArrayBegin(RpcValue::Type::Null, N);
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				const RpcValue::Array array = cp2.toArray();
				for (size_t i = 0; i < array.size(); ++i) {
					QVERIFY(RpcValue(nullptr) == array.valueAt(i));
				}
			}
		}
#endif
		{
			qDebug() << "------------- List";
			{
				for(const std::string s : {"[]", "[[]]", R"(["a",123,true,[1,2,3],null])"}) {
					string err;
					RpcValue cp1 = RpcValue::fromCpon(s, &err);
					std::stringstream out;
					{ ChainPackWriter wr(out);  wr.write(cp1); }
					ChainPackReader rd(out); RpcValue cp2 = rd.read();
					qDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
					QVERIFY(cp1.type() == cp2.type());
					QVERIFY(cp1.toList() == cp2.toList());
				}
			}
			{
				RpcValue cp1{RpcValue::List{1,2,3}};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toList() == cp2.toList());
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
				qDebug() << cp2.toCpon() << " dump: " << binary_dump(out.str()).c_str();
				const RpcValue::List list = cp2.toList();
				QVERIFY(list.size() == N);
				for (size_t i = 0; i < list.size(); ++i) {
					std::string s("foo-bar");
					QVERIFY(RpcValue(s + shv::chainpack::Utils::toString(i)) == list.at(i));
				}
			}
		}
		{
			qDebug() << "------------- Map";
			{
				RpcValue cp1{{
						{"foo", RpcValue::List{11,12,13}},
						{"bar", 2},
						{"baz", 3},
							 }};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toMap() == cp2.toMap());
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
					QVERIFY(it.second == m[it.first]);
				}
			}
			for(const std::string s : {"{}", R"({"a":{}})", R"({"foo":{"bar":"baz"}})"}) {
				string err;
				RpcValue cp1 = RpcValue::fromCpon(s, &err);
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << s << " " << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str()).c_str();
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1 == cp2);
			}
		}
		{
			qDebug() << "------------- IMap";
			{
				RpcValue::IMap map {
					{1, "foo"},
					{2, "bar"},
					{333, 15u},
				};
				RpcValue cp1{map};
				std::stringstream out;
				{ ChainPackWriter wr(out);  wr.write(cp1); }
				ChainPackReader rd(out); RpcValue cp2 = rd.read();
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toIMap() == cp2.toIMap());
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
				qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " dump: " << binary_dump(out.str());
				QVERIFY(cp1.type() == cp2.type());
				QVERIFY(cp1.toIMap() == cp2.toIMap());
			}
		}
		{
			qDebug() << "------------- Meta1";
			RpcValue cp1{RpcValue::String{"META1"}};
			cp1.setMetaValue(meta::Tag::MetaTypeId, 11);
			std::stringstream out;
			{ ChainPackWriter wr(out);  wr.write(cp1); }
			ChainPackReader rd(out); RpcValue cp2 = rd.read();
			std::ostream::pos_type consumed = out.tellg();
			qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			qDebug() << "hex:" << hex_dump(out.str());
			QVERIFY(out.str().size() == (size_t)consumed);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.metaData() == cp2.metaData());
		}
		{
			qDebug() << "------------- Meta2";
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
			qDebug() << cp1.toCpon() << " " << cp2.toCpon() << " len: " << out.str().size() << " consumed: " << consumed << " dump: " << binary_dump(out.str());
			qDebug() << "hex:" << hex_dump(out.str());
			QVERIFY(out.str().size() == (size_t)consumed);
			QVERIFY(cp1.type() == cp2.type());
			QVERIFY(cp1.metaData() == cp2.metaData());
		}
#ifdef __linux
		{
			qDebug() << "------------- Memory usage";
			static constexpr size_t count = 1000000;
			qDebug() << "===before creating" << count << "values";
			dump_memory();
			{
				RpcValue::List lst;
				for (size_t i = 0; i < count; ++i) {
					lst.push_back(i);
				}
				/*
				std::vector<std::string> v;
				v.resize(count);
				*/
				qDebug() << "===after creating" << count << "values";
				dump_memory();
			}
			qDebug() << "===after releasing" << count << "values";
			dump_memory();
		}
#endif
	}

private slots:
	void initTestCase()
	{
		//qDebug("called before everything else");
		shv::chainpack::Exception::setAbortOnException(true);
	}
	void testProtocol()
	{
		textTest();
		binaryTest();
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
				uint64_t *pn = (uint64_t*)&d;
				uint64_t mant_mask = ((static_cast<uint64_t>(1) << 52) - 1);
				uint64_t umant = *pn & mant_mask;
				uint64_t exp_mask = ((static_cast<uint64_t>(1) << 11) - 1) << 52;
				uint uexp = (*pn & exp_mask) >> 52;
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
						qDebug() << sgn << "* INF";
						mantisa = 0;
					}
					else {
						// NaN
						qDebug() << "NaN";
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
				qDebug() << d << "----------------------------------";
				qDebug()<< "mantisa:" << mantisa << "exp:" << exponent;
				qDebug()<< "neg:" << sgn << "umant:" << umant << "uexp:" << uexp;
				qDebug() << binary_dump_rev(pn, sizeof (*pn)).c_str();
				//qDebug() << binary_dump_rev((void*)&exp_mask, sizeof (exp_mask)).c_str();
				//qDebug() << binary_dump_rev((void*)&mant_mask, sizeof (mant_mask)).c_str();
				//qDebug() << binary_dump_rev((void*)&uexp, sizeof (uexp)).c_str();
				//qDebug() << binary_dump_rev((void*)&umant, sizeof (umant)).c_str();
			}
		}
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestRpcValue)
#include "tst_chainpack_rpcvalue.moc"
