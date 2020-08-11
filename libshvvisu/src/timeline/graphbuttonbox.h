#ifndef SHV_VISU_TIMELINE_GRAPHBUTTONBOX_H
#define SHV_VISU_TIMELINE_GRAPHBUTTONBOX_H

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
class GraphButtonBox //: public QObject
{
	//Q_OBJECT
public:
	enum class ButtonId { Properties, Hide };
public:
	GraphButtonBox(const QVector<ButtonId> &button_ids, Graph *graph);
	virtual ~GraphButtonBox() {}

	void moveTopRight(const QPoint &p);

	virtual void event(QEvent *ev);

	virtual void draw(QPainter *painter);

	//QRect rect() const { return m_rect; }
	//void setRect(const QRect &r) { m_rect = r; }
	QSize size() const;
	int buttonSize() const;
	int buttonSpace() const;
protected:
	int buttonCount() const { return m_buttonIds.count(); }
	QRect buttonRect(int ix) const;

	void drawButton(QPainter *painter, const QRect &rect, ButtonId btid);

private:
	//QVector<GraphButton*> m_buttons;
	Graph *m_graph;
	QVector<ButtonId> m_buttonIds;
	QRect m_rect;
	bool m_mouseOver = false;
};

} // namespace timeline
} // namespace visu
} // namespace shv

#endif // SHV_VISU_TIMELINE_GRAPHBUTTONBOX_H
