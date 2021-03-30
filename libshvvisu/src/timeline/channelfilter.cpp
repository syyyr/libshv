#include "channelfilter.h"

#include <shv/core/log.h>

namespace shv {
namespace visu {
namespace timeline {

ChannelFilter::ChannelFilter(const QSet<QString> &matching_paths)
{
	setMatchingPaths(matching_paths);
}

QSet<QString> ChannelFilter::matchingPaths() const
{
	return m_matchingPaths;
}

void ChannelFilter::addMatchingPath(const QString &shv_path)
{
	m_matchingPaths.insert(shv_path);
}

void ChannelFilter::removeMatchingPath(const QString &shv_path)
{
	m_matchingPaths.remove(shv_path);
}

void ChannelFilter::setMatchingPaths(const QSet<QString> &paths)
{
	m_matchingPaths = paths;
}

bool ChannelFilter::isPathMatch(const QString &path) const
{
	return m_matchingPaths.contains(path);
}

}
}
}
