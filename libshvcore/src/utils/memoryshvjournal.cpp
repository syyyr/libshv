#include "memoryshvjournal.h"
#include "shvpath.h"

#include "../log.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

namespace cp = shv::chainpack;

namespace shv {
namespace core {
namespace utils {

MemoryShvJournal::MemoryShvJournal()
{
}

MemoryShvJournal::MemoryShvJournal(const ShvJournalGetLogParams &input_filter)
	: m_patternMatcher(input_filter)
{
	if(input_filter.since.isDateTime())
		m_sinceMsec = input_filter.since.toDateTime().msecsSinceEpoch();
	if(input_filter.until.isDateTime())
		m_untilMsec = input_filter.until.toDateTime().msecsSinceEpoch();
	m_maxRecordCount = input_filter.maxRecordCount;
}

void MemoryShvJournal::setTypeInfo(const chainpack::RpcValue &i, const std::string &path_prefix)
{
	if(path_prefix.empty())
		m_typeInfos["."] = i;
	else
		m_typeInfos[path_prefix] = i;
}

void MemoryShvJournal::loadLog(const chainpack::RpcValue &log)
{
	const chainpack::RpcValue::IMap &dict = log.metaValue(KEY_PATHS_DICT).toIMap();
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
		e.course = row.value(Column::Course).toInt() == 0? ShvJournalEntry::CourseType::Continuous: ShvJournalEntry::CourseType::Discrete;
		append(e);
	}
}

void MemoryShvJournal::append(const ShvJournalEntry &entry)
{
	if((int)m_entries.size() >= m_maxRecordCount)
		return;
	int64_t epoch_msec = entry.epochMsec;
	if(epoch_msec == 0)
		epoch_msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	if(m_sinceMsec > 0 && epoch_msec < m_sinceMsec)
		return;
	if(m_untilMsec > 0 && epoch_msec >= m_untilMsec)
		return;
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

chainpack::RpcValue MemoryShvJournal::getLog(const ShvJournalGetLogParams &params)
{
	logIShvJournal() << "========================= getLog ==================";
	logIShvJournal() << "params:" << params.toRpcValue().toCpon();
	cp::RpcValue::List log;
	cp::RpcValue::Map path_cache;
	int max_path_index = 0;
	int rec_cnt = 0;
	auto since_msec = params.since.isDateTime()? params.since.toDateTime().msecsSinceEpoch(): 0;
	auto until_msec = params.until.isDateTime()? params.until.toDateTime().msecsSinceEpoch(): 0;

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
		it1 = std::upper_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
			return e1.epochMsec < e2.epochMsec;
		});
	}

	/// this ensure that there be only one copy of each path in memory
	auto make_path_shared = [&path_cache, &max_path_index, &params](const std::string &path) -> cp::RpcValue {
		cp::RpcValue ret = path_cache.value(path);
		if(ret.isValid())
			return ret;
		if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::PathsDict))
			ret = ++max_path_index;
		else
			ret = path;
		logIShvJournal() << "Adding record to path cache:" << path << "-->" << ret.toCpon();
		path_cache[path] = ret;
		return ret;
	};
	int max_rec_cnt = std::min(params.maxRecordCount, m_getLogRecordCountLimit);
	std::map<std::string, Entry> snapshot;

	PatternMatcher pm(params);
	if(params.withSnapshot) {
		for(auto it = m_entries.begin(); it != it1; ++it) {
			const Entry &e = *it;
			if(!pm.match(e))
				continue;
			if(e.course == ShvJournalEntry::CourseType::Continuous) {
				snapshot[e.path] = e;
			}
		}
	}
	for(auto it = it1; it != it2; ++it) {

		if(!snapshot.empty()) {
			logDShvJournal() << "\t -------------- Snapshot";
			for(const auto &kv : snapshot) {
				cp::RpcValue::List rec;
				rec.push_back(params.since);
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
			cp::RpcValue::List rec;
			rec.push_back(it->epochMsec);
			rec.push_back(make_path_shared(it->path));
			rec.push_back(it->value);
			rec.push_back(it->shortTime);
			rec.push_back(it->domain.empty()? cp::RpcValue(nullptr): it->domain);
			rec.push_back(it->course);
			log.push_back(rec);
			rec_cnt++;
			if(rec_cnt >= max_rec_cnt)
				goto log_finish;
		}
	}

log_finish:
	cp::RpcValue ret = log;
	cp::RpcValue::MetaData md;
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::BasicInfo)) {
		{
			cp::RpcValue::Map device;
			device["id"] = m_deviceId;
			device["type"] = m_deviceType;
			md.setValue("device", device); // required
		}
		md.setValue("logVersion", 1); // required
		md.setValue("dateTime", cp::RpcValue::DateTime::now());
		ShvJournalGetLogParams params2 = params;
		params2.since = (since_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(since_msec)): cp::RpcValue(nullptr);
		params2.until = (until_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec)): cp::RpcValue(nullptr);
		md.setValue("params", params2.toRpcValue());
		md.setValue(KEY_RECORD_COUNT, rec_cnt);
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::FieldInfo)) {
		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Domain)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Course)}});
		md.setValue("fields", std::move(fields));
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::TypeInfo)) {
		if(m_typeInfos.empty())
			md.setValue("sources", m_typeInfos);
	}
	if(params.headerOptions & static_cast<unsigned>(ShvJournalGetLogParams::HeaderOptions::PathsDict)) {
		logIShvJournal() << "Generating paths dict";
		cp::RpcValue::IMap path_dict;
		for(auto kv : path_cache) {
			logIShvJournal() << "Adding record to paths dict:" << kv.second.toInt() << "-->" << kv.first;
			path_dict[kv.second.toInt()] = kv.first;
		}
		md.setValue(KEY_PATHS_DICT, path_dict);
	}
	if(!md.isEmpty())
		ret.setMetaData(std::move(md));
	return ret;
}

} // namespace utils
} // namespace core
} // namespace shv
