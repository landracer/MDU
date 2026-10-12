/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "UnitConverter.h"
#include <QSettings>

UnitConverter& UnitConverter::instance() {
    static UnitConverter inst;
    return inst;
}

UnitConverter::UnitConverter() {
    // First entry of each family is the canonical unit (scale 1, offset 0).
    // display = canonical * scale + offset
    m_families[QStringLiteral("°C")] = {
        { QStringLiteral("°C"), 1.0, 0.0 },
        { QStringLiteral("°F"), 1.8, 32.0 },
        { QStringLiteral("K"),  1.0, 273.15 },
    };
    m_families[QStringLiteral("km/h")] = {
        { QStringLiteral("km/h"), 1.0, 0.0 },
        { QStringLiteral("mph"),  0.621371, 0.0 },
        { QStringLiteral("m/s"),  1.0 / 3.6, 0.0 },
    };
    m_families[QStringLiteral("mph")] = {
        { QStringLiteral("mph"),  1.0, 0.0 },
        { QStringLiteral("km/h"), 1.609344, 0.0 },
    };
    m_families[QStringLiteral("kPa")] = {
        { QStringLiteral("kPa"), 1.0, 0.0 },
        { QStringLiteral("PSI"), 0.145038, 0.0 },
        { QStringLiteral("bar"), 0.01, 0.0 },
    };
    // Stoichiometric gasoline: AFR = λ × 14.7
    m_families[QStringLiteral("λ")] = {
        { QStringLiteral("λ"),   1.0, 0.0 },
        { QStringLiteral("AFR"), 14.7, 0.0 },
    };
    m_families[QStringLiteral("AFR")] = {
        { QStringLiteral("AFR"), 1.0, 0.0 },
        { QStringLiteral("λ"),   1.0 / 14.7, 0.0 },
    };
    m_families[QStringLiteral("meters")] = {
        { QStringLiteral("meters"), 1.0, 0.0 },
        { QStringLiteral("ft"),     3.28084, 0.0 },
    };
    m_families[QStringLiteral("g")] = {
        { QStringLiteral("g"),     1.0, 0.0 },
        { QStringLiteral("m/s²"),  9.80665, 0.0 },
    };
    m_families[QStringLiteral("m/s²")] = {
        { QStringLiteral("m/s²"), 1.0, 0.0 },
        { QStringLiteral("g"),    1.0 / 9.80665, 0.0 },
    };
    m_families[QStringLiteral("ms")] = {
        { QStringLiteral("ms"), 1.0, 0.0 },
        { QStringLiteral("s"),  0.001, 0.0 },
    };
    m_families[QStringLiteral("mV")] = {
        { QStringLiteral("mV"), 1.0, 0.0 },
        { QStringLiteral("V"),  0.001, 0.0 },
    };

    // Restore persisted selections
    QSettings settings;
    for (auto it = m_families.constBegin(); it != m_families.constEnd(); ++it) {
        const QString saved = settings.value(settingsKey(it.key())).toString();
        if (saved.isEmpty()) continue;
        for (int i = 0; i < it.value().size(); ++i) {
            if (it.value()[i].name == saved) {
                m_selected[it.key()] = i;
                break;
            }
        }
    }
}

QString UnitConverter::settingsKey(const QString& canonicalUnit) {
    QString sanitized = canonicalUnit;
    sanitized.replace('/', '_');   // '/' is the QSettings group separator
    return QStringLiteral("units/") + sanitized;
}

const QVector<UnitConverter::UnitOption>* UnitConverter::family(const QString& canonicalUnit) const {
    auto it = m_families.constFind(canonicalUnit);
    return (it != m_families.constEnd()) ? &it.value() : nullptr;
}

int UnitConverter::selectedIndex(const QString& canonicalUnit) const {
    return m_selected.value(canonicalUnit, 0);
}

bool UnitConverter::isConvertible(const QString& canonicalUnit) const {
    return family(canonicalUnit) != nullptr;
}

QStringList UnitConverter::familyUnits(const QString& canonicalUnit) const {
    QStringList out;
    if (const auto* fam = family(canonicalUnit)) {
        for (const auto& opt : *fam) out << opt.name;
    }
    return out;
}

QString UnitConverter::displayUnit(const QString& canonicalUnit) const {
    if (const auto* fam = family(canonicalUnit)) {
        return (*fam)[selectedIndex(canonicalUnit)].name;
    }
    return canonicalUnit;
}

double UnitConverter::toDisplay(const QString& canonicalUnit, double value) const {
    if (const auto* fam = family(canonicalUnit)) {
        const UnitOption& opt = (*fam)[selectedIndex(canonicalUnit)];
        return value * opt.scale + opt.offset;
    }
    return value;
}

QString UnitConverter::nextUnit(const QString& canonicalUnit) const {
    if (const auto* fam = family(canonicalUnit)) {
        const int next = (selectedIndex(canonicalUnit) + 1) % fam->size();
        return (*fam)[next].name;
    }
    return canonicalUnit;
}

QString UnitConverter::cycleUnit(const QString& canonicalUnit) {
    const auto* fam = family(canonicalUnit);
    if (!fam) return canonicalUnit;

    const int next = (selectedIndex(canonicalUnit) + 1) % fam->size();
    m_selected[canonicalUnit] = next;

    QSettings settings;
    settings.setValue(settingsKey(canonicalUnit), (*fam)[next].name);

    emit unitsChanged(canonicalUnit);
    return (*fam)[next].name;
}

void UnitConverter::setDisplayUnit(const QString& canonicalUnit, const QString& unitName) {
    const auto* fam = family(canonicalUnit);
    if (!fam) return;

    for (int i = 0; i < fam->size(); ++i) {
        if ((*fam)[i].name == unitName) {
            if (m_selected.value(canonicalUnit, 0) != i) {
                m_selected[canonicalUnit] = i;
                QSettings settings;
                settings.setValue(settingsKey(canonicalUnit), unitName);
                emit unitsChanged(canonicalUnit);
            }
            return;
        }
    }
}
