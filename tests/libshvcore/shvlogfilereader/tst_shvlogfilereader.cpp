#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogtypeinfo.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvdirtylog.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/log.h>

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

string JOURNAL_DIR = "/tmp/TestShvFileReader/journal";

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
					{"LastDir", "UInt", RpcValue::List{1,2}, "N,L,R,S"},
					{"CommError", 3},
				},
			}
		},
		{
			"VehicleDectedMap",
			{
				ShvLogTypeDescr::Type::Map,
				{
					{"vehicleId", "UInt"},
					{"dir", "String", RpcValue::List{"", "L", "R", "S"}},
					{"CommError", "Bool"},
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
					{"vehicleId", "UInt"},
					{"dir", "String", RpcValue::List{"", "L", "R", "S"}},
					{"CommError", "Bool"},
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
			std::random_device randev;
			std::mt19937 mt(randev());
			std::uniform_int_distribution<int> rndmsec(0, 2000);
			std::uniform_int_distribution<int> rndval(0, 1000);

			for (int i = 0; i < 100000; ++i) {
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
			qDebug() << "------------- Generating dirty log";
			string dirty_fn = JOURNAL_DIR + "/dirty.log2";
			shv::core::utils::ShvDirtyLog dirty_log(dirty_fn);
			for (int i = 0; i < 5000; ++i) {
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
						dirty_log.append(e);
					}
				}
			}
			{
				shv::core::utils::ShvLogFileReader rd2(dirty_fn);
				while (true) {
					shv::core::utils::ShvJournalEntry e = rd2.next();
					if(!e.isValid())
						break;
				}
			}
//				QVERIFY(s5.empty());
		}
	}
private slots:
	void initTestCase()
	{
		NecroLog::setTopicsLogTresholds("ShvJournal:D");
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
		qDebug() << "------------- Journal init:" << JOURNAL_DIR;
		if(!QDir(QString::fromStdString(JOURNAL_DIR)).removeRecursively())
			qWarning() << "Cannot delete journal dir:" << JOURNAL_DIR;
		//if(!QDir().mkpath(QString::fromStdString(JOURNAL_DIR)))
		//	qWarning() << "Cannot create journal dir:" << JOURNAL_DIR;
		fileJournal.setJournalDir(JOURNAL_DIR);
		fileJournal.setFileSizeLimit(1024*1024);
		fileJournal.setJournalSizeLimit(1024*256*32);
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
