#include "channelprobewidget.h"
#include "channelprobe.h"

#include "ui_channelprobewidget.h"

#include <shv/core/log.h>

#include <QMouseEvent>

namespace shv {
namespace visu {
namespace timeline {

ChannelProbeWidget::ChannelProbeWidget(QWidget *parent, ChannelProbe *probe) :
	QWidget(parent),
	ui(new Ui::ChannelProbeWidget)
{
	ui->setupUi(this);
	m_probe = probe;

	ui->lblTitle->setText(m_probe->shvPath());

	ui->edCurentTime->setStyleSheet("background-color: white");
	ui->fHeader->setStyleSheet("background-color:" + m_probe->color().name() + ";");
	ui->tbClose->setStyleSheet("background-color:white;");
	ui->lblTitle->setStyleSheet("color:white;");

	setAttribute(Qt::WA_DeleteOnClose, true);
	setWindowFlags(Qt::FramelessWindowHint | Qt::ToolTip | Qt::WindowType::Window);

	ui->twData->setColumnCount(DataTableColumn::ColCount);
	ui->twData->setHorizontalHeaderItem(DataTableColumn::ColProperty, new QTableWidgetItem("Property"));
	ui->twData->setHorizontalHeaderItem(DataTableColumn::ColValue, new QTableWidgetItem("Value"));
	ui->twData->verticalHeader()->setDefaultSectionSize((int)(fontMetrics().lineSpacing() * 1.3));

	loadValues();

	ui->twData->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);

	connect(ui->tbClose, &QToolButton::clicked, this, &ChannelProbeWidget::close);
	connect(m_probe, &ChannelProbe::currentTimeChanged, this, &ChannelProbeWidget::loadValues);
	connect(ui->tbPrevSample, &QToolButton::clicked, m_probe, &ChannelProbe::prevSample);
	connect(ui->tbNextSample, &QToolButton::clicked, m_probe, &ChannelProbe::nextSample);
}

ChannelProbeWidget::~ChannelProbeWidget()
{
	delete ui;
}

void ChannelProbeWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if (ui->fHeader->rect().contains(pos)) {
		setCursor(QCursor(Qt::DragMoveCursor));
		m_recentMousePos = pos;
		m_mouseOperation = MouseOperation::Move;
	}
}

void ChannelProbeWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	m_mouseOperation = MouseOperation::None;
	setCursor(QCursor(Qt::ArrowCursor));
}

void ChannelProbeWidget::mouseMoveEvent(QMouseEvent *event)
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

void ChannelProbeWidget::loadValues()
{
	ui->edCurentTime->setText(m_probe->currentTimeIsoFormat());
	ui->twData->clearContents();
	ui->twData->setRowCount(0);

	QMap<QString, QString> values = m_probe->yValues();
	QMapIterator<QString, QString> i(values);

	while (i.hasNext()) {
		i.next();
		int ix = ui->twData->rowCount();
		ui->twData->insertRow(ix);

		QTableWidgetItem *item = new QTableWidgetItem(i.key());
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twData->setItem(ix, DataTableColumn::ColProperty, item);

		item = new QTableWidgetItem(i.value());
		item->setFlags(item->flags() & ~Qt::ItemIsEditable);
		ui->twData->setItem(ix, DataTableColumn::ColValue, item);
	}
}

}}}
