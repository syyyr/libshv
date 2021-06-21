#pragma once

#include "../shvvisuglobal.h"

#include <QSet>
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
	enum class MouseOperation { None = 0, LeftMouseClick, ResizeWidget, MoveWidget };
	enum WindowFrameSection { LeftSection = 0, TopSection, RightSection, BottomSection };
	enum DataTableColumn { ColProperty = 0, ColValue, ColCount };

	bool eventFilter(QObject *o, QEvent *e) override;

	void loadValues();
	QSet<ChannelProbeWidget::WindowFrameSection> getWindowFrameSection();
	QCursor windowFrameSectionCursor(QSet<WindowFrameSection> wfs);


	ChannelProbe *m_probe = nullptr;
	QPoint m_recentMousePos;

	MouseOperation m_mouseOperation = MouseOperation::None;
	QSet<ChannelProbeWidget::WindowFrameSection> m_windowFrameSection;

	Ui::ChannelProbeWidget *ui;
};

}}}
