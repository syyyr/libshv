#include "localfsnode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/coreqt/log.h>

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

chainpack::RpcValue LocalFSNode::call2(const std::string &method, const chainpack::RpcValue &params, const std::string &shv_path)
{
	if(method == M_SIZE) {
		return ndSize(shv_path);
	}
	else if(method == M_READ) {
		return ndRead(shv_path);
	}
	return Super::call2(method, params, shv_path);
}

ShvNode::StringList LocalFSNode::childNames2(const std::string &shv_path)
{
	QString qpath = QString::fromStdString(shv_path);
	QFileInfo fi_path(m_rootDir.absolutePath() + '/' + qpath);
	//shvInfo() << __FUNCTION__ << fi_path.absoluteFilePath() << "is dir:" << fi_path.isDir();
	if(fi_path.isDir()) {
		QDir d2(fi_path.absoluteFilePath());
		if(!d2.exists())
			SHV_EXCEPTION("Path " + d2.absolutePath().toStdString() + " do not exists.");
		ShvNode::StringList lst;
		for(const QFileInfo &fi : d2.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot)) {
			lst.push_back(fi.fileName().toStdString());
		}
		return lst;
	}
	return ShvNode::StringList();
}

chainpack::RpcValue LocalFSNode::hasChildren2(const std::string &shv_path)
{
	QString qpath = QString::fromStdString(shv_path);
	QFileInfo fi(m_rootDir.absolutePath() + '/' + qpath);
	if(!fi.exists())
		shvError() << "Invalid path:" << fi.absoluteFilePath();
	shvDebug() << __FUNCTION__ << "shv path:" << shv_path << "file info:" << fi.absoluteFilePath() << "is dir:" << fi.isDir();
	return fi.isDir();
}

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{M_SIZE, cp::MetaMethod::Signature::RetVoid, false},
	{M_READ, cp::MetaMethod::Signature::RetVoid, false},
};

size_t LocalFSNode::methodCount2(const std::string &shv_path)
{
	QFileInfo fi = ndFileInfo(shv_path);
	return fi.isFile()? 4: 2;
}

const chainpack::MetaMethod *LocalFSNode::metaMethod2(size_t ix, const std::string &shv_path)
{
	Q_UNUSED(shv_path)
	if(meta_methods.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	return &(meta_methods[ix]);
}

QFileInfo LocalFSNode::ndFileInfo(const std::string &path)
{
	QFileInfo fi(m_rootDir.absolutePath() + '/' + QString::fromStdString(path));
	return fi;
}
/*
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
*/
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
/*
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
*/
} // namespace node
} // namespace iotqt
} // namespace shv
