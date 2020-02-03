#include "shvmemoryjournal.h"
#include "shvlogheader.h"
#include "shvpath.h"

#include "../log.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

ShvMemoryJournal::ShvMemoryJournal()
{
}

ShvMemoryJournal::ShvMemoryJournal(const ShvGetLogParams &input_filter)
	: m_patternMatcher(input_filter)
{
	if(input_filter.since.isDateTime())
		m_inputFilterSinceMsec = input_filter.since.toDateTime().msecsSinceEpoch();
	if(input_filter.until.isDateTime())
		m_inputFilterUntilMsec = input_filter.until.toDateTime().msecsSinceEpoch();
	m_inputFilterRecordCountLimit = std::min(input_filter.maxRecordCount, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);
}

void ShvMemoryJournal::loadLog(const chainpack::RpcValue &log)
{
	ShvLogHeader hdr = ShvLogHeader::fromMetaData(log.metaData());
	using Column = ShvLogHeader::Column;

	std::map<std::string, ShvLogTypeDescr> paths_type_descr = hdr.pathsTypeDescr();
	auto paths_sample_type = [paths_type_descr](const std::string &path) {
		auto it = paths_type_descr.find(path);
		return it == paths_type_descr.end()? ShvJournalEntry::SampleType::Continuous: it->second.sampleType;
	};

	const chainpack::RpcValue::IMap &dict = hdr.pathDictCRef();

	const ShvGetLogParams params = ShvGetLogParams::fromRpcValue(log.metaValue("params"));

	const chainpack::RpcValue::List &lst = log.toList();
	for(const cp::RpcValue &v : lst) {
		const cp::RpcValue::List &row = v.toList();
		int64_t time = row.value(Column::Timestamp).toDateTime().msecsSinceEpoch();
		cp::RpcValue p = row.value(Column::Path);
		if(p.isInt())
			p = dict.value(p.toInt());
		const std::string &path = p.toString();
		if(path.empty()) {
			logWShvJournal() << "Path dictionary corrupted, row:" << v.toCpon();
			continue;
		}
		Entry e;
		e.epochMsec = time;
		e.path = path;
		e.value = row.value(Column::Value);
		cp::RpcValue st = row.value(Column::ShortTime);
		e.shortTime = st.isInt() && st.toInt() >= 0? st.toInt(): ShvJournalEntry::NO_SHORT_TIME;
		e.domain = row.value(Column::Domain).toString();
		e.sampleType = paths_sample_type(e.path);
		append(e);
	}
}

void ShvMemoryJournal::append(const ShvJournalEntry &entry)
{
	if((int)m_entries.size() >= m_inputFilterRecordCountLimit)
		return;
	int64_t epoch_msec = entry.epochMsec;
	if(epoch_msec == 0)
		epoch_msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	if(m_inputFilterUntilMsec > 0 && epoch_msec >= m_inputFilterUntilMsec)
		return;
	if(m_inputFilterSinceMsec > 0 && epoch_msec < m_inputFilterSinceMsec) {
		if(m_inputFilter.withSnapshot) {
			Entry &e = m_snapshot[entry.path];
			if(e.epochMsec < entry.epochMsec
					&& entry.sampleType == ShvJournalEntry::SampleType::Continuous
					&& m_patternMatcher.match(entry))
			{
				e = entry;
			}
		}
		return;
	}
	if(!m_patternMatcher.match(entry))
		return;
	{
		auto it = m_pathDictionary.find(entry.path);
		if(it == m_pathDictionary.end())
			m_pathDictionary[entry.path] = m_pathDictionaryIndex++;
	}
	Entry e(entry);
	e.epochMsec = epoch_msec;
	int64_t last_time = m_entries.empty()? 0: m_entries[m_entries.size()-1].epochMsec;
	if(epoch_msec < last_time) {
		auto it = std::upper_bound(m_entries.begin(), m_entries.end(), entry, [](const Entry &e1, const Entry &e2) {
			return e1.epochMsec < e2.epochMsec;
		});
		m_entries.insert(it, std::move(e));
	}
	else {
		m_entries.push_back(std::move(e));
	}
}

