#include <shv/core/utils/fileshvjournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogtypeinfo.h>
#include <shv/core/utils/shvjournalentry.h>

#include <QtTest/QtTest>
#include <QDebug>
#include <QDir>

using std::string;
using namespace shv::core::utils;
using namespace shv::chainpack;

namespace {

QDebug operator<<(QDebug debug, const std::string &s)
{
	debug << s.c_str();
	return debug;
}

struct Channel
{
	ShvLogTypeInfo typeInfo;
	string domain;
	int minVal = 0;
	int maxVal = 0;
	uint16_t period = 1;
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

ShvFileJournal fileJournal("testdev", snapshot_fn);
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
		qDebug() << "============= TestShvFileReader ============\n";
		{
			qDebug() << "------------- Generating log files";
			auto msec = RpcValue::DateTime::now().msecsSinceEpoch();
			std::random_device rd;
			std::mt19937 mt(rd());
			std::uniform_int_distribution<int> rndmsec(0, 2000);
			std::uniform_int_distribution<int> rndval(0, 1000);

			for (int i = 0; i < 50000; ++i) {
				msec += rndmsec(mt);
				for(auto &kv : channels) {
					Channel &c = kv.second;
					if(i % c.period == 0) {
						ShvJournalEntry e;
						e.epochMsec = msec;
						e.path = kv.first;
						RpcValue rv((c.maxVal - c.minVal) * rndval(mt) / 1000 + c.minVal);
						if(e.path == "temperature") {
							e.value = RpcValue::Decimal(rv.toInt(), -2);
						}
						else if(e.path == "vetra/vehicleDetected") {
							e.value = RpcValue::List{rv, i %2? "R": "L"};
						}
						else {
							e.value = rv;
						}
						e.domain = c.domain;
						e.shortTime = msec % 0x100;
						fileJournal.append(e);
					}
				}
			}
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
private slots:
	void initTestCase()
	{
		{
			Channel &c = channels["temperature"];
			c.value = RpcValue::Decimal(2200, -2);
			c.period = 50;
			c.minVal = 0;
			c.maxVal = 5000;
		}
		{
			Channel &c = channels["voltage"];
			c.value = 0;
			c.minVal = 0;
			c.maxVal = 1500;
		}
		{
			Channel &c = channels["doorOpen"];
			c.value = false;
			c.period = 500;
			c.minVal = 0;
			c.maxVal = 1;
		}
		{
			Channel &c = channels["vetra/vehicleDetected"];
			c.value = RpcValue::List{1234, "R"};
			c.period = 100;
			c.minVal = 6000;
			c.maxVal = 6999;
		}
		{
			Channel &c = channels["vetra/status"];
			c.value = 10;
			c.minVal = 0;
			c.maxVal = 0xFF;
		}
		string JOURNAL_DIR = "/tmp/TestShvFileReader/journal";
		qDebug() << "------------- Journal init:" << JOURNAL_DIR;
		if(!QDir(QString::fromStdString(JOURNAL_DIR)).removeRecursively())
			qWarning() << "Cannot delete journal dir:" << JOURNAL_DIR;
		//if(!QDir().mkpath(QString::fromStdString(JOURNAL_DIR)))
		//	qWarning() << "Cannot create journal dir:" << JOURNAL_DIR;
		fileJournal.setJournalDir(JOURNAL_DIR);
		fileJournal.setFileSizeLimit(1024*1024);
		fileJournal.setJournalSizeLimit(1024*1024*10);
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
