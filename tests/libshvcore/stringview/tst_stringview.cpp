#include <shv/core/stringview.h>

#include <QtTest/QtTest>
#include <QDebug>

using std::string;

namespace {
/*
QDebug operator<<(QDebug debug, const std::string &s)
{
	debug << s.c_str();
	return debug;
}
*/
}

class TestStringView: public QObject
{
	Q_OBJECT
private:
	void testStringView()
	{
		qDebug() << "============= shv StringView test ============\n";
		{
			qDebug() << "------------- normalize";
			{
				std::string str;
				shv::core::StringView s1(str);
				shv::core::StringView s2(str, 5);
				shv::core::StringView s3(str, 6, 1000);
				shv::core::StringView s4(str, 7, 0);
				shv::core::StringView s5(str, 0, 0);
				QVERIFY(s1.valid());
				QVERIFY(s1.empty());
				QVERIFY(s2.valid());
				QVERIFY(s2.empty());
				QVERIFY(s3.valid());
				QVERIFY(s3.empty());
				QVERIFY(s4.valid());
				QVERIFY(s4.empty());
				QVERIFY(s5.valid());
				QVERIFY(s5.empty());
			}
			{
				std::string str("abcde");
				shv::core::StringView s1(str);
				shv::core::StringView s2(str, 5);
				shv::core::StringView s3(str, 6, 1000);
				shv::core::StringView s4(str, 7, 0);
				shv::core::StringView s5(str, 0, 0);
				QVERIFY(s1.valid());
				QVERIFY(!s1.empty());
				QVERIFY(s2.valid());
				QVERIFY(s2.empty());
				QVERIFY(s3.valid());
				QVERIFY(s3.empty());
				QVERIFY(s4.valid());
				QVERIFY(s4.empty());
				QVERIFY(s5.valid());
				QVERIFY(s5.empty());
			}
			{
				std::string str("abcde");
				shv::core::StringView s1(str);
				shv::core::StringView s2(str, 4);
				shv::core::StringView s3(str, 3, 1000);
				shv::core::StringView s4(str, 4, 0);
				shv::core::StringView s5(str, 0, 0);
				QVERIFY(s1.length() == 5);
				QVERIFY(s2.length() == 1);
				QVERIFY(s3.length() == 2);
				QVERIFY(s4.length() == 0);
				QVERIFY(s5.length() == 0);
			}
		}
		{
			qDebug() << "------------- mid";
			std::string str("1234567890");
			shv::core::StringView s(str);
			shv::core::StringView s1 = s.mid(0);
			QVERIFY(s1.length() == 10);
			shv::core::StringView s11 = s.mid(1, 2);
			QVERIFY(s11 == "23");
			//QVERIFY(s11.space() == 7);
			shv::core::StringView s2 = s.mid(0, 50);
			QVERIFY(s2.length() == 10);
			shv::core::StringView s3 = s.mid(3, 50);
			QVERIFY(s3.length() == 7);
			shv::core::StringView s31 = s.mid(9, 50);
			QVERIFY(s31 == "0");
			//QVERIFY(s31.space() == 0);
			shv::core::StringView s32 = s.mid(10);
			QVERIFY(s32 == "");
			//QVERIFY(s32.space() == 0);
			shv::core::StringView s4 = s.mid(30, 50);
			QVERIFY(s4.empty());
			//QVERIFY(s4.space() == 0);
		}
		{
			qDebug() << "------------- getToken";
			std::string str("///foo/bar//baz");
			shv::core::StringView s(str);
			shv::core::StringView s1 = s.getToken('/');
			QVERIFY(s1.length() == 0);
			s1 = s.mid(s1.end()+1).getToken('/');
			QVERIFY(s1.length() == 0);
			s1 = s.mid(s1.end()+1).getToken('/');
			QVERIFY(s1.length() == 0);
			s1 = s.mid(s1.end()+1).getToken('/');
			QVERIFY(s1 == "foo");
		}
		qDebug() << "------------- split";
		{
			std::string str("a:");
			shv::core::StringView s(str);
			std::vector<shv::core::StringView> sl = s.split(':', shv::core::StringView::KeepEmptyParts);
			QVERIFY(sl.size() == 2);
			QVERIFY(sl[0] == "a");
			QVERIFY(sl[1].empty());
		}
		{
			std::string str("a:\"b:b\":c");
			shv::core::StringView s(str);
			std::vector<shv::core::StringView> sl = s.split(':', '"', shv::core::StringView::KeepEmptyParts);
			QVERIFY(sl.size() == 3);
			QVERIFY(sl[0] == "a");
			QVERIFY(sl[1] == "\"b:b\"");
			QVERIFY(sl[2] == "c");
		}
		{
			std::string str("///foo/bar//baz//");
			shv::core::StringView s(str);
			std::vector<shv::core::StringView> sl1 = s.split('/');
			QVERIFY(sl1.size() == 3);
			QVERIFY(sl1[1] == "bar");
			std::vector<shv::core::StringView> sl2 = s.split('/', shv::core::StringView::KeepEmptyParts);
			QVERIFY(sl2.size() == 9);
			QVERIFY(sl2[5].empty());
			QVERIFY(sl2[6] == "baz");
		}
	}
private slots:
	void initTestCase()
	{
		//qDebug("called before everything else");
	}
	void tests()
	{
		testStringView();
	}

	void cleanupTestCase()
	{
		//qDebug("called after firstTest and secondTest");
	}
};

QTEST_MAIN(TestStringView)
#include "tst_stringview.moc"
