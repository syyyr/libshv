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
/**
Log only ANY->NOT_DEFAULT changes or changes from NOT_DEFAULT->DEFAULT
Adding DEFAULT value to snapshot will remove this path, so it will be MISSING next time

We can save many log lines thanks to this simple rule
1. short snapshot does not contain paths with default values (can have thousands of lines otherwise)
2. when OPC-UA is disconnected and connected again, then short snapshot is created

Short snapshot is snapshot which does not contain paths with DEFAULT values
*/
bool AbstractShvJournal::addToSnapshot(ShvSnapshot &snapshot, const ShvJournalEntry &entry)
{
	if(entry.domain != ShvJournalEntry::DOMAIN_VAL_CHANGE) {
		//shvDebug() << "remove not CHNG from snapshot:" << RpcValue(entry.toRpcValueMap()).toCpon();
		// always add not-chng to log
		return true;
	}
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodeDrop) {
		// NODE_DROP
		auto it = snapshot.keyvals.lower_bound(entry.path);
		while(it != snapshot.keyvals.end()) {
			if(it->first.rfind(entry.path, 0) == 0 && (it->first.size() == entry.path.size() || it->first[entry.path.size()] == '/')) {
				// it.key starts with key, then delete it from snapshot
				it = snapshot.keyvals.erase(it);
			}
			else {
				break;
			}
		}
		// always add node-drop to log
		snapshot.liveNodePropertyMaps.erase(entry.path);
		return true;
	}
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodePropertyMap) {
		// bulk node update, possibly NODE_NEW
		snapshot.keyvals[entry.path] = entry;
		snapshot.liveNodePropertyMaps.insert(entry.path);
		/*
		We have to keep track of live node objects created by NodePropertyMap

		Imagine this log example
		-> 2022-04-16T18:59:47Z tram/1234 <PropertyMap>{"coupled": true}
		-> 2022-04-16T19:59:47Z tram/1234/coupled false
		Making snapshot from this log will lead to
		-> 2022-04-16T18:59:47Z tram/1234 <PropertyMap>{"coupled": true}
		because
		-> 2022-04-16T19:59:47Z tram/1234/coupled false
		has default value.

		We are registering active NodePropertyMaps to mitigate this problem.
		This is not optimal solution, but it is also not bad solution.
		In the worst case, the snapshot will behave like if 'not-default-values' optimisation will be OFF
		*/
		return true;
	}
	else if(entry.value.hasDefaultValue()) {
		// writing default value to the snapshot must erase previous value if any
		auto it = snapshot.keyvals.find(entry.path);
		if(it == snapshot.keyvals.end()) {
			// DEFAULT->DEFAULT
			// not-spontaneous values are sent to log for all the nodes after app restart
			// this can create snapshot in new log file which is created when device is restarted

			// change to default value is not present in snapshot
			// that means that it is firs time after restart or two default-values in the row
			// exclude it from logging if the change is not spontaneous

			// this can create snapshot from not-default values only when device is restarted
			// this optimization can make snapshot on start of log file 10x smaller
			return entry.isSpontaneous();
		}
		else {
			// NOT_DEFAULT->DEFAULT
			// change from not-default to default must be logged
			// we know, that value is not-default, bacause default values are not stored in snapshot
			snapshot.keyvals.erase(it);
			return true;
		}
	}
	else {
		snapshot.keyvals[entry.path] = entry;
	}
	return true;
};

} // namespace utils
} // namespace core
} // namespace shv
