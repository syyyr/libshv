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
		return ndSize(QString::fromStdString(shv_path.join('/')));
	}
	else if(method == M_READ) {
		return ndRead(QString::fromStdString(shv_path.join('/')));
	}
	else if(method == M_WRITE) {
		return ndWrite(QString::fromStdString(shv_path.join('/')), params);
	}
	else if(method == M_DELETE) {
		return ndDelete(QString::fromStdString(shv_path.join('/')));
	}
	else if(method == M_MKFILE) {
		return ndMkfile(QString::fromStdString(shv_path.join('/')), params);
	}
	else if(method == M_MKDIR) {
		return ndMkdir(QString::fromStdString(shv_path.join('/')), params);
	}
	else if(method == M_RMDIR) {
		bool recursively = (params.isBool()) ? params.toBool() : false;
		return ndRmdir(QString::fromStdString(shv_path.join('/')), recursively);
	}

	return Super::callMethod(shv_path, method, params);
}

ShvNode::StringList LocalFSNode::childNames(const ShvNode::StringViewList &shv_path)
{
	QString qpath = QString::fromStdString(shv_path.join('/'));
	QFileInfo fi_path(m_rootDir.absolutePath() + '/' + qpath);
	//shvInfo() << __FUNCTION__ << fi_path.absoluteFilePath() << "is dir:" << fi_path.isDir();
	if(fi_path.isDir()) {
		QDir d2(fi_path.absoluteFilePath());
		if(!d2.exists())
			SHV_EXCEPTION("Path " + d2.absolutePath().toStdString() + " do not exists.");
		ShvNode::StringList lst;
		for(const QFileInfo &fi : d2.entryInfoList(QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase | QDir::DirsFirst)) {
			//shvInfo() << fi.fileName();
			lst.push_back(fi.fileName().toStdString());
		}
		return lst;
	}
	return ShvNode::StringList();
}

chainpack::RpcValue LocalFSNode::hasChildren(const ShvNode::StringViewList &shv_path)
{
	return isDir(shv_path);
}

static std::vector<cp::MetaMethod> meta_methods_dir {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{M_MKFILE, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_MKDIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_RMDIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_SERVICE}
};

static std::vector<cp::MetaMethod> meta_methods_dir_write_file {
	{M_WRITE, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
};

static std::vector<cp::MetaMethod> meta_methods_file {
	{cp::Rpc::METH_DIR, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{cp::Rpc::METH_LS, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_BROWSE},
	{M_SIZE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_BROWSE},
	{M_READ, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_READ},
	{M_WRITE, cp::MetaMethod::Signature::RetParam, 0, cp::Rpc::GRANT_WRITE},
	{M_DELETE, cp::MetaMethod::Signature::RetVoid, 0, cp::Rpc::GRANT_SERVICE}
};

size_t LocalFSNode::methodCount(const ShvNode::StringViewList &shv_path)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(shv_path.join('/')));
	if(fi.exists())
		return isDir(shv_path) ? meta_methods_dir.size() : meta_methods_file.size();
	if(!shv_path.empty()) {
		StringViewList dir_path = shv_path.mid(0, shv_path.size() - 1);
		fi = ndFileInfo(QString::fromStdString(dir_path.join('/')));
		if(fi.isDir())
			return meta_methods_dir_write_file.size();
	}
	return 0;
}

const chainpack::MetaMethod *LocalFSNode::metaMethod(const StringViewList &shv_path, size_t ix)
{
	QFileInfo fi = ndFileInfo(QString::fromStdString(shv_path.join('/')));
	if(fi.exists()) {
		size_t meta_methods_size = (fi.isDir()) ? meta_methods_dir.size() : meta_methods_file.size();

		if(meta_methods_size <= ix)
			SHV_EXCEPTION("Invalid method index: " + std::to_string(ix) + " of: " + std::to_string(meta_methods_size));

		return (fi.isDir()) ? &(meta_methods_dir[ix]) : &(meta_methods_file[ix]);
	}
	if(!shv_path.empty()) {
		StringViewList dir_path = shv_path.mid(0, shv_path.size() - 1);
		fi = ndFileInfo(QString::fromStdString(dir_path.join('/')));
		if(fi.isDir() && ix < meta_methods_dir_write_file.size())
			return &(meta_methods_dir_write_file[ix]);
	}
	SHV_EXCEPTION("Invalid method index: " + std::to_string(ix));
}

