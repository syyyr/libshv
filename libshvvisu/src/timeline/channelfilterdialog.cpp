#include "channelfilterdialog.h"
#include "ui_channelfilterdialog.h"

#include "channelfiltermodel.h"
#include "channelfiltersortfilterproxymodel.h"

#include <shv/core/log.h>

#include <QLineEdit>
#include <QMenu>
#include <QSettings>

namespace shv {
namespace visu {
namespace timeline {

static QString USER_PROFILES_KEY = QStringLiteral("userProfiles");
static QString SITES_KEY = QStringLiteral("sites");
static QString CHANNEL_FILTERS_KEY = QStringLiteral("channelFilters");

ChannelFilterDialog::ChannelFilterDialog(QWidget *parent) :
	QDialog(parent),
	ui(new Ui::ChannelFilterDialog)
{
	ui->setupUi(this);

	m_channelsFilterModel = new ChannelFilterModel(this);
	connect(m_channelsFilterModel, &ChannelFilterModel::itemChanged, this, &ChannelFilterDialog::onItemChanged);

	m_channelsFilterProxyModel = new ChannelFilterSortFilterProxyModel(this);
	m_channelsFilterProxyModel->setSourceModel(m_channelsFilterModel);
	m_channelsFilterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
	m_channelsFilterProxyModel->setRecursiveFilteringEnabled(true);

	ui->tvChannelsFilter->setModel(m_channelsFilterProxyModel);
	ui->tvChannelsFilter->header()->hide();
	ui->tvChannelsFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
	connect(ui->tvChannelsFilter, &QTreeView::customContextMenuRequested, this, &ChannelFilterDialog::onCustomContextMenuRequested);

	connect(ui->pbDeleteFilter, &QPushButton::clicked, this, &ChannelFilterDialog::onDeleteFilterClicked);
	connect(ui->pbSaveFilter, &QPushButton::clicked, this, &ChannelFilterDialog::onSaveFilterClicked);
	connect(ui->cbFilters, QOverload<int>::of(&QComboBox::activated), this, &ChannelFilterDialog::onCbFiltersActivated);
	connect(ui->leMatchingFilterText, &QLineEdit::textChanged, this, &ChannelFilterDialog::onLeMatchingFilterTextChanged);
	connect(ui->pbClearMatchingText, &QPushButton::clicked, this, &ChannelFilterDialog::onPbClearMatchingTextClicked);
	//	connect(ui->chbFindRegex, &QCheckBox::stateChanged, this, &ChannelFilterDialog::onChbFindRegexChanged);
	connect(ui->pbCheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbCheckItemsClicked);
	connect(ui->pbUncheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbUncheckItemsClicked);
}

ChannelFilterDialog::~ChannelFilterDialog()
{
	delete ui;
}

void ChannelFilterDialog::load(const QString &site_path, const QStringList &logged_paths)
{
	setSitePath(site_path);
	load(logged_paths);
}

void ChannelFilterDialog::load(const QStringList &logged_paths)
{
	m_channelsFilterModel->createNodes(logged_paths);
	ui->cbFilters->addItems(savedFilterNames());
	ui->cbFilters->setCurrentIndex(-1);
	m_isSelectedFilterDirty = true;
}

QStringList ChannelFilterDialog::selectedChannels()
{
	return m_channelsFilterModel->selectedChannels();
}

void ChannelFilterDialog::setSelectedChannels(const QStringList &channels)
{
	bool is_selected_filter_dirty = m_isSelectedFilterDirty;
	m_channelsFilterModel->setSelectedChannels(channels);
	m_isSelectedFilterDirty = is_selected_filter_dirty;
}

QString ChannelFilterDialog::selectedFilterName() const
{
	if (m_isSelectedFilterDirty) {
		return QString();
	}
	return ui->cbFilters->currentText();
}

void ChannelFilterDialog::setSitePath(const QString &site_path)
{
	m_sitePath = site_path;
}

void ChannelFilterDialog::setSettingsUserName(const QString &user)
{
	m_settingsUserName = user;
}

void ChannelFilterDialog::applyTextFilter()
{
	m_channelsFilterProxyModel->setFilterString(ui->leMatchingFilterText->text());

	if (m_channelsFilterProxyModel->rowCount() == 1){
		ui->tvChannelsFilter->setCurrentIndex(m_channelsFilterProxyModel->index(0, 0));
		ui->tvChannelsFilter->expandRecursively(ui->tvChannelsFilter->currentIndex());
	}
}

void ChannelFilterDialog::saveChannelFilter(const QString &name)
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(m_sitePath);
	settings.beginGroup(CHANNEL_FILTERS_KEY);
	settings.setValue(name, m_channelsFilterModel->selectedChannels());
}

QStringList ChannelFilterDialog::loadChannelFilter(const QString &name)
{
	std::vector<std::string> channels;

	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(m_sitePath);
	settings.beginGroup(CHANNEL_FILTERS_KEY);
	return settings.value(name, m_channelsFilterModel->selectedChannels()).toStringList();
}

void ChannelFilterDialog::deleteChannelFilter(const QString &name)
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(m_sitePath);
	settings.beginGroup(CHANNEL_FILTERS_KEY);
	settings.remove(name);
}

QStringList ChannelFilterDialog::savedFilterNames()
{
	QStringList filters;
	QSettings settings;

	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup(m_settingsUserName);
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(m_sitePath);
	settings.beginGroup(CHANNEL_FILTERS_KEY);
	filters = settings.childKeys();
	return filters;
}

void ChannelFilterDialog::onCustomContextMenuRequested(QPoint pos)
{
	QModelIndex ix = ui->tvChannelsFilter->indexAt(pos);

	if (ix.isValid()) {
		QMenu menu(this);

		menu.addAction(tr("Expand"), [this, ix]() {
			ui->tvChannelsFilter->expandRecursively(ix);
		});
		menu.addAction(tr("Collapse"), [this, ix]() {
			ui->tvChannelsFilter->collapse(ix);
		});

		menu.addSeparator();

		menu.addAction(tr("Expand all nodes"), [this]() {
			ui->tvChannelsFilter->expandAll();
		});

		menu.exec(ui->tvChannelsFilter->mapToGlobal(pos));
	}
}

void ChannelFilterDialog::onDeleteFilterClicked()
{
	deleteChannelFilter(ui->cbFilters->currentText());
	ui->cbFilters->clear();
	ui->cbFilters->addItems(savedFilterNames());
	ui->cbFilters->setCurrentIndex(-1);
}

void ChannelFilterDialog::onCbFiltersActivated(int index)
{
	Q_UNUSED(index);
	QStringList channels = loadChannelFilter(ui->cbFilters->currentText());
	setSelectedChannels(channels);
	m_isSelectedFilterDirty = false;
}

void ChannelFilterDialog::onSaveFilterClicked()
{
	m_isSelectedFilterDirty = false;
	if (ui->cbFilters->currentText().isEmpty()){
		return;
	}

	QString current_filter_name = ui->cbFilters->currentText();
	saveChannelFilter(current_filter_name);

	ui->cbFilters->clear();
	ui->cbFilters->addItems(savedFilterNames());

	ui->cbFilters->setCurrentIndex(ui->cbFilters->findText(current_filter_name));
}

void ChannelFilterDialog::onPbCheckItemsClicked()
{
	setVisibleItemsCheckState(Qt::Checked);
}

void ChannelFilterDialog::onPbUncheckItemsClicked()
{
	setVisibleItemsCheckState(Qt::Unchecked);
}

void ChannelFilterDialog::onPbClearMatchingTextClicked()
{
	ui->leMatchingFilterText->setText(QString());
}

void ChannelFilterDialog::onLeMatchingFilterTextChanged(const QString &text)
{
	Q_UNUSED(text);
	applyTextFilter();
}

void ChannelFilterDialog::onChbFindRegexChanged(int state)
{
	Q_UNUSED(state);
	applyTextFilter();
}

void ChannelFilterDialog::onItemChanged()
{
	m_isSelectedFilterDirty = true;
}

void ChannelFilterDialog::setVisibleItemsCheckState(Qt::CheckState state)
{
	for (int row = 0; row < m_channelsFilterProxyModel->rowCount(); row++) {
		setVisibleItemsCheckState_helper(m_channelsFilterProxyModel->index(row, 0), state);
	}

	m_channelsFilterModel->fixCheckBoxesIntegrity();
}

void ChannelFilterDialog::setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state)
{
	if (!mi.isValid()) {
		return;
	}

	m_channelsFilterModel->setItemCheckState(m_channelsFilterProxyModel->mapToSource(mi), state);

	for (int row = 0; row < m_channelsFilterProxyModel->rowCount(mi); row++) {
		setVisibleItemsCheckState_helper(m_channelsFilterProxyModel->index(row, 0, mi), state);
	}
}

}
}
}
