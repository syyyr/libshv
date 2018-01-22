#include "log.h"
#include <QByteArray>
#include <QString>
#include <QtGlobal>



Q_DECL_CONSTEXPR inline char toHexUpper(uint value) Q_DECL_NOTHROW
{
	return "0123456789ABCDEF"[value & 0xF];
}

inline int fromHex(uint c)
{
	if ((c >= '0') && (c <= '9'))
		return c - '0';
	if ((c >= 'A') && (c <= 'F'))
		return c - 'A' + 10;
	if ((c >= 'a') && (c <= 'f'))
		return c - 'a' + 10;
	return -1;
}

static inline bool isPrintable(uchar c)
{ 
	return c >= ' ' && c <= '~'; 
}

SHVCOREQT_DECL_EXPORT NecroLog &operator<<(NecroLog log, const QByteArray &input)
{
	const char *begin = input.constData();
	int length = input.length();
	char quote(QLatin1Char('"').toLatin1());
	
	log << quote;

	bool lastWasHexEscape = false;
	const char *end = begin + length;
	for (const char *p = begin; p != end; ++p) {
		// check if we need to insert "" to break an hex escape sequence
		if (Q_UNLIKELY(lastWasHexEscape)) {
			if (fromHex(*p) != -1) {
				// yes, insert it
				char quotes[] = "\"\"";
				log << quotes;
			}
			lastWasHexEscape = false;
		}

		if (isPrintable(*p) && *p != '\\' && *p != '"') {
			char c = *p;
			log << c;
			continue;
		}

		// print as an escape sequence (maybe, see below for surrogate pairs)
		int buflen = 2;
		char buf[sizeof "\\U12345678" - 1];
		buf[0] = '\\';

		switch (*p) {
		case '"':
		case '\\':
			buf[1] = *p;
			break;
		case '\b':
			buf[1] = 'b';
			break;
		case '\f':
			buf[1] = 'f';
			break;
		case '\n':
			buf[1] = 'n';
			break;
		case '\r':
			buf[1] = 'r';
			break;
		case '\t':
			buf[1] = 't';
			break;
		default:
			{
				// print as hex escape
				buf[1] = 'x';
				buf[2] = toHexUpper(uchar(*p) >> 4);
				buf[3] = toHexUpper(uchar(*p));
				buflen = 4;
				lastWasHexEscape = true;
				break;
			}
		}
		for (int i = 0; i < buflen; i++)
			log << buf[i];
	}

	return log << quote;
}

