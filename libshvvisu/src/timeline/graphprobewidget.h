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

public:
	explicit GraphProbeWidget(QWidget *parent, ChannelProbe *probe);
	~GraphProbeWidget();

private:
	ChannelProbe *m_probe = nullptr;

	Ui::GraphProbeWidget *ui;
};

}}}
