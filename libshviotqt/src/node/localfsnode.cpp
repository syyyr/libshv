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
static const char M_WRITE[] = "write";
static const char M_DELETE[] = "delete";
static const char M_MKFILE[] = "mkfile";
static const char M_MKDIR[] = "mkdir";
static const char M_RMDIR[] = "rmdir";

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
	else if(method == M_WRITE) {
		return ndWrite(QString::fromStdString(core::StringView::join(shv_path, '/')), params);
	}
	else if(method == M_DELETE) {
		return ndDelete(QString::fromStdString(core::StringView::join(shv_path, '/')));
	}
	else if(method == M_MKFILE) {
		return ndMkfile(QString::fromStdString(core::StringView::join(shv_path, '/')), params);
	}
	else if(method == M_MKDIR) {
		return ndMkdir(QString::fromStdString(core::StringView::join(shv_path, '/')), params);
	}
	else if(method == M_RMDIR) {
		bool recursively = (params.isBool()) ? params.toBool() : false;
		return ndRmdir(QString::fromStdString(core::StringView::join(shv_path, '/')), recursively);
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

chainpack::RpcValue LocalFSNode::hasChildren(const ShvNode::StringViewList &shv_path)
{
	QString qpath = QString::fromStdString(core::StringView::join(shv_path, '/'));
	QFileInfo fi(m_rootDir.absolutePath() + '/' + qpath);
	if(!fi.exists())
		shvError() << "Invalid path:" << fi.absoluteFilePath();
	shvDebug() << __FUNCTION__ << "shv path:" << qpath << "file info:" << fi.absoluteFilePath() << "is dir:" << fi.isDir();
	return fi.isDir();
}

static std::vector<cp::MetaMethod> meta_methods_dir {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{M_MKFILE, cp::MetaMethod::Signature::RetParam, false},
	{M_MKDIR, cp::MetaMethod::Signature::RetParam, false},
	{M_RMDIR, cp::MetaMethod::Signature::RetParam, false}
};

static std::vector<cp::MetaMethod> meta_methods_file {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, false},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, false},
	{M_SIZE, cp::MetaMethod::Signature::RetVoid, false},
	{M_READ, cp::MetaMethod::Signature::RetVoid, false},
	{M_WRITE, cp::MetaMethod::Signature::RetParam, false},
	{M_DELETE, cp::MetaMethod::Signature::RetParam, false}
};

size_t LocalFSNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	return hasChildren(shv_path).toBool() ? meta_methods_dir.size() : meta_methods_file.size();
}

const chainpack::MetaMethod *LocalFSNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	Q_UNUSED(shv_path)

	bool has_children = hasChildren(shv_path).toBool();
	size_t meta_methods_size = (has_children) ? meta_methods_dir.size() : meta_methods_file.size();

	if(meta_methods_size <= ix)
		SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_size));

	return (has_children) ? &(meta_methods_dir[ix]) : &(meta_methods_file[ix]);
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
		f.close();
		return cp::RpcValue::String(ba.constData(), (size_t)ba.size());
	}
	SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for reading.");
}

chainpack::RpcValue LocalFSNode::ndWrite(const QString &path, const chainpack::RpcValue &methods_params)
{
	QFile f(m_rootDir.absolutePath() + '/' + path);
	if(f.open(QFile::WriteOnly)) {
		const chainpack::RpcValue::String &content = methods_params.toString();
		f.write(content.data(), content.size());
		f.close();
		return true;
	}
	SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");

	return false;
}

chainpack::RpcValue LocalFSNode::ndDelete(const QString &path)
{
	QFile file (m_rootDir.absolutePath() + '/' + path);
	shvDebug() << "delete file" << m_rootDir.absolutePath() + '/' + path;
	return file.remove();
}

chainpack::RpcValue LocalFSNode::ndMkfile(const QString &path, const chainpack::RpcValue &methods_params)
{
	QString dir_path = m_rootDir.absolutePath() + '/' + path;
	if (!methods_params.isString()){
		SHV_EXCEPTION("Cannot create file in directory " + dir_path.toStdString() + ". Invalid parameter: " + methods_params.toCpon());
	}

	QString file_path = dir_path + "/" + QString::fromStdString(methods_params.toString());

	QFile f(file_path);
	if(f.open(QFile::WriteOnly)) {
		f.close();
		return true;
	}

	SHV_EXCEPTION("Cannot create file " + file_path.toStdString() + ".");

	return false;
}

chainpack::RpcValue LocalFSNode::ndMkdir(const QString &path, const chainpack::RpcValue &methods_params)
{
	QDir d(m_rootDir.absolutePath()+ '/' + path);

	if (!methods_params.isString()){
		SHV_EXCEPTION("Cannot create directory in directory " + d.absolutePath().toStdString() + ". Invalid parameter: " + methods_params.toString());
	}

	shvDebug() << m_rootDir.absolutePath()  << " : " << path << " : " << methods_params.toString();
	return d.mkpath(m_rootDir.absolutePath()+ '/' + path + '/' + QString::fromStdString(methods_params.toString()));
}

chainpack::RpcValue LocalFSNode::ndRmdir(const QString &path, bool recursively)
{
	if (path.isEmpty()){
		return false;
	}

	QDir dir(m_rootDir.absolutePath() + '/' + path);
	shvDebug() << "delete dir" << m_rootDir.absolutePath() + '/' + path << "path" << path << recursively;

	/*
	if (recursively){
		return dir.removeRecursively();
	}
	else{
		return dir.remove(m_rootDir.absolutePath() + '/' + path);
	}*/
}

} // namespace node
} // namespace iotqt
} // namespace shv
