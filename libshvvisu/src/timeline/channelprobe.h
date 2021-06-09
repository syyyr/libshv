#pragma once

#include "../shvvisuglobal.h"

#include <QObject>

namespace shv {
namespace visu {
namespace timeline {

class Graph;

using timemsec_t = int64_t;

class SHVVISU_DECL_EXPORT ChannelProbe: public QObject
{
	Q_OBJECT

public:
	ChannelProbe(Graph *graph, int channel_ix, timemsec_t time);

	QColor color();
	QString shvPath();

	void setCurrentTime(timemsec_t time);
	timemsec_t currentTime();
	QString currentTimeIsoFormat();

	void nextSample();
	void prevSample();
	QMap<QString, QString> yValues();

	Q_SIGNAL void currentTimeChanged();

protected:
	const Graph *m_graph = nullptr;
private:
	timemsec_t m_currentTime;
	int m_channelIndex;
};

}
}
}
