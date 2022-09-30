#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvlogtypeinfo.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvlogfilereader.h>
#include <shv/core/utils/shvjournalfilewriter.h>
#include <shv/core/utils/shvjournalfilereader.h>
#include <shv/core/utils/shvmemoryjournal.h>
#include <shv/core/utils/shvlogfilter.h>
#include <shv/core/utils/shvlogrpcvaluereader.h>

#include <shv/core/log.h>

#include <shv/chainpack/chainpackwriter.h>
#include <shv/chainpack/cponwriter.h>

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

RpcValue read_cpon_file(const string &fn)
{
	QFile f(QString::fromStdString(fn));
	if(f.open(QFile::ReadOnly)) {
		auto ba = f.readAll();
		string s(ba.constData(), ba.size());
		return RpcValue::fromCpon(s);
	}
	else {
		throw runtime_error("cannot open file: " + fn + " for reading");
	}
}

void write_cpon_file(const string &fn, const RpcValue &log)
{
	ofstream out(fn, std::ios::binary | std::ios::out);
	if(out) {
		CponWriterOptions opts;
		opts.setIndent("\t");
		CponWriter wr(out, opts);
		wr << log;
	}
	else {
		throw runtime_error("cannot open file: " + fn + " for writing");
	}
}

const string TEST_DIR = "/tmp/tst_ShvTypeInfo";
//const string JOURNAL_DIR = TEST_DIR + "/journal";
}

class TestShvTypeInfo: public QObject
{
	Q_OBJECT
private:
	void convertNodesTreeToTypeInfo()
	{
		qDebug() << "=============" << __FUNCTION__ << "============";
		auto rv = read_cpon_file(":/test/libshvcore/shvtypeinfo/files/nodesTree.cpon");
		auto type_info = ShvTypeInfo::fromRpcValue(rv);
		write_cpon_file(TEST_DIR + "/typeInfo.cpon", type_info.toRpcValue());
		QVERIFY(type_info.nodeDescriptionForDevice("TC_G3", "").dataValue("deviceType") == "TC_G3");
		QVERIFY(type_info.nodeDescriptionForDevice("Zone_G3", "").dataValue("deviceType") == "Zone_G3");
		QVERIFY(type_info.nodeDescriptionForDevice("TC_G3", "status").typeName() == "StatusTC");
		QVERIFY(type_info.nodeDescriptionForDevice("SignalSymbol_G3", "status").typeName() == "StatusSignalSymbol");
		QVERIFY(type_info.nodeDescriptionForDevice("SignalSymbol_G3@1", "status").typeName() == "StatusSignalSymbolWhite");
		QVERIFY(type_info.nodeDescriptionForDevice("SignalSymbol_G3@2", "status").typeName() == "StatusSignalSymbolP80Y");
		QVERIFY(type_info.nodeDescriptionForDevice("SignalSymbol_G3@3", "status").typeName() == "");
	}
private slots:
	void initTestCase()
	{
		//shv::chainpack::Exception::setAbortOnException(true);
		//NecroLog::setTopicsLogTresholds("ShvJournal:D");
		qDebug() << "Registered topics:" << NecroLog::registeredTopicsInfo();
		QDir dir(QString::fromStdString(TEST_DIR));
		if(dir.exists()) {
			if(!dir.removeRecursively())
				throw runtime_error("Cannot delete test dir: " + TEST_DIR);
		}
		if(!dir.mkpath(QString::fromStdString(TEST_DIR)))
			throw runtime_error("Cannot create test dir: " + TEST_DIR);
	}
	void tests()
	{
		convertNodesTreeToTypeInfo();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestShvTypeInfo)
#include "tst_shvtypeinfo.moc"
