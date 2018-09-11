#include "cpon1.h"

namespace shv {
namespace chainpack1 {

const std::string Cpon::STR_NULL("null");
const std::string Cpon::STR_TRUE("true");
const std::string Cpon::STR_FALSE("false");
const std::string Cpon::STR_IMAP_BEGIN("i{");
//const std::string Cpon::STR_ARRAY_BEGIN("a[");
const std::string Cpon::STR_ESC_BLOB_BEGIN("b\"");
const std::string Cpon::STR_HEX_BLOB_BEGIN("x\"");
const std::string Cpon::STR_DATETIME_BEGIN("d\"");

const char Cpon::C_LIST_BEGIN('[');
const char Cpon::C_LIST_END(']');
const char Cpon::C_MAP_BEGIN('{');
const char Cpon::C_MAP_END('}');
const char Cpon::C_META_BEGIN('<');
const char Cpon::C_META_END('>');
const char Cpon::C_DECIMAL_END('n');
const char Cpon::C_UNSIGNED_END('u');

} // namespace chainpack
} // namespace shv
