#include "groupitem.h"

#include <QPainter>

namespace shv::visu::svgscene {

GroupItem::GroupItem(QGraphicsItem *parent)
	: Super(parent)
{

}

void GroupItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
	Super::paint(painter, option, widget);
	//painter->setPen(QPen(Qt::red, 10));
	//painter->drawRect(childrenBoundingRect());
	//painter->setPen(QPen(Qt::green, 10));
	//painter->drawRect(boundingRect());
}

} // namespace shv
