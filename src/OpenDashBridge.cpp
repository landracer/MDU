/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "OpenDashBridge.h"
#include <QDebug>
#include <QStringList>
#include <QByteArray>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <cstring>

#define DP_PACKET_SIZE  (13)  // [0xAA][dp_id:2B][value:4B][timestamp:4B][crc:2B]

// Debug output to file for logging (size-capped: rotates to .old at 5 MB)
static QFile s_debugFile("/tmp/espnow_bridge.log");
static QTextStream s_debugStream(&s_debugFile);
static constexpr qint64 kDebugLogMaxBytes = 5 * 1024 * 1024;

void writeDebug(const QString& msg) {
    static bool fileOpen = false;
    if (fileOpen && s_debugFile.size() > kDebugLogMaxBytes) {
        // Rotate: keep one previous generation, never grow unbounded
        s_debugStream.flush();
        s_debugFile.close();
        QFile::remove("/tmp/espnow_bridge.log.old");
        QFile::rename("/tmp/espnow_bridge.log", "/tmp/espnow_bridge.log.old");
        fileOpen = false;
    }
    if (!fileOpen && s_debugFile.open(QIODevice::Append | QIODevice::Text)) {
        fileOpen = true;
    }
    if (fileOpen) {
        s_debugStream << "[" << QDateTime::currentDateTime().toString("HH:mm:ss.zzz") << "] " << msg << "\n";
        s_debugStream.flush();
    }
}

OpenDashBridge::OpenDashBridge(const QString& portName, QObject* parent)
    : ICommunication("ESP-NOW-Bridge", parent)
    , m_portName(portName)
    , m_serial(new QSerialPort(this))
{
    m_serial->setPortName(m_portName);
    
    connect(m_serial, &QSerialPort::readyRead, this, &OpenDashBridge::onReadyRead);
    connect(m_serial, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit errorOccurred(m_serial->errorString());
        }
    });
}

bool OpenDashBridge::open() {
    m_serial->setBaudRate(921600);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_connected = true;
        emit connectionStateChanged(true);
        writeDebug(QString("[ESP-NOW-Bridge] Connected to %1").arg(m_portName));
        return true;
    }
    
    emit errorOccurred("Cannot open bridge port: " + m_serial->errorString());
    return false;
}

void OpenDashBridge::close() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
    m_connected = false;
    emit connectionStateChanged(false);
}

bool OpenDashBridge::isConnected() const {
    return m_connected && m_serial->isOpen();
}

void OpenDashBridge::onReadyRead() {
    QByteArray data = m_serial->readAll();
    if (data.isEmpty()) return;

    writeDebug(QString("[ESP-NOW-Bridge] Raw bytes received: %1 bytes").arg(data.length()));
    
    // Log raw hex for debugging
    QString rawHex;
    for (int i = 0; i < data.length() && i < 64; i++) {
        rawHex += QString(" %1").arg((uint8_t)data[i] & 0xFF, 2, 16, QChar('0'));
    }
    if (data.length() > 64) rawHex += " ...";
    writeDebug(QString("[ESP-NOW-Bridge] Raw hex:%1").arg(rawHex));
    
    emit rawReceived(data, data.length());
    m_receiveBuffer.append(data);
    
    // Debug: show buffer state before processing
    writeDebug(QString("[ESP-NOW-Bridge] Buffer before processing: %1 bytes").arg(m_receiveBuffer.size()));
    QString bufferStr = QString::fromUtf8(m_receiveBuffer.left(qMin(200, m_receiveBuffer.size())));
    writeDebug(QString("[ESP-NOW-Bridge] Buffer content (first 200 chars): %1").arg(bufferStr.replace('\n', "\\n").replace('\r', "\\r")));

    // Process any text lines FIRST (DP: and CFG: messages)
    int linesProcessed = 0;
    while (m_receiveBuffer.contains('\n')) {
        int newlinePos = m_receiveBuffer.indexOf('\n');
        
        // Debug: show what we're about to extract
        QString beforeExtract = QString::fromUtf8(m_receiveBuffer.left(newlinePos));
        writeDebug(QString("[ESP-NOW-Bridge] About to extract line (pos=%1): '%2'").arg(newlinePos).arg(beforeExtract.replace('\n', "\\n").replace('\r', "\\r")));
        
        QString line = beforeExtract.trimmed();
        m_receiveBuffer.remove(0, newlinePos + 1);
        
        linesProcessed++;
        
        if (line.isEmpty()) {
            writeDebug("[ESP-NOW-Bridge] Empty line, skipping");
            continue;
        }

        writeDebug(QString("[ESP-NOW-Bridge] Text line %1: %2").arg(linesProcessed).arg(line));
        
        // Check if this is a DP: line
        if (line.startsWith("DP:")) {
            processDPMessage(line);
        } else {
            processConfigMessage(line);
        }
    }
    
    // Debug: show remaining buffer
    if (!m_receiveBuffer.isEmpty()) {
        writeDebug(QString("[ESP-NOW-Bridge] Remaining buffer after processing: %1 bytes").arg(m_receiveBuffer.size()));
        QString remainingStr = QString::fromUtf8(m_receiveBuffer.left(qMin(100, m_receiveBuffer.size())));
        writeDebug(QString("[ESP-NOW-Bridge] Remaining content (first 100 chars): %1").arg(remainingStr.replace('\n', "\\n").replace('\r', "\\r")));
    } else {
        writeDebug("[ESP-NOW-Bridge] Buffer is now empty");
    }

    // Process any remaining binary packets (fallback for raw ESP-NOW)
    if (!m_receiveBuffer.isEmpty()) {
        processBinaryPackets();
    }

    if (m_receiveBuffer.size() > 4096) {
        m_receiveBuffer.clear();
        emit errorOccurred("Bridge buffer overflow");
    }
}

