#pragma once

#include "graphchannel.h"

#include <QSet>
#include <QRegularExpression>

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT ChannelFilter
{
public:
	enum class PathPatternFormat {Substring, Regex, PathList};

	ChannelFilter(const QString &pattern = QString(), PathPatternFormat fmt = PathPatternFormat::Substring);
	ChannelFilter(const QStringList &matching_paths);

	QString pathPattern() { return m_pathPattern; }
	void setPathPattern(const QString &pattern, PathPatternFormat fmt = PathPatternFormat::Substring);

	void addMatchingPath(const QString &shv_path);
	void removeMatchingPath(const QString &shv_path);

	QStringList matchingPaths() const;
	void setMatchingPaths(const QStringList &paths);

	bool isPathMatch(const QString &path) const;
private:
	QString m_pathPattern;
	PathPatternFormat m_pathPatternFormat = PathPatternFormat::Substring;
	bool m_hideFlat = false;

	QRegularExpression m_pathPatternRx;
	QSet<QString> m_matchingPaths;
};

}
}
}
