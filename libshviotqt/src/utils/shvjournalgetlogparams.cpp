#include "shvjournalgetlogparams.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace utils {

ShvJournalGetLogParams::ShvJournalGetLogParams(const chainpack::RpcValue &opts)
	: ShvJournalGetLogParams()
{
	const cp::RpcValue::Map m = opts.toMap();
	since = m.value("since", since).toDateTime();
	if(!since.isValid())
		since = m.value("from").toDateTime();
	until = m.value("until", until).toDateTime();
	if(!until.isValid())
		until = m.value("to").toDateTime();
	pathPattern = m.value("pathPattern", pathPattern).toString();
	headerOptions = m.value("headerOptions", headerOptions).toUInt();
	valuesMask = m.value("valuesMask", valuesMask).toUInt();
	maxRecordCount = m.value("maxRecordCount", maxRecordCount).toInt();
	withSnapshot = m.value("withSnapshot", withSnapshot).toBool();
}

chainpack::RpcValue ShvJournalGetLogParams::toRpcValue() const
{
	cp::RpcValue::Map m;
	if(since.isValid())
		m["since"] = since;
	if(until.isValid())
		m["until"] = until;
	if(!pathPattern.empty())
		m["pathPattern"] = pathPattern;
	m["headerOptions"] = headerOptions;
	m["valuesMask"] = valuesMask;
	m["maxRecordCount"] = maxRecordCount;
	m["withSnapshot"] = withSnapshot;
	return chainpack::RpcValue{m};
}

std::vector<unsigned> ShvJournalGetLogParams::requestedValuesIndicies() const
{
	std::vector<unsigned> ret;
	for (unsigned i = 0; i < sizeof (valuesMask); ++i) {
		uint32_t mask = 1 << i;
		if(valuesMask & mask)
			ret.push_back(i);
	}
	return ret;
}


} // namespace utils
} // namespace iotqt
} // namespace shv
