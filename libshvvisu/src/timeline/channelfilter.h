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
	ChannelFilter(const QSet<QString> &matching_paths = QSet<QString>());

	void addMatchingPath(const QString &shv_path);
	void removeMatchingPath(const QString &shv_path);

	QSet<QString> matchingPaths() const;
	void setMatchingPaths(const QSet<QString> &paths);

	bool isPathMatch(const QString &path) const;
private:
	QSet<QString> m_matchingPaths;
};

}
}
}