chainpack::RpcValue ShvMemoryJournal::getLog(const ShvGetLogParams &params)
{
	logIShvJournal() << "========================= getLog ==================";
	logIShvJournal() << "params:" << params.toRpcValue().toCpon();
	using Column = ShvLogHeader::Column;
	cp::RpcValue::List log;
	cp::RpcValue::Map path_cache;
	int max_path_index = 0;
	int rec_cnt = 0;
	auto since_msec = params.since.isDateTime()? params.since.toDateTime().msecsSinceEpoch(): 0;
	auto until_msec = params.until.isDateTime()? params.until.toDateTime().msecsSinceEpoch(): 0;
	int64_t since_msec2 = since_msec;
	int64_t until_msec2 = since_msec;

	auto it1 = m_entries.begin();
	if(since_msec > 0) {
		Entry e;
		e.epochMsec = since_msec;
		it1 = std::lower_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
			return e1.epochMsec < e2.epochMsec;
		});
	}
	auto it2 = m_entries.end();
	if(until_msec > 0) {
		Entry e;
		e.epochMsec = until_msec;
		it2 = std::upper_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
			return e1.epochMsec < e2.epochMsec;
		});
	}

	/// this ensure that there be only one copy of each path in memory
	auto make_path_shared = [&path_cache, &max_path_index, &params](const std::string &path) -> cp::RpcValue {
		cp::RpcValue ret = path_cache.value(path);
		if(ret.isValid())
			return ret;
		if(params.withPathsDict)
			ret = ++max_path_index;
		else
			ret = path;
		logIShvJournal() << "Adding record to path cache:" << path << "-->" << ret.toCpon();
		path_cache[path] = ret;
		return ret;
	};
	int max_rec_cnt = std::min(params.maxRecordCount, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);

	PatternMatcher pm(params);

	std::map<std::string, Entry> snapshot;
	if(params.withSnapshot) for(auto kv : m_snapshot) {
		const auto &e = kv.second;
		if(pm.match(e)) {
			if(since_msec2 == 0)
				since_msec2 = e.epochMsec;
			snapshot[e.path] = e;
		}
	}

	if(params.withSnapshot) {
		for(auto it = m_entries.begin(); it != it1; ++it) {
			const Entry &e = *it;
			if(!pm.match(e))
				continue;
			if(e.sampleType == ShvJournalEntry::SampleType::Continuous) {
				snapshot[e.path] = e;
			}
		}
	}
	for(auto it = it1; it != it2; ++it) {

		if(!snapshot.empty()) {
			logDShvJournal() << "\t -------------- Snapshot";
			until_msec2 = since_msec2;
			for(const auto &kv : snapshot) {
				cp::RpcValue::List rec;
				rec.push_back(since_msec2);
				rec.push_back(make_path_shared(kv.first));
				rec.push_back(kv.second.value);
				rec.push_back(kv.second.shortTime);
				rec.push_back(kv.second.domain.empty()? cp::RpcValue(nullptr): kv.second.domain);
				log.push_back(rec);
				rec_cnt++;
				if(rec_cnt >= max_rec_cnt)
					goto log_finish;
			}
			snapshot.clear();
		}
		if(pm.match(*it)) { // keep interval open to make log merge simpler
			if(since_msec2 == 0)
				since_msec2 = it->epochMsec;
			until_msec2 = it->epochMsec;
			cp::RpcValue::List rec;
			rec.push_back(cp::RpcValue::DateTime::fromMSecsSinceEpoch(it->epochMsec));
			rec.push_back(make_path_shared(it->path));
			rec.push_back(it->value);
			rec.push_back(it->shortTime);
			rec.push_back(it->domain.empty()? cp::RpcValue(nullptr): it->domain);
			log.push_back(rec);
			rec_cnt++;
			if(rec_cnt >= max_rec_cnt)
				goto log_finish;
		}
	}

log_finish:
	cp::RpcValue ret = log;
	ShvLogHeader hdr;
	{
		hdr.setDeviceId(m_logHeader.deviceId());
		hdr.setDeviceType(m_logHeader.deviceType());
		hdr.setDateTime(cp::RpcValue::DateTime::now());
		hdr.setLogParams(params);
		hdr.setSince((since_msec2 > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(since_msec2)): cp::RpcValue(nullptr));
		hdr.setUntil((until_msec2 > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec2)): cp::RpcValue(nullptr));
		hdr.setRecordCount(rec_cnt);
		hdr.setRecordCountLimit(max_rec_cnt);
		hdr.setWithSnapShot(params.withSnapshot);

		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Domain)}});
		hdr.setFields(std::move(fields));

		hdr.setSources(m_logHeader.sources());
	}
	if(params.withPathsDict) {
		logIShvJournal() << "Generating paths dict";
		cp::RpcValue::IMap path_dict;
		for(auto kv : path_cache) {
			logIShvJournal() << "Adding record to paths dict:" << kv.second.toInt() << "-->" << kv.first;
			path_dict[kv.second.toInt()] = kv.first;
		}
		hdr.setPathDict(std::move(path_dict));
	}
	ret.setMetaData(hdr.toMetaData());
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
