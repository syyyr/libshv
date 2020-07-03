#pragma once

#include <QComboBox>

class QTimeZone;

namespace shv {
namespace visu {
namespace logview {

class TimeZoneComboBox : public QComboBox
{
	using Super = QComboBox;
public:
	TimeZoneComboBox(QWidget *parent = nullptr);

	QTimeZone currentTimeZone() const;
protected:
	void keyPressEvent(QKeyEvent *event) override;
private:
	//static const char *LOCAL_TZ_ID;
	QString m_searchText;
};

}}}

