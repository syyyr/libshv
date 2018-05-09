#include "serie.h"

#include "backgroundstripe.h"
#include "view.h"
#include "outsideseriegroup.h"

#include <shv/core/exception.h>

namespace shv {
namespace gui {
namespace graphview {

Serie::Serie(ValueType type, int serie_index, const QString &name, const QColor &color, QObject *parent)
	: QObject(parent)
	, m_name(name)
	, m_type(type)
	, m_color(color)
	, m_serieIndex(serie_index)
{
	View *graph = qobject_cast<View*>(parent);
	if (graph) {
		graph->addSerie(this);
	}
	else {
		Serie *serie = qobject_cast<Serie*>(parent);
		if (serie) {
			serie->addDependentSerie(this);
			m_fill = serie->fill();
			graph = view();
		}
	}
	if (graph) {
		graph->computeDataRange(this);
	}
}

Serie::~Serie()
{
	for (const QMetaObject::Connection &connection : m_connections) {
		disconnect(connection);
	}
}

void Serie::setName(const QString &name)
{
	if (m_name != name) {
		m_name = name;
		update();
	}
}

Serie::YAxis Serie::relatedAxis() const
{
	const Serie *master_serie = masterSerie();
	if (master_serie) {
		return master_serie->m_relatedAxis;
	}
	return YAxis::Y1;
}

void Serie::setRelatedAxis(Serie::YAxis axis)
{
	if (masterSerie() != this) {
		SHV_EXCEPTION("Cannot set color on dependent serie");
	}
	if (m_relatedAxis != axis) {
		m_relatedAxis = axis;
		update();
	}
}

QColor Serie::color() const
{
	const Serie *master_serie = masterSerie();
	if (master_serie) {
		return master_serie->m_color;
	}
	return QColor();
}

void Serie::setColor(const QColor &color)
{
	if (masterSerie() != this) {
		SHV_EXCEPTION("Cannot set color on dependent serie");
	}
	if (m_color != color) {
		m_color = color;
		update();
	}
}

void Serie::addBackgroundStripe(BackgroundStripe *stripe)
{
	if (!m_backgroundStripes.contains(stripe)) {
		m_backgroundStripes.append(stripe);
		stripe->setParent(this);
		m_connections << connect(stripe, &BackgroundStripe::destroyed, [this, stripe]() {
			m_backgroundStripes.removeOne(stripe);
			update();
		});
		update();
	}
}

void Serie::setLineWidth(int width)
{
	if (m_lineWidth != width) {
		m_lineWidth = width;
		update();
	}
}

const QVector<Serie *> &Serie::dependentSeries() const
{
	return m_dependentSeries;
}

void Serie::addDependentSerie(Serie *serie)
{
	if (!m_dependentSeries.contains(serie)) {
		m_dependentSeries.append(serie);
		m_connections << connect(serie, &Serie::destroyed, [this, serie] {
			m_dependentSeries.removeOne(serie);
			update();
		});
		update();
	}
}

void Serie::addToSerieGroup(OutsideSerieGroup *group)
{
	if (!m_serieGroup) {
		m_serieGroup = group;
		m_connections << connect(group, &OutsideSerieGroup::destroyed, [this, group] {
			m_serieGroup = nullptr;
		});
		group->addSerie(this);
	}
}

void Serie::setLineType(Serie::LineType line_type)
{
	if (m_lineType != line_type) {
		m_lineType = line_type;
		update();
	}
}

void Serie::setLegendValueFormatter(std::function<QString (const ValueChange &)> formatter)
{
	m_legendValueFormatter = formatter;
	update();
}

void Serie::setValueFormatter(std::function<ValueChange::ValueY (const ValueChange &)> formatter)
{
	m_valueFormatter = formatter;
	update();
}

void Serie::setBoolValue(double value)
{
	if (m_boolValue != value) {
		m_boolValue = value;
		update();
	}
}

void Serie::setFill(const Serie::Fill &fill)
{
	if ((fill.type() != m_fill.type()) ||
		((fill.type() == Fill::Type::Color  || fill.type() == Fill::Type::Gradient)&& fill.color() != m_fill.color())) {
		m_fill = fill;
		update();
	}
}

void Serie::show(bool enable)
{
	if (enable != m_show) {
		m_show = enable;
		update(true);
		Q_EMIT visibilityChanged();
	}
}

void Serie::hide()
{
	show(false);
}

void Serie::setShowCurrent(bool show)
{
	if (m_showCurrent != show) {
		m_showCurrent = show;
		update();
	}
}

const SerieData &Serie::serieModelData(const View *view) const
{
	return serieModelData(view->model());
}

const SerieData &Serie::serieModelData(const GraphModel *model) const
{
	return model->serieData(m_serieIndex);
}

void Serie::update(bool force)
{
	if (force || m_show) {
		View *graph = view();
		if (graph) {
			graph->computeGeometry();
			graph->update();
		}
	}
}

const Serie *Serie::masterSerie() const
{
	const Serie *master = this;
	Serie *parent_serie = nullptr;
	while ((parent_serie = qobject_cast<Serie*>(master->parent()))) {
		master = parent_serie;
	}
	return master;
}

View *Serie::view() const
{
	return qobject_cast<View*>(masterSerie()->parent());
}

void Serie::paintSerie(QPainter *painter, const QRect &rect, double vertical_zoom, int x_axis_position,
					   shv::gui::SerieData::const_iterator data_begin, shv::gui::SerieData::const_iterator data_end,
					   const QPen &pen, const Serie::Fill &fill_rect, int fill_base) const
{
	if (m_seriePainter) {
		m_seriePainter(painter, rect, vertical_zoom, x_axis_position, this, data_begin, data_end, pen, fill_rect, fill_base);
	}
}

const Serie::SeriePainter &Serie::seriePainter() const
{
	return m_seriePainter;
}

void Serie::setSeriePainter(const SeriePainter &seriePainter)
{
	m_seriePainter = seriePainter;
}

}
}
}