bool LocalFSNode::isDir(const ShvNode::StringViewList &shv_path) const
{
	QString qpath = QString::fromStdString(shv_path.join('/'));
	QFileInfo fi(m_rootDir.absolutePath() + '/' + qpath);
	if(!fi.exists())
		shvError() << "Invalid path:" << fi.absoluteFilePath();
	shvDebug() << __FUNCTION__ << "shv path:" << qpath << "file info:" << fi.absoluteFilePath() << "is dir:" << fi.isDir();
	return fi.isDir();
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

chainpack::RpcValue LocalFSNode::ndWrite(const QString &path, const chainpack::RpcValue &methods_params)
{
	QFile f(m_rootDir.absolutePath() + '/' + path);

	if (methods_params.isString()){
		if(f.open(QFile::WriteOnly)) {
			const chainpack::RpcValue::String &content = methods_params.toString();
			f.write(content.data(), content.size());
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else if (methods_params.isList()){
		chainpack::RpcValue::List params = methods_params.toList();

		if (params.size() != 2){
			SHV_EXCEPTION("Cannot write to file " + f.fileName().toStdString() + ". Invalid parameters count.");
		}
		chainpack::RpcValue::Map flags = (params[1].isMap()) ? params[1].toMap() : chainpack::RpcValue::Map();
		QFile::OpenMode open_mode = (flags.hasKey("append") && flags.value("append").toBool()) ? QFile::Append : QFile::WriteOnly;

		if(f.open(open_mode)) {
			const chainpack::RpcValue::String &content = (params[0].isString()) ? params[0].toString() : "";
			f.write(content.data(), content.size());
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else{
		SHV_EXCEPTION("Unsupported param type.");
	}

	return false;
}

chainpack::RpcValue LocalFSNode::ndDelete(const QString &path)
{
	QFile file (m_rootDir.absolutePath() + '/' + path);
	return file.remove();
}

chainpack::RpcValue LocalFSNode::ndMkfile(const QString &path, const chainpack::RpcValue &methods_params)
{
	QString dir_path = m_rootDir.absolutePath() + '/' + path;
	std::string error;

	if (methods_params.isString()){
		QString file_path = dir_path + "/" + QString::fromStdString(methods_params.toString());

		QFile f(file_path);
		if(f.open(QFile::WriteOnly)) {
			return true;
		}
		error = "Cannot open file " + file_path.toStdString() + " for writing.";
	}
	else if (methods_params.isList()){
		chainpack::RpcValue::List params = methods_params.toList();

		if (params.size() != 2){
			SHV_EXCEPTION("Invalid params count.");
		}

		QString file_path = dir_path + QString::fromStdString(params[0].toString());
		QDir d = QFileInfo(file_path).dir();

		if (!d.mkpath(d.absolutePath())){
			SHV_EXCEPTION("Cannot create path " + file_path.toStdString() + ".");
		}

		QFile f(file_path);

		if (f.exists()){
			SHV_EXCEPTION("File " + f.fileName().toStdString() + " already exists.");
		}

		if(f.open(QFile::WriteOnly)) {
			f.write(params[1].toString().data(), params[1].toString().size());
			return true;
		}
		SHV_EXCEPTION("Cannot open file " + f.fileName().toStdString() + " for writing.");
	}
	else{
		SHV_EXCEPTION("Unsupported param type.");
	}

	return false;
}

chainpack::RpcValue LocalFSNode::ndMkdir(const QString &path, const chainpack::RpcValue &methods_params)
{
	QDir d(m_rootDir.absolutePath()+ '/' + path);

	if (!methods_params.isString())
		SHV_EXCEPTION("Cannot create directory in directory " + d.absolutePath().toStdString() + ". Invalid parameter: " + methods_params.toCpon());

	return d.mkpath(m_rootDir.absolutePath()+ '/' + path + '/' + QString::fromStdString(methods_params.toString()));
}

chainpack::RpcValue LocalFSNode::ndRmdir(const QString &path, bool recursively)
{
	QDir d(m_rootDir.absolutePath() + '/' + path);

	if (path.isEmpty())
		SHV_EXCEPTION("Cannot remove root directory " + d.absolutePath().toStdString());

	if (recursively)
		return d.removeRecursively();
	else
		return d.rmdir(m_rootDir.absolutePath() + '/' + path);
}

} // namespace node
} // namespace iotqt
} // namespace shv
