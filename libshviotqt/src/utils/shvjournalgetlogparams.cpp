#include "shvjournalgetlogparams.h"

namespace cp = shv::chainpack;

namespace shv {
namespace iotqt {
namespace utils {

ShvJournalGetLogParams::ShvJournalGetLogParams(const chainpack::RpcValue &opts)
	: ShvJournalGetLogParams()
{
	const cp::RpcValue::Map m = opts.toMap();
	since = m.value("since", since);
	if(!since.isValid())
		since = m.value("from");
	until = m.value("until", until);
	if(!until.isValid())
		until = m.value("to");
	pathPattern = m.value("pathPattern", pathPattern).toString();
	headerOptions = m.value("headerOptions", headerOptions).toUInt();
	maxRecordCount = m.value("maxRecordCount", maxRecordCount).toInt();
	withSnapshot = m.value("withSnapshot", withSnapshot).toBool();
	withUptime = m.value("withUptime", withUptime).toBool();
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
	m["withUptime"] = withUptime;
	return chainpack::RpcValue{m};
}

} // namespace utils
} // namespace iotqt
} // namespace shv
