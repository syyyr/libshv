#ifndef SHV_VISU_TIMELINE_GRAPHCHANNEL_H
#define SHV_VISU_TIMELINE_GRAPHCHANNEL_H

#include "graphbuttonbox.h"
#include "sample.h"

#include "../shvvisuglobal.h"

#include <shv/coreqt/utils.h>

#include <QVariantMap>
#include <QColor>
#include <QRect>
#include <QObject>

namespace shv {
namespace visu {
namespace timeline {

class Graph;
class GraphButtonBox;

class SHVVISU_DECL_EXPORT GraphChannel : public QObject
{
	Q_OBJECT

	friend class Graph;
public:
	class SHVVISU_DECL_EXPORT Style : public QVariantMap
	{
	public:
		struct Interpolation { enum Enum {None = 0, Line, Stepped};};
		struct LineAreaStyle { enum Enum {Blank = 0, Filled};};
		static constexpr double CosmeticLineWidth = 0;

		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMin, 2.5) // units
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMax, 1000) // units
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::magenta))
		//SHV_VARIANTMAP_FIELD(QColor, c, setC, olorLineArea)
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::darkGreen))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorAxis, QColor(Qt::gray))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor("#232323"))

		SHV_VARIANTMAP_FIELD2(int, i, setI, nterpolation, Interpolation::Stepped)
		SHV_VARIANTMAP_FIELD2(int, l, setL, ineAreaStyle, LineAreaStyle::Blank)
		SHV_VARIANTMAP_FIELD2(double, l, setL, ineWidth, 0.3)

		SHV_VARIANTMAP_FIELD(bool, is, set, Hidden)

	public:
		Style() : QVariantMap() {}
		Style(const QVariantMap &o) : QVariantMap(o) {}

		static constexpr double HEIGHT_HUGE = 10e3;

		double heightRange() const
		{
			if(!heightMax_isset())
				return HEIGHT_HUGE;
			double ret = heightMax() - heightMin();
			return ret < 0? 0: ret;
		}
	};
public:
	GraphChannel(Graph *graph);
	~GraphChannel() {}

	void setMetaTypeId(int id) { m_metaTypeId = id; }
	int metaTypeId() const { return m_metaTypeId; }

	inline int modelIndex() const {return m_modelIndex;}
	void setModelIndex(int ix) {m_modelIndex = ix;}

	YRange yRange() const { return m_state.yRange; }
	YRange yRangeZoom() const { return m_state.yRangeZoom; }

	const Style& style() const {return m_style;}
	void setStyle(const Style& st) { m_style = st; }
	Style effectiveStyle() const { return m_effectiveStyle; }

	const QRect& graphRect() const { return  m_layout.graphRect; }
	const QRect& dataAreaRect() const { return  m_layout.dataAreaRect; }
	const QRect& verticalHeaderRect() const { return  m_layout.verticalHeaderRect; }
	const QRect& yAxisRect() const { return  m_layout.yAxisRect; }

	int valueToPos(double val) const;
	double posToValue(int y) const;

	struct YAxis {
		double tickInterval = 0;
		int subtickEvery = 1;
	};

	const GraphButtonBox *buttonBox() const { return m_buttonBox; }
	GraphButtonBox *buttonBox() { return m_buttonBox; }

	void setVisible(bool b);
	bool isVisible() const;

	bool isMaximized() const { return m_state.isMaximized; }
	void setMaximized(bool b) { m_state.isMaximized = b; }

	Graph *graph() const;
protected:
	void onButtonBoxClicked(int button_id);
	int graphChannelIndex() const;
protected:
	GraphButtonBox *m_buttonBox = nullptr;
	struct
	{
		YRange yRange;
		YRange yRangeZoom;
		YAxis axis;
		bool isMaximized = false;
	} m_state;

	struct
	{
		QRect graphRect;
		QRect dataAreaRect;
		QRect verticalHeaderRect;
		QRect yAxisRect;
	} m_layout;

	Style m_style;
	Style m_effectiveStyle;
	int m_modelIndex = 0;
	int m_metaTypeId = 0;
};

} // namespace timeline
} // namespace visu
} // namespace shv

#endif // SHV_VISU_TIMELINE_GRAPHCHANNEL_H
