#include "graphchannel.h"
#include "graph.h"

namespace shv {
namespace visu {
namespace timeline {

//==========================================
// Graph::Channel
//==========================================

GraphChannel::GraphChannel(Graph *graph)
	: QObject(graph)
	, m_buttonBox(new GraphButtonBox({GraphButtonBox::ButtonId::Hide, GraphButtonBox::ButtonId::Properties}, this))
{
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

Graph *GraphChannel::graph() const
{
	return qobject_cast<Graph*>(parent());
}

void GraphChannel::onButtonBoxClicked(int button_id)
{
	if(button_id == (int)GraphButtonBox::ButtonId::Properties) {

	}
	else if(button_id == (int)GraphButtonBox::ButtonId::Hide) {
		auto *g = graph();
		for (int i = 0; i < g->channelCount(); ++i) {
			if(g->channelAt(i) == this) {
				g->setChannelVisible(i, false);
				return;
			}
		}
	}
}

} // namespace timeline
} // namespace visu
} // namespace shv
