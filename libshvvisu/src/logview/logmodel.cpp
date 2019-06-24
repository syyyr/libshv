#include "logmodel.h"

#include <shv/iotqt/utils/fileshvjournal.h>

namespace cp = shv::chainpack;

namespace shv {
namespace visu {
namespace logview {

LogModel::LogModel(QObject *parent)
	: Super(parent)
{

}

void LogModel::setLog(const shv::chainpack::RpcValue &log)
{
	beginResetModel();
	m_log = log;
	endResetModel();
}

int LogModel::rowCount(const QModelIndex &) const
{
	const shv::chainpack::RpcValue::List &lst = m_log.toList();
	return (int)lst.size();
}

QVariant LogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if(orientation == Qt::Horizontal) {
		if(role == Qt::DisplayRole) {
			switch (section) {
			case ColDateTime: return tr("DateTime");
			case ColPath: return tr("Path");
			case ColValue: return tr("Value");
			}
		}
	}
	return Super::headerData(section, orientation, role);
}

QVariant LogModel::data(const QModelIndex &index, int role) const
{
	if(index.isValid() && index.row() < rowCount()) {
		if(role == Qt::DisplayRole) {
			const shv::chainpack::RpcValue::List &lst = m_log.toList();
			shv::chainpack::RpcValue row = lst.value((unsigned)index.row());
			shv::chainpack::RpcValue val = row.toList().value((unsigned)index.column());
			if(index.column() == ColPath && (val.type() == cp::RpcValue::Type::UInt || val.type() == cp::RpcValue::Type::Int)) {
				static std::string KEY_PATHS_DICT = shv::iotqt::utils::FileShvJournal::KEY_PATHS_DICT;
				const chainpack::RpcValue::IMap &dict = m_log.metaValue(KEY_PATHS_DICT).toIMap();
				auto it = dict.find(val.toInt());
				if(it != dict.end())
					val = it->second;
			}
			return QString::fromStdString(val.toCpon());
		}
	}
	return QVariant();
}

}}}
