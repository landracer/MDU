/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef OPENDASHBRIDGE_H
#define OPENDASHBRIDGE_H

#include "ICommunication.h"
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>

/**
 * @brief OpenDash ESP-NOW Bridge communication handler
 * 
 * Connects to the ESP-NOW bridge device (USB CDC-ACM) that sits on the ESP-NOW mesh
 * and relays data to/from the Qt application via serial.
 * 
 * The bridge device appears as a virtual serial port and sends DP-formatted lines:
 *   "DP:0x0400:14.285\n"  (BMS Pack Voltage = 14.285V)
 *   "DP:0x0100:3200.0\n"  (Engine RPM = 3200)
 *   "DP:0x0600:42.5\n"    (VESC Motor Voltage = 42.5V)
 * 
 * Also supports configuration commands FROM Qt TO the bridge:
 *   "CFG:LAYOUT:LEFT:0x0100,0x0102,0x0106\n"  (Set LEFT node layout)
 *   "CFG:OTA:start\n"  (Trigger OTA on node)
 */
class OpenDashBridge : public ICommunication {
    Q_OBJECT

public:
    explicit OpenDashBridge(const QString& portName, QObject* parent = nullptr);
    ~OpenDashBridge() override = default;

    bool open() override;
    void close() override;
    bool isConnected() const override;

    void setPortName(const QString& name);
    QString portName() const { return m_portName; }

public slots:
    /**
     * @brief Process incoming serial data from bridge
     */
    void onReadyRead();

    /**
     * @brief Send a configuration command to the bridge
     * @param command Command string (e.g., "CFG:LAYOUT:LEFT:0x0100,0x0102")
     */
    void sendCommand(const QString& command);

    /**
     * @brief Parse a single DP line from bridge
     */
    void parseDPLine(const QString& line);

signals:
    void frameParsed(const QHash<DataPointID, double>& values);
    void layoutPushed(uint8_t node, const QByteArray& layoutData);
    void otaTriggered(uint8_t node);

private:
    void processDPMessage(const QString& line);
    void processConfigMessage(const QString& line);
    void processBinaryPackets();
    QString stripAnsiCodes(const QString& input);

    QString m_portName;
    QSerialPort* m_serial;
    QByteArray m_receiveBuffer;
};

#endif // OPENDASHBRIDGE_H
