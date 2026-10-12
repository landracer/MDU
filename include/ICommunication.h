/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef ICOMMUNICATION_H
#define ICOMMUNICATION_H

#include <QObject>
#include <QString>
#include "UnifiedDataPoint.h"

/**
 * @brief Abstract interface for ALL hardware communication channels
 * 
 * Every data source (USB serial, ESP-NOW bridge, file replay, Bluetooth)
 * implements this interface. The UnifiedDataStore doesn't care where data comes from —
 * it just receives parsed DataPointID → double mappings.
 */
class ICommunication : public QObject {
    Q_OBJECT

public:
    explicit ICommunication(const QString& name, QObject* parent = nullptr)
        : QObject(parent), m_name(name) {}
    virtual ~ICommunication() = default;

    /**
     * @brief Connect to the hardware source
     * @return true if connection successful
     */
    virtual bool open() = 0;

    /**
     * @brief Disconnect from the hardware source
     */
    virtual void close() = 0;

    /**
     * @brief Check if connected and healthy
     */
    virtual bool isConnected() const { return m_connected; }

    /**
     * @brief Get the name/identifier of this communication channel
     */
    QString name() const { return m_name; }

    /**
     * @brief Set the name/identifier
     */
    void setName(const QString& name) { m_name = name; }

signals:
    /**
     * @brief Emitted when raw data is received (for debugging/logging)
     * @param data Raw bytes received
     * @param length Number of bytes
     */
    void rawReceived(const QByteArray& data, int length);

    /**
     * @brief Emitted when a complete frame/message is parsed
     * @param values Parsed data points from this frame
     * @param source Source identifier (usually the channel name)
     */
    void framesParsed(const QHash<DataPointID, double>& values, const QString& source);

    /**
     * @brief Emitted when connection status changes
     * @param connected New connection state
     */
    void connectionStateChanged(bool connected);

    /**
     * @brief Emitted for error reporting
     * @param message Error description
     */
    void errorOccurred(const QString& message);

protected:
    QString m_name;
    bool m_connected = false;
};

#endif // ICOMMUNICATION_H
