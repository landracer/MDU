/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */

#include "RAtTraxBMS.h"
#include <QDebug>
#include <QStringList>

// Enable verbose debug logging for data flow tracing
#define DEBUG_RATTRAX_BMS 1

#if DEBUG_RATTRAX_BMS
#define BMS_DEBUG qDebug() << "[rAtTrax-BMS]"
#else
#define BMS_DEBUG while(false) qDebug()
#endif

RAtTraxBMS::RAtTraxBMS(const QString& portName, QObject* parent)
    : ICommunication("rAtTrax-BMS", parent)
    , m_portName(portName)
    , m_serial(new QSerialPort(this))
{
    m_serial->setPortName(m_portName);
    
    connect(m_serial, &QSerialPort::readyRead, this, &RAtTraxBMS::onReadyRead);
    connect(m_serial, QOverload<QSerialPort::SerialPortError>::of(&QSerialPort::errorOccurred),
            this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            emit errorOccurred(m_serial->errorString());
        }
    });
}

void RAtTraxBMS::setPortName(const QString& name) {
    m_portName = name;
    m_serial->setPortName(name);
}

void RAtTraxBMS::setCSVMode(bool enabled) {
    m_csvMode = enabled;
}
bool RAtTraxBMS::open() {
    m_serial->setBaudRate(115200);
    m_serial->setDataBits(QSerialPort::Data8);
    m_serial->setParity(QSerialPort::NoParity);
    m_serial->setStopBits(QSerialPort::OneStop);
    m_serial->setFlowControl(QSerialPort::NoFlowControl);

    if (m_serial->open(QIODevice::ReadWrite)) {
        m_connected = true;
        emit connectionStateChanged(true);
        qDebug() << "[rAtTrax-BMS] Connected to" << m_portName;
        return true;
    }
    
    emit errorOccurred("Cannot open BMS port: " + m_serial->errorString());
    return false;
}

void RAtTraxBMS::close() {
    if (m_serial->isOpen()) {
        m_serial->close();
    }
    m_connected = false;
    emit connectionStateChanged(false);
}

bool RAtTraxBMS::isConnected() const {
    return m_connected && m_serial->isOpen();
}

void RAtTraxBMS::onReadyRead() {
    QByteArray data = m_serial->readAll();
    if (data.isEmpty()) return;

    // Debug: Log raw received bytes as hex
    BMS_DEBUG << "Raw bytes:" << data.length() << "bytes" << data.toHex(' ').data();

    emit rawReceived(data, data.length());

    if (m_csvMode) {
        // CSV mode: split by newlines and parse each line
        m_receiveBuffer.append(data);
        while (m_receiveBuffer.contains('\n')) {
            int newlinePos = m_receiveBuffer.indexOf('\n');
            QString line = QString::fromUtf8(m_receiveBuffer.left(newlinePos)).trimmed();
            m_receiveBuffer.remove(0, newlinePos + 1);
            
            BMS_DEBUG << "CSV Line:" << line;
            
            if (!line.isEmpty() && !line.startsWith("Timestamp")) { // Skip header
                parseCSVLine(line);
            }
        }
    } else {
        // Binary mode: process raw bytes
        processBMSPacket(data);
    }
}

