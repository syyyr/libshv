#pragma once

#include "graph.h"
#include "sample.h"

#include <QObject>
#include <QVariant>
#include <QVector>

#include <shv/core/utils/shvlogtypeinfo.h>

namespace shv {
namespace visu {
namespace timeline {

class SHVVISU_DECL_EXPORT GraphModel : public QObject
{
	Q_OBJECT

	using Super = QObject;
public:
	class SHVVISU_DECL_EXPORT ChannelInfo
	{
	public:
		QString shvPath;
		QString name;
		shv::core::utils::ShvTypeDescr typeDescr;
	};

	SHV_FIELD_BOOL_IMPL2(a, A, utoCreateChannels, true)

public:
	explicit GraphModel(QObject *parent = nullptr);

	XRange xRange() const;
	XRange xRange(qsizetype channel_ix) const;
	YRange yRange(qsizetype channel_ix) const;
	void clear();
	void appendChannel() {appendChannel({}, {}, {});}
	void appendChannel(const std::string &shv_path, const std::string &name, const shv::core::utils::ShvTypeDescr &type_descr);
public:
	virtual qsizetype channelCount() const { return qMin(m_channelsInfo.count(), m_samples.count()); }
	const ChannelInfo& channelInfo(qsizetype channel_ix) const { return m_channelsInfo.at(channel_ix); }
	QString typeDescrFieldName( const shv::core::utils::ShvTypeDescr &type_descr, int field_index);
	const shv::core::utils::ShvTypeInfo &typeInfo() const { return m_typeInfo; }
	void setTypeInfo(const shv::core::utils::ShvTypeInfo &type_info) { m_typeInfo = type_info; }

	virtual qsizetype count(qsizetype channel) const;
	/// without bounds check
	virtual Sample sampleAt(qsizetype channel, qsizetype ix) const;
	/// returns Sample() if out of bounds
	Sample sampleValue(qsizetype channel, qsizetype ix) const;
	/// sometimes is needed to show samples in transformed time scale (hide empty areas without samples)
	/// displaySampleValue() returns original time and value
	virtual Sample displaySampleValue(qsizetype channel, qsizetype ix) const { return sampleValue(channel, ix); }

	qsizetype lessTimeIndex(qsizetype channel, timemsec_t time) const;
	qsizetype lessOrEqualTimeIndex(qsizetype channel, timemsec_t time) const;
	qsizetype greaterTimeIndex(qsizetype channel, timemsec_t time) const;
	qsizetype greaterOrEqualTimeIndex(qsizetype channel, timemsec_t time) const;

	virtual void beginAppendValues();
	virtual void endAppendValues();
	virtual void appendValue(qsizetype channel, Sample &&sample);
	void appendValueShvPath(const std::string &shv_path, Sample &&sample);
	void forgetValuesBefore(timemsec_t time, int min_samples_count = 100);

	qsizetype pathToChannelIndex(const std::string &path) const;
	QString channelShvPath(qsizetype channel) const { return channelInfo(channel).shvPath; }

	Q_SIGNAL void xRangeChanged(XRange range);
	Q_SIGNAL void channelCountChanged(qsizetype cnt);
public:
	static double valueToDouble(const QVariant v, core::utils::ShvTypeDescr::Type type_id = core::utils::ShvTypeDescr::Type::Invalid, bool *ok = nullptr);
protected:
	QString guessTypeName(qsizetype channel_ix) const;
protected:
	using ChannelSamples = QVector<Sample>;
	QVector<ChannelSamples> m_samples;
	QVector<ChannelInfo> m_channelsInfo;
	XRange m_begginAppendXRange;

	mutable std::map<std::string, qsizetype> m_pathToChannelCache;
	shv::core::utils::ShvTypeInfo m_typeInfo;
};

}}}
