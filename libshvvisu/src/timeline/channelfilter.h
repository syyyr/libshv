#pragma once

#include "../shvvisuglobal.h"

#include <QSet>
#include <QRegularExpression>

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT ChannelFilter
{
public:
	ChannelFilter();
	ChannelFilter(const QSet<QString> &matching_paths);

	void addMatchingPath(const QString &shv_path);
	void removeMatchingPath(const QString &shv_path);

	QSet<QString> matchingPaths() const;
	void setMatchingPaths(const QSet<QString> &paths);

	bool isPathMatch(const QString &path) const;
	bool isValid() const { return m_isValid; }
private:
	QSet<QString> m_matchingPaths;
	bool m_isValid;
};

}
}
}