/**
 * @brief Process ESP-NOW binary packets from bridge.
 * 
 * Format: [SYNC=0xAA][CMD=0x81][LEN=6][dp_id_hi][dp_id_lo][float] + checksum
 * The actual DP_ID is in bytes 3-4 (big-endian), value is bytes 5-8 (float)
 */
void OpenDashBridge::processBinaryPackets() {
    // Look for SYNC byte (0xAA) at start of packet
    if (m_receiveBuffer.length() < 10) return;  // Minimum: SYNC(1)+CMD(1)+LEN(1)+DP_ID(2)+VALUE(4)+CHK(1)
    
    int i = 0;
    
    while (i <= m_receiveBuffer.length() - 10) {
        uint8_t sync = (uint8_t)m_receiveBuffer[i] & 0xFF;
        
        if (sync == 0xAA && i + 9 < m_receiveBuffer.length()) {
            // ESP-NOW packet format from BMS:
            // Byte 0: SYNC = 0xAA
            // Byte 1: CMD = 0x81 (DATA_RESPONSE)
            // Byte 2: LEN = 6 (payload length - dp_id + value)
            // Bytes 3-4: dp_id (16-bit, big-endian: hi byte first)
            // Bytes 5-8: value (32-bit float, little-endian)
            // Byte 9: checksum
            
            uint8_t cmd = (uint8_t)m_receiveBuffer[i+1] & 0xFF;
            uint8_t payloadLen = (uint8_t)m_receiveBuffer[i+2] & 0xFF;
            
            // Verify CMD and LEN match expected BMS format
            if (cmd == 0x81 && payloadLen == 6) {
                // Extract DP_ID (big-endian: hi byte first)
                uint16_t dpId = ((uint16_t)m_receiveBuffer[i+3] << 8) | m_receiveBuffer[i+4];
                
                // Value is bytes 5-8 (little-endian float)
                const char* valBytes = &m_receiveBuffer.constData()[i+5];
                float value;
                std::memcpy(&value, valBytes, sizeof(float));
                
                int totalPacketLen = 10;  // SYNC + CMD + LEN + DP_ID(2) + VALUE(4) + CHK
                
                // Debug: show raw bytes and parsed values
                QString rawBytes;
                for (int j = 0; j < totalPacketLen && i + j < m_receiveBuffer.length(); j++) {
                    rawBytes += QString(" %1").arg((uint8_t)m_receiveBuffer[i+j] & 0xFF, 2, 16, QChar('0'));
                }
                writeDebug(QString("[ESP-NOW-Bridge] RX DP:0x%1 = %2 (raw:%3)")
                           .arg(QString("%1").arg(dpId, 4, 16, QChar('0')), QString::number(value, 'f', 3))
                           .arg(rawBytes));
                
                DataPointID id = static_cast<DataPointID>(dpId);
                QHash<DataPointID, double> values;
                values[id] = (double)value;
                
                emit frameParsed(values);
                
                i += totalPacketLen;  // Skip this complete packet
            } else {
                // Non-standard CMD or LEN, skip one byte and try again
                writeDebug(QString("[ESP-NOW-Bridge] Unknown format: CMD=0x%1 LEN=%2").arg(cmd, 2, 16).arg(payloadLen));
                i++;
            }
        } else {
            i++;  // Not a valid sync byte, try next byte
        }
    }
    
    // Remove processed bytes
    if (i > 0) {
        m_receiveBuffer.remove(0, i);
    }
}