void RAtTraxBMS::parseCSVLine(const QString& line) {
    qDebug() << "[RAtTrax-BMS] Raw line: " << line;
    
    if (!line.startsWith("BMS:") && !line.startsWith("VESC:") && !line.startsWith("GPS:")) {
        qDebug() << "[RAtTrax-BMS] Unknown format (not BMS/VESC/GPS):" << line;
        return;  // Unknown format - ignore
    }
    
    QStringList fields = line.split(',', Qt::SkipEmptyParts);
    if (fields.isEmpty()) {
        BMS_DEBUG << "No fields found in line";
        return;
    }
    
    BMS_DEBUG << "Fields count:" << fields.size();
    for (int i = 0; i < fields.size(); ++i) {
        BMS_DEBUG << "  Field[" << i << "]:" << fields[i];
    }
    
    QHash<DataPointID, double> values;

    // Parse based on prefix
    if (line.startsWith("BMS:")) {
        // BMS:voltage,current,temp,stack,soc,min,max,delta,cell1,cell2,...,cell6
        // At minimum: 8 fields + individual cell voltages (auto-adjusts to CONNECTED_CELLS)
        // Minimum required: voltage,current,temp,stack,soc,min,max,delta = 8 fields
        if (fields.size() < 8) {
            qWarning() << "[rAtTrax-BMS] Invalid BMS line (need at least 8 fields, got" << fields.size() << "):" << line;
            return;
        }
        
        // Parse voltage from first field (after the colon)
        bool ok0 = false, ok1 = false, ok2 = false, ok3 = false, ok4 = false, ok5 = false, ok6 = false, ok7 = false;
        QString voltageStr = fields[0].mid(4);  // Skip "BMS:" prefix
        double voltage = voltageStr.toDouble(&ok0);
        double current = fields[1].toDouble(&ok1);
        double temp = fields[2].toDouble(&ok2);
        double stack = fields[3].toDouble(&ok3);    // Stack voltage (sum of cells)
        double soc = fields[4].toDouble(&ok4);
        double cellMin = fields[5].toDouble(&ok5);
        double cellMax = fields[6].toDouble(&ok6);
        double delta = fields[7].toDouble(&ok7);     // Delta (max-min)*1000
        
        if (!ok0 || !ok1 || !ok2 || !ok3 || !ok4 || !ok5 || !ok6 || !ok7) {
            qWarning() << "[rAtTrax-BMS] Failed to parse BMS values from:" << line;
            return;
        }
        
        values[DataPointID::BMSPackVoltage] = voltage;
        values[DataPointID::BMSPackCurrent] = current;
        values[DataPointID::BMSTemp] = temp;
        values[DataPointID::BMSStackVoltage] = stack;
        values[DataPointID::BMSSOC] = soc;
        values[DataPointID::BMSMinCellVoltage] = cellMin;
        values[DataPointID::BMSMaxCellVoltage] = cellMax;
        values[DataPointID::BMSCellDelta] = delta;
        
        BMS_DEBUG << "Parsed BMS: V=" << voltage << "V, I=" << current << "A, T=" << temp << "C, SOC=" << soc << "%";
        BMS_DEBUG << "  Stack=" << stack << "V, Cell min=" << cellMin << "V, max=" << cellMax << "V, delta=" << delta << "mV";
        
        // Parse individual cell voltages (if present)
        uint32_t baseId = static_cast<uint32_t>(DataPointID::BMSCell1);
        int numCells = qMin(fields.size() - 8, 16);  // Cap at 16 cells
        for (int i = 0; i < numCells; i++) {
            bool okCell = false;
            double cellVolt = fields[8 + i].toDouble(&okCell);
            if (okCell) {
                values[static_cast<DataPointID>(baseId + i)] = cellVolt;
                BMS_DEBUG << "  Cell" << (i+1) << "=" << cellVolt << "V";
            }
        }
        
    } else if (line.startsWith("VESC:")) {
        // VESC:RPM,MotorCurrent,Duty%,InputVoltage,FaultCode
        if (fields.size() < 6) {
            qWarning() << "[rAtTrax-BMS] Invalid VESC line:" << line;
            return;
        }
        
        bool ok1 = false, ok2 = false, ok3 = false, ok4 = false, ok5 = false;
        double rpm = fields[1].toDouble(&ok1);
        double motorCurrent = fields[2].toDouble(&ok2);
        double dutyPct = fields[3].toDouble(&ok3);
        double inputVoltage = fields[4].toDouble(&ok4);
        double faultCode = fields[5].toDouble(&ok5);
        
        if (!ok1 || !ok2 || !ok3 || !ok4 || !ok5) {
            qWarning() << "[rAtTrax-BMS] Failed to parse VESC values from:" << line;
            return;
        }
        
        values[DataPointID::VESCERPM] = rpm;
        values[DataPointID::VESCMotorCurrent] = motorCurrent;
        values[DataPointID::VESCDutyCycle] = dutyPct / 100.0;  // Convert % to ratio
        values[DataPointID::VESCInputVoltage] = inputVoltage;
        values[DataPointID::VESCFaultCode] = faultCode;
        
        BMS_DEBUG << "Parsed VESC: RPM=" << rpm << ", I=" << motorCurrent << "A, Duty=" << dutyPct << "%";
        
    } else if (line.startsWith("GPS:")) {
        // GPS:Lat,Lng,Altitude,Speed_mph,Satellites
        if (fields.size() < 6) {
            qWarning() << "[rAtTrax-BMS] Invalid GPS line:" << line;
            return;
        }
        
        bool ok1 = false, ok2 = false, ok3 = false, ok4 = false, ok5 = false;
        double lat = fields[1].toDouble(&ok1);
        double lng = fields[2].toDouble(&ok2);
        double alt = fields[3].toDouble(&ok3);
        double speedMph = fields[4].toDouble(&ok4);
        double satellites = fields[5].toDouble(&ok5);
        
        if (!ok1 || !ok2 || !ok3 || !ok4 || !ok5) {
            qWarning() << "[rAtTrax-BMS] Failed to parse GPS values from:" << line;
            return;
        }
        
        // Convert speed from mph to km/h
        double speedKmh = speedMph * 1.60934;
        
        values[DataPointID::GPSLatitude] = lat;
        values[DataPointID::GPSLongitude] = lng;
        values[DataPointID::GPSAltitude] = alt;
        values[DataPointID::GPSSpeed] = speedKmh;
        values[DataPointID::GPSSatellites] = satellites;
        
        BMS_DEBUG << "Parsed GPS: Lat=" << lat << ", Lng=" << lng << ", Alt=" << alt << "m, Speed=" << speedKmh << "km/h";
    }
    
    if (!values.isEmpty()) {
        BMS_DEBUG << "Emitting" << values.size() << "data points from rAtTrax-BMS";
        emit framesParsed(values, "rAtTrax-BMS");
        emit frameParsed(values);
        
        // Safety checks for BMS data
        double soc = values.value(DataPointID::BMSSOC, 100.0);
        if (soc < 5.0 && qFuzzyCompare(m_lastSOC, 0.0) || soc < m_lastSOC - 10.0) {
            emit batteryCritical(soc);
        }
        m_lastSOC = soc;
        
        double temp = values.value(DataPointID::BMSTemp, 0.0);
        if (temp > 60.0) {
            emit thermalWarning(temp);
        }
        m_lastTemp = temp;
    } else {
        BMS_DEBUG << "No values parsed from line:" << line;
    }
}

