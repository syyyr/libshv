#include <shv/core/utils/fileshvjournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogtypeinfo.h>

#include <QtTest/QtTest>
#include <QDebug>

using std::string;
using namespace shv::core::utils;
using namespace shv::chainpack;

namespace {
/*
QDebug operator<<(QDebug debug, const std::string &s)
{
	debug << s.c_str();
	return debug;
}
*/

struct Channel
{
	ShvLogTypeInfo typeInfo;
	string domain;
	int minVal = 0;
	int maxVal = 0;
	uint16_t period = 0;
	uint16_t fasttime = 0;
	RpcValue value;
};

std::map<string, Channel> channels;

void snapshot_fn(std::vector<ShvJournalEntry> &ev)
{
	for(const auto &kv : channels) {
		ShvJournalEntry e;
		e.path = kv.first;
		e.value = kv.second.value;
		ev.push_back(std::move(e));
	}
}

FileShvJournal2 fileJournal("testdev", snapshot_fn);
ShvLogTypeInfo typeInfo
{
	// types
	{
		{
			"Temperature",
			{
				ShvLogTypeDescr::Type::Decimal,
				{},
				"",
				ShvLogTypeDescr::SampleType::Continuous,
				RpcValue::Decimal(0, -2),
				RpcValue::Decimal(10000, -2)
			}
		},
		{
			"Voltage",
			{
				ShvLogTypeDescr::Type::Int,
				{},
				"",
				ShvLogTypeDescr::SampleType::Continuous,
				RpcValue::Decimal(-1500),
				RpcValue::Decimal(1500)
			}
		},
		{
			"VetraStatus",
			{
				ShvLogTypeDescr::Type::BitField,
				{
					{"Active", 0},
					{"LastDir", "N,L,R,S", RpcValue::List{1,2}},
					{"CommError", 3},
				},
			}
		},
		{
			"VehicleDectedMap",
			{
				ShvLogTypeDescr::Type::Map,
				{
					{"vehicleId"},
					{"dir"},
					{"CommError", 3},
				},
				"",
				ShvLogTypeDescr::SampleType::Discrete
			}
		},
		{
			"VehicleDectedList",
			{
				ShvLogTypeDescr::Type::List,
				{
					{"vehicleId"},
					{"dir", "N,L,R,S"},
				},
				"",
				ShvLogTypeDescr::SampleType::Discrete
			}
		},
	},
	// paths
	{
		{ "temperature", {"Temperature"} },
		{ "voltage", {"Voltage"} },
		{ "doorOpen", {"Bool"} },
		{ "vetra/status", {"VetraStatus"} },
		{ "vetra/vehicleDetected", {"VehicleDectedList"} },
	},
};
}

class TestShvFileReader: public QObject
{
	Q_OBJECT
private:
	void test1()
	{
		qDebug() << "============= shv StringView test ============\n";
		{
			qDebug() << "------------- normalize";
			{
//				std::string str;
//				shv::core::StringView s1(str);
//				shv::core::StringView s2(str, 5);
//				shv::core::StringView s3(str, 6, 1000);
//				shv::core::StringView s4(str, 7, 0);
//				shv::core::StringView s5(str, 0, 0);
//				QVERIFY(s1.valid());
//				QVERIFY(s1.empty());
//				QVERIFY(s2.valid());
//				QVERIFY(s2.empty());
//				QVERIFY(s3.valid());
//				QVERIFY(s3.empty());
//				QVERIFY(s4.valid());
//				QVERIFY(s4.empty());
//				QVERIFY(s5.valid());
//				QVERIFY(s5.empty());
			}
		}
	}
private slots:
	void initTestCase()
	{
		{
			Channel &c = channels["temperature"];
			c.value = RpcValue::Decimal(2200, -2);
		}
		{
			Channel &c = channels["voltage"];
			c.value = 0;
		}
		{
			Channel &c = channels["doorOpen"];
			c.value = false;
		}
		{
			Channel &c = channels["vetra/vehicleDetected"];
			c.value = RpcValue::List{1234, "R"};
		}
		{
			Channel &c = channels["vetra/status"];
			c.value = 0;
		}
	}
	void tests()
	{
		test1();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestShvFileReader)
#include "tst_shvlogfilereader.moc"
