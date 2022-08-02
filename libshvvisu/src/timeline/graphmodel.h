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
		//int metaTypeId = QMetaType::UnknownType;
		shv::core::utils::ShvTypeDescr typeDescr;

		//QString caption() const { return name.isEmpty()? shvPath: name; }
	};

	SHV_FIELD_BOOL_IMPL2(a, A, utoCreateChannels, true)

public:
	explicit GraphModel(QObject *parent = nullptr);

	XRange xRange() const;
	XRange xRange(int channel_ix) const;
	YRange yRange(int channel_ix) const;
	void clear();
	void appendChannel() {appendChannel(std::string(), std::string());}
	void appendChannel(const std::string &shv_path, const std::string &name,
					   const shv::core::utils::ShvTypeDescr &type_descr = shv::core::utils::ShvTypeDescr());
public:
	virtual int channelCount() const { return qMin(m_channelsInfo.count(), m_samples.count()); }
	const ChannelInfo& channelInfo(int channel_ix) const { return m_channelsInfo.at(channel_ix); }
	ChannelInfo& channelInfo(int channel_ix) { return m_channelsInfo[channel_ix]; }
	QString typeDescrFieldName( const shv::core::utils::ShvTypeDescr &type_descr, int field_index);
	const shv::core::utils::ShvTypeInfo &typeInfo() const { return m_typeInfo; }
	void setTypeInfo(const shv::core::utils::ShvTypeInfo &type_info) { m_typeInfo = type_info; }

	virtual int count(int channel) const;
	/// without bounds check
	virtual Sample sampleAt(int channel, int ix) const;
	/// returns Sample() if out of bounds
	Sample sampleValue(int channel, int ix) const;
	/// sometimes is needed to show samples in transformed time scale (hide empty areas without samples)
	/// displaySampleValue() returns original time and value
	virtual Sample displaySampleValue(int channel, int ix) const { return sampleValue(channel, ix); }

	int lessTimeIndex(int channel, timemsec_t time) const;
	int lessOrEqualTimeIndex(int channel, timemsec_t time) const;
	int greaterTimeIndex(int channel, timemsec_t time) const;
	int greaterOrEqualTimeIndex(int channel, timemsec_t time) const;

	virtual void beginAppendValues();
	virtual void endAppendValues();
	virtual void appendValue(int channel, Sample &&sample);
	void appendValueShvPath(const std::string &shv_path, Sample &&sample);

	int pathToChannelIndex(const std::string &path) const;
	QString channelShvPath(int channel) const { return channelInfo(channel).shvPath; }

	Q_SIGNAL void xRangeChanged(XRange range);
	Q_SIGNAL void channelCountChanged(int cnt);
public:
	static double valueToDouble(const QVariant v, core::utils::ShvTypeDescr::Type type_id = core::utils::ShvTypeDescr::Type::Invalid, bool *ok = nullptr);
protected:
	QString guessTypeName(int channel_ix);
protected:
	using ChannelSamples = QVector<Sample>;
	QVector<ChannelSamples> m_samples;
	QVector<ChannelInfo> m_channelsInfo;
	XRange m_begginAppendXRange;

	mutable std::map<std::string, int> m_pathToChannelCache;
	shv::core::utils::ShvTypeInfo m_typeInfo;
};

}}}
