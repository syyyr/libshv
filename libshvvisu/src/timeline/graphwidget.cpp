#include "graphwidget.h"
#include "graphmodel.h"
#include "graphview.h"

#include <shv/core/exception.h>
#include <shv/coreqt/log.h>
//#include <shv/chainpack/rpcvalue.h>

#include <QPainter>
#include <QFontMetrics>
#include <QLabel>
#include <QMouseEvent>
#include <QToolTip>
#include <QDateTime>
#include <QMenu>
#include <QScrollBar>

#include <cmath>

#define logMouseSelection() nCDebug("MouseSelection")

namespace cp = shv::chainpack;

namespace shv {
namespace visu {
namespace timeline {

GraphWidget::GraphWidget(QWidget *parent)
	: Super(parent)
{
	setMouseTracking(true);
	setContextMenuPolicy(Qt::DefaultContextMenu);
}

void GraphWidget::setGraph(Graph *g)
{
	if(m_graph)
		m_graph->disconnect(this);
	m_graph = g;
	Graph::GraphStyle style = graph()->style();
	style.init(this);
	graph()->setStyle(style);
	update();
	connect(m_graph, &Graph::presentationDirty, this, [this]() {
		update();
	});
}

Graph *GraphWidget::graph()
{
	return m_graph;
}

const Graph *GraphWidget::graph() const
{
	return m_graph;
}

void GraphWidget::makeLayout(const QSize &pref_size)
{
	shvLogFuncFrame();
	graph()->makeLayout(QRect(QPoint(), pref_size));
	setMinimumSize(graph()->rect().size());
	setMaximumSize(graph()->rect().size());
	update();
}

void GraphWidget::makeLayout()
{
	makeLayout(graph()->rect().size());
}

void GraphWidget::paintEvent(QPaintEvent *event)
{
	Super::paintEvent(event);
	auto *view_port = parentWidget();
	auto *scroll_area = qobject_cast<GraphView*>(view_port->parentWidget());
	if(!scroll_area) {
		shvError() << "Cannot find GraphView";
		return;
	}
	//auto rect_to_string = [](const QRect &r) {
	//	QString s = "%1,%2 %3x%4";
	//	return s.arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
	//};
	QPainter painter(this);
	const QRect dirty_rect = event->rect();
	QRect view_rect = view_port->geometry();
	view_rect.moveTop(-geometry().y());
	//shvInfo() << "-----------------------------------" << view_port->objectName();
	//shvInfo() << "dirty rect:"  << rect_to_string(dirty_rect);
	//shvInfo() << "view port :"  << rect_to_string(view_port->geometry());
	//shvInfo() << "widget    :"  << rect_to_string(geometry());
	//shvInfo() << "view_rect :"  << rect_to_string(view_rect);
	graph()->draw(&painter, dirty_rect,  view_rect);
}
/*
void GraphWidget::keyPressEvent(QKeyEvent *event)
{
	shvDebug() << event->key();
	//if(event->key() == Qt::)
	Super::keyPressEvent(event);
}

void GraphWidget::keyReleaseEvent(QKeyEvent *event)
{
	shvLogFuncFrame();
	Super::keyReleaseEvent(event);
}
*/
bool GraphWidget::isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const
{
	const Graph *gr = graph();
	if(mouse_pos.y() < gr->miniMapRect().top() || mouse_pos.y() > gr->miniMapRect().bottom())
		return false;
	int x = left
			? gr->miniMapTimeToPos(gr->xRangeZoom().min)
			: gr->miniMapTimeToPos(gr->xRangeZoom().max);
	int w = gr->u2px(0.5);
	int x1 = left ? x - w : x - w/2;
	int x2 = left ? x + w/2 : x + w;
	return mouse_pos.x() > x1 && mouse_pos.x() < x2;
}

bool GraphWidget::isMouseAboveLeftMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, true);
}

bool GraphWidget::isMouseAboveRightMiniMapHandle(const QPoint &pos) const
{
	return isMouseAboveMiniMapHandle(pos, false);
}

bool GraphWidget::isMouseAboveMiniMapSlider(const QPoint &pos) const
{
	const Graph *gr = graph();
	if(pos.y() < gr->miniMapRect().top() || pos.y() > gr->miniMapRect().bottom())
		return false;
	int x1 = gr->miniMapTimeToPos(gr->xRangeZoom().min);
	int x2 = gr->miniMapTimeToPos(gr->xRangeZoom().max);
	return (x1 < pos.x()) && (pos.x() < x2);
}

int GraphWidget::isMouseAboveGraphArea(const QPoint &pos) const
{
	const Graph *gr = graph();
	int ch_ix = gr->posToChannel(pos);
	return ch_ix >= 0;
}

void GraphWidget::mousePressEvent(QMouseEvent *event)
{
	QPoint pos = event->pos();
	if(event->button() == Qt::LeftButton) {
		if(isMouseAboveLeftMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapLeftResize;
			shvDebug() << "LEFT resize";
			event->accept();
			return;
		}
		else if(isMouseAboveRightMiniMapHandle(pos)) {
			m_mouseOperation = MouseOperation::MiniMapRightResize;
			event->accept();
			return;
		}
		else if(isMouseAboveMiniMapSlider(pos)) {
			m_mouseOperation = MouseOperation::MiniMapScrollZoom;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		else if(isMouseAboveGraphArea(pos) && event->modifiers() == Qt::ControlModifier) {
			m_mouseOperation = MouseOperation::GraphAreaMove;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
		else if(isMouseAboveGraphArea(pos)) {
			logMouseSelection() << "GraphAreaPress";
			m_mouseOperation = MouseOperation::GraphAreaPress;
			m_recentMousePos = pos;
			event->accept();
			return;
		}
	}
	Super::mousePressEvent(event);
}

void GraphWidget::mouseReleaseEvent(QMouseEvent *event)
{
	logMouseSelection() << "mouseReleaseEvent, button:" << event->button() << "op:" << (int)m_mouseOperation;
	auto old_mouse_op = m_mouseOperation;
	m_mouseOperation = MouseOperation::None;
	if(event->button() == Qt::LeftButton) {
		if(old_mouse_op == MouseOperation::GraphAreaPress) {
			QPoint pos = event->pos();
			if(event->modifiers() == Qt::NoModifier) {
				Graph *gr = graph();
				logMouseSelection() << "Set cross bar 1 pos to:" << pos.x() << pos.y();
				gr->setCrossBarPos1(pos);
				event->accept();
				update();
				return;
			}
			else if(event->modifiers() == Qt::ShiftModifier) {
				Graph *gr = graph();
				gr->setCrossBarPos2(pos);
				event->accept();
				update();
				return;
			}
		}
		else if(old_mouse_op == MouseOperation::GraphAreaSelection) {
			graph()->zoomToSelection();
			graph()->setSelectionRect(QRect());
			event->accept();
			update();
			return;
		}
	}
	Super::mouseReleaseEvent(event);
}

void GraphWidget::mouseMoveEvent(QMouseEvent *event)
{
	//logMouseSelection() << event->pos().x() << event->pos().y() << (int)m_mouseOperation;
	Super::mouseMoveEvent(event);
	QPoint pos = event->pos();
	Graph *gr = graph();
	if(isMouseAboveLeftMiniMapHandle(pos) || isMouseAboveRightMiniMapHandle(pos)) {
		//shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
		setCursor(QCursor(Qt::SplitHCursor));
	}
	else if(isMouseAboveMiniMapSlider(pos)) {
		//shvDebug() << (vpos.x() - gr->miniMapRect().left()) << (vpos.y() - gr->miniMapRect().top());
		setCursor(QCursor(Qt::SizeHorCursor));
	}
	else {
		setCursor(QCursor(Qt::ArrowCursor));
	}
	switch (m_mouseOperation) {
	case MouseOperation::MiniMapLeftResize: {
		timemsec_t t = gr->miniMapPosToTime(pos.x());
		XRange r = gr->xRangeZoom();
		r.min = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapRightResize: {
		timemsec_t t = gr->miniMapPosToTime(pos.x());
		XRange r = gr->xRangeZoom();
		r.max = t;
		if(r.interval() > 0) {
			gr->setXRangeZoom(r);
			update();
		}
		return;
	}
	case MouseOperation::MiniMapScrollZoom: {
		timemsec_t t2 = gr->miniMapPosToTime(pos.x());
		timemsec_t t1 = gr->miniMapPosToTime(m_recentMousePos.x());
		m_recentMousePos = pos;
		XRange r = gr->xRangeZoom();
		shvDebug() << "r.min:" << r.min << "r.max:" << r.max;
		timemsec_t dt = t2 - t1;
		shvDebug() << dt << "r.min:" << r.min << "-->" << (r.min + dt);
		r.min += dt;
		r.max += dt;
		if(r.min >= gr->xRange().min && r.max <= gr->xRange().max) {
			gr->setXRangeZoom(r);
			r = gr->xRangeZoom();
			shvDebug() << "new r.min:" << r.min << "r.max:" << r.max;
			update();
		}
		return;
	}
	case MouseOperation::GraphAreaPress: {
		QPoint point = pos - m_recentMousePos;
		if (point.manhattanLength() > 3) {
			m_mouseOperation = MouseOperation::GraphAreaSelection;
		}
		break;
	}
	case MouseOperation::GraphAreaSelection: {
		gr->setSelectionRect(QRect(m_recentMousePos, pos));
		update();
		break;
	}
	case MouseOperation::GraphAreaMove: {
		timemsec_t t0 = gr->posToTime(m_recentMousePos.x());
		timemsec_t t1 = gr->posToTime(pos.x());
		timemsec_t dt = t0 - t1;
		XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max += dt;
		gr->setXRangeZoom(r);
		m_recentMousePos = pos;
		update();
		return;
	}
	case MouseOperation::None:
		break;
	}
	/*
	int ch_ix = gr->posToChannel(pos);
	if(ch_ix >= 0) {
		setCursor(Qt::BlankCursor);
		gr->setCrossBarPos(pos);
		timemsec_t t = gr->posToTime(pos.x());
		Sample s = gr->timeToSample(ch_ix, t);
		if(s.isValid()) {
			const Graph::Channel &ch = gr->channelAt(ch_ix);
			shvDebug() << "time:" << s.time << "value:" << s.value.toDouble();
			QDateTime dt = QDateTime::fromMSecsSinceEpoch(s.time);
			QString text = QStringLiteral("%1\n%2: %3")
					.arg(dt.toString(Qt::ISODateWithMs))
					.arg(gr->model()->channelData(ch.modelIndex(), timeline::GraphModel::ChannelDataRole::Name).toString())
					.arg(s.value.toString());
			QToolTip::showText(mapToGlobal(pos + QPoint{gr->u2px(0.8), 0}), text, this);
		}
		else {
			QToolTip::showText(QPoint(), QString());
		}
		update();
	}
	else {
		if(!gr->crossBarPos().isNull()) {
			gr->setCrossBarPos(QPoint());
			setCursor(Qt::ArrowCursor);
			QToolTip::showText(QPoint(), QString());
			update();
		}
	}
	*/
}

void GraphWidget::wheelEvent(QWheelEvent *event)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	QPoint pos = event->pos();
#else
	QPoint pos = event->position().toPoint();
#endif
	bool is_zoom_on_slider = isMouseAboveMiniMapSlider(pos);
	bool is_zoom_on_graph = (event->modifiers() == Qt::ControlModifier) && isMouseAboveGraphArea(pos);
	static constexpr int ZOOM_STEP = 10;
	if(is_zoom_on_slider) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		//deg /= 120;
		// 120 deg ~ 1/20 of range
		timemsec_t dt = static_cast<timemsec_t>(deg * gr->xRangeZoom().interval() / 120 / ZOOM_STEP);
		XRange r = gr->xRangeZoom();
		r.min += dt;
		r.max -= dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			update();
		}
		event->accept();
		return;
	}
	if(is_zoom_on_graph) {
		Graph *gr = graph();
		double deg = event->angleDelta().y();
		//deg /= 120;
		// 120 deg ~ 1/20 of range
		timemsec_t dt = static_cast<timemsec_t>(deg * gr->xRangeZoom().interval() / 120 / ZOOM_STEP);
		XRange r = gr->xRangeZoom();
		double ratio = 0.5;
		// shift new zoom to center it horizontally on the mouse position
		timemsec_t t_mouse = gr->posToTime(pos.x());
		ratio = static_cast<double>(t_mouse - r.min) / r.interval();
		r.min += ratio * dt;
		r.max -= (1 - ratio) * dt;
		if(r.interval() > 1) {
			gr->setXRangeZoom(r);
			//r = gr->xRangeZoom();
			update();
		}
		event->accept();
		return;
	}
	if(event->modifiers() == Qt::ControlModifier) {
		/// resize vertical header cell on Ctrl + mouse_wheel
		for (int i = 0; i < m_graph->channelCount(); ++i) {
			Graph::Channel &ch = m_graph->channelAt(i);
			if(ch.verticalHeaderRect().contains(pos)) {
				static constexpr int STEP = 2;
				timeline::Graph::ChannelStyle ch_style = ch.style();
				double deg = event->angleDelta().y();
				int new_h = static_cast<int>(deg * ch.verticalHeaderRect().height() / 120 / STEP);
				new_h = ch.verticalHeaderRect().height() + new_h;
				double new_u = m_graph->px2u(new_h);
				//shvInfo() << i << ch.verticalHeaderRect().height() << new_h << new_u;
				ch_style.setHeightMax(new_u);
				ch_style.setHeightMin(new_u);
				ch.setStyle(ch_style);
				makeLayout();
				return;
			}
		}
	}
	Super::wheelEvent(event);
}

void GraphWidget::contextMenuEvent(QContextMenuEvent *event)
{
	if(!m_graph)
		return;
	QMenu menu(this);
	const QPoint pos = event->pos();
	for (int i = 0; i < m_graph->channelCount(); ++i) {
		const Graph::Channel &ch = m_graph->channelAt(i);
		if(ch.verticalHeaderRect().contains(pos)) {
			menu.addAction(tr("Reset Y-range"), [this, ch, i]() {
				//shvInfo() << "settings";
				timeline::GraphModel *m = m_graph->model();
				if(!m)
					return;
				timeline::YRange rng = m->yRange(ch.modelIndex());
				m_graph->setYRange(i, rng);
				this->update();
			});
		}
	}
	if(menu.actions().count())
		menu.exec(mapToGlobal(pos));
}

}}}

