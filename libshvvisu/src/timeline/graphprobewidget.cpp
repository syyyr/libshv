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

	setWindowTitle(tr("Probe:") + " " + m_probe->shvPath());
	setAttribute(Qt::WA_DeleteOnClose, true);

	loadValues();

	connect(m_probe, &ChannelProbe::currentTimeChanged, this, &GraphProbeWidget::loadValues);
	connect(ui->tbPrevSample, &QToolButton::clicked, this, &GraphProbeWidget::onTbPrevSampleClicked);
	connect(ui->tbNextSample, &QToolButton::clicked, this, &GraphProbeWidget::onTbNextSampleClicked);
}

GraphProbeWidget::~GraphProbeWidget()
{
	delete ui;
}

void GraphProbeWidget::loadValues()
{
	ui->teCurrentTime->setDateTime(QDateTime::fromMSecsSinceEpoch(m_probe->currentTime()));
	ui->tbYvalues->setText(m_probe->yValues());
}

void GraphProbeWidget::onTbPrevSampleClicked()
{
	m_probe->prevValue();
}

void GraphProbeWidget::onTbNextSampleClicked()
{
	m_probe->nextValue();
}

}}}
