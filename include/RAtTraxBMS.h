/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef RATTRAXBMS_H
#define RATTRAXBMS_H

#include "ICommunication.h"
#include <QSerialPort>
#include <QByteArray>
#include <QTimer>

/**
 * @brief rAtTrax-BMS communication handler
 * 
 * Handles data from the ESP32-DOIT running rAtTrax-BMS firmware.
 * Connects via USB CDC-ACM (virtual serial port) or Bluetooth SPP.
 * Parses CSV-formatted logging data and/or binary protocol frames.
 * 
 * Data sources on rAtTrax-BMS:
 * - BQ76952 6S BMS (I2C @ 400kHz)
 * - MPU6050 DMP IMU (I2C)
 * - VESC via CAN @ 500kbps
 * - Reyax RYS352A GPS @ 115200 baud
 */
class RAtTraxBMS : public ICommunication {
    Q_OBJECT

public:
    explicit RAtTraxBMS(const QString& portName, QObject* parent = nullptr);
    ~RAtTraxBMS() override = default;

    bool open() override;
    void close() override;
    bool isConnected() const override;

    /**
     * @brief Set the communication port (USB serial or Bluetooth)
     */
    void setPortName(const QString& name);
    QString portName() const { return m_portName; }
    
    /**
     * @brief Set CSV parsing mode (default: true)
     */
    void setCSVMode(bool enabled);

public slots:
    /**
     * @brief Process incoming data from BMS system
     */
    void onReadyRead();

    /**
     * @brief Parse a CSV line from BMS logger
     * Format: "Timestamp,GPS_Lat,GPS_Lng,GPS_Alt,GPS_Speed_kmh,GPS_Sats,BMS_SOC,BMS_Voltage,BMS_Current,BMS_Temp,IMU_Pitch,IMU_Roll,IMU_Yaw,Speed_Avg_mph,RPM_FL,RPM_FR,RPM_RL,RPM_RR,VESC_Voltage,VESC_Motor_Temp,VESC_Current,VESC_Duty,VESC_RPM,VESC_Fault"
     */
    void parseCSVLine(const QString& line);

    /**
     * @brief Parse binary ESP-NOW style frame from BMS
     */
    void parseBinaryFrame(const QByteArray& data);

signals:
    void frameParsed(const QHash<DataPointID, double>& values);
    void batteryCritical(double soc);
    void thermalWarning(double temp);
    void faultDetected(uint32_t faultCode);

private:
    void processBMSPacket(const QByteArray& data);
    void processGPSData(const QByteArray& data);
    void processVESCData(const QByteArray& data);
    void processIMUData(const QByteArray& data);

    QString m_portName;
    QSerialPort* m_serial;
    QByteArray m_receiveBuffer;
    bool m_csvMode = true; // Default to CSV parsing (from logger)

    // BMS state tracking
    double m_lastSOC = 0.0;
    double m_lastTemp = 0.0;
};

#endif // RATTRAXBMS_H
