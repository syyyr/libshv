#pragma once

#include "../shvvisuglobal.h"

#include <shv/core/utils.h>

#include <QDialog>

namespace shv { namespace chainpack { class RpcValue; }}
namespace shv { namespace iotqt { namespace rpc { class ClientConnection; }}}
namespace shv { namespace visu { namespace timeline { class GraphWidget; class GraphModel; class Graph;}}}

class QSortFilterProxyModel;

namespace shv {
namespace visu {
namespace logview {

namespace Ui {
class DlgLogInspector;
}

class LogModel;

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
private:
	Ui::DlgLogInspector *ui;

	shv::iotqt::rpc::ClientConnection* m_rpcConnection = nullptr;

	LogModel *m_logModel = nullptr;
	QSortFilterProxyModel *m_sortFilterProxy = nullptr;

	shv::visu::timeline::GraphModel *m_graphModel = nullptr;
	shv::visu::timeline::Graph *m_graph = nullptr;
	shv::visu::timeline::GraphWidget *m_graphWidget = nullptr;
};

}}}
