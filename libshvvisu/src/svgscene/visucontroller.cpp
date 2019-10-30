#include "visucontroller.h"

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
}

}}}

