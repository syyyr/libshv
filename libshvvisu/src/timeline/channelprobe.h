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

	friend class Graph;
public:
	ChannelProbe(Graph *graph, int channel_ix, timemsec_t time);

	int channelIndex();
	timemsec_t currentTime();
	QString yValues();

protected:
	Graph *graph() const;
private:
	timemsec_t m_currentTime;
	int m_channelIndex;
};

}
}
}
