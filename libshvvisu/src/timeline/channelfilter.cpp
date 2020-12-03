#include "channelfilter.h"

#include <shv/core/log.h>

namespace shv {
namespace visu {
namespace timeline {

ChannelFilter::ChannelFilter(const QString &pattern, ChannelFilter::PathPatternFormat fmt)
{
	setPathPattern(pattern, fmt);
}

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

void ChannelFilter::setPathPattern(const QString &pattern, ChannelFilter::PathPatternFormat fmt)
{
	m_pathPattern = pattern;
	m_pathPatternFormat = fmt;

	if(m_pathPatternFormat == PathPatternFormat::Regex) {
		m_pathPatternRx.setPattern(pattern);
	}
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
		return (m_matchingPaths.empty()) ? true : m_matchingPaths.contains(path);
	}

	return false;
}

}
}
}
