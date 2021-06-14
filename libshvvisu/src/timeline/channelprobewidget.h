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
	enum class MouseOperation { None = 0, Move };
	enum DataTableColumn { ColProperty = 0, ColValue, ColCount };

	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

	void loadValues();

	ChannelProbe *m_probe = nullptr;
	QPoint m_recentMousePos;

	MouseOperation m_mouseOperation = MouseOperation::None;

	Ui::ChannelProbeWidget *ui;
};

}}}
