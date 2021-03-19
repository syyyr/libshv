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
	static const QString DEFAULT_USER_PROFILE;

	explicit ChannelFilterDialog(QWidget *parent = nullptr);
	~ChannelFilterDialog();

	void load(const QString &site_path, const QStringList &logged_paths);

	QStringList selectedChannels();
	void setSelectedChannels(const QStringList &channels);

	void selectFilter(const QString &name) const;
	QString selectedFilterName() const;

	void setSettingsUserName(const QString &user);

	static QStringList savedFilterNames(const QString &site_path, const QString &user_name = DEFAULT_USER_PROFILE);
	static QStringList loadChannelFilter(const QString &site_path, const QString &name, const QString &user_name = DEFAULT_USER_PROFILE);

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
	QString m_settingsUserName = DEFAULT_USER_PROFILE;
	bool m_isSelectedFilterDirty = true;
};

}
}
}
