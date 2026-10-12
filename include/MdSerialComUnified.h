/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
/**
 * @file MdSerialComUnified.h
 * @brief MultiDisplay ECU (ATmega2560) binary frame parser.
 *
 * Wire truth, ported from the original app's MdSerialComBinary:
 * - Frame: STX(0x02) | TAG | PAYLOAD... | ETX(0x03)
 * - The TAG byte value IS the total frame length in bytes
 *   (main sensor tag 0x5F = 95 decimal = 92-byte payload + STX + TAG + ETX).
 * - ETX is only *validated* at byte index (frameLength - 1). A 0x03 byte
 *   inside the payload must NOT terminate the frame — framing ends by
 *   position, not by the first ETX-like byte.
 */
#ifndef MD_SERIALCOM_UNIFIED_H
#define MD_SERIALCOM_UNIFIED_H

#include "ICommunication.h"
#include <QSerialPort>
#include <QByteArray>

// Frame markers (matching original app)
#define MD_FRAME_BEGIN 0x02   // STX
#define MD_FRAME_END   0x03   // ETX

// Tag constants (matching original app). A tag value IS the total frame length.
#define MD_SERIALOUT_BINARY_TAG                   95  // 0x5F main sensor frame (92-byte payload)
#define MD_SERIALOUT_BINARY_BOOSTPID_TAG          69  // 0x45 boost PID frame
#define MD_SERIALOUT_BINARY_TAG_N75_DUTY_MAP      22  // 0x16 N75 duty map frame
#define MD_SERIALOUT_BINARY_TAG_N75_SETPOINT_MAP  38  // 0x26 N75 setpoint map frame
#define MD_SERIALOUT_BINARY_TAG_GEAR_RATIO_6G     17  // 0x11 6-speed gear ratio frame
#define MD_SERIALOUT_BINARY_TAG_ACK                4  // 0x04 acknowledgement frame
#define MD_SERIALOUT_BINARY_TAG_N75_PARAMS         23  // 0x17 N75 params frame

#define MD_MAINFRAME_PAYLOAD_SIZE 92
#define MD_MAXFRAME_SIZE          97   // main frame (95) + margin, matching original app

// Frame state machine states (matching original app)
enum FrameStatus {
    MD_STATUS_RECEIVING      = 1,
    MD_STATUS_FRAME_COMPLETE = 2,
    MD_STATUS_FRAMEERROR     = 3,
    MD_STATUS_WAITING_FOR_TAG = 4
};

/**
 * @brief MultiDisplay ECU binary frame parser
 *
 * Length-tagged, ETX-validated framing. Decodes the 95-byte main sensor
 * frame into unified DataPointIDs. Control/config frames (N75 maps, gear
 * ratios, boost PID) are framed correctly but not yet decoded (roadmap).
 */
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
    /** Process incoming serial data (byte-by-byte state machine) */
    void onReadyRead();

    /** Set baud rate (default 115200) */
    void setBaudRate(int baud);

signals:
    /** Emitted when a complete frame is parsed into data points */
    void frameParsed(const QHash<DataPointID, double>& values);

private:
    /** Route a fully received frame (STX..ETX) by its tag */
    void dispatchFrame(const QByteArray& frame);
    /** Decode the 92-byte main sensor payload into data points */
    void processMainData(const QByteArray& payload);

    QString m_portName;
    QSerialPort* m_serial;
    int m_baudRate;

    // Frame state machine (matching original app)
    int m_index;
    int m_status;
    int m_framelength;
    int m_discardedFrames = 0;

    // Receive buffer for frame data
    quint8 rcvData[MD_MAXFRAME_SIZE];
};

#endif // MD_SERIALCOM_UNIFIED_H
