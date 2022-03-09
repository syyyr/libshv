#include "abstractshvjournal.h"
#include "shvjournalentry.h"
#include "shvgetlogparams.h"
#include "shvpath.h"
#include "../stringview.h"

#include "../log.h"
#include "../exception.h"

#define logWShvJournal() shvCWarning("ShvJournal")
#define logIShvJournal() shvCInfo("ShvJournal")
#define logDShvJournal() shvCDebug("ShvJournal")

using namespace shv::chainpack;

namespace shv {
namespace core {
namespace utils {

const int AbstractShvJournal::DEFAULT_GET_LOG_RECORD_COUNT_LIMIT = 100 * 1000;
const char *AbstractShvJournal::KEY_NAME = "name";
const char *AbstractShvJournal::KEY_RECORD_COUNT = "recordCount";
const char *AbstractShvJournal::KEY_PATHS_DICT = "pathsDict";

AbstractShvJournal::~AbstractShvJournal()
{
}

chainpack::RpcValue AbstractShvJournal::getSnapShotMap()
{
	SHV_EXCEPTION("getSnapShot() not implemented");
}

bool AbstractShvJournal::addToSnapshot(std::map<std::string, ShvJournalEntry> &snapshot, const ShvJournalEntry &entry)
{
	if(entry.domain != ShvJournalEntry::DOMAIN_VAL_CHANGE) {
		//shvDebug() << "remove not CHNG from snapshot:" << RpcValue(entry.toRpcValueMap()).toCpon();
		// always add not-chng to log
		return true;
	}
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodeDrop) {
		auto it = snapshot.lower_bound(entry.path);
		while(it != snapshot.end()) {
			if(it->first.rfind(entry.path, 0) == 0 && (it->first.size() == entry.path.size() || it->first[entry.path.size()] == '/')) {
				// it.key starts with key, then delete it from snapshot
				it = snapshot.erase(it);
			}
			else {
				break;
			}
		}
		// always add node-drop to log
		return true;
	}
	else if(entry.value.hasDefaultValue()) {
		// writing default value to the snapshot must erase previous value if any
		auto it = snapshot.find(entry.path);
		if(it == snapshot.end()) {
			// change to default value is not present in snapshot
			// that means that it is firs time after restart
			// or two default-values in the row
			// exclude it from logging
			// this can create snapshot from mot-default values only after device restart
			// when all nodes properties values are send to log, default and not-default
			// this optimization can make snapshot on start of log file 10x smaller
			return false;
		}
		else {
			// change from not-default to default must be logged
			snapshot.erase(it);
			return true;
		}
	}
	else {
		/*entry.sampleType == ShvJournalEntry::NO_VALUE_FLAGS && */
		snapshot[entry.path] = entry;
	}
	return true;
};

} // namespace utils
} // namespace core
} // namespace shv
