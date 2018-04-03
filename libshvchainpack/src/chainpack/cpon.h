#pragma once

#include <string>

namespace shv {
namespace chainpack {

class Cpon
{
public:
	static const std::string STR_NULL;
	static const std::string STR_TRUE;
	static const std::string STR_FALSE;
	static const std::string STR_IMAP_BEGIN;
	static const std::string STR_ARRAY_BEGIN;
	static const std::string STR_ESC_BLOB_BEGIN;
	static const std::string STR_HEX_BLOB_BEGIN;
	static const std::string STR_DATETIME_BEGIN;

	static const char C_LIST_BEGIN;
	static const char C_LIST_END;
	static const char C_MAP_BEGIN;
	static const char C_MAP_END;
	static const char C_META_BEGIN;
	static const char C_META_END;
	static const char C_DECIMAL_END;
	static const char C_UNSIGNED_END;
};

} // namespace chainpack
} // namespace shv
