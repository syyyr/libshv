#pragma once

#include "graphbuttonbox.h"
#include "sample.h"
#include "../shvvisuglobal.h"

#include <shv/coreqt/utils.h>

#include <QObject>
#include <QVector>
#include <QVariantMap>
#include <QColor>
#include <QFont>
#include <QPixmap>
#include <QRect>

namespace shv {
namespace visu {
namespace timeline {

class GraphModel;

class SHVVISU_DECL_EXPORT Graph : public QObject
{
	Q_OBJECT
public:
	struct SHVVISU_DECL_EXPORT DataRect
	{
		XRange xRange;
		YRange yRange;

		DataRect() {}
		DataRect(const XRange &xr, const YRange &yr) : xRange(xr), yRange(yr) {}

		bool isValid() const { return xRange.isValid() && yRange.isValid(); }
		//bool isNull() const { return xRange.isNull() && yRange.isNull(); }
	};

	using WidgetRange = Range<int>;

	class SHVVISU_DECL_EXPORT GraphStyle : public QVariantMap
	{
		SHV_VARIANTMAP_FIELD2(int, u, setU, nitSize, 20) // px
		SHV_VARIANTMAP_FIELD2(double, h, setH, eaderInset, 0.2) // units
		SHV_VARIANTMAP_FIELD2(double, b, setB, uttonSize, 1.2) // units
		SHV_VARIANTMAP_FIELD2(double, l, setL, eftMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, r, setR, ightMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, t, setT, opMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, b, setb, ottomMargin, 0.3) // units
		SHV_VARIANTMAP_FIELD2(double, c, setC, hannelSpacing, 0.1) // units
		SHV_VARIANTMAP_FIELD2(double, x, setX, AxisHeight, 1.5) // units
		SHV_VARIANTMAP_FIELD2(double, y, setY, AxisWidth, 2.5) // units
		SHV_VARIANTMAP_FIELD2(double, m, setM, iniMapHeight, 2) // units
		SHV_VARIANTMAP_FIELD2(double, v, setV, erticalHeaderWidth, 10) // units
		SHV_VARIANTMAP_FIELD2(bool, s, setS, eparateChannels, true)

		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olor, QColor("#c8c8c8"))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorPanel, QColor("#414343"))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorBackground, QColor(Qt::black))
		//SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorGrid, QColor(Qt::darkGreen))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorAxis, QColor(Qt::gray))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorCrossBar1, QColor(QStringLiteral("white")))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorCrossBar2, QColor(QStringLiteral("salmon")))
		SHV_VARIANTMAP_FIELD2(QColor, c, setC, olorSelection, QColor(QStringLiteral("deepskyblue")))

		SHV_VARIANTMAP_FIELD(QFont, f, setF, ont)
	public:
		void init(QWidget *widget);

		//double buttonSpacing() const { return buttonSize() / 5; }
	};
	class SHVVISU_DECL_EXPORT ChannelStyle : public QVariantMap
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

		//SHV_VARIANTMAP_FIELD(QFont, f, setF, ont)

	public:
		ChannelStyle() : QVariantMap() {}
		ChannelStyle(const QVariantMap &o) : QVariantMap(o) {}

		static constexpr double HEIGHT_HUGE = 10e3;

		double heightRange() const
		{
			if(!heightMax_isset())
				return HEIGHT_HUGE;
			double ret = heightMax() - heightMin();
			return ret < 0? 0: ret;
		}
	};

	class SHVVISU_DECL_EXPORT Channel
	{
		friend class Graph;
	public:
		Channel(Graph *graph)
			: m_graph(graph)
			, m_buttonBox(new GraphButtonBox({GraphButtonBox::ButtonId::Hide, GraphButtonBox::ButtonId::Properties}, graph))
		{}
		~Channel() {if(m_buttonBox) delete m_buttonBox;}

		void setMetaTypeId(int id) { m_metaTypeId = id; }
		int metaTypeId() const { return m_metaTypeId; }

		inline int modelIndex() const {return m_modelIndex;}
		void setModelIndex(int ix) {m_modelIndex = ix;}

		YRange yRange() const { return m_state.yRange; }
		YRange yRangeZoom() const { return m_state.yRangeZoom; }

		const ChannelStyle& style() const {return m_style;}
		void setStyle(const ChannelStyle& st) { m_style = st; }
		ChannelStyle effectiveStyle;

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
	protected:
		Graph *m_graph;
		GraphButtonBox *m_buttonBox = nullptr;
		struct
		{
			YRange yRange;
			YRange yRangeZoom;
			YAxis axis;
		} m_state;

		struct
		{
			//QRect rect;
			QRect graphRect;
			QRect dataAreaRect;
			QRect verticalHeaderRect;
			QRect yAxisRect;
		} m_layout;

		ChannelStyle m_style;
		int m_modelIndex = 0;
		int m_metaTypeId = 0;
		int m_buttonCount = 2;
	};

	//SHV_FIELD_BOOL_IMPL2(a, A, utoCreateChannels, true)
