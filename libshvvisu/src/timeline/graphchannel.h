#pragma once

#include "channelprobe.h"
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
		static constexpr double DEFAULT_HEIGHT_MIN = 2.5;

		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMin, DEFAULT_HEIGHT_MIN) // units
		SHV_VARIANTMAP_FIELD2(double, h, setH, eightMax, 1000) // units
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor(Qt::magenta))
		//SHV_VARIANTMAP_FIELD(QColor, c, setC, olorLineArea)
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::darkGreen))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorAxis, QColor(Qt::gray))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor("#232323"))

		SHV_VARIANTMAP_FIELD2(int, i, setI, nterpolation, Interpolation::Stepped)
		SHV_VARIANTMAP_FIELD2(int, l, setL, ineAreaStyle, LineAreaStyle::Blank)
		SHV_VARIANTMAP_FIELD2(double, l, setL, ineWidth, 0.2)

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

	inline int modelIndex() const {return m_modelIndex;}
	void setModelIndex(int ix) {m_modelIndex = ix;}

	QString shvPath() const;

	YRange yRange() const { return m_state.yRange; }
	YRange yRangeZoom() const { return m_state.yRangeZoom; }

	const Style& style() const {return m_style;}
	void setStyle(const Style& st) { m_style = st; }
	Style effectiveStyle() const { return m_effectiveStyle; }

	const QRect& graphAreaRect() const { return  m_layout.graphAreaRect; }
	const QRect& graphDataGridRect() const { return  m_layout.graphDataGridRect; }
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
		QRect graphAreaRect;
		QRect graphDataGridRect;
		QRect verticalHeaderRect;
		QRect yAxisRect;
	} m_layout;

	Style m_style;
	Style m_effectiveStyle;
	int m_modelIndex = 0;
};

} // namespace timeline
} // namespace visu
} // namespace shv

