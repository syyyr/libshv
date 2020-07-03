#include "timezonecombobox.h"

#include <QKeyEvent>
#include <QLineEdit>
#include <QTimeZone>
#include <QDebug>

namespace shv {
namespace visu {
namespace logview {

//const char *LOCAL_TZ_ID = "Local";

TimeZoneComboBox::TimeZoneComboBox(QWidget *parent)
	: Super(parent)
{
	setEditable(true);
	//addItem(LOCAL_TZ_ID);
	for(auto tzn : QTimeZone::availableTimeZoneIds()) {
		addItem(tzn);
	}
	setCurrentIndex(findText(QTimeZone::systemTimeZoneId()));
	/*
	connect(this, &TimeZoneComboBox::editTextChanged, [this](const QString &text) {
		int ix = findText(text, Qt::MatchContains);
		if(ix >= 0) {
			setCurrentIndex(ix);
		}
	});
	*/
}

QTimeZone TimeZoneComboBox::currentTimeZone() const
{
	if(currentIndex() < 0)
		return QTimeZone();
	if(currentIndex() == 0)
		return QTimeZone::systemTimeZone();
	return QTimeZone(currentText().toUtf8());
}

void TimeZoneComboBox::keyPressEvent(QKeyEvent *event)
{
	QString c = event->text();
	qDebug() << "pressed:" << event;
	if (c.isEmpty()) {
		m_searchText.clear();
		Super::keyPressEvent(event);
		return;
	}

	c = m_searchText + c;
	int ix = findText(c, Qt::MatchContains);
	if(ix >= 0) {
		event->accept();
		m_searchText = c;
		setCurrentIndex(ix);
		auto curr_text = currentText();
		QLineEdit *ed = lineEdit();
		ed->setText(curr_text);
		int sel_start = curr_text.indexOf(m_searchText, 0, Qt::CaseInsensitive);
		qDebug() << "curr:" << curr_text << "search:" << m_searchText << "sel_start:" << sel_start;
		ed->setSelection(sel_start, m_searchText.length());
		ed->setFocus();
	}
}

}}}
