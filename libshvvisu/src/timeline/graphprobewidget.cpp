#include "graphprobewidget.h"
#include "channelprobe.h"

#include "ui_graphprobewidget.h"

namespace shv {
namespace visu {
namespace timeline {

GraphProbeWidget::GraphProbeWidget(QWidget *parent, ChannelProbe *probe) :
	QWidget(parent),
	ui(new Ui::GraphProbeWidget)
{
	ui->setupUi(this);

	m_probe = probe;
	ui->teCurrentTime->setDateTime(QDateTime::fromMSecsSinceEpoch(m_probe->currentTime()));
	ui->tbYvalues->setText(m_probe->yValues())
}

GraphProbeWidget::~GraphProbeWidget()
{
	delete ui;
}

}}}
