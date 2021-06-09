#pragma once

#include <QWidget>

namespace shv {
namespace visu {
namespace timeline {

namespace Ui {
class GraphProbeWidget;
}

class ChannelProbe;

class GraphProbeWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;

public:
	explicit GraphProbeWidget(QWidget *parent, ChannelProbe *probe);
	~GraphProbeWidget();

protected:
	enum class MouseOperation { None = 0, Move };
	enum DataTableColumn { ColProperty = 0, ColValue, ColCount };

	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;

	void loadValues();
	void onTbPrevSampleClicked();
	void onTbNextSampleClicked();

	ChannelProbe *m_probe = nullptr;
	QPoint m_recentMousePos;

	MouseOperation m_mouseOperation = MouseOperation::None;

	Ui::GraphProbeWidget *ui;
};

}}}
