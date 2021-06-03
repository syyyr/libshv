#include "channelprobe.h"
#include "graph.h"

namespace shv {
namespace visu {
namespace timeline {

ChannelProbe::ChannelProbe(Graph *graph, int channel_ix, timemsec_t time)
	: QObject(graph)
{
	m_channelIndex = channel_ix;
	m_currentTime = time;
}

int ChannelProbe::channelIndex()
{
	return m_channelIndex;
}

timemsec_t ChannelProbe::currentTime()
{
	return m_currentTime;
}

Graph *ChannelProbe::graph() const
{
	return qobject_cast<Graph*>(parent());
}

}
}
}
