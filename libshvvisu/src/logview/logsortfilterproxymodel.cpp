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

bool LogSortFilterProxyModel::filterAcceptsRow(int source_row, const QModelIndex &source_parent) const
{
	if (m_shvPathColumn >= 0) {
		QModelIndex ix = sourceModel()->index(source_row, m_shvPathColumn, source_parent);
		return m_channelFilter.isPathMatch(sourceModel()->data(ix).toString());
	}

	return false;
}

}}}
