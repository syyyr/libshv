#include "logsortfilterproxymodel.h"

#include <shv/core/log.h>

namespace shv::visu::logview {

LogSortFilterProxyModel::LogSortFilterProxyModel(QObject *parent) :
	Super(parent)
{

}

void LogSortFilterProxyModel::setChannelFilter(const shv::visu::timeline::ChannelFilter &filter)
{
	m_channelFilter = filter;
	invalidateFilter();
}

void LogSortFilterProxyModel::setShvPathColumn(int column)
{
	m_shvPathColumn = column;
}

void LogSortFilterProxyModel::setValueColumn(int column)
{
	m_valueColumn = column;
}

void LogSortFilterProxyModel::setFulltextFilter(const timeline::FullTextFilter &filter)
{
	m_fulltextFilter = filter;
	invalidateFilter();
}

bool LogSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	bool row_accepted = false;
	if (m_shvPathColumn >= 0) {
		QModelIndex ix = sourceModel()->index(source_row, m_shvPathColumn, source_parent);
		row_accepted = !m_channelFilter.isValid();
		if (!row_accepted) {
			for (const QString &selected_path : m_channelFilter.matchingPaths()) {
				auto row_path = sourceModel()->data(ix).toString();
				if (selected_path == row_path || selected_path.startsWith(row_path)) {
					row_accepted = true;
					break;
				}
			}
		}
	}
	if (row_accepted && !m_fulltextFilter.pattern().isEmpty()) {
		bool fulltext_match = false;
		{
			QModelIndex ix = sourceModel()->index(source_row, m_shvPathColumn, source_parent);
			fulltext_match = fulltext_match || m_fulltextFilter.matches(sourceModel()->data(ix).toString());
		}
		if (m_valueColumn >= 0) {
			QModelIndex ix = sourceModel()->index(source_row, m_valueColumn, source_parent);
			fulltext_match = fulltext_match || m_fulltextFilter.matches(sourceModel()->data(ix).toString());
		}
		row_accepted = fulltext_match;
	}
	return row_accepted;
}

}
