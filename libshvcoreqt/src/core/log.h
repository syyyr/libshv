#pragma once

#include "../shvcoreqtglobal.h"
#include <shv/core/log.h>

#include <QString>
#include <QByteArray>

inline shv::core::ShvLog &operator<<(shv::core::ShvLog log, const QString &s) { return log.operator <<(s.toUtf8().constData()); }
shv::core::ShvLog &operator<<(shv::core::ShvLog log, const QByteArray &s);
