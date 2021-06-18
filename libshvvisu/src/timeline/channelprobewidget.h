#pragma once

#include "../shvvisuglobal.h"

#include <QWidget>

namespace shv {
namespace visu {
namespace timeline {

namespace Ui {
class ChannelProbeWidget;
}

class ChannelProbe;

class SHVVISU_DECL_EXPORT ChannelProbeWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;

public:
	explicit ChannelProbeWidget(ChannelProbe *probe, QWidget *parent);
	~ChannelProbeWidget();

protected:
	enum class MouseOperation { None = 0, LeftMouseClick, Resize, Move };
	enum class WindowFarmeSection { NoSection = 0, LeftSection, TopLeftSection, TopSection, TopRightSection, RightSection, BottomRightSection, BottomSection, BottomLeftSection };
	enum DataTableColumn { ColProperty = 0, ColValue, ColCount };

	bool eventFilter(QObject *o, QEvent *e) override;

	//void mousePressEvent(QMouseEvent *event) override;
	//void mouseReleaseEvent(QMouseEvent *event) override;
	//void mouseMoveEvent(QMouseEvent *event) override;

//	void enterEvent(QEvent *event) override;
//	void leaveEvent(QEvent *event) override;

	void loadValues();
	WindowFarmeSection getWindowFrameSection();

	ChannelProbe *m_probe = nullptr;
	QPoint m_recentMousePos;

	MouseOperation m_mouseOperation = MouseOperation::None;
	WindowFarmeSection m_windowFrameSection = WindowFarmeSection::NoSection;

	Ui::ChannelProbeWidget *ui;
};

}}}
