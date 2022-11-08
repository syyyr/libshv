#include "channelfilter.h"

#include <shv/core/log.h>

namespace shv::visu::timeline {

ChannelFilter::ChannelFilter()
	: m_isValid(false)
{
}

ChannelFilter::ChannelFilter(const QSet<QString> &matching_paths)
	: m_isValid(true)
{
	setMatchingPaths(matching_paths);
}

QSet<QString> ChannelFilter::matchingPaths() const
{
	return m_matchingPaths;
}

void ChannelFilter::addMatchingPath(const QString &shv_path)
{
	m_isValid = true;
	m_matchingPaths.insert(shv_path);
}

void ChannelFilter::removeMatchingPath(const QString &shv_path)
{
	m_isValid = true;
	m_matchingPaths.remove(shv_path);
}

void ChannelFilter::setMatchingPaths(const QSet<QString> &paths)
{
	m_isValid = true;
	m_matchingPaths = paths;
}

bool ChannelFilter::isPathMatch(const QString &path) const
{
	return m_matchingPaths.contains(path);
}

}
