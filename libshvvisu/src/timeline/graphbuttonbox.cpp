#include "graphbuttonbox.h"
#include "graph.h"

#include <QMouseEvent>
#include <QPainter>

namespace shv {
namespace visu {
namespace timeline {
/*
//===================================================
// GraphButton
//===================================================
GraphButton::GraphButton(QObject *parent)
	: QObject(parent)
{

}
*/
//===================================================
// GraphButtonBox
//===================================================
GraphButtonBox::GraphButtonBox(const QVector<ButtonId> &button_ids, Graph *graph)
	: m_graph(graph)
	, m_buttonIds(button_ids)
{

}

void GraphButtonBox::moveTopRight(const QPoint &p)
{
	int button_size = buttonSize();
	m_rect.moveTopRight(p);
	m_rect.setHeight(button_size);
	m_rect.setRight(buttonRect(m_buttonIds.count()).left() - buttonSpace());
}

void GraphButtonBox::event(QEvent *ev)
{
	switch (ev->type()) {
	case QEvent::MouseMove: {
		QMouseEvent *event = static_cast<QMouseEvent*>(ev);
		QPoint pos = event->pos();
		if(m_rect.contains(pos)) {
			if(!m_mouseOver) {
				m_mouseOver = true;
				m_graph->emitPresentationDirty(m_rect);
				ev->accept();
				return;
			}
		}
		else {
			if(m_mouseOver) {
				m_mouseOver = false;
				m_graph->emitPresentationDirty(m_rect);
			}
		}
		break;
	}
	default:
		break;
	}
}

void GraphButtonBox::draw(QPainter *painter)
{
	if(!m_mouseOver)
		return;
	painter->fillRect(m_rect, m_graph->effectiveStyle().colorPanel());
	for (int i = 0; i < m_buttonIds.count(); ++i) {
		auto btid = m_buttonIds[i];
		drawButton(painter, buttonRect(i), btid);
	}
}

int GraphButtonBox::buttonSize() const
{
	return m_graph->u2px(m_graph->effectiveStyle().buttonSize());
}

int GraphButtonBox::buttonSpace() const
{
	return buttonSize() / 5;
}

QRect GraphButtonBox::buttonRect(int ix) const
{
	if(ix >= 0) {
		int offset = ix * buttonSize() + ((ix > 0)? (ix-1) * buttonSpace(): 0);
		QRect ret = m_rect;
		ret.setX(ret.x() + offset);
		ret.setWidth(buttonSize());
		return ret;
	}
	return QRect();
}

void GraphButtonBox::drawButton(QPainter *painter, const QRect &rect, GraphButtonBox::ButtonId btid)
{
	QPen p1(m_graph->effectiveStyle().color());
	p1.setWidth(m_graph->u2px(0.1));
	p1.setCapStyle(Qt::RoundCap);
	switch (btid) {
	case ButtonId::Properties: {
		QPen p = p1;
		p.setWidth(buttonSize() / 6);
		painter->setPen(p);
		int x1 = rect.width() / 8;
		int x2 = rect.width() - rect.width() / 8;
		int h = rect.height() / 3;
		for (int i = 0; i < 3; ++i) {
			int y = h * i + h/2;
			painter->drawLine(rect.x() + x1, rect.top() + y, rect.x() + x2, rect.top() + y);
		}
		break;
	}
	case ButtonId::Hide: {
		QPen p = p1;
		p.setWidth(buttonSize() / 6);
		painter->setPen(p);
		int offset = rect.height() / 4;
		QRect r = rect.adjusted(offset, offset, -offset, -offset);
		painter->drawRect(r);
		break;
	}
	}
	painter->setPen(p1);
	painter->drawRoundedRect(rect, rect.width() / 8, rect.height() / 8);
}

} // namespace timeline
} // namespace visu
} // namespace shv
