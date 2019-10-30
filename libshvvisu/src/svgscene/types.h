#pragma once

#include "../shvvisuglobal.h"

#include <QString>

namespace shv {
namespace visu {
namespace svgscene {

struct SHVVISU_DECL_EXPORT Types
{
	struct SHVVISU_DECL_EXPORT DataKey
	{
		enum {XmlAttributes = 1};
	};

	static const QString ATTR_CHILD_ID;
	static const QString ATTR_SHV_PATH;
	static const QString ATTR_SHV_TYPE;
};

}}}
