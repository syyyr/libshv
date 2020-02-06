#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogtypeinfo.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <shv/core/log.h>

#include <shv/chainpack/chainpackwriter.h>

#include <QtTest/QtTest>
#include <QDebug>
#include <QDir>

#include <fstream>

using namespace std;
using namespace shv::core::utils;
using namespace shv::chainpack;

namespace {

QDebug operator<<(QDebug debug, const std::string &s)
{
	debug << s.c_str();
	return debug;
}

const string TEST_DIR = "/tmp/TestShvFileReader";
const string JOURNAL_DIR = TEST_DIR + "/journal";

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
			qDebug() << "------------- Journal init:" << JOURNAL_DIR;
			if(!QDir(QString::fromStdString(JOURNAL_DIR)).removeRecursively())
				qWarning() << "Cannot delete journal dir:" << JOURNAL_DIR;
			//if(!QDir().mkpath(QString::fromStdString(JOURNAL_DIR)))
			//	qWarning() << "Cannot create journal dir:" << JOURNAL_DIR;
			ShvFileJournal file_journal("testdev", snapshot_fn);
			file_journal.setJournalDir(JOURNAL_DIR);
			file_journal.setFileSizeLimit(1024*64*20);
			file_journal.setJournalSizeLimit(file_journal.fileSizeLimit() * 3);
			qDebug() << "------------- Generating log files";
			auto msec = RpcValue::DateTime::now().msecsSinceEpoch();
			int64_t msec1 = msec;
			std::random_device randev;
			std::mt19937 mt(randev());
			std::uniform_int_distribution<int> rndmsec(0, 2000);
			std::uniform_int_distribution<int> rndval(0, 1000);
			constexpr int CNT = 6000;//0;
			for (int i = 0; i < CNT; ++i) {
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
						file_journal.append(e);
					}
				}
			}
			int64_t msec2 = msec;
			{
				ShvGetLogParams params;
				params.since = RpcValue::DateTime::fromMSecsSinceEpoch(msec1 + (msec2 - msec1) / 2);
				params.until = RpcValue::DateTime::fromMSecsSinceEpoch(msec2 - (msec2 - msec1) / 20);
				RpcValue log1 = file_journal.getLog(params);
				string fn = TEST_DIR + "/log1.chpk";
				{
					ofstream out(fn, std::ios::binary | std::ios::out);
					ChainPackWriter wr(out);
					wr << log1;
				}
				int cnt = 0;
				ShvLogFileReader rd(fn);
				while(rd.next()) {
					rd.entry();
					cnt++;
				}
				QVERIFY(cnt == rd.logHeader().recordCount());
				QVERIFY(cnt == (int)log1.toList().size());
			}
			qDebug() << "------------- Generating dirty log + memory log";
			unsigned dirty_cnt = 0;
			string dirty_fn = JOURNAL_DIR + "/dirty.log2";
			ShvJournalFileWriter dirty_log(dirty_fn);
			ShvMemoryJournal memory_jurnal;
			memory_jurnal.setTypeInfo(typeInfo);
			for (int i = 0; i < 10000; ++i) {
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
							//e.sampleType = ShvLogTypeDescr::SampleType::Discrete;
						}
						else {
							e.value = rv;
						}
						e.domain = c.domain;
						e.shortTime = msec % 0x100;
						dirty_log.append(e);
						memory_jurnal.append(e);
						dirty_cnt++;
						//if(dirty_cnt < 10)
						//	qDebug() << dirty_cnt << "INS:" << e.toRpcValueMap().toCpon();
					}
				}
			}
			{
				ShvLogHeader hdr;
				hdr.setTypeInfo(typeInfo);
				ShvJournalFileReader rd2(dirty_fn, &hdr);
				unsigned cnt = 0;
				while (rd2.next()) {
					const ShvJournalEntry &e1 = rd2.entry();
					QVERIFY(cnt < dirty_cnt);
					const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
					//qDebug() << cnt << e1.toRpcValueMap().toCpon() << "vs" << e2.toRpcValueMap().toCpon();
					QVERIFY(e1 == e2);
				}
				QVERIFY(cnt == dirty_cnt);
				{
					cnt--;
					rd2.last();
					const ShvJournalEntry &e1 = rd2.entry();
					const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
					//qDebug() << cnt << e1.toRpcValueMap().toCpon() << "vs" << e2.toRpcValueMap().toCpon();
					QVERIFY(e1 == e2);
				}
			}
			{
				qDebug() << "------------- Read chainpack to memory log";
				string fn1 = TEST_DIR + "/log1.chpk";
				string fn2 = TEST_DIR + "/log2.chpk";
				//ShvMemoryJournal memory_jurnal;
				memory_jurnal.loadLog(fn1);
				{
					ofstream out(fn2, std::ios::binary | std::ios::out);
					ChainPackWriter wr(out);
					wr << memory_jurnal.getLog(ShvGetLogParams());
				}
				unsigned cnt = 0;
				ShvLogFileReader rd2(fn2);
				while (rd2.next()) {
					const ShvJournalEntry &e1 = rd2.entry();
					QVERIFY(cnt < memory_jurnal.entries().size());
					const ShvJournalEntry &e2 = memory_jurnal.entries()[cnt++];
					//qDebug() << cnt << e1.toRpcValueMap().toCpon() << "vs" << e2.toRpcValueMap().toCpon();
					QVERIFY(e1 == e2);
				}
			}
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
