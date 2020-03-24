#include "shvmemoryjournal.h"
#include "shvlogheader.h"
#include "shvpath.h"
#include "shvfilejournal.h"

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
	m_inputFilterRecordCountLimit = std::min(input_filter.recordCountLimit, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);
}

void ShvMemoryJournal::loadLog(const chainpack::RpcValue &log, bool append_records)
{
	ShvLogHeader hdr = ShvLogHeader::fromMetaData(log.metaData());
	using Column = ShvLogHeader::Column;

	std::map<std::string, ShvLogTypeDescr> paths_type_descr = hdr.pathsTypeDescr();
	auto paths_sample_type = [paths_type_descr](const std::string &path) {
		auto it = paths_type_descr.find(path);
		return it == paths_type_descr.end()? ShvJournalEntry::SampleType::Continuous: it->second.sampleType;
	};

	const chainpack::RpcValue::IMap &dict = hdr.pathDictCRef();

	if(!append_records) {
		m_entries.clear();
		m_logHeader = hdr;
	}

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
		if(entry.sampleType == ShvJournalEntry::SampleType::Continuous
			&& m_patternMatcher.match(entry))
		{
			Entry &e = m_snapshot[entry.path];
			if(e.epochMsec < entry.epochMsec)
				e = entry;
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
	checkSampleType(e);
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

static int64_t min_valid(int64_t a, int64_t b)
{
	if(a == 0)
		return b;
	if(b == 0)
		return a;
	return std::min(a, b);
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

	auto filter_since_msec = m_inputFilter.since.toDateTime().msecsSinceEpoch();
	auto filter_until_msec = m_inputFilter.until.toDateTime().msecsSinceEpoch();

	auto params_since_msec = params.since.toDateTime().msecsSinceEpoch();
	auto params_until_msec = params.until.toDateTime().msecsSinceEpoch();

	int64_t log_since_msec = m_logHeader.sinceMsec();
	int64_t log_until_msec = m_logHeader.untilMsec();

	int64_t datavalid_since_msec = std::max(log_since_msec, filter_since_msec);
	int64_t datavalid_until_msec = min_valid(log_until_msec, filter_until_msec);

	int64_t since_msec = std::max(datavalid_since_msec, params_since_msec);
	int64_t until_msec = min_valid(datavalid_until_msec, params_until_msec);

	int rec_cnt_limit = std::min(params.recordCountLimit, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);
	bool all_entries_visited = false;

	//int64_t first_record_msec = 0;
	int64_t last_record_msec = 0;

	if(params_since_msec > 0 && datavalid_until_msec > 0 && params_since_msec >= datavalid_until_msec)
		goto log_finish;
	if(params_until_msec > 0 && datavalid_since_msec > 0 && params_until_msec < datavalid_since_msec)
		goto log_finish;

	{

		auto it1 = m_entries.begin();
		if(params_since_msec > 0) {
			Entry e;
			e.epochMsec = params_since_msec;
			it1 = std::lower_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
				return e1.epochMsec < e2.epochMsec;
			});
		}
		auto it2 = m_entries.end();
		if(params_until_msec > 0) {
			Entry e;
			e.epochMsec = params_until_msec;
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

		PatternMatcher pm(params);

		std::map<std::string, Entry> snapshot;
		if(params.withSnapshot) {
			for(auto kv : m_snapshot) {
				const auto &e = kv.second;
				if(e.epochMsec < params_until_msec && pm.match(e)) {
					snapshot[e.path] = e;
				}
			}
			for(auto it = m_entries.begin(); it != m_entries.end() && it <= it1; ++it) {
				const Entry &e = *it;
				if (it < it1 || e.epochMsec == since_msec) {
					if(!pm.match(e))
						continue;
					if(e.sampleType == ShvJournalEntry::SampleType::Continuous) {
						snapshot[e.path] = e;
					}
				}
			}
			if(!snapshot.empty()) {
				logDShvJournal() << "\t -------------- Snapshot";
				for(const auto &kv : snapshot) {
					cp::RpcValue::List rec;
					const auto &entry = kv.second;
					if(since_msec == 0)
						since_msec = entry.epochMsec;
					last_record_msec = since_msec;
					rec.push_back(cp::RpcValue::DateTime::fromMSecsSinceEpoch(since_msec));
					rec.push_back(make_path_shared(entry.path));
					rec.push_back(entry.value);
					rec.push_back(entry.shortTime);
					rec.push_back(entry.domain.empty()? cp::RpcValue(nullptr): entry.domain);
					log.push_back(std::move(rec));
					rec_cnt++;
					if(rec_cnt >= rec_cnt_limit)
						goto log_finish;
				}
			}
		}
		// keep <since, until) interval open to make log merge simpler
		{
			auto it = it1;
			for(; it != it2; ++it) {
				if(pm.match(*it)) {
					if(since_msec == 0)
						since_msec = it->epochMsec;
					last_record_msec = it->epochMsec;
					cp::RpcValue::List rec;
					rec.push_back(cp::RpcValue::DateTime::fromMSecsSinceEpoch(it->epochMsec));
					rec.push_back(make_path_shared(it->path));
					rec.push_back(it->value);
					rec.push_back(it->shortTime);
					rec.push_back(it->domain.empty()? cp::RpcValue(nullptr): it->domain);
					log.push_back(std::move(rec));
					rec_cnt++;
					if(rec_cnt >= rec_cnt_limit) {
						++it;
						break;
					}
				}
			}
		all_entries_visited = (it == it2);
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

		hdr.setSince((since_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(since_msec)): cp::RpcValue(nullptr));
		// if record count < limit and params until is specified and it is > log end, then set until to log end
		if(!(rec_cnt < rec_cnt_limit || all_entries_visited)) {
			until_msec = last_record_msec;
		}
		hdr.setUntil((until_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec)): cp::RpcValue(nullptr));
		hdr.setRecordCount(rec_cnt);
		hdr.setRecordCountLimit(rec_cnt_limit);
		hdr.setWithSnapShot(params.withSnapshot);
		hdr.setWithPathsDict(params.withPathsDict);

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

void ShvMemoryJournal::checkSampleType(ShvJournalEntry &entry) const
{
	ShvLogTypeDescr::SampleType st = m_logHeader.pathsSampleType(entry.path);
	if(st != ShvLogTypeDescr::SampleType::Invalid)
		entry.sampleType = st;
}

} // namespace utils
} // namespace core
} // namespace shv
