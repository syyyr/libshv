#include "channelfilter.h"

#include <shv/core/log.h>

namespace shv {
namespace visu {
namespace timeline {

ChannelFilter::ChannelFilter(const QStringList &matching_paths)
{
	setMatchingPaths(matching_paths);
}

QStringList ChannelFilter::matchingPaths() const
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	return m_matchingPaths.toList();
#else
	return QStringList(m_matchingPaths.begin(), m_matchingPaths.end());
#endif
}

void ChannelFilter::addMatchingPath(const QString &shv_path)
{
	m_matchingPaths.insert(shv_path);
}

void ChannelFilter::removeMatchingPath(const QString &shv_path)
{
	m_matchingPaths.remove(shv_path);
}

void ChannelFilter::setMatchingPaths(const QStringList &paths)
{
#if QT_VERSION < QT_VERSION_CHECK(5, 14, 0)
	m_matchingPaths = QSet<QString>::fromList(paths);
#else
	m_matchingPaths = QSet<QString>(paths.begin(), paths.end());
#endif
}

bool ChannelFilter::isPathMatch(const QString &path) const
{
	return (m_matchingPaths.empty()) ? true : m_matchingPaths.contains(path);
}

}
}
}
