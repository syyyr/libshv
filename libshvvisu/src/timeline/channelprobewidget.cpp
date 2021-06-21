#include "channelprobewidget.h"
#include "channelprobe.h"

#include "ui_channelprobewidget.h"

#include <shv/core/log.h>

#include <QMouseEvent>

namespace shv {
namespace visu {
namespace timeline {

static const int FRAME_RECT_WIDTH = 4;

ChannelProbeWidget::ChannelProbeWidget(ChannelProbe *probe, QWidget *parent) :
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

	installEventFilter(this);
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

bool ChannelProbeWidget::eventFilter(QObject *o, QEvent *e)
{

	if (e->type() == QEvent::MouseButtonPress) {
		QPoint pos = static_cast<QMouseEvent*>(e)->pos();
		m_windowFrameSection = getWindowFrameSection();

		if (!m_windowFrameSection.isEmpty()) {
			m_mouseOperation = MouseOperation::Resize;
			e->accept();
			return true;
		}

		else if (ui->fHeader->rect().contains(pos)) {
			setCursor(QCursor(Qt::DragMoveCursor));
			m_recentMousePos = pos;
			m_mouseOperation = MouseOperation::Move;
			e->accept();
			return true;
		}
	}
	else if (e->type() == QEvent::MouseButtonRelease) {
		m_mouseOperation = MouseOperation::None;
		setCursor(QCursor(Qt::ArrowCursor));
	}
	else if (e->type() == QEvent::MouseMove) {
		QPoint pos = static_cast<QMouseEvent*>(e)->pos();

		if (m_mouseOperation == MouseOperation::Move) {
			QPoint dist = pos - m_recentMousePos;
			move(geometry().topLeft() + dist);
			m_recentMousePos = pos - dist;
			e->accept();
			return true;
		}
		else if (m_mouseOperation == MouseOperation::Resize) {
			QPoint pos =  QCursor::pos();
			QRect g = geometry();

			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::LeftSection})
				g.setLeft(pos.x());
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::RightSection})
				g.setRight(pos.x());
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::TopSection})
				g.setTop(pos.y());
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::BottomSection})
				g.setBottom(pos.y());
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::TopSection, WindowFarmeSection::LeftSection})
				g.setTopLeft(pos);
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::BottomSection, WindowFarmeSection::RightSection})
				g.setBottomRight(pos);
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::TopSection, WindowFarmeSection::RightSection})
				g.setTopRight(pos);
			if (m_windowFrameSection == QSet<WindowFarmeSection>{WindowFarmeSection::BottomSection, WindowFarmeSection::LeftSection})
				g.setBottomLeft(pos);

			setGeometry(g);
			e->accept();
			return true;
		}

		QSet<WindowFarmeSection> wfs = getWindowFrameSection();
		setCursor(windowFrameSectionCursor(wfs));
	}

/*	if (o == ui->twData)
	{
		if (e->type() == QEvent::MouseMove)
			shvInfo() << "table mouse moveevent";
	}
	else if (o == ui->twData->viewport())
	{
		if (e->type() == QEvent::MouseMove)
			shvInfo() << "table->viewport mouse moveevent";
	}*/

	return Super::eventFilter(o, e);
}

/*
void ChannelProbeWidget::mousePressEvent(QMouseEvent *event)
{

}

void ChannelProbeWidget::mouseReleaseEvent(QMouseEvent *event)
{
	Q_UNUSED(event);
	m_mouseOperation = MouseOperation::None;
	setCursor(QCursor(Qt::ArrowCursor));
}
*/
/*void ChannelProbeWidget::mouseMoveEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();

	pos = event->pos();

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
*/
/*
void ChannelProbeWidget::enterEvent(QEvent *event)
{
	QPoint pos = QCursor::pos();
	int w = 20;

	shvInfo() << geometry().left() - pos.x() << geometry().right() - pos.x();


		//shvInfo() << QCursor::pos().x() << geometry().center() << geometry().width();


	Super::enterEvent(event);
}

void ChannelProbeWidget::leaveEvent(QEvent *event)
{
	setCursor(QCursor(Qt::ArrowCursor));
	Super::leaveEvent(event);
}
*/

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

	ui->twData->viewport()->setMouseTracking(true);
	ui->twData->viewport()->installEventFilter(this);
}

QSet<ChannelProbeWidget::WindowFarmeSection> ChannelProbeWidget::getWindowFrameSection()
{
	QSet<WindowFarmeSection> ret;

	QPoint pos = QCursor::pos();
	if (pos.x() - geometry().left() < FRAME_RECT_WIDTH)
		ret.insert(WindowFarmeSection::LeftSection);
	if (geometry().right() - pos.x() < FRAME_RECT_WIDTH)
		ret.insert(WindowFarmeSection::RightSection);
	if (pos.y() - geometry().top() < FRAME_RECT_WIDTH)
		ret.insert(WindowFarmeSection::TopSection);
	if (geometry().bottom() - pos.y() < FRAME_RECT_WIDTH)
		ret.insert(WindowFarmeSection::BottomSection);

	return ret;
}

QCursor ChannelProbeWidget::windowFrameSectionCursor(QSet<ChannelProbeWidget::WindowFarmeSection> wfs)
{
	if ((wfs == QSet<WindowFarmeSection>{WindowFarmeSection::LeftSection}) || (wfs == QSet<WindowFarmeSection>{WindowFarmeSection::RightSection}))
		return QCursor(Qt::SizeHorCursor);
	if ((wfs == QSet<WindowFarmeSection>{WindowFarmeSection::TopSection}) || (wfs == QSet<WindowFarmeSection>{WindowFarmeSection::BottomSection}))
		return QCursor(Qt::SizeVerCursor);
	if ((wfs == QSet<WindowFarmeSection>{WindowFarmeSection::TopSection, WindowFarmeSection::LeftSection}) ||
		(wfs == QSet<WindowFarmeSection>{WindowFarmeSection::BottomSection, WindowFarmeSection::RightSection}))
		return QCursor(Qt::SizeFDiagCursor);
	if ((wfs == QSet<WindowFarmeSection>{WindowFarmeSection::TopSection, WindowFarmeSection::RightSection}) ||
		(wfs == QSet<WindowFarmeSection>{WindowFarmeSection::BottomSection, WindowFarmeSection::LeftSection}))
		return QCursor(Qt::SizeBDiagCursor);

	return QCursor(Qt::ArrowCursor);
}

}}}
