#include "visucontroller.h"
#include "types.h"

#include <shv/coreqt/log.h>

#include <QPainter>

namespace shv {
namespace visu {
namespace svgscene {
/*
static std::string transform2string(const QTransform &t)
{
	std::string ret;
	ret += std::to_string(t.m11()) + '\t';
	ret += std::to_string(t.m12()) + '\t';
	ret += std::to_string(t.m13()) + '\n';

	ret += std::to_string(t.m21()) + '\t';
	ret += std::to_string(t.m22()) + '\t';
	ret += std::to_string(t.m23()) + '\n';

	ret += std::to_string(t.m31()) + '\t';
	ret += std::to_string(t.m32()) + '\t';
	ret += std::to_string(t.m33()) + '\n';

	return ret;
}
*/
//===========================================================================
// VisuController
//===========================================================================
VisuController::VisuController(QGraphicsItem *graphics_item, QObject *parent)
	: Super(parent)
	, m_graphicsItem(graphics_item)
{
	XmlAttributes attrs = qvariant_cast<Types::XmlAttributes>(graphics_item->data(Types::DataKey::XmlAttributes));
	setId(attrs.value(Types::ATTR_ID));
	setObjectName(id());
	setShvType(attrs.value(Types::ATTR_SHV_TYPE));
	setShvPath(attrs.value(Types::ATTR_SHV_PATH));
	for(auto key : attrs.keys())
		shvDebug() << key << "->" << attrs.value(key);
}

}}}

