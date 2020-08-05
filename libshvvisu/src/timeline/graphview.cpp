#include "graphview.h"
#include "graphwidget.h"

#include <shv/coreqt/log.h>

namespace shv {
namespace visu {
namespace timeline {

GraphView::GraphView(QWidget *parent)
	: Super(parent)
{
}

void GraphView::makeLayout()
{
	if(GraphWidget *w = qobject_cast<GraphWidget*>(widget())) {
		w->makeLayout(geometry().size() - QSize(30, 0)); // space for scroll bar
	}
}

void GraphView::resizeEvent(QResizeEvent *event)
{
	Super::resizeEvent(event);
	makeLayout();
}

void GraphView::scrollContentsBy(int dx, int dy)
{
	Super::scrollContentsBy(dx, dy);
	if(dy != 0) {
		// update whole widget to update floating minimap
		// scroll area updates scrolled-in region only
		// previous minimap is scrolled up and remains on screen
		widget()->update();
	}
}

}}}
