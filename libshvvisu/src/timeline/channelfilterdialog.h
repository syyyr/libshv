#pragma once

#include "../shvvisuglobal.h"

#include "channelfiltermodel.h"

#include <QDialog>
#include <QSortFilterProxyModel>

namespace shv {
namespace visu {
namespace timeline {

namespace Ui {
class ChannelFilterDialog;
}

class SHVVISU_DECL_EXPORT ChannelFilterDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChannelFilterDialog(QWidget *parent = nullptr);
	~ChannelFilterDialog();

	void load(const std::string site_path, const shv::chainpack::RpcValue &logged_paths);

	QStringList selectedChannels();
	void setSelectedChannels(const QStringList &channels);

private:
	void applyTextFilter();
	void saveChannelFilter(const QString &name);
	QStringList loadChannelFilter(const QString &name);
	void deleteChannelFilter(const QString &name);

	QStringList savedFilterNames();

	void onCustomContextMenuRequested(QPoint pos);
	void onDeleteFilterClicked();
	void onCbFiltersActivated(int index);
	void onSaveFilterClicked();

	void onPbCheckItemsClicked();
	void onPbUncheckItemsClicked();

	void onLeMatchingFilterTextEdited(const QString &text);
	void onChbFindRegexChanged(int state);
	void setVisibleItemsCheckState(Qt::CheckState state);

	Ui::ChannelFilterDialog *ui;
	ChannelFilterModel *m_channelsFilterModel = nullptr;
	QSortFilterProxyModel *m_channelsFilterProxyModel = nullptr;
	std::string m_sitePath;
};

}
}
}
