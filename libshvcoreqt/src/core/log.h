#pragma once

#include "../shvcoreqtglobal.h"
#include <shv/core/log.h>

#include <QString>
#include <QByteArray>

inline NecroLog &operator<<(NecroLog log, const QString &s) { return log.operator <<(s.toUtf8().constData()); }
NecroLog &operator<<(NecroLog log, const QByteArray &s);