void OpenDashBridge::parseDPLine(const QString& line) {
    processDPMessage(line);
}

void OpenDashBridge::processDPMessage(const QString& line) {
    if (line.length() < 8) {
        writeDebug(QString("[ESP-NOW-Bridge] Line too short: %1").arg(line));
        return;
    }
    
    static QRegularExpression dpRegex("^DP:(0x[0-9a-fA-F]+):([\\-+]?[0-9]*\\.?[0-9]+([eE][\\-+]?[0-9]+)?)$");
    QRegularExpressionMatch match = dpRegex.match(line);

    if (!match.hasMatch()) {
        writeDebug(QString("[ESP-NOW-Bridge] Failed to parse DP line: %1").arg(line));
        m_receiveBuffer.clear();
        return;
    }
    
    bool ok;
    uint32_t dpId = match.captured(1).toUInt(&ok, 16);
    double value = match.captured(2).toDouble();

    if (!ok || dpId == 0) {
        writeDebug(QString("[ESP-NOW-Bridge] Invalid DP ID or value: %1").arg(line));
        return;
    }

    DataPointID id = static_cast<DataPointID>(dpId);
    
    uint32_t val = static_cast<uint32_t>(id);
    
    // Remap ESPNOW BMS cell voltages to separate ID range (0x1410-0x141F)
    // This avoids conflict with UART BMS cells (0x0410-0x041F)
    if (val >= 0x0410 && val <= 0x041F) {
        uint8_t cellIndex = val - 0x0410;  // 0-15 for cells 1-16
        id = static_cast<DataPointID>(0x1410 + cellIndex);  // Map to ESPNOW range
        writeDebug(QString("[ESP-NOW-Bridge] Remapped BMS Cell %1: 0x%2 -> 0x%3")
                   .arg(cellIndex + 1).arg(val, 4, 16).arg(static_cast<uint32_t>(id), 4, 16));
    }
    
    if (val < 0x0100 || val > 0x07FF) {
        writeDebug(QString("[ESP-NOW-Bridge] DP ID out of range: %1").arg(line));
        return;
    }
    
    qDebug() << "[OpenDashBridge] Parsed DP line:" << line 
             << "ID=0x" << QString("%1").arg(static_cast<uint32_t>(id), 4, 16, QChar('0'))
             << "Value=" << value;

    QHash<DataPointID, double> values;
    values[id] = value;
    
    qDebug() << "[OpenDashBridge] Emitting frameParsed with" << values.size() << "points";
    emit frameParsed(values);
}

QString OpenDashBridge::stripAnsiCodes(const QString& input) {
    QString result = input;
    int pos = 0;
    while ((pos = result.indexOf("\x1B[", pos)) != -1) {
        int endPos = result.indexOf('m', pos);
        if (endPos != -1) {
            result.remove(pos, endPos - pos + 1);
        } else {
            break;
        }
    }
    return result;
}

void OpenDashBridge::processConfigMessage(const QString& line) {
    writeDebug(QString("[ESP-NOW-Bridge] Config: %1").arg(line));
}

void OpenDashBridge::sendCommand(const QString& command) {
    if (!isConnected()) {
        emit errorOccurred("Cannot send command: not connected");
        return;
    }

    QByteArray data = command.toUtf8();
    data.append('\n');
    
    qint64 bytesWritten = m_serial->write(data);
    if (bytesWritten > 0) {
        writeDebug(QString("[ESP-NOW-Bridge] Sent: %1").arg(command));
    } else {
        emit errorOccurred("Failed to send command");
    }
}