#include <shv/core/utils/shvfilejournal.h>
#include <shv/core/utils/shvlogheader.h>
#include <shv/core/utils/shvjournalentry.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QtTest/QtTest>
#include <QDebug>

using namespace shv::core::utils;
using namespace shv::chainpack;

class TestShvMemoryJournal : public QObject
{
	Q_OBJECT
private:
	static void appendEntry(ShvMemoryJournal &journal, const std::string &path, const RpcValue &value, int64_t time)
	{
		journal.append(ShvJournalEntry(
						   path,
						   value,
						   shv::chainpack::Rpc::SIG_VAL_CHANGED,
						   ShvJournalEntry::NO_SHORT_TIME,
						   ShvJournalEntry::SampleType::Continuous,
						   time));
	}

	void test1()
	{

		qDebug() << "============= TestShvMemoryJournal ============\n";
		ShvMemoryJournal journal1;
		appendEntry(journal1, "path1", 10,  100);
		appendEntry(journal1, "path2", 11,  110);
		appendEntry(journal1, "path3", 12,  120);
		appendEntry(journal1, "path4", 13,  130);
		appendEntry(journal1, "path5", 120, 140);

		appendEntry(journal1, "path3", 15,   1000);
		appendEntry(journal1, "path2", 110,  1101);
		appendEntry(journal1, "path1", 120,  1260);
		appendEntry(journal1, "path4", 113,  3000);
		appendEntry(journal1, "path5", 121, 14000);

		appendEntry(journal1, "path5", 101, 21000);
		appendEntry(journal1, "path4", 11,  22000);
		appendEntry(journal1, "path3", 12,  22200);
		appendEntry(journal1, "path2", 130, 23000);
		appendEntry(journal1, "path1", 120, 24800);

		ShvGetLogParams params;
		params.since = RpcValue::DateTime::fromMSecsSinceEpoch(150);
		params.withSnapshot = true;

		RpcValue log1 = journal1.getLog(params);
		qDebug() << log1.toCpon("    ").c_str();

		ShvMemoryJournal journal2;
		journal2.loadLog(log1);

		RpcValue log2 = journal2.getLog(params);
		qDebug() << log2.toCpon("    ").c_str();

		QVERIFY(log1.toList().size() == log2.toList().size());
	}
private slots:
	void initTestCase()
	{
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

QTEST_MAIN(TestShvMemoryJournal)
#include "tst_shvmemoryjournal.moc"
