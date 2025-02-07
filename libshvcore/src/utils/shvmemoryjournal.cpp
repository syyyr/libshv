#include "shvmemoryjournal.h"
#include "shvlogheader.h"
#include "shvpath.h"
#include "shvfilejournal.h"
#include "shvlogrpcvaluereader.h"
#include "patternmatcher.h"

#include "../exception.h"
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

void ShvMemoryJournal::loadLog(const chainpack::RpcValue &log, bool append_records)
{
	shvLogFuncFrame();
	shv::core::utils::ShvLogRpcValueReader rd(log, !shv::core::Exception::Throw);
	if(!append_records) {
		m_entries.clear();
		m_logHeader = rd.logHeader();
	}
	while(rd.next()) {
		const core::utils::ShvJournalEntry &entry = rd.entry();
		append(entry);
	}
}

void ShvMemoryJournal::loadLog(const chainpack::RpcValue &log, const chainpack::RpcValue::Map &default_snapshot_values)
{
	shvLogFuncFrame();
	shv::core::utils::ShvLogRpcValueReader rd(log, !shv::core::Exception::Throw);
	m_entries.clear();
	m_logHeader = rd.logHeader();
	chainpack::RpcValue::Map missing_snapshot_values = default_snapshot_values;
	auto append_default_snapshot_values = [&missing_snapshot_values, this]() {
		for(const auto &kv : missing_snapshot_values) {
			core::utils::ShvJournalEntry e;
			e.epochMsec = m_logHeader.sinceMsec();
			e.path = kv.first;
			e.value = kv.second;
			shvDebug() << "generating snapshot entry with default value, path:" << e.path;
			append(e);
		}
		missing_snapshot_values.clear();
	};
	while(rd.next()) {
		const core::utils::ShvJournalEntry &entry = rd.entry();
		if(rd.isInSnapshot()) {
			shvDebug() << "--:" << entry.path;
			missing_snapshot_values.erase(entry.path);
		}
		else if(!missing_snapshot_values.empty()) {
			append_default_snapshot_values();
		}
		append(entry);
	}
	// call append_default_snapshot_values() for case, that log contains snapshot only
	append_default_snapshot_values();
}

