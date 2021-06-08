#include "graphprobewidget.h"
#include "channelprobe.h"

#include "ui_graphprobewidget.h"

#include <shv/core/log.h>

#include <QMouseEvent>

namespace shv {
namespace visu {
namespace timeline {

GraphProbeWidget::GraphProbeWidget(QWidget *parent, ChannelProbe *probe) :
	QWidget(parent),
	ui(new Ui::GraphProbeWidget)
{
	ui->setupUi(this);
	m_probe = probe;

	ui->lblShvPath->setText(m_probe->shvPath());
	ui->fHeader->setStyleSheet("background-color:" + m_probe->color().name() + ";");
	ui->tbClose->setStyleSheet("background-color:white;");
	ui->lblProbe->setStyleSheet("QLabel { color : white; font-weight: bold;}");
	ui->lblShvPath->setStyleSheet("QLabel { color : white;}");

	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowType::Window);

	loadValues();

	connect(ui->tbClose, &QToolButton::clicked, this, &GraphProbeWidget::close);
	connect(m_probe, &ChannelProbe::currentTimeChanged, this, &GraphProbeWidget::loadValues);
	connect(ui->tbPrevSample, &QToolButton::clicked, this, &GraphProbeWidget::onTbPrevSampleClicked);
	connect(ui->tbNextSample, &QToolButton::clicked, this, &GraphProbeWidget::onTbNextSampleClicked);
}

GraphProbeWidget::~GraphProbeWidget()
{
	delete ui;
}

void GraphProbeWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if (ui->fHeader->rect().contains(pos)) {
		setCursor(QCursor(Qt::DragMoveCursor));
		m_recentMousePos = pos;
		m_mouseOperation = MouseOperation::Move;
	}
}

void GraphProbeWidget::mouseReleaseEvent(QMouseEvent *event)
{
	m_mouseOperation = MouseOperation::None;
	setCursor(QCursor(Qt::ArrowCursor));
}

void GraphProbeWidget::mouseMoveEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();

	if (m_mouseOperation == MouseOperation::Move) {
		QPoint dist = pos - m_recentMousePos;
		move(geometry().topLeft() + dist);
		m_recentMousePos = pos - dist;
		event->accept();
	}
	else {
		Super::mouseMoveEvent(event);
	}
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
