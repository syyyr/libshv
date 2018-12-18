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
	m["maxRecordCount"] = maxRecordCount;
	m["withSnapshot"] = withSnapshot;
	return chainpack::RpcValue{m};
}


} // namespace utils
} // namespace iotqt
} // namespace shv
