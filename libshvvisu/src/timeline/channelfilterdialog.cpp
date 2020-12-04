#include "channelfilterdialog.h"
#include "ui_channelfilterdialog.h"

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

	m_channelsFilterProxyModel = new QSortFilterProxyModel(this);
	m_channelsFilterProxyModel->setSourceModel(m_channelsFilterModel);
	m_channelsFilterProxyModel->setFilterCaseSensitivity(Qt::CaseSensitivity::CaseInsensitive);
	m_channelsFilterProxyModel->setRecursiveFilteringEnabled(true);

	ui->tvChannelsFilter->setModel(m_channelsFilterProxyModel);
	ui->tvChannelsFilter->header()->hide();
	ui->tvChannelsFilter->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
	connect(ui->tvChannelsFilter, &QTreeView::customContextMenuRequested, this, &ChannelFilterDialog::onCustomContextMenuRequested);

	connect(ui->pbDeleteFilter, &QPushButton::clicked, this, &ChannelFilterDialog::onDeleteFilterClicked);
	connect(ui->pbSaveFilter, &QPushButton::clicked, this, &ChannelFilterDialog::onSaveFilterClicked);
	connect(ui->cbFilters, qOverload<int>(&QComboBox::activated), this, &ChannelFilterDialog::onCbFiltersActivated);
	connect(ui->pbCheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbCheckItemsClicked);
	connect(ui->pbUncheckItems, &QPushButton::clicked, this, &ChannelFilterDialog::onPbUncheckItemsClicked);
}

ChannelFilterDialog::~ChannelFilterDialog()
{
	delete ui;
}

void ChannelFilterDialog::load(const QString &site_path, const QStringList &logged_paths)
{
	m_sitePath = site_path;
	m_channelsFilterModel->createNodes(logged_paths);
	ui->cbFilters->addItems(savedFilterNames());
	ui->cbFilters->setCurrentIndex(-1);
}

QStringList ChannelFilterDialog::selectedChannels()
{
	return m_channelsFilterModel->selectedChannels();
}

void ChannelFilterDialog::setSelectedChannels(const QStringList &channels)
{
	m_channelsFilterModel->setSelectedChannels(channels);
}

void ChannelFilterDialog::saveChannelFilter(const QString &name)
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup("user");
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
	settings.beginGroup("user");
	settings.beginGroup(SITES_KEY);
	settings.beginGroup(m_sitePath);
	settings.beginGroup(CHANNEL_FILTERS_KEY);
	return settings.value(name, m_channelsFilterModel->selectedChannels()).toStringList();
}

void ChannelFilterDialog::deleteChannelFilter(const QString &name)
{
	QSettings settings;
	settings.beginGroup(USER_PROFILES_KEY);
	settings.beginGroup("user");
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
	settings.beginGroup("user");
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
		menu.addAction(tr("Collapse"), [this, ix]() {
			ui->tvChannelsFilter->collapse(ix);
		});
		menu.addAction(tr("Expand"), [this, ix]() {
			ui->tvChannelsFilter->expandRecursively(ix);
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
}

void ChannelFilterDialog::onSaveFilterClicked()
{
	if (ui->cbFilters->currentText().isEmpty()){
		return;
	}

	QString current_layout = ui->cbFilters->currentText();
	saveChannelFilter(ui->cbFilters->currentText());

	ui->cbFilters->clear();
	ui->cbFilters->addItems(savedFilterNames());

	for (int i = 0; i < ui->cbFilters->count(); i++) {
		if (ui->cbFilters->itemText(i) == current_layout) {
			ui->cbFilters->setCurrentIndex(i);
		}
	}
}

void ChannelFilterDialog::onPbCheckItemsClicked()
{
	setVisibleItemsCheckState(Qt::Checked);
}

void ChannelFilterDialog::onPbUncheckItemsClicked()
{
	setVisibleItemsCheckState(Qt::Unchecked);
}

void ChannelFilterDialog::setVisibleItemsCheckState(Qt::CheckState state)
{
	for (int row = 0; row < m_channelsFilterProxyModel->rowCount(); row++) {
		shvInfo() << "row" << row;
		//setVisibleItemsCheckState_helper(m_channelsFilterProxyModel->index(row, 0), state);
	}
}

void ChannelFilterDialog::setVisibleItemsCheckState_helper(const QModelIndex &mi, Qt::CheckState state)
{
	if (!mi.isValid()) {
		return;
	}

	m_channelsFilterProxyModel->hasChildren(mi);

/*	if (m_channelsFilterProxyModel->children().isEmpty()) {
		QModelIndex ix = m_channelsFilterProxyModel->mapToSource(mi);
		m_channelsFilterModel->setItemCheckState(ix, state);
	}
	else {
		for (auto ch: m_channelsFilterProxyModel->children()) {
			setVisibleItemsCheckState_helper(ch, state);
		}
	}
*/}

}
}
}
