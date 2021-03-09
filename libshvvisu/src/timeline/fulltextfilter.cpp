#include "fulltextfilter.h"

#include <QRegularExpression>

namespace shv {
namespace visu {
namespace timeline {

FullTextFilter::FullTextFilter()
{
}

const QString &FullTextFilter::pattern() const
{
	return m_pattern;
}

void FullTextFilter::setPattern(const QString &pattern)
{
	m_pattern = pattern;
}

bool FullTextFilter::isCaseSensitive() const
{
	return m_isCaseSensitive;
}

void FullTextFilter::setCaseSensitive(bool case_sensitive)
{
	m_isCaseSensitive = case_sensitive;
}

bool FullTextFilter::isRegularExpression() const
{
	return m_isRegularExpression;
}

void FullTextFilter::setRegularExpression(bool regular_expression)
{
	m_isRegularExpression = regular_expression;
}

bool FullTextFilter::matches(const QString &value) const
{
	if (m_pattern.isEmpty()) {
		return true;
	}
	if (m_isRegularExpression) {
		QRegularExpression::PatternOptions options = QRegularExpression::DontCaptureOption;
		if (!m_isCaseSensitive) {
			options |= QRegularExpression::CaseInsensitiveOption;
		}
		QRegularExpression reg(m_pattern, options);
		QRegularExpressionMatch match = reg.match(value, 0, QRegularExpression::PartialPreferFirstMatch);
		return match.hasMatch();
	}
	else {
		if (m_isCaseSensitive) {
			return value.contains(m_pattern);
		}
		else {
			return value.toLower().contains(m_pattern.toLower());
		}
	}
}

}
}
}
