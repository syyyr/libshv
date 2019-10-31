#pragma once

#include "../shvvisuglobal.h"

#include <QString>
#include <QMap>

namespace shv {
namespace visu {
namespace svgscene {

struct SHVVISU_DECL_EXPORT Types
{
	struct SHVVISU_DECL_EXPORT DataKey
	{
		enum {XmlAttributes = 1};
	};

	using XmlAttributes = QMap<QString, QString>;
	using CssAttributes = QMap<QString, QString>;

	static const QString ATTR_ID;
	static const QString ATTR_CHILD_ID;
	static const QString ATTR_SHV_PATH;
	static const QString ATTR_SHV_TYPE;
};

}}}
