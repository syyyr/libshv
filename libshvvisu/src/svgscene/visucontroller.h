#pragma once 

#include "saxhandler.h"
#include "types.h"

#include <shv/core/utils.h>

#include <QObject>
#include <QGraphicsItem>

namespace shv {
namespace visu {
namespace svgscene {
	
class SHVVISU_DECL_EXPORT VisuController : public QObject
{
	Q_OBJECT

	using Super = QObject;

	SHV_FIELD_IMPL(QString, i, I, d)
	SHV_FIELD_IMPL(QString, s, S, hvType)
	SHV_FIELD_IMPL(QString, s, S, hvPath)
public:
	VisuController(QGraphicsItem *graphics_item, QObject *parent = nullptr);
protected:
	template<typename T>
	T findChildGraphicsItem(const QString &attr_name = QString(), const QString &attr_value = QString()) const
	{
		return findChildGraphicsItem<T>(m_graphicsItem, attr_name, attr_value);
	}

	template<typename T>
	static T findChildGraphicsItem(const QGraphicsItem *parent_it, const QString &attr_name = QString(), const QString &attr_value = QString())
	{
		if(!parent_it)
			return nullptr;
		for(QGraphicsItem *it : parent_it->childItems()) {
			if(T tit = dynamic_cast<T>(it)) {
				if(attr_name.isEmpty()) {
					return tit;
				}
				else {
					svgscene::XmlAttributes attrs = qvariant_cast<svgscene::XmlAttributes>(tit->data(Types::DataKey::XmlAttributes));
					if(attrs.value(attr_name) == attr_value)
						return tit;
				}
			}
			T tit = findChildGraphicsItem<T>(it, attr_name, attr_value);
			if(tit)
				return tit;
		}
		return nullptr;
	}
protected:
	QGraphicsItem *m_graphicsItem;
};

}}}

