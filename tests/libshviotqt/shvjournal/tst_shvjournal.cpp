#include <shv/iotqt/utils/fileshvjournal.h>
#include <shv/iotqt/utils/shvpath.h>

#include <QtTest/QtTest>
#include <QDebug>

using std::string;

namespace {

QDebug operator<<(QDebug debug, const std::string &s)
{
	debug << QString::fromUtf8(s.c_str());
	return debug;
}

}

class TestFileShvJournal: public QObject
{
	Q_OBJECT
private:
	void testShvPath()
	{
		qDebug() << "============= ShvPath test ============\n";
		{
			qDebug() << "------------- pattern match";
			struct Test {
				std::string path;
				std::string pattern;
				bool result;
			};
			Test cases[] {
				{"", "aa", false},
				{"aa", "", false},
				{"", "", true},
				{"", "**", true},
				{"aa/bb/cc/dd", "aa/*/cc/dd", true},
				{"aa/bb/cc/dd", "aa/bb/**/cc/dd", true},
				{"aa/bb/cc/dd", "aa/bb/*/**/dd", true},
				{"aa/bb/cc/dd", "*/*/cc/dd", true},
				{"aa/bb/cc/dd", "*/cc/dd/**", false},
				{"aa/bb/cc/dd", "*/*/*/*", true},
				{"aa/bb/cc/dd", "aa/*/**", true},
				{"aa/bb/cc/dd", "aa/**/dd", true},
				{"aa/bb/cc/dd", "**", true},
				{"aa/bb/cc/dd", "**/dd", true},
				{"aa/bb/cc/dd", "**/*/**", false},
				{"aa/bb/cc/dd", "**/**", false},
				{"aa/bb/cc/dd", "**/ddd", false},
				{"aa/bb/cc/dd", "aa1/bb/cc/dd", false},
				{"aa/bb/cc/dd", "aa/bb/cc/dd1", false},
				{"aa/bb/cc/dd", "aa/bb/cc1/dd", false},
				{"aa/bb/cc/dd", "bb/cc/dd", false},
				{"aa/bb/cc/dd", "aa/bb/cc", false},
				{"aa/bb/cc/dd", "aa/*/bb/cc", false},
				{"aa/bb/cc/dd", "aa/bb/cc/dd/*", false},
				{"aa/bb/cc/dd", "*/aa/bb/cc/dd", false},
				{"aa/bb/cc/dd", "*/aa/bb/cc/dd/*", false},
				{"aa/bb/cc/dd", "aa/bb/cc/dd/ee", false},
				{"aa/bb/cc/dd", "aa/bb/cc", false},
				{"aa/bb/cc/dd", "*/*/*", false},
				{"aa/bb/cc/dd", "*/*/*/*/*", false},
				{"aa/bb/cc/dd", "*/**/*/*/*", false},
			};
			for(const Test &t : cases) {
				qDebug() << t.path << "vs. pattern" << t.pattern << "should be:" << t.result;
				QVERIFY(shv::iotqt::utils::ShvPath(t.path).matchWild(t.pattern) == t.result);
			}
		}
	}
private slots:
	void initTestCase()
	{
		//qDebug("called before everything else");
	}
	void tests()
	{
		testShvPath();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestFileShvJournal)
#include "tst_shvjournal.moc"
