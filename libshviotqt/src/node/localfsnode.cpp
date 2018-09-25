#include "localfsnode.h"

#include <shv/chainpack/rpcmessage.h>
#include <shv/chainpack/metamethod.h>
#include <shv/core/exception.h>
#include <shv/core/stringview.h>
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

chainpack::RpcValue LocalFSNode::callMethod(const ShvNode::StringViewList &shv_path, const std::string &method, const chainpack::RpcValue &params)
{
	if(method == M_SIZE) {
		return ndSize(QString::fromStdString(core::StringView::join(shv_path, '/')));
	}
	else if(method == M_READ) {
		return ndRead(QString::fromStdString(core::StringView::join(shv_path, '/')));
	}
	return Super::callMethod(shv_path, method, params);
}

ShvNode::StringList LocalFSNode::childNames(const ShvNode::StringViewList &shv_path)
{
	QString qpath = QString::fromStdString(core::StringView::join(shv_path, '/'));
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

bool LocalFSNode::hasChildren(const ShvNode::StringViewList &shv_path)
{
	QString qpath = QString::fromStdString(core::StringView::join(shv_path, '/'));
	QFileInfo fi(m_rootDir.absolutePath() + '/' + qpath);
	if(!fi.exists())
		shvError() << "Invalid path:" << fi.absoluteFilePath();
	shvDebug() << __FUNCTION__ << "shv path:" << qpath << "file info:" << fi.absoluteFilePath() << "is dir:" << fi.isDir();
	return fi.isDir();
}

static std::vector<cp::MetaMethod> meta_methods {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{M_SIZE, cp::MetaMethod::Signature::RetVoid, false},
	{M_READ, cp::MetaMethod::Signature::RetVoid, false},
};

size_t LocalFSNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(core::StringView::join(shv_path, '/')));
	return fi.isFile()? 4: 2;
}

const chainpack::MetaMethod *LocalFSNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	Q_UNUSED(shv_path)
	if(meta_methods.size() <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods.size()));
	return &(meta_methods[ix]);
}

QFileInfo LocalFSNode::ndFileInfo(const QString &path)
{
	QFileInfo fi(m_rootDir.absolutePath() + '/' + path);
	return fi;
}

cp::RpcValue LocalFSNode::ndSize(const QString &path)
{
	return (unsigned)ndFileInfo(path).size();
}

chainpack::RpcValue LocalFSNode::ndRead(const QString &path)
{
	QFile f(m_rootDir.absolutePath() + '/' + path);
	if(f.open(QFile::ReadOnly)) {
		QByteArray ba = f.readAll();
		return cp::RpcValue::String(ba.constData(), (size_t)ba.size());
	}
	SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for reading.");
}

} // namespace node
} // namespace iotqt
} // namespace shv
