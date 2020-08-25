#include "graphchannel.h"
#include "graph.h"

#include <shv/coreqt/log.h>

namespace shv {
namespace visu {
namespace timeline {

//==========================================
// Graph::Channel
//==========================================

GraphChannel::GraphChannel(Graph *graph)
	: QObject(graph)
	, m_buttonBox(new GraphButtonBox({GraphButtonBox::ButtonId::Hide, GraphButtonBox::ButtonId::Menu}, this))
{
	static int n = 0;
	m_buttonBox->setObjectName(QString("channelButtonBox_%1").arg(++n));
	connect(m_buttonBox, &GraphButtonBox::buttonClicked, this, &GraphChannel::onButtonBoxClicked);
}

int GraphChannel::valueToPos(double val) const
{
	auto val2pos = Graph::valueToPosFn(yRangeZoom(), Graph::WidgetRange{m_layout.yAxisRect.bottom(), m_layout.yAxisRect.top()});
	return val2pos? val2pos(val): 0;
}

double GraphChannel::posToValue(int y) const
{
	auto pos2val = Graph::posToValueFn(Graph::WidgetRange{m_layout.yAxisRect.bottom(), m_layout.yAxisRect.top()}, yRangeZoom());
	return pos2val? pos2val(y): 0;
}

void GraphChannel::setVisible(bool b)
{
	m_effectiveStyle.setHidden(!b);
}

bool GraphChannel::isVisible() const
{
	return !m_effectiveStyle.isHidden();
}

Graph *GraphChannel::graph() const
{
	return qobject_cast<Graph*>(parent());
}

void GraphChannel::onButtonBoxClicked(int button_id)
{
	shvLogFuncFrame();
	if(button_id == (int)GraphButtonBox::ButtonId::Menu) {
		QPoint pos = buttonBox()->buttonRect((GraphButtonBox::ButtonId)button_id).center();
		graph()->emitChannelContextMenuRequest(graphChannelIndex(), pos);
	}
	else if(button_id == (int)GraphButtonBox::ButtonId::Hide) {
		graph()->setChannelVisible(graphChannelIndex(), false);
	}
}

int GraphChannel::graphChannelIndex() const
{
	auto *g = graph();
	for (int i = 0; i < g->channelCount(); ++i) {
		if(g->channelAt(i) == this) {
			return i;
		}
	}
	return -1;
}

} // namespace timeline
} // namespace visu
} // namespace shv
