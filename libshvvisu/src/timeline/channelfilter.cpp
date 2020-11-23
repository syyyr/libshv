#include "channelfilter.h"

#include <shv/core/log.h>

namespace shv {
namespace visu {
namespace timeline {

ChannelFilter::ChannelFilter(const QString &pattern, ChannelFilter::PathPatternFormat fmt)
{
	setPathPattern(pattern, fmt);
}

ChannelFilter::ChannelFilter(const QStringList &filtered_paths)
{
	setFilteredPaths(filtered_paths);
}

QStringList ChannelFilter::filteredPaths() const
{
	return QStringList(m_filteredPaths.begin(), m_filteredPaths.end());
}

void ChannelFilter::setPathPattern(const QString &pattern, ChannelFilter::PathPatternFormat fmt)
{
	m_pathPattern = pattern;
	m_pathPatternFormat = fmt;

	if(m_pathPatternFormat == PathPatternFormat::Regex) {
		m_pathPatternRx.setPattern(pattern);
	}
}

void ChannelFilter::setFilteredPaths(const QStringList &paths)
{
	m_filteredPaths = QSet<QString>(paths.begin(), paths.end());
	m_pathPatternFormat = PathPatternFormat::PathList;
}

bool ChannelFilter::isPathMatch(const QString &path) const
{
	shvLogFuncFrame() << "path:" << path.toStdString() << "pattern:" << m_pathPattern.toStdString();

	if (m_pathPatternFormat == PathPatternFormat::Substring) {
		return (m_pathPattern.isEmpty()) ? true : (path.contains(m_pathPattern));
	}
	else if (m_pathPatternFormat == PathPatternFormat::Regex) {
		return (m_pathPatternRx.pattern().isEmpty()) ? true : m_pathPatternRx.match(path).hasMatch();
	}
	else if (m_pathPatternFormat == PathPatternFormat::PathList) {
		return (m_filteredPaths.empty()) ? true : m_filteredPaths.contains(path);
	}

	return false;
}

}
}
}
