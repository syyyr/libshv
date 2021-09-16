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

void AbstractShvJournal::addToSnapshot(std::map<std::string, ShvJournalEntry> &snapshot, const ShvJournalEntry &entry)
{
	if(entry.domain != ShvJournalEntry::DOMAIN_VAL_CHANGE)
		return;
	if(entry.value.metaTypeNameSpaceId() == shv::chainpack::meta::GlobalNS::ID && entry.value.metaTypeId() == shv::chainpack::meta::GlobalNS::MetaTypeId::NodeDrop) {
		auto it = snapshot.lower_bound(entry.path);
		while(it != snapshot.end()) {
			if(it->first.rfind(entry.path, 0) == 0 && (it->first.size() == entry.path.size() || it->first[entry.path.size()] == '/')) {
				// it.key starts with key, then delete it from snapshot
				it = snapshot.erase(it);
			}
			else {
				return;
			}
		}
	}
	else if(entry.value.hasDefaultValue()) {
		// writing default value to the snapshot must erase previous value if any
		auto it = snapshot.find(entry.path);
		if(it != snapshot.end())
			snapshot.erase(it);
	}
	else {
		/*entry.sampleType == ShvJournalEntry::SampleType::Continuous && */
		snapshot[entry.path] = entry;
	}
};

} // namespace utils
} // namespace core
} // namespace shv
