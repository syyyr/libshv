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
{
	if(input_filter.since.isDateTime())
		m_sinceMsec = input_filter.since.toDateTime().msecsSinceEpoch();
	if(input_filter.until.isDateTime())
		m_untilMsec = input_filter.until.toDateTime().msecsSinceEpoch();
	m_maxRecordCount = input_filter.maxRecordCount;
	if(!input_filter.pathPattern.empty()) {
		if(input_filter.pathPatternType == ShvJournalGetLogParams::PatternType::RegEx) {
			try {
				m_pathPatternRegEx = std::regex(input_filter.pathPattern);
				m_usePathPatternregEx = true;
			}
			catch (const std::regex_error &e) {
				logWShvJournal() << "Invalid path pattern regex:" << input_filter.pathPattern << " - " << e.what();
			}
		}
		else {
			m_pathPatternWildCard = input_filter.pathPattern;
		}
	}
	if(!input_filter.domainPattern.empty()) try {
		m_domainPatternRegEx = std::regex(input_filter.domainPattern);
		m_useDomainPatternregEx = true;
	}
	catch (const std::regex_error &e) {
		logWShvJournal() << "Invalid domain pattern regex:" << input_filter.domainPattern << " - " << e.what();
	}
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
		e.timeMsec = time;
		e.path = path;
		e.value = row.value(Column::Value);
		cp::RpcValue st = row.value(Column::ShortTime);
		e.shortTime = st.isInt() && st.toInt() >= 0? st.toInt(): ShvJournalEntry::NO_SHORT_TIME;
		e.domain = row.value(Column::Domain).toString();
		e.course = row.value(Column::Course).toInt() == 0? ShvJournalEntry::CourseType::Continuous: ShvJournalEntry::CourseType::Discrete;
		append(std::move(e));
	}
}

void MemoryShvJournal::append(const ShvJournalEntry &entry, int64_t epoch_msec)
{
	Entry e(entry);
	e.timeMsec = epoch_msec;
	append(std::move(e));
}

void MemoryShvJournal::append(MemoryShvJournal::Entry &&entry)
{
	if((int)m_entries.size() >= m_maxRecordCount)
		return;
	if(entry.timeMsec == 0)
		entry.timeMsec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	if(m_sinceMsec > 0 && entry.timeMsec < m_sinceMsec)
		return;
	if(m_untilMsec > 0 && entry.timeMsec >= m_untilMsec)
		return;
	if(m_usePathPatternregEx)
		if(!std::regex_match(entry.path, m_pathPatternRegEx))
			return;
	if(!m_pathPatternWildCard.empty()) {
		ShvPath path(entry.path);
		if(!path.matchWild(m_pathPatternWildCard))
			return;
	}
	if(m_useDomainPatternregEx)
		if(!std::regex_match(entry.domain, m_domainPatternRegEx))
			return;
	{
		auto it = m_pathDictionary.find(entry.path);
		if(it == m_pathDictionary.end())
			m_pathDictionary[entry.path] = m_pathDictionaryIndex++;
	}
	int64_t last_time = m_entries.empty()? 0: m_entries[m_entries.size()-1].timeMsec;
	if(entry.timeMsec < last_time) {
		auto it = std::upper_bound(m_entries.begin(), m_entries.end(), entry, [](const Entry &e1, const Entry &e2) {
			return e1.timeMsec < e2.timeMsec;
		});
		m_entries.insert(it, std::move(entry));
	}
	else {
		m_entries.push_back(std::move(entry));
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
		e.timeMsec = since_msec;
		it1 = std::lower_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
			return e1.timeMsec < e2.timeMsec;
		});
	}
	auto it2 = m_entries.end();
	if(until_msec > 0) {
		Entry e;
		e.timeMsec = until_msec;
		it1 = std::upper_bound(m_entries.begin(), m_entries.end(), e, [](const Entry &e1, const Entry &e2) {
			return e1.timeMsec < e2.timeMsec;
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

	//const std::string fnames[] = {"foo.txt", "bar.txt", "baz.dat", "zoidberg"};
	std::regex domain_regex;
	if(!params.domainPattern.empty()) try {
		domain_regex = std::regex{params.domainPattern};
	}
	catch (const std::regex_error &e) {
		logWShvJournal() << "Invalid domain pattern regex:" << params.domainPattern << " - " << e.what();
	}
	std::regex path_regex;
	bool use_path_regex = false;
	if(!params.pathPattern.empty() && params.pathPatternType == ShvJournalGetLogParams::PatternType::RegEx) try {
		path_regex = std::regex{params.pathPattern};
		use_path_regex = true;
	}
	catch (const std::regex_error &e) {
		logWShvJournal() << "Invalid path pattern regex:" << params.pathPattern << " - " << e.what();
	}

	auto match_pattern = [&params, use_path_regex, &path_regex, &domain_regex](const Entry &e) {
		if(!params.pathPattern.empty()) {
			logDShvJournal() << "\t MATCHING:" << params.pathPattern << "vs:" << e.path;
			if(use_path_regex) {
				if(!std::regex_match(e.path, path_regex))
					return false;
			}
			else {
				ShvPath path(e.path);
				if(!path.matchWild(params.pathPattern))
					return false;
			}
			logDShvJournal() << "\t\t MATCH";
		}
		if(!params.domainPattern.empty()) {
			if(!std::regex_match(e.domain, domain_regex))
				return false;
		}
		return true;
	};
	if(params.withSnapshot) {
		for(auto it = m_entries.begin(); it != it1; ++it) {
			const Entry &e = *it;
			if(!match_pattern(e))
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
		{ // keep interval open to make log merge simpler
			cp::RpcValue::List rec;
			rec.push_back(it->timeMsec);
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
