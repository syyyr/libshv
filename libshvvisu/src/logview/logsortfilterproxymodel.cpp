#include "logsortfilterproxymodel.h"

#include <shv/core/log.h>

namespace shv {
namespace visu {
namespace logview {

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
	bool res = false;
	if (m_shvPathColumn >= 0) {
		QModelIndex ix = sourceModel()->index(source_row, m_shvPathColumn, source_parent);
		res = m_channelFilter.isPathMatch(sourceModel()->data(ix).toString());
	}
	if (res) {
		if (m_valueColumn >= 0) {
			QModelIndex ix = sourceModel()->index(source_row, m_valueColumn, source_parent);
			res = m_fulltextFilter.matches(sourceModel()->data(ix).toString());
		}
		else {
			res = false;
		}
	}
	return res;
}

}}}
