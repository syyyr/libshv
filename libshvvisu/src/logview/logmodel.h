#pragma once

#include "../shvvisuglobal.h"

#include <shv/chainpack/rpcvalue.h>
#include <shv/core/utils/shvmemoryjournal.h>

#include <QAbstractTableModel>
#include <QTimeZone>

namespace shv {
namespace visu {
namespace logview {

class SHVVISU_DECL_EXPORT LogModel : public QAbstractTableModel
{
	Q_OBJECT

	using Super = QAbstractTableModel;
public:
	enum {ColDateTime = 0, ColPath, ColValue, ColShortTime, ColDomain, ColSampleType, ColUserId,  ColCnt};
public:
	LogModel(QObject *parent = nullptr);

	void setTimeZone(const QTimeZone &tz);

	void setLog(const shv::chainpack::RpcValue &log);
	shv::chainpack::RpcValue log() const { return m_log; }

	int rowCount(const QModelIndex & = QModelIndex()) const override;
	int columnCount(const QModelIndex & = QModelIndex()) const override {return ColCnt;}
	QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
	QVariant data(const QModelIndex &index, int role) const override;
protected:
	shv::chainpack::RpcValue m_log;
	QTimeZone m_timeZone;
};

}}}
