/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef UNITCONVERTER_H
#define UNITCONVERTER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>
#include <QHash>

/**
 * @brief Central unit conversion service.
 *
 * All data is stored internally in canonical (SI-ish) units as delivered by
 * the parsers (°C, km/h, kPa, λ, meters, ...). This class converts values to
 * the user's preferred display unit and lets the user cycle through the
 * available units of a family (e.g. °C → °F, kPa → PSI → bar, λ → AFR).
 *
 * The selected unit per family is persisted in QSettings and applies
 * app-wide (LiveDataTable, RealtimePlot, ...). unitsChanged is emitted on
 * every change so views can refresh.
 */
class UnitConverter : public QObject {
    Q_OBJECT

public:
    struct UnitOption {
        QString name;    // display unit label
        double scale;    // display = canonical * scale + offset
        double offset;
    };

    static UnitConverter& instance();

    /// True if the canonical unit belongs to a convertible family
    bool isConvertible(const QString& canonicalUnit) const;

    /// All unit labels of the family (first entry = canonical)
    QStringList familyUnits(const QString& canonicalUnit) const;

    /// Currently selected display unit label (canonicalUnit itself if not convertible)
    QString displayUnit(const QString& canonicalUnit) const;

    /// Convert a canonical value to the currently selected display unit
    double toDisplay(const QString& canonicalUnit, double value) const;

    /// The unit that cycleUnit() would switch to next
    QString nextUnit(const QString& canonicalUnit) const;

    /// Cycle to the next unit in the family; returns the new display unit
    QString cycleUnit(const QString& canonicalUnit);

    /// Explicitly select a display unit within the family
    void setDisplayUnit(const QString& canonicalUnit, const QString& unitName);

signals:
    /// Emitted when the display unit of a family changed
    void unitsChanged(const QString& canonicalUnit);

private:
    UnitConverter();
    Q_DISABLE_COPY(UnitConverter)

    const QVector<UnitOption>* family(const QString& canonicalUnit) const;
    int selectedIndex(const QString& canonicalUnit) const;
    static QString settingsKey(const QString& canonicalUnit);

    QHash<QString, QVector<UnitOption>> m_families;  // canonical unit → options
    QHash<QString, int> m_selected;                  // canonical unit → index into family
};

#endif // UNITCONVERTER_H
