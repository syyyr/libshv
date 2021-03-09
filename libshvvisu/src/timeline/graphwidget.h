#pragma once

#include "graph.h"
#include "graphmodel.h"

#include <shv/coreqt/utils.h>

#include <QWidget>

namespace shv {
namespace visu {
namespace timeline {

class Graph;

class SHVVISU_DECL_EXPORT GraphWidget : public QWidget
{
	Q_OBJECT

	using Super = QWidget;
	//friend class GraphWidget;
public:
	GraphWidget(QWidget *parent = nullptr);

	void setGraph(Graph *g);
	Graph *graph();
	const Graph *graph() const;

	void setTimeZone(const QTimeZone &tz);

	void makeLayout(const QSize &preferred_size);
	void makeLayout();

	Q_SIGNAL void graphChannelDoubleClicked(const QPoint &mouse_pos);
protected:
	bool event(QEvent *event) override;
	void paintEvent(QPaintEvent *event) override;
	//void keyPressEvent(QKeyEvent *event) override;
	//void keyReleaseEvent(QKeyEvent *event) override;

	void mouseDoubleClickEvent(QMouseEvent *event) override;
	void mousePressEvent(QMouseEvent *event) override;
	void mouseReleaseEvent(QMouseEvent *event) override;
	void mouseMoveEvent(QMouseEvent *event) override;
	void leaveEvent(QEvent *event) override;
	void wheelEvent(QWheelEvent *event) override;
	void contextMenuEvent(QContextMenuEvent *event) override;

	void hideCrossHair();

	virtual void showGraphContextMenu(const QPoint &mouse_pos);
	virtual void showChannelContextMenu(int channel_ix, const QPoint &mouse_pos);
protected:
	bool isMouseAboveMiniMap(const QPoint &mouse_pos) const;
	bool isMouseAboveMiniMapHandle(const QPoint &mouse_pos, bool left) const;
	bool isMouseAboveLeftMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveRightMiniMapHandle(const QPoint &pos) const;
	bool isMouseAboveMiniMapSlider(const QPoint &pos) const;
	int channelIndexOnGraphVerticalHeader(const QPoint &pos) const;
	int channelIndexOnGraphDataAreaIndex(const QPoint &pos) const;
	QString interpretEnum(int value, const ShvLogTypeDescr &type_descr);

protected:
	Graph *m_graph = nullptr;
	QSize m_graphPreferredSize;

	enum class MouseOperation { None = 0, MiniMapLeftResize, MiniMapRightResize, MiniMapScrollZoom, GraphDataAreaPress, GraphAreaMove, GraphAreaSelection };
	MouseOperation m_mouseOperation = MouseOperation::None;
	QPoint m_recentMousePos;
};

}}}
