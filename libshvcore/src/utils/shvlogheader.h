#ifndef SHV_CORE_UTILS_SHVLOGHEADER_H
#define SHV_CORE_UTILS_SHVLOGHEADER_H

#include "../shvcoreglobal.h"

#include "shvjournalgetlogparams.h"
#include "../utils.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace core {
class StringViewList;
namespace utils {

class ShvJournalGetLogParams;

class SHVCORE_DECL_EXPORT ShvLogHeader //: public shv::chainpack::RpcValue::MetaData
{
	using Super = shv::chainpack::RpcValue::MetaData;

	SHV_FIELD_IMPL(std::string, d, D, eviceType)
	SHV_FIELD_IMPL(std::string, d, D, eviceId)
	SHV_FIELD_IMPL2(int, l, L, ogVersion, 2)
	SHV_FIELD_IMPL(ShvJournalGetLogParams, l, L, ogParams)
	SHV_FIELD_IMPL(int, r, R, ecordCount)
	SHV_FIELD_IMPL(int, r, R, ecordCountLimit)
	SHV_FIELD_IMPL(bool, w, W, ithUptime)
	SHV_FIELD_IMPL(bool, w, W, ithSnapShot)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, f, F, ields)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, p, P, athDict)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, ateTime)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, s, S, ince)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, u, U, ntil)
public:
	ShvLogHeader() {}

	static ShvLogHeader fromMetaData(const chainpack::RpcValue::MetaData &md);
	chainpack::RpcValue::MetaData toMetaData() const;

	void setTypeInfos(const shv::chainpack::RpcValue::Map &ti) {m_typeInfos = ti;}
	void setTypeInfos(shv::chainpack::RpcValue::Map &&ti) {m_typeInfos = std::move(ti);}
	void setTypeInfo(const shv::chainpack::RpcValue &i) {setTypeInfo(std::string(), i);}
	void setTypeInfo(const std::string &path_prefix, const shv::chainpack::RpcValue &i);
private:
	shv::chainpack::RpcValue::Map m_typeInfos;

#if 0
	ShvLogHeader(const Super &super) : Super(super) {}
	ShvLogHeader(Super &&super) : Super(std::move(super)) {}

	std::string deviceId() const;
	void setDeviceId(const std::string &device_id);

	std::string deviceType() const;
	void setDeviceType(const std::string &device_type);

	int logVersion() const;
	void setLogVersion(int log_version);

	shv::chainpack::RpcValue::DateTime dateTime() const;
	void setDateTime(const shv::chainpack::RpcValue::DateTime &dt);

	int recordCount() const;
	void setRecordCount(int rec_cnt);

	int recordCountLimit() const;
	void setRecordCountLimit(int rec_cnt);
private:
	shv::chainpack::RpcValue valueOnPath(const std::string &path) const;
	shv::chainpack::RpcValue valueOnPath(const shv::core::StringViewList &path) const;
	shv::chainpack::RpcValue valueOnPath_helper(const shv::chainpack::RpcValue &parent, const shv::core::StringViewList &path) const;

	void setValueOnPath(const std::string &path, const shv::chainpack::RpcValue &val);
	void setValueOnPath(const shv::core::StringViewList &path, const shv::chainpack::RpcValue &val);
	void setValueOnPath_helper(const shv::chainpack::RpcValue &parent, const shv::core::StringViewList &path, const shv::chainpack::RpcValue &val);
#endif
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGHEADER_H
