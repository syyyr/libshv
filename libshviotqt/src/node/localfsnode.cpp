#include "localfsnode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/core/exception.h>

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace node {

static const char M_SIZE[] = "size";
static const char M_READ[] = "read";

LocalFSNode::LocalFSNode(const QString &root_path, Super *parent)
	: Super(parent)
	, m_rootDir(root_path)
{
}

QFileInfo LocalFSNode::ndFileInfo(const std::string &path)
{
	QFileInfo fi(m_rootDir.absolutePath() + '/' + QString::fromStdString(path));
	return fi;
}

cp::RpcValue LocalFSNode::ndLs(const std::string &path, const chainpack::RpcValue &methods_params)
{
	Q_UNUSED(methods_params)
	cp::RpcValue::List ret;
	QDir d2(m_rootDir.absolutePath() + '/' + QString::fromStdString(path));
	if(!d2.exists())
		return ret;
	for(const QFileInfo &fi : d2.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
		ret.push_back(fi.fileName().toStdString());
	}
	return ret;
}

cp::RpcValue LocalFSNode::ndSize(const std::string &path)
{
	return (unsigned)ndFileInfo(path).size();
}

chainpack::RpcValue LocalFSNode::ndRead(const std::string &path)
{
	QFile f(m_rootDir.absolutePath() + '/' + QString::fromStdString(path));
	if(f.open(QFile::ReadOnly)) {
		QByteArray ba = f.readAll();
		return cp::RpcValue::Blob(ba.constData(), ba.size());
	}
	SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for reading.");
}

cp::RpcValue LocalFSNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	cp::RpcValue ret;
	cp::RpcValue::String method = rq.method();
	if(method == cp::Rpc::METH_LS) {
		ret = ndLs(rq.shvPath(), rq.params());
	}
	else if(method == M_SIZE) {
		ret = ndSize(rq.shvPath());
	}
	else if(method == M_READ) {
		ret = ndRead(rq.shvPath());
	}
	else if(method == cp::Rpc::METH_DIR) {
		QFileInfo fi = ndFileInfo(rq.shvPath());
		cp::RpcValue::List lst{cp::Rpc::METH_DIR, cp::Rpc::METH_LS};
		if(fi.isFile()) {
			lst.push_back(M_SIZE);
			lst.push_back(M_READ);
		}
		ret = lst;
	}
	else {
		SHV_EXCEPTION("Invalid method: " + method + " called for node: " + rq.shvPath());
	}
	return ret;
}

} // namespace node
} // namespace iotqt
} // namespace shv
