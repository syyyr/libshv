#include "visucontroller.h"
#include "types.h"

#include <shv/coreqt/log.h>

#include <QPainter>

namespace shv::visu::svgscene {
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
	//XmlAttributes attrs = qvariant_cast<Types::XmlAttributes>(graphics_item->data(Types::DataKey::XmlAttributes));
	setId(graphics_item->data(Types::DataKey::Id).toString());
	setObjectName(id());
	setShvType(graphics_item->data(Types::DataKey::ShvType).toString());
	setShvPath(graphics_item->data(Types::DataKey::ShvPath).toString());
	//for(auto key : attrs.keys())
	//	shvDebug() << key << "->" << attrs.value(key);
}

QString VisuController::graphicsItemAttributeValue(const QGraphicsItem *it, const QString &attr_name, const QString &default_value)
{
	svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(it->data(Types::DataKey::XmlAttributes));
	return attrs.value(attr_name, default_value);
}

QString VisuController::graphicsItemCssAttributeValue(const QGraphicsItem *it, const QString &attr_name, const QString &default_value)
{
	svgscene::CssAttributes attrs = qvariant_cast<svgscene::CssAttributes>(it->data(Types::DataKey::CssAttributes));
	return attrs.value(attr_name, default_value);
}


}