public:
	Graph(QObject *parent = nullptr);
	virtual ~Graph();

	const GraphStyle& effectiveStyle() const { return  m_effectiveStyle; }

	void setModel(GraphModel *model);
	GraphModel *model() const;

	struct SHVVISU_DECL_EXPORT CreateChannelsOptions
	{
		std::string pathPattern;
		enum class PathPatternFormat {Substring, Regex};
		PathPatternFormat pathPatternFormat = PathPatternFormat::Substring;
		bool hideConstant = false;

		CreateChannelsOptions(const std::string &pattern = std::string(), PathPatternFormat fmt = PathPatternFormat::Substring)
			: pathPattern(pattern)
			, pathPatternFormat(fmt)
		{}
	};
	void createChannelsFromModel(const CreateChannelsOptions &opts = CreateChannelsOptions());

	int channelCount() const { return  m_channels.count(); }
	void clearChannels();
	Channel& appendChannel(int model_index = -1);
	Channel& channelAt(int ix);
	const Channel& channelAt(int ix) const;
	DataRect dataRect(int channel_ix) const;

	timemsec_t miniMapPosToTime(int pos) const;
	int miniMapTimeToPos(timemsec_t time) const;

	timemsec_t posToTime(int pos) const;
	int timeToPos(timemsec_t time) const;
	Sample timeToSample(int channel_ix, timemsec_t time) const;
	int posToChannel(const QPoint &pos) const;
	Sample posToData(const QPoint &pos) const;
	//QVariant posToValue(const QPoint &pos) const;
	QPoint dataToPos(int ch_ix, const Sample &s) const;

	const QRect& rect() const { return  m_layout.rect; }
	const QRect& miniMapRect() const { return  m_layout.miniMapRect; }
	QRect southFloatingBarRect() const;
	QPoint crossBarPos1() const {return m_state.crossBarPos1;}
	QPoint crossBarPos2() const {return m_state.crossBarPos2;}
	void setCrossBarPos1(const QPoint &pos);
	void setCrossBarPos2(const QPoint &pos);

	void setSelectionRect(const QRect &rect);

	XRange xRange() const { return m_state.xRange; }
	XRange xRangeZoom() const { return m_state.xRangeZoom; }
	void setXRange(const XRange &r, bool keep_zoom = false);
	void setXRangeZoom(const XRange &r);

	void setYRange(int channel_ix, const YRange &r);
	void enlargeYRange(int channel_ix, double step);
	void setYRangeZoom(int channel_ix, const YRange &r);
	void resetZoom(int channel_ix);
	void zoomToSelection();

	const GraphStyle& style() const { return m_style; }
	void setStyle(const GraphStyle &st);
	void setDefaultChannelStyle(const ChannelStyle &st);
	ChannelStyle defaultChannelStyle() const { return m_defaultChannelStyle; }

	void makeLayout(const QRect &rect);
	void draw(QPainter *painter, const QRect &dirty_rect, const QRect &view_rect);

	int u2px(double u) const;
	double px2u(int px) const;

	static std::function<QPoint (const Sample&)> dataToPointFn(const DataRect &src, const QRect &dest);
	static std::function<Sample (const QPoint &)> pointToDataFn(const QRect &src, const DataRect &dest);
	static std::function<timemsec_t (int)> posToTimeFn(const QPoint &src, const XRange &dest);
	static std::function<int (timemsec_t)> timeToPosFn(const XRange &src, const WidgetRange &dest);
	static std::function<int (double)> valueToPosFn(const YRange &src, const WidgetRange &dest);
	static std::function<double (int)> posToValueFn(const WidgetRange &src, const YRange &dest);

	Q_SIGNAL void presentationDirty(const QRect &rect);
	void emitPresentationDirty(const QRect &rect) { emit presentationDirty(rect); }
protected:
	void sanityXRangeZoom();
	//void onModelXRangeChanged(const timeline::XRange &range);

	void drawRectText(QPainter *painter, const QRect &rect, const QString &text, const QFont &font, const QColor &color, const QColor &background = QColor());

	void drawBackground(QPainter *painter, const QRect &dirty_rect);
	virtual void drawCornerCell(QPainter *painter);
	virtual void drawMiniMap(QPainter *painter);
	virtual void drawXAxis(QPainter *painter);

	virtual void drawVerticalHeader(QPainter *painter, int channel);
	virtual void drawBackground(QPainter *painter, int channel);
	virtual void drawGrid(QPainter *painter, int channel);
	virtual void drawYAxis(QPainter *painter, int channel);
	virtual void drawSamples(QPainter *painter, int channel_ix
			, const DataRect &src_rect = DataRect()
			, const QRect &dest_rect = QRect()
			, const ChannelStyle &channel_style = ChannelStyle());
	virtual void drawCrossBar(QPainter *painter, int channel_ix, const QPoint &crossbar_pos, const QColor &color);
	virtual void drawSelection(QPainter *painter);

	QVariantMap mergeMaps(const QVariantMap &base, const QVariantMap &overlay) const;
	void makeXAxis();
	void makeYAxis(int channel);

	void moveSouthFloatingBarBottom(int bottom);
protected:
	GraphModel *m_model = nullptr;

	GraphStyle m_effectiveStyle;

	GraphStyle m_style;
	ChannelStyle m_defaultChannelStyle;

	QVector<Channel*> m_channels;

	struct SHVVISU_DECL_EXPORT XAxis {
		enum class LabelFormat {MSec, Sec, Min, Hour, Day, Month, Year};
		timemsec_t tickInterval = 0;
		int subtickEvery = 1;
		LabelFormat labelFormat = LabelFormat::MSec;

		XAxis() {}
		XAxis(timemsec_t t, int se, LabelFormat f)
			: tickInterval(t)
			, subtickEvery(se)
			, labelFormat(f)
		{}
	};
	struct
	{
		XRange xRange;
		XRange xRangeZoom;
		QPoint crossBarPos1;
		QPoint crossBarPos2;
		QRect selectionRect;
		XAxis axis;
	} m_state;

	struct
	{
		QRect rect;
		QRect miniMapRect;
		QRect xAxisRect;
		QRect cornerCellRect;
	} m_layout;

	QPixmap m_miniMapCache;
};

}}}