void RAtTraxBMS::processBMSPacket(const QByteArray& data) {
    // Binary packet processing for direct BMS communication
    // This would handle binary protocol from BQ76952 via ESP32 bridge
    // Implementation depends on exact binary format used by rAtTrax firmware
    
    QHash<DataPointID, double> values;
    
    // Placeholder: parse based on actual binary protocol
    // Will be filled in when binary protocol spec is confirmed
    
    if (!values.isEmpty()) {
        emit framesParsed(values, "rAtTrax-BMS");
    }
}

void RAtTraxBMS::parseBinaryFrame(const QByteArray& data) {
    // Parse binary frame from BMS system
    // Binary format: [header][data payload][checksum]
    // Header: 2 bytes sync, 1 byte type, 1 byte length
    
    if (data.size() < 4) {
        qWarning() << "[rAtTrax-BMS] Binary frame too short:" << data.size() << "bytes";
        return;
    }
    
    // Placeholder: implement based on actual binary protocol
    qDebug() << "[rAtTrax-BMS] Received binary frame:" << data.size() << "bytes";
}

void RAtTraxBMS::processGPSData(const QByteArray& data) {
    // Parse NMEA-style GPS data from Reyax RYS352A
    // Implementation depends on exact format
}

void RAtTraxBMS::processVESCData(const QByteArray& data) {
    // Parse VESC telemetry via CAN bridge
    // Implementation depends on exact binary format
}

void RAtTraxBMS::processIMUData(const QByteArray& data) {
    // Parse MPU6050 DMP quaternion/attitude data
    // Implementation depends on exact binary format
}