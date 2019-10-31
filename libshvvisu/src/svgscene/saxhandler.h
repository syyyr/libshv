#pragma once

#include "types.h"

#include <QPen>
#include <QMap>
#include <QStack>

class QXmlStreamReader;
class QXmlStreamAttributes;
class QGraphicsScene;
class QGraphicsItem;
class QGraphicsSimpleTextItem;
class QGraphicsTextItem;
class QAbstractGraphicsShapeItem;

namespace shv {
namespace visu {
namespace svgscene {

using XmlAttributes = Types::XmlAttributes;
using CssAttributes = Types::CssAttributes;

class SHVVISU_DECL_EXPORT SaxHandler
{
public:
	struct SvgElement
	{
		QString name;
		XmlAttributes xmlAttributes;
		CssAttributes styleAttributes;
		bool itemCreated = false;

		SvgElement() {}
		SvgElement(const QString &n, bool created = false) : name(n), itemCreated(created) {}
	};
public:
	SaxHandler(QGraphicsScene *scene);
	virtual ~SaxHandler();

	void load(QXmlStreamReader *data);
protected:
	virtual QGraphicsItem *createGroupItem(const SvgElement &el);
	virtual void createVisuController(QGraphicsItem *it, const SvgElement &el);
	virtual void setXmlAttributes(QGraphicsItem *git, const SvgElement &el);
private:
	void parse();
	XmlAttributes parseXmlAttributes(const QXmlStreamAttributes &attributes);
	void mergeCSSAttributes(CssAttributes &css_attributes, const QString &attr_name, const XmlAttributes &xml_attributes);

	void setTransform(QGraphicsItem *it, const QString &str_val);
	void setStyle(QAbstractGraphicsShapeItem *it, const CssAttributes &attributes);
	void setTextStyle(QFont &font, const CssAttributes &attributes);
	void setTextStyle(QGraphicsSimpleTextItem *text, const CssAttributes &attributes);
	void setTextStyle(QGraphicsTextItem *text, const CssAttributes &attributes);

	bool startElement();
	void addItem(QGraphicsItem *it);
private:
	QStack<SvgElement> m_elementStack;

	QGraphicsScene *m_scene;
	//QGraphicsItemGroup *m_topLevelGroup = nullptr;
	QGraphicsItem *m_topLevelItem = nullptr;
	QXmlStreamReader *m_xml = nullptr;
	QPen m_defaultPen;
};

}}}

Q_DECLARE_METATYPE(shv::visu::svgscene::XmlAttributes)
