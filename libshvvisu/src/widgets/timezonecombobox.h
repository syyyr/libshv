#pragma once

#include "../shvvisuglobal.h"

#include <QComboBox>

class QTimeZone;

namespace shv {
namespace visu {

class SHVVISU_DECL_EXPORT TimeZoneComboBox : public QComboBox
{
	using Super = QComboBox;
public:
	TimeZoneComboBox(QWidget *parent = nullptr);

#if SHVVISU_HAS_TIMEZONE
	QTimeZone currentTimeZone() const;
protected:
	void keyPressEvent(QKeyEvent *event) override;
private:
	//static constexpr auto LOCAL_TZ_ID = "Local";
	QString m_searchText;
#endif
};

}}