void ShvMemoryJournal::append(const ShvJournalEntry &entry)
{
	int64_t epoch_msec = entry.epochMsec;
	if(epoch_msec == 0) {
		epoch_msec = cp::RpcValue::DateTime::now().msecsSinceEpoch();
	}
	else if(isShortTimeCorrection()) {
		if(!entry.isSpontaneous() && entry.shortTime != shv::core::utils::ShvJournalEntry::NO_SHORT_TIME) {
			uint16_t short_msec = static_cast<uint16_t>(entry.shortTime);
			ShortTime &st = m_recentShortTimes[entry.path];
			if(entry.shortTime == st.recentShortTime) {
				// the same short times in the row, this can happen only when
				// data is badly generated, we will ignore such values
				shvWarning() << "two same short-times in the row:" << entry.shortTime << entry.path << cp::RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString() << entry.epochMsec;
				st.epochTime = epoch_msec;
			}
			else {
				if(st.epochTime == 0) {
					st.epochTime = epoch_msec;
					st.recentShortTime = entry.shortTime;
				}
				else {
					int64_t new_epoch_msec = st.toEpochTime(short_msec);
					int64_t dt_diff = new_epoch_msec - epoch_msec;
					constexpr int64_t MAX_SKEW = 500;
					if(dt_diff < -MAX_SKEW) {
						shvWarning() << "short time too late:" << -dt_diff << entry.path << cp::RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString() << entry.epochMsec;
						// short time is too late
						// pin it to date time from log entry
						st.epochTime = epoch_msec - MAX_SKEW;
						st.recentShortTime = entry.shortTime;
						epoch_msec = st.epochTime;
					}
					else if(dt_diff > MAX_SKEW) {
						shvWarning() << "short time too ahead:" << dt_diff << entry.path << cp::RpcValue::DateTime::fromMSecsSinceEpoch(entry.epochMsec).toIsoString() << entry.epochMsec;
						// short time is too ahead
						// pin it to greater of recent short-time and epoch-time
						st.epochTime = epoch_msec + MAX_SKEW;
						st.recentShortTime = entry.shortTime;
						epoch_msec = st.epochTime;
					}
					else {
						epoch_msec = st.addShortTime(short_msec);
					}
				}
			}
		}
	}

	if (m_pathDictionary.find(entry.path) == m_pathDictionary.end()) {
		m_pathDictionary[entry.path] = m_pathDictionary.size() + 1;
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

	auto params_since_msec = params.since.toDateTime().msecsSinceEpoch();
	auto params_until_msec = params.until.toDateTime().msecsSinceEpoch();

	int64_t log_since_msec = m_logHeader.sinceMsec();
	int64_t log_until_msec = m_logHeader.untilMsec();

	int64_t since_msec = std::max(log_since_msec, params_since_msec);
	int64_t until_msec = min_valid(log_until_msec, params_until_msec);

	int rec_cnt_limit = std::min(params.recordCountLimit, DEFAULT_GET_LOG_RECORD_COUNT_LIMIT);
	bool rec_cnt_limit_hit = false;

	//int64_t first_record_msec = 0;
	int64_t last_record_msec = 0;

	if(params_since_msec > 0 && log_until_msec > 0 && params_since_msec >= log_until_msec)
		goto log_finish;
	if(params_until_msec > 0 && log_since_msec > 0 && params_until_msec < log_since_msec)
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

		auto entry_to_rpcvalue = [&make_path_shared](int64_t epoch_msec, const Entry &e){
			cp::RpcValue::List rec;
			rec.push_back(cp::RpcValue::DateTime::fromMSecsSinceEpoch(epoch_msec));
			rec.push_back(make_path_shared(e.path));
			rec.push_back(e.value);
			rec.push_back(e.shortTime == ShvJournalEntry::NO_SHORT_TIME ? cp::RpcValue(nullptr): cp::RpcValue(e.shortTime));
			rec.push_back((e.domain.empty() || e.domain == ShvJournalEntry::DOMAIN_VAL_CHANGE) ? cp::RpcValue(nullptr): e.domain);
			rec.push_back(e.valueFlags);
			rec.push_back(e.userId.empty()? cp::RpcValue(nullptr): cp::RpcValue(e.userId));
			return rec;
		};

		ShvSnapshot snapshot;
		if(params.withSnapshot) {
			for(auto it = m_entries.begin(); it != m_entries.end() && it < it1; ++it) {
				const Entry &e = *it;
				if (it < it1 || e.epochMsec == since_msec) {  //it1 (lower_bound) can be == since_msec (we want in snapshot)
					if(!pm.match(e))                          //or > since_msec (we don't want in snapshot)
						continue;
					addToSnapshot(snapshot, e);
				}
			}
			if(!snapshot.keyvals.empty()) {
				logDShvJournal() << "\t -------------- Snapshot";
				for(const auto &kv : snapshot.keyvals) {
					if(rec_cnt >= rec_cnt_limit) {
						rec_cnt_limit_hit = true;
						goto log_finish;
					}
					auto entry = kv.second;
					entry.setSnapshotValue(true);
					entry.setSpontaneous(false);
					if(since_msec == 0)
						since_msec = entry.epochMsec;
					last_record_msec = since_msec;
					log.push_back(entry_to_rpcvalue(since_msec, entry));
					rec_cnt++;
				}
			}
		}
		// keep <since, until) interval open to make log merge simpler
		{
			auto it = it1;
			for(; it != it2; ++it) {
				if(pm.match(*it)) {
					if(rec_cnt >= rec_cnt_limit) {
						rec_cnt_limit_hit = true;
						goto log_finish;
					}
					if(since_msec == 0)
						since_msec = it->epochMsec;
					last_record_msec = it->epochMsec;

					log.push_back(entry_to_rpcvalue(it->epochMsec, *it));
					rec_cnt++;
				}
			}
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
		if(until_msec == 0 || rec_cnt_limit_hit) {
			until_msec = last_record_msec;
		}

		hdr.setUntil((until_msec > 0)? cp::RpcValue(cp::RpcValue::DateTime::fromMSecsSinceEpoch(until_msec)): cp::RpcValue(nullptr));
		hdr.setRecordCount(rec_cnt);
		hdr.setRecordCountLimit(rec_cnt_limit);
		hdr.setRecordCountLimitHit(rec_cnt_limit_hit);
		hdr.setWithSnapShot(params.withSnapshot);
		hdr.setWithPathsDict(params.withPathsDict);

		cp::RpcValue::List fields;
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Timestamp)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Path)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Value)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ShortTime)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::Domain)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::ValueFlags)}});
		fields.push_back(cp::RpcValue::Map{{KEY_NAME, Column::name(Column::Enum::UserId)}});
		hdr.setFields(std::move(fields));

		if(params.withTypeInfo)
			hdr.copyTypeInfo(m_logHeader);
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
/*
size_t ShvMemoryJournal::timeToUpperBoundIndex(int64_t time) const
{
	Entry entry;
	entry.epochMsec = time;
	auto it = std::upper_bound(m_entries.begin(), m_entries.end(), entry, [](const Entry &e1, const Entry &e2) {
		return e1.epochMsec < e2.epochMsec;
	});
	return it - m_entries.begin();
}
*/
} // namespace utils
} // namespace core
} // namespace shv
