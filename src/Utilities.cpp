/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "Utilities.h"
#include <QString>
#include <QDateTime>

namespace MdUtils {

QString formatValue(double value, const QString& unit, int decimals) {
    return QString("%1 %2").arg(value, 0, 'f', decimals).trimmed() + " " + unit.trimmed();
}

QString formatDuration(qint64 milliseconds) {
    if (milliseconds < 0) return "-";
    
    qint64 seconds = milliseconds / 1000;
    qint64 minutes = seconds / 60;
    qint64 hours = minutes / 60;
    qint64 days = hours / 24;
    
    if (days > 0) {
        return QString("%1d %2h %3m").arg(days).arg(hours % 24).arg(minutes % 60);
    } else if (hours > 0) {
        return QString("%1h %2m %3s").arg(hours).arg(minutes % 60).arg(seconds % 60);
    } else if (minutes > 0) {
        return QString("%1m %2s").arg(minutes).arg(seconds % 60);
    } else {
        return QString("%1ms").arg(milliseconds);
    }
}

QString sanitizeDisplay(const QString& input) {
    QString result = input;
    // Remove control characters except newline and tab
    for (int i = 0; i < result.size(); i++) {
        ushort ch = result[i].unicode();
        if (ch < 32 && ch != '\n' && ch != '\t') {
            result.remove(i, 1);
            i--;
        }
    }
    return result;
}

} // namespace MdUtils
