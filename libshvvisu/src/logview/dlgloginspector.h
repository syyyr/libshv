#pragma once

#include "../shvvisuglobal.h"

#include <shv/core/utils.h>

#include <QDialog>
#include <QTimeZone>

namespace shv { namespace chainpack { class RpcValue; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace visu { namespace timeline { class GraphWidget; class GraphModel; class Graph; class ChannelFilterDialog;}}}

namespace shv {
namespace visu {
namespace logview {

namespace Ui {
class DlgLogInspector;
}

class LogModel;
class LogSortFilterProxyModel;

class SHVVISU_DECL_EXPORT DlgLogInspector : public QDialog
{
	Q_OBJECT
public:
	explicit DlgLogInspector(QWidget *parent = nullptr);
	~DlgLogInspector();

	shv::iotqt::rpc::ClientConnection* rpcConnection();
	void setRpcConnection(shv::iotqt::rpc::ClientConnection *c);

	QString shvPath() const;
	void setShvPath(const QString &s);

private:
	void downloadLog();
	void loadSettings();
	void saveSettings();

	shv::chainpack::RpcValue getLogParams();
	void parseLog(shv::chainpack::RpcValue log);

	void showInfo(const QString &msg = QString(), bool is_error = false);
	void saveData(const std::string &data, QString ext);

	void setTimeZone(const QTimeZone &tz);

	void applyFilters(const QStringList &channels_filter);
	void onChannelsFilterClicked();
	void onGraphChannelFilterChanged();
private:
	Ui::DlgLogInspector *ui;

	QTimeZone m_timeZone;

	shv::iotqt::rpc::ClientConnection* m_rpcConnection = nullptr;

	LogModel *m_logModel = nullptr;
	LogSortFilterProxyModel *m_logSortFilterProxy = nullptr;

	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;
	shv::visu::timeline::ChannelFilterDialog *m_channelFilterDialog = nullptr;
};

}}}
