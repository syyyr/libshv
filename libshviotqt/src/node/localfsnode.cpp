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
	cp::RpcValue::List ret;
	QDir d2(m_rootDir.absolutePath() + '/' + QString::fromStdString(path));
	if(!d2.exists())
		SHV_EXCEPTION("Path " + d2.absolutePath().toStdString() + " do not exists.");

	MethParamsList mpl(methods_params);
	for(const QFileInfo &fi : d2.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
		std::string fn = fi.fileName().toStdString();
		if(mpl.empty()) {
			ret.push_back(fn);
		}
		else {
			cp::RpcValue::List ret1;
			ret1.push_back(fn);
			for(const MethParams &mp : mpl) {
				try {
					std::string p2 = path + '/' + fn;
					ret1.push_back(ndCall(p2, mp.method(), mp.params()));
				} catch (shv::core::Exception &) {
					ret1.push_back(nullptr);
				}
			}
			ret.push_back(ret1);
		}
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

chainpack::RpcValue LocalFSNode::ndCall(const std::string &path, const std::string &method, const chainpack::RpcValue &params)
{
	cp::RpcValue ret;
	if(method == cp::Rpc::METH_LS) {
		ret = ndLs(path, params);
	}
	else if(method == M_SIZE) {
		ret = ndSize(path);
	}
	else if(method == M_READ) {
		ret = ndRead(path);
	}
	else if(method == cp::Rpc::METH_DIR) {
		QFileInfo fi = ndFileInfo(path);
		cp::RpcValue::List lst{cp::Rpc::METH_DIR};
		if(fi.isFile()) {
			lst.push_back(M_SIZE);
			lst.push_back(M_READ);
		}
		else if(fi.isDir()) {
			lst.push_back(cp::Rpc::METH_LS);
		}
		ret = lst;
	}
	else {
		SHV_EXCEPTION("Invalid method: " + method + " called for node: " + path);
	}
	return ret;
}

cp::RpcValue LocalFSNode::processRpcRequest(const chainpack::RpcRequest &rq)
{
	cp::RpcValue ret;
	cp::RpcValue::String method = rq.method().toString();
	chainpack::RpcValue shv_path = rq.shvPath();
	if(method == cp::Rpc::METH_LS) {
		ret = ndLs(shv_path.toString(), rq.params());
	}
	else if(method == M_SIZE) {
		ret = ndSize(shv_path.toString());
	}
	else if(method == M_READ) {
		ret = ndRead(shv_path.toString());
	}
	else if(method == cp::Rpc::METH_DIR) {
		QFileInfo fi = ndFileInfo(shv_path.toString());
		cp::RpcValue::List lst{cp::Rpc::METH_DIR, cp::Rpc::METH_LS};
		if(fi.isFile()) {
			lst.push_back(M_SIZE);
			lst.push_back(M_READ);
		}
		ret = lst;
	}
	else {
		SHV_EXCEPTION("Invalid method: " + method + " called for node: " + shv_path.toString());
	}
	return ret;
}

} // namespace node
} // namespace iotqt
} // namespace shv
