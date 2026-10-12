/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef DATAPointREGISTRY_H
#define DATAPointREGISTRY_H

#include "UnifiedDataPoint.h"
#include <QHash>
#include <QVector>
#include <QString>
#include <functional>

/**
 * @brief Central registry for all unified data points
 * 
 * Provides a single source of truth for:
 * - Validating DataPointID values
 * - Getting metadata (name, unit, category, scaling)
 * - Converting between raw hardware values and SI units
 * - Iterating over all known data points
 * 
 * This is the "dictionary" that every parser and UI component consults.
 */
class DataPointRegistry {
public:
    /**
     * @brief Get the singleton instance
     */
    static DataPointRegistry& instance();

    /**
     * @brief Check if a data point ID is valid
     */
    bool isValid(DataPointID id) const;

    /**
     * @brief Get all registered data point IDs sorted by category and then name
     */
    QVector<DataPointID> allPoints() const;

    /**
     * @brief Get points by category, sorted by name within each category
     */
    QVector<DataPointID> pointsInCategory(const QString& category) const;

    /**
     * @brief Get all registered data point IDs sorted by category and then name
     */
    QVector<DataPointID> allPointsSorted() const;

    /**
     * @brief Get points by category, sorted by name within each category (sorted version)
     */
    QVector<DataPointID> pointsInCategorySorted(const QString& category) const;

    /**
     * @brief Get metadata for a data point
     */
    struct PointInfo {
        DataPointID id;
        QString name;      // Changed from const char* to QString
        QString unit;      // Changed from const char* to QString  
        QString category;  // Changed from const char* to QString
        double rawToSI = 1.0;   // multiplier to convert raw → SI
        double siOffset = 0.0;  // offset to apply after multiplication
        bool signedValue = true;
    };

    PointInfo getInfo(DataPointID id) const;

    /**
     * @brief Convert raw hardware value to SI units
     * @param id Data point ID
     * @param raw Raw value from hardware
     * @return SI value (e.g., RPM, kPa, degrees)
     */
    double toSI(DataPointID id, int16_t raw) const;
    double toSI(DataPointID id, uint16_t raw) const;
    double toSI(DataPointID id, int32_t raw) const;
    double toSI(DataPointID id, uint32_t raw) const;
    double toSI(DataPointID id, float raw) const;
    double toSI(DataPointID id, double raw) const;

    /**
     * @brief Convert SI value back to raw hardware format
     */
    int16_t fromSI(DataPointID id, double si) const;
    uint16_t fromSI_signed(DataPointID id, double si) const;

    /**
     * @brief Get human-readable description
     */
    QString description(DataPointID id) const;

    /**
     * @brief Check if data point is numeric (vs bitmask/enum)
     */
    bool isNumeric(DataPointID id) const;

    /**
     * @brief Check if data point represents a temperature
     */
    bool isTemperature(DataPointID id) const;

    /**
     * @brief Check if data point represents a voltage
     */
    bool isVoltage(DataPointID id) const;

private:
    DataPointRegistry();
    
    void initializeRegistry();
    
    QHash<DataPointID, PointInfo> m_registry;
    QHash<QString, QVector<DataPointID>> m_categories;
};

#endif // DATAPointREGISTRY_H
