#include "channelprobe.h"
#include "graph.h"
#include "graphmodel.h"

namespace shv {
namespace visu {
namespace timeline {

ChannelProbe::ChannelProbe(Graph *graph, int channel_ix, timemsec_t time)
	: QObject(graph)
{
	m_graph = graph;
	m_channelIndex = channel_ix;
	m_currentTime = time;
}

QColor ChannelProbe::color() const
{
	return m_graph->channelAt(m_channelIndex)->style().color();
}

void ChannelProbe::setCurrentTime(timemsec_t time)
{
	m_currentTime = time;
	emit currentTimeChanged();
}

timemsec_t ChannelProbe::currentTime() const
{
	return m_currentTime;
}

QString ChannelProbe::currentTimeIsoFormat() const
{
	QDateTime dt = QDateTime::fromMSecsSinceEpoch(m_currentTime);
	dt = dt.toTimeZone(m_graph->timeZone());
	return dt.toString(Qt::ISODateWithMs);
}

void ChannelProbe::nextSample()
{
	GraphModel *m = m_graph->model();
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	int model_ix = ch->modelIndex();
	int ix = m->lessOrEqualIndex(model_ix, m_currentTime);

	if(ix >= 0) {
		Sample s = m->sampleValue(model_ix, ix + 1);

		if (s.isValid()) {
			setCurrentTime(s.time);
		}
	}
}

void ChannelProbe::prevSample()
{
	GraphModel *m = m_graph->model();
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	int model_ix = ch->modelIndex();
	int ix = m->lessOrEqualIndex(model_ix, m_currentTime);

	if(ix >= 0) {
		Sample s = m->sampleValue(model_ix, ix - 1);

		if (s.isValid()) {
			setCurrentTime(s.time);
		}
	}
}

QMap<QString, QString> ChannelProbe::yValues() const
{
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	GraphModel::ChannelInfo &channel_info = m_graph->model()->channelInfo(ch->modelIndex());
	Sample s;

	if (channel_info.typeDescr.sampleType == shv::chainpack::DataChange::SampleType::Discrete) {
		s = m_graph->nearestSample(m_channelIndex, m_currentTime);
	}
	else {
		s = m_graph->timeToSample(m_channelIndex, m_currentTime);
	}
	return m_graph->yValuesToMap(m_channelIndex, s);
}

QString ChannelProbe::shvPath() const
{
	const GraphChannel *ch = m_graph->channelAt(m_channelIndex);
	return m_graph->model()->channelInfo(ch->modelIndex()).shvPath;
}

int ChannelProbe::channelIndex() const
{
	return m_channelIndex;
}

}
}
}
