#pragma once

#include "../shvvisuglobal.h"
#include "../timeline/channelfilter.h"

#include <QSortFilterProxyModel>

namespace shv {
namespace visu {
namespace logview {

class SHVVISU_DECL_EXPORT LogSortFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	using Super = QSortFilterProxyModel;
public:
	explicit LogSortFilterProxyModel(QObject *parent = nullptr);

	void setChannelFilter(const shv::visu::timeline::ChannelFilter &filter);
	void setFilterShvPaths(const QStringList &shv_paths);
	void setShvPathColumn(int column);

	bool filterAcceptsRow(int source_rrow, const QModelIndex &source_parent) const override;

private:
	shv::visu::timeline::ChannelFilter m_channelFilter;
	int m_shvPathColumn = -1;
};

}}}
