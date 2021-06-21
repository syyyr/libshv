#include "channelprobewidget.h"
#include "channelprobe.h"

#include "ui_channelprobewidget.h"

#include <shv/core/log.h>

#include <QMouseEvent>

namespace shv {
namespace visu {
namespace timeline {

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
	ui->fHeader->installEventFilter(this);
	ui->twData->viewport()->installEventFilter(this);

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
			m_mouseOperation = MouseOperation::ResizeWidget;
			e->accept();
			return true;
		}
		else if (o == ui->fHeader) {
			setCursor(QCursor(Qt::DragMoveCursor));
			m_recentMousePos = pos;
			m_mouseOperation = MouseOperation::MoveWidget;
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

		if (m_mouseOperation == MouseOperation::MoveWidget) {
			QPoint dist = pos - m_recentMousePos;
			move(geometry().topLeft() + dist);
			m_recentMousePos = pos - dist;
			e->accept();
			return true;
		}
		else if (m_mouseOperation == MouseOperation::ResizeWidget) {
			QPoint pos =  QCursor::pos();
			QRect g = geometry();

			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::LeftSection})
				g.setLeft(pos.x());
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::RightSection})
				g.setRight(pos.x());
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::TopSection})
				g.setTop(pos.y());
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::BottomSection})
				g.setBottom(pos.y());
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::TopSection, WindowFrameSection::LeftSection})
				g.setTopLeft(pos);
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::BottomSection, WindowFrameSection::RightSection})
				g.setBottomRight(pos);
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::TopSection, WindowFrameSection::RightSection})
				g.setTopRight(pos);
			if (m_windowFrameSection == QSet<WindowFrameSection>{WindowFrameSection::BottomSection, WindowFrameSection::LeftSection})
				g.setBottomLeft(pos);

			setGeometry(g);
			e->accept();
			return true;
		}

		QSet<WindowFrameSection> wfs = getWindowFrameSection();
		setCursor(windowFrameSectionCursor(wfs));
	}

	return Super::eventFilter(o, e);
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

QSet<ChannelProbeWidget::WindowFrameSection> ChannelProbeWidget::getWindowFrameSection()
{
	static const int FRAME_RECT_WIDTH = 6;

	QSet<WindowFrameSection> ret;

	QPoint pos = QCursor::pos();
	if (pos.x() - geometry().left() < FRAME_RECT_WIDTH)
		ret.insert(WindowFrameSection::LeftSection);
	if (geometry().right() - pos.x() < FRAME_RECT_WIDTH)
		ret.insert(WindowFrameSection::RightSection);
	if (pos.y() - geometry().top() < FRAME_RECT_WIDTH)
		ret.insert(WindowFrameSection::TopSection);
	if (geometry().bottom() - pos.y() < FRAME_RECT_WIDTH)
		ret.insert(WindowFrameSection::BottomSection);

	return ret;
}

QCursor ChannelProbeWidget::windowFrameSectionCursor(QSet<ChannelProbeWidget::WindowFrameSection> wfs)
{
	if ((wfs == QSet<WindowFrameSection>{WindowFrameSection::LeftSection}) || (wfs == QSet<WindowFrameSection>{WindowFrameSection::RightSection}))
		return QCursor(Qt::SizeHorCursor);
	if ((wfs == QSet<WindowFrameSection>{WindowFrameSection::TopSection}) || (wfs == QSet<WindowFrameSection>{WindowFrameSection::BottomSection}))
		return QCursor(Qt::SizeVerCursor);
	if ((wfs == QSet<WindowFrameSection>{WindowFrameSection::TopSection, WindowFrameSection::LeftSection}) ||
		(wfs == QSet<WindowFrameSection>{WindowFrameSection::BottomSection, WindowFrameSection::RightSection}))
		return QCursor(Qt::SizeFDiagCursor);
	if ((wfs == QSet<WindowFrameSection>{WindowFrameSection::TopSection, WindowFrameSection::RightSection}) ||
		(wfs == QSet<WindowFrameSection>{WindowFrameSection::BottomSection, WindowFrameSection::LeftSection}))
		return QCursor(Qt::SizeBDiagCursor);

	return QCursor(Qt::ArrowCursor);
}

}}}
