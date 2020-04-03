#ifndef SHV_CORE_UTILS_SHVLOGHEADER_H
#define SHV_CORE_UTILS_SHVLOGHEADER_H

#include "../shvcoreglobal.h"

#include "shvlogtypeinfo.h"
#include "shvgetlogparams.h"
#include "../utils.h"

#include <shv/chainpack/rpcvalue.h>

namespace shv {
namespace core {
class StringViewList;
namespace utils {

class ShvGetLogParams;

class SHVCORE_DECL_EXPORT ShvLogHeader //: public shv::chainpack::RpcValue::MetaData
{
	using Super = shv::chainpack::RpcValue::MetaData;

	static const std::string EMPTY_PREFIX_KEY;

	SHV_FIELD_IMPL(std::string, d, D, eviceType)
	SHV_FIELD_IMPL(std::string, d, D, eviceId)
	SHV_FIELD_IMPL2(int, l, L, ogVersion, 2)
	SHV_FIELD_IMPL(ShvGetLogParams, l, L, ogParams)
	SHV_FIELD_IMPL2(int, r, R, ecordCount, 0)
	SHV_FIELD_IMPL2(int, r, R, ecordCountLimit, 0)
	SHV_FIELD_IMPL2(bool, r, R, ecordCountLimitHit, false)
	SHV_FIELD_IMPL2(bool, w, W, ithSnapShot, false)
	SHV_FIELD_IMPL2(bool, w, W, ithPathsDict, true)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue::List, f, F, ields)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue::IMap, p, P, athDict)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, d, D, ateTime)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, s, S, ince)
	SHV_FIELD_IMPL(shv::chainpack::RpcValue, u, U, ntil)
public:
	struct Column
	{
		enum Enum {
			Timestamp = 0,
			Path,
			Value,
			ShortTime,
			Domain,
		};
		static const char* name(Enum e);
	};
public:
	ShvLogHeader() {}

	int64_t sinceMsec() const { return since().toDateTime().msecsSinceEpoch(); }
	int64_t untilMsec() const { return until().toDateTime().msecsSinceEpoch(); }

	static ShvLogHeader fromMetaData(const chainpack::RpcValue::MetaData &md);
	chainpack::RpcValue::MetaData toMetaData() const;

	const std::map<std::string, ShvLogTypeInfo>& sources() const {return m_sources;}
	void setSources(std::map<std::string, ShvLogTypeInfo> &&ss) {m_sources = std::move(ss);}
	void setSources(const std::map<std::string, ShvLogTypeInfo> &ss) {m_sources = ss;}

	const ShvLogTypeInfo& typeInfo(const std::string &path_prefix = EMPTY_PREFIX_KEY) const;
	void setTypeInfo(ShvLogTypeInfo &&ti);
	void setTypeInfo(const ShvLogTypeInfo &ti);
	void setTypeInfo(const std::string &path_prefix, ShvLogTypeInfo &&ti);

	std::map<std::string, shv::core::utils::ShvLogTypeDescr> pathsTypeDescr() const;
	ShvLogTypeDescr::SampleType pathsSampleType(const std::string &path) const;
private:
	std::map<std::string, ShvLogTypeInfo> m_sources;
	mutable std::map<std::string, shv::core::utils::ShvLogTypeDescr> m_pathsTypeDescr;
	mutable bool m_pathsTypeDescrValid = false;
};

} // namespace utils
} // namespace core
} // namespace shv

#endif // SHV_CORE_UTILS_SHVLOGHEADER_H
