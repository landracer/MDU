/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef UNIFIEDDATASTORE_H
#define UNIFIEDDATASTORE_H

#include <QObject>
#include <QReadWriteLock>
#include <QHash>
#include <QDateTime>
#include <QTimer>
#include "UnifiedDataPoint.h"

/**
 * @brief Thread-safe singleton storing ALL unified data points
 * 
 * This is the central nervous system of the application. Every hardware system
 * (MultiDisplay ECU, rAtTrax-BMS, OpenDash Mesh, MDLogger) writes into this store.
 * UI components read from it to display real-time data.
 * 
 * Design principles:
 * - Lock-free reads where possible (QReadWriteLock)
 * - Timestamped values for sync/analysis
 * - Signal-based updates for reactive UI
 * - Extensible: new data points added by enum, not code changes
 */
class UnifiedDataStore : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     */
    static UnifiedDataStore& instance();

    /**
     * @brief Set a data point value with automatic timestamp
     * @param id Data point ID
     * @param value Value to store
     * @param source Optional source identifier (e.g., "MD-ECU", "BMS", "ESP-NOW")
     */
    void setDataPoint(DataPointID id, double value, const QString& source = QString());

    /**
     * @brief Get a data point value (thread-safe)
     * @param id Data point ID
     * @param valid Output: whether value has been set
     * @return Current value (0.0 if not set)
     */
    double getDataPoint(DataPointID id, bool* valid = nullptr) const;

    /**
     * @brief Get the timestamp when a data point was last updated
     */
    QDateTime getTimestamp(DataPointID id) const;

    /**
     * @brief Check if a data point has been set since last reset
     */
    bool isValid(DataPointID id) const;

    /**
     * @brief Reset validity flags (for session tracking)
     */
    void resetValidity();

    /**
     * @brief Get all currently valid data points
     */
    QVector<DataPointID> getValidPoints() const;

    /**
     * @brief Get source identifier for a data point
     */
    QString getSource(DataPointID id) const;

signals:
    /**
     * @brief Emitted when any data point is updated
     * @param id Data point that changed
     * @param value New value
     * @param source Source identifier
     */
    void dataPointUpdated(DataPointID id, double value, const QString& source);

    /**
     * @brief Emitted when a batch of data points is updated (for frame parsing)
     */
    void batchUpdated(const QHash<DataPointID, double>& values, const QString& source);

    /**
     * @brief Emitted periodically for UI refresh throttling
     */
    void refreshTick();

private:
    explicit UnifiedDataStore(QObject* parent = nullptr);
    ~UnifiedDataStore() = default;

    // Prevent copying
    UnifiedDataStore(const UnifiedDataStore&) = delete;
    UnifiedDataStore& operator=(const UnifiedDataStore&) = delete;

    struct DataPointEntry {
        double value = 0.0;
        QDateTime timestamp;
        QString source;
        bool valid = false;
    };

    mutable QReadWriteLock m_lock;
    QHash<DataPointID, DataPointEntry> m_data;
    QTimer* m_refreshTimer;

    void initializeDefaultPoints();
};

#endif // UNIFIEDDATASTORE_H
