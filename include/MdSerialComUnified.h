/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file MdSerialComUnified.h
 * @brief MultiDisplay ECU (ATmega2560) serial transport.
 *
 * Thin QSerialPort wrapper around the pure MdFrameDecoder state machine, so the
 * wire protocol is unit-testable without hardware. Framing contract: STX(0x02) |
 * TAG | payload | ETX(0x03), where the TAG byte's decimal value IS the total
 * frame length in bytes (main sensor tag 0x5F = 95 → 92-byte payload).
 */
#ifndef MD_SERIALCOM_UNIFIED_H
#define MD_SERIALCOM_UNIFIED_H

#include "ICommunication.h"
#include "MdFrameDecoder.h"
#include <QSerialPort>
#include <QByteArray>

class MdSerialComUnified : public ICommunication {
    Q_OBJECT

public:
    explicit MdSerialComUnified(const QString& portName, QObject* parent = nullptr);
    ~MdSerialComUnified() override = default;

    bool open() override;
    void close() override;
    bool isConnected() const override;

    /** Set the serial port name (e.g., "/dev/ttyUSB0") */
    void setPortName(const QString& name);
    QString portName() const { return m_portName; }

public slots:
    /** Process incoming serial data (byte-at-a-time state machine) */
    void onReadyRead();

    /** Set baud rate (default 115200) */
    void setBaudRate(int baud);

signals:
    /** Emitted when a complete frame is parsed into data points */
    void frameParsed(const QHash<DataPointID, double>& values);

private:
    QString m_portName;
    QSerialPort* m_serial;
    int m_baudRate;

    MdFrameDecoder m_decoder;   // pure protocol state machine (unit-tested)
};

#endif // MD_SERIALCOM_UNIFIED_H
