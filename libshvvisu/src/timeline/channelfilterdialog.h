#pragma once

#include "../shvvisuglobal.h"

#include <QDialog>

namespace shv {
namespace visu {
namespace timeline {

namespace Ui {
class ChannelFilterDialog;
}

class ChannelFilterModel;
class ChannelFilterSortFilterProxyModel;

class SHVVISU_DECL_EXPORT ChannelFilterDialog : public QDialog
{
	Q_OBJECT

public:
	explicit ChannelFilterDialog(QWidget *parent = nullptr);
	~ChannelFilterDialog();

	void load(const QString &site_path, const QStringList &logged_paths);
	void load(const QStringList &logged_paths);

	QStringList selectedChannels();
	void setSelectedChannels(const QStringList &channels);
	QString selectedFilter() const;

	void setSitePath(const QString &site_path);
	void setSettingsUserName(const QString &user);

	QStringList savedFilterNames();
	QStringList loadChannelFilter(const QString &name);

private:
	void saveChannelFilter(const QString &name);
	void deleteChannelFilter(const QString &name);


	void applyTextFilter();

	void setVisibleItemsCheckState(Qt::CheckState state);
	void setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state);

	void onCustomContextMenuRequested(QPoint pos);
	void onDeleteFilterClicked();
	void onCbFiltersActivated(int index);
	void onSaveFilterClicked();

	void onPbCheckItemsClicked();
	void onPbUncheckItemsClicked();
	void onPbClearMatchingTextClicked();
	void onLeMatchingFilterTextChanged(const QString &text);
	void onChbFindRegexChanged(int state);
	void onItemChanged();

	Ui::ChannelFilterDialog *ui;
	ChannelFilterModel *m_channelsFilterModel = nullptr;
	ChannelFilterSortFilterProxyModel *m_channelsFilterProxyModel = nullptr;
	QString m_sitePath;
	QString m_settingsUserName ="default";
	bool m_isSavedFilterDirty = true;
};

}
}
}
