#include "graphview.h"
#include "graphwidget.h"

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

}}}
