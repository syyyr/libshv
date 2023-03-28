#pragma once

#include "shvcoreglobal.h"
#include "stringview.h"

#include <shv/chainpack/rpcvalue.h>

#include <string>
#include <vector>

// do while is to suppress of semicolon warning SHV_SAFE_DELETE(ptr);
#define SHV_SAFE_DELETE(x) do if(x != nullptr) {delete x; x = nullptr;} while(false)

#define SHV_QUOTE(x) #x
#define SHV_EXPAND_AND_QUOTE(x) SHV_QUOTE(x)
//#define SHV_QUOTE_QSTRINGLITERAL(x) QStringLiteral(#x)

#define SHV_FIELD_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	protected: ptype m_##lower_letter##name_rest; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: void set##upper_letter##name_rest(const ptype &val) { m_##lower_letter##name_rest = val; } \
	public: void set##upper_letter##name_rest(ptype &&val) { m_##lower_letter##name_rest = std::move(val); }

#define SHV_FIELD_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	protected: ptype m_##lower_letter##name_rest = default_value; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: void set##upper_letter##name_rest(const ptype &val) { m_##lower_letter##name_rest = val; } \
	public: void set##upper_letter##name_rest(ptype &&val) { m_##lower_letter##name_rest = std::move(val); }
/*
#define SHV_FIELD_BOOL2(name, default_value) \
	protected: bool m_is##name = default_value; \
	public: bool is##name() const {return m_is##name;} \
	public: void set##name(bool val) { m_is##name = val; }

#define SHV_FIELD_BOOL(name) \
	SHV_FIELD_BOOL2(name, false)
*/
#define SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	protected: bool m_##lower_letter##name_rest = default_value; \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: void set##upper_letter##name_rest(bool val) { m_##lower_letter##name_rest = val; }

#define SHV_FIELD_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)



#define SHV_FIELD_CMP_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	protected: ptype m_##lower_letter##name_rest; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	} \
	public: bool set##upper_letter##name_rest(ptype &&val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = std::move(val); return true; } \
		return false; \
	}

#define SHV_FIELD_CMP_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	protected: ptype m_##lower_letter##name_rest = default_value; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##CRef() const {return m_##lower_letter##name_rest;} \
	public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	} \
	public: bool set##upper_letter##name_rest(ptype &&val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = std::move(val); return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_CMP_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	protected: bool m_##lower_letter##name_rest = default_value; \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(bool val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_CMP_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_FIELD_BOOL_CMP_IMPL2(lower_letter, upper_letter, name_rest, false)

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT Utils
{
public:
	static int versionStringToInt(const std::string &version_string);
	static std::string intToVersionString(int ver);

	std::string binaryDump(const std::string &bytes);
	static std::string toHex(const std::string &bytes);
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static std::string fromHex(const std::string &bytes);

	static shv::chainpack::RpcValue foldMap(const shv::chainpack::RpcValue::Map &plain_map, char key_delimiter = '.');

	[[deprecated("use shv::core::utils::joinPath")]] static std::string joinPath(const StringView &p1, const StringView &p2);
	[[deprecated("use shv::core::utils::joinPath")]] static std::string joinPath(const StringViewList &p);
	static std::string simplifyPath(const std::string &p);

	static std::vector<char> readAllFd(int fd);

	template<typename T>
	static T getIntLE(const char *buff, unsigned len)
	{
		using uT = typename std::make_unsigned<T>::type;
		uT val = 0;
		for (unsigned i = len; i > 0; --i) {
			val = val * 256 + static_cast<unsigned char>(buff[i - 1]);
		}
		if(std::is_signed<T>::value && len < sizeof(T)) {
			uT mask = ~0;
			for (unsigned i = 0; i < len; i++)
				mask <<= 8;
			val |= mask;
		}
		return static_cast<T>(val);
	}

	//static shv::chainpack::RpcValue decompressNodesTree(const shv::chainpack::RpcValue &compressed_nodes_tree);

};

namespace utils {
SHVCORE_DECL_EXPORT std::string joinPath(const StringView &p1, const StringView &p2);
SHVCORE_DECL_EXPORT std::string joinPath();
template <typename StringType>
StringType joinPath(const StringType& str)
{
	return str;
}

template <typename HeadStringType, typename... StringTypes>
std::string joinPath(const HeadStringType& head, const StringTypes& ...rest)
{
	return joinPath(StringView(head), StringView(joinPath(rest...)));
}

template <typename Container>
auto findLongestPrefix(const Container& cont, std::string value) -> typename std::remove_reference_t<decltype(cont)>::const_iterator
{
	while (true) {
		if (auto it = cont.find(value); it != cont.end()) {
			return it;
		}

		auto last_slash_pos = value.rfind('/');
		if (last_slash_pos == std::string::npos) {
			break;
		}
		value.erase(last_slash_pos);
	}

	return cont.end();
}
}
}}

