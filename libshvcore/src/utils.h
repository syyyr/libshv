#pragma once

#include "shvcoreglobal.h"

#include <string>
#include <vector>

#if defined LIBC_NEWLIB || defined ANDROID_BUILD
#include <sstream>
#endif

#define SHV_SAFE_DELETE(x) if(x != nullptr) {delete x; x = nullptr;}
/*
#define SHV_CARG(s) "" + QString(s) + ""
#define SHV_SARG(s) "'" + QString(s) + "'"
#define SHV_IARG(i) "" + QString::number(i) + ""
*/
#define SHV_QUOTE(x) #x
#define SHV_EXPAND_AND_QUOTE(x) SHV_QUOTE(x)
//#define SHV_QUOTE_QSTRINGLITERAL(x) QStringLiteral(#x)

//public: ptype& lower_letter##name_rest##Ref() {return m_##lower_letter##name_rest;}

#define SHV_FIELD_IMPL(ptype, lower_letter, upper_letter, name_rest) \
	protected: ptype m_##lower_letter##name_rest; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##Ref() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_IMPL2(ptype, lower_letter, upper_letter, name_rest, default_value) \
	protected: ptype m_##lower_letter##name_rest = default_value; \
	public: ptype lower_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: const ptype& lower_letter##name_rest##Ref() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(const ptype &val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, default_value) \
	protected: bool m_##lower_letter##name_rest = default_value; \
	public: bool is##upper_letter##name_rest() const {return m_##lower_letter##name_rest;} \
	public: bool set##upper_letter##name_rest(bool val) { \
		if(!(m_##lower_letter##name_rest == val)) { m_##lower_letter##name_rest = val; return true; } \
		return false; \
	}

#define SHV_FIELD_BOOL_IMPL(lower_letter, upper_letter, name_rest) \
	SHV_FIELD_BOOL_IMPL2(lower_letter, upper_letter, name_rest, false)

namespace shv {
namespace core {

class SHVCORE_DECL_EXPORT Utils
{
public:
	static std::string removeJsonComments(const std::string &json_str);

	static int versionStringToInt(const std::string &version_string);
	static std::string intToVersionString(int ver);

	std::string binaryDump(const std::string &bytes);
	static std::string toHex(const std::string &bytes);
	static std::string toHex(const std::basic_string<uint8_t> &bytes);
	static std::string fromHex(const std::string &bytes);


	static std::string joinPath(const std::string &p1, const std::string &p2);
	static std::string simplifyPath(const std::string &p);

	static std::vector<char> readAllFd(int fd);

	template<typename T>
	static std::string toString(T i)
	{
#if defined LIBC_NEWLIB || defined ANDROID_BUILD
		std::ostringstream ss;
		ss << i;
		return ss.str();
#else
		return std::to_string(i); //not supported by newlib
#endif
	}

#if 0
	static const QString &nullValueString();
	static void parseFieldName(const QString& full_field_name, QString *pfield_name = NULL, QString *ptable_name = NULL, QString *pdb_name = NULL);
	static QString composeFieldName(const QString &field_name, const QString &table_name = QString(), const QString &db_name = QString());
	/// @returns: True if @a field_name1 ends with @a field_name2. Comparision is case insensitive
	static bool fieldNameEndsWith(const QString &field_name1, const QString &field_name2);
	static bool fieldNameCmp(const QString &fld_name1, const QString &fld_name2);
	static QVariant retypeVariant(const QVariant &_val, int meta_type_id);
	static QVariant retypeStringValue(const QString &str_val, const QString &type_name);

	static int findCaption(const QString &caption_format, int from_ix, QString *caption);
	/**
	 * @brief findCaptions
	 * Finds in string all captions in form {{captionName}}
	 * @param str
	 * @return Set of found captions.
	 */
	static QSet<QString> findCaptions(const QString caption_format);
	static QString replaceCaptions(const QString format_str, const QString &caption_name, const QVariant &caption_value);
	static QString replaceCaptions(const QString format_str, const QVariantMap &replacements);

	/// invoke method of prototype bool method()
	static bool invokeMethod_B_V(QObject *obj, const char *method_name);

	template <typename V, typename... T>
	constexpr static inline auto make_array(T&&... t) -> std::array < V, sizeof...(T) >
	{
		return {{ std::forward<T>(t)... }};
	}
#endif
};

}}

