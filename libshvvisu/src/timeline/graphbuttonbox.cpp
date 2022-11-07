#include "graphbuttonbox.h"
#include "graph.h"

#include <shv/coreqt/log.h>

#include <QMouseEvent>
#include <QPainter>

#include <cmath>

namespace shv::visu::timeline {


//===================================================
// GraphButtonBox
//===================================================
GraphButtonBox::GraphButtonBox(const QVector<ButtonId> &button_ids, QObject *parent)
	: QObject(parent)
	, m_buttonIds(button_ids)
{

}

void GraphButtonBox::moveTopRight(const QPoint &p)
{
	m_rect.setSize(size());
	m_rect.moveTopRight(p);
}

void GraphButtonBox::hide()
{
	m_rect = QRect();
}

bool GraphButtonBox::processEvent(QEvent *ev)
{
	//auto rect_to_string = [](const QRect &r) {
	//	QString s = "%1,%2 %3x%4";
	//	return s.arg(r.x()).arg(r.y()).arg(r.width()).arg(r.height());
	//};
	auto invalidate_bb = [this]() {
		graph()->emitPresentationDirty(m_rect.adjusted(-10, -10, 10, 10));
	};
	switch (ev->type()) {
	case QEvent::MouseMove: {
		QMouseEvent *event = static_cast<QMouseEvent*>(ev);
		QPoint pos = event->pos();
		//shvDebug() << "------------------------";
		//shvDebug() << pos.x() << pos.y();
		//shvDebug() << rect_to_string(m_rect);
		if(m_rect.contains(pos)) {
			m_mouseOver = true;
			for (int i = 0; i < m_buttonIds.count(); ++i) {
				if(buttonRect(i).contains(pos)) {
					shvDebug() << objectName() << "mouseOver:" << i;
					m_mouseOverButtonIndex = i;
					break;
				}
			}
			invalidate_bb();
			ev->accept();
			return true;
		}
		else {
			if(m_mouseOver) {
				m_mouseOver = false;
				m_mouseOverButtonIndex = -1;
				m_mousePressButtonIndex = -1;
				invalidate_bb();
				ev->accept();
				return true;
			}
		}
		break;
	}
	case QEvent::MouseButtonPress: {
		//shvDebug() << objectName() << "press";
		QMouseEvent *event = static_cast<QMouseEvent*>(ev);
		QPoint pos = event->pos();
		for (int i = 0; i < m_buttonIds.count(); ++i) {
			if(buttonRect(i).contains(pos)) {
				shvDebug() << this << m_mousePressButtonIndex << i << "MouseButtonPress" << ev << objectName();
				m_mousePressButtonIndex = i;
				invalidate_bb();
				ev->accept();
				return true;
			}
		}
		break;
	}
	case QEvent::MouseButtonRelease: {
		if(m_mousePressButtonIndex < 0)
			break;
		shvDebug() << this << m_mousePressButtonIndex << "MouseButtonRelease" << ev;
		int ix = m_mousePressButtonIndex;
		m_mousePressButtonIndex = -1;
		invalidate_bb();
		shvDebug() << "emit button clicked";
		emit buttonClicked(static_cast<int>(m_buttonIds.value(ix)));
		ev->accept();
		return true;
	}
	default:
		break;
	}
	return false;
}

void GraphButtonBox::draw(QPainter *painter)
{
	if(isAutoRaise() && m_mouseOverButtonIndex < 0)
		return;
	painter->fillRect(m_rect, graph()->effectiveStyle().colorPanel());
	for (int i = 0; i < m_buttonIds.count(); ++i) {
		//shvDebug() << "name:" << this->objectName() << "DRAW2" << i << (int)m_buttonIds[i];
		drawButton(painter, buttonRect(i), i);
	}
}

QSize GraphButtonBox::size() const
{
	int w = buttonCount() * (buttonSpace() + buttonSize());
	return QSize{w, buttonSize()};
}

int GraphButtonBox::buttonSize() const
{
	return graph()->u2px(graph()->effectiveStyle().buttonSize());
}

int GraphButtonBox::buttonSpace() const
{
	return buttonSize() / 6;
}

QRect GraphButtonBox::buttonRect(GraphButtonBox::ButtonId id) const
{
	for (int i = 0; i < m_buttonIds.count(); ++i) {
		if(m_buttonIds[i] == id)
			return buttonRect(i);
	}
	return QRect();
}

QRect GraphButtonBox::buttonRect(int ix) const
{
	if(ix >= 0) {
		int offset = ix * (buttonSize() + buttonSpace());
		QRect ret = m_rect;
		ret.setX(ret.x() + offset + buttonSpace());
		ret.setWidth(buttonSize());
		return ret;
	}
	return QRect();
}

void GraphButtonBox::drawButton(QPainter *painter, const QRect &rect, int button_index)
{
	//shvDebug() << __FILE__ << __LINE__ << button_index << "over:" << m_mouseOverButtonIndex;
	painter->save();
	int corner_radius = rect.height() / 8;
	QPen p1(graph()->effectiveStyle().color());
	p1.setWidth(graph()->u2px(0.1));
	if(m_mouseOverButtonIndex == button_index) {
		painter->setPen(p1);
		if(m_mousePressButtonIndex == button_index)
			painter->setBrush(Qt::darkGray);
		else
			painter->setBrush(graph()->effectiveStyle().colorBackground());
		painter->drawRoundedRect(rect, corner_radius, corner_radius);
	}
	auto btid = m_buttonIds[button_index];
	switch (btid) {
	case ButtonId::Menu: {
		QPen p = p1;
		p.setWidth(buttonSize() / 6);
		p.setCapStyle(Qt::RoundCap);
		p.setColor(QColor("orange"));
		painter->setPen(p);
		int x1 = rect.width() / 4;
		int x2 = rect.width() - x1;
		int h = rect.height() / 4;
		for (int i = 0; i < 3; ++i) {
			int y = h + h/4 + h * i;
			painter->drawLine(rect.x() + x1, rect.top() + y, rect.x() + x2, rect.top() + y);
		}
		break;
	}
	case ButtonId::Hide: {
		int inset = rect.height() / 8;
		QPen p = p1;
		p.setColor(QColor("skyblue"));
		painter->setPen(p);
		QRect r1 = rect.adjusted(inset, inset, -inset, -inset);

		QRect r = r1;
		int start_angle = 25;
		const double pi = std::acos(-1);
		int offset = static_cast<int>(std::sin(start_angle * pi / 180) * r.height() / 2);
		int span_angle = 180 - 2*start_angle;
		r.moveTop(r.top() + offset);
		painter->drawArc(r, start_angle * 16, span_angle * 16);
		r.moveTop(r.top() - 2*offset);
		painter->drawArc(r, (180 + start_angle) * 16, span_angle * 16);

		int w = r1.width() / 3;
		QRect r2{0, 0, w, w};
		r2.moveCenter(rect.center());
		//painter->save();
		//painter->setBrush(p.color());
		painter->drawEllipse(r2);
		//painter->restore();
		r = r1.adjusted(inset, inset, -inset, -inset);
		painter->drawLine(r.bottomLeft(), r.topRight());
		break;
	}
	default:
		break;
	}
	painter->restore();
}

Graph *GraphButtonBox::graph() const
{
	auto *o = parent();
	while(o) {
		auto *g = qobject_cast<Graph*>(o);
		if(g)
			return g;
		o = o->parent();
	}
	return nullptr;
}

} // namespace shv
