#pragma once

#include <shv/core/utils.h>

#include <QObject>
#include <QRect>

class QPainter;

namespace shv {
namespace visu {
namespace timeline {

class Graph;

/*
class GraphButton : public QObject
{
	Q_OBJECT
public:
	GraphButton(QObject *parent = nullptr);
};
*/
class GraphButtonBox : public QObject
{
	Q_OBJECT

	SHV_FIELD_BOOL_IMPL2(a, A, utoRaise, true)
public:
	enum class ButtonId { Invalid = 0, Menu, Hide, User };
public:
	GraphButtonBox(const QVector<ButtonId> &button_ids, QObject *parent);
	virtual ~GraphButtonBox() {}

	Q_SIGNAL void buttonClicked(int button_id);

	void moveTopRight(const QPoint &p);
	void hide();

	bool processEvent(QEvent *ev);

	virtual void draw(QPainter *painter);

	//QRect rect() const { return m_rect; }
	//void setRect(const QRect &r) { m_rect = r; }
	QSize size() const;
	int buttonSize() const;
	int buttonSpace() const;
	QRect buttonRect(ButtonId id) const;
protected:
	int buttonCount() const { return m_buttonIds.count(); }
	QRect buttonRect(int ix) const;

	void drawButton(QPainter *painter, const QRect &rect, int button_index);

	Graph *graph() const;
private:
	//QVector<GraphButton*> m_buttons;
	//Graph *m_graph;
	QVector<ButtonId> m_buttonIds;
	QRect m_rect;
	bool m_mouseOver = false;
	int m_mouseOverButtonIndex = -1;
	int m_mousePressButtonIndex = -1;
};

} // namespace timeline
} // namespace visu
} // namespace shv
