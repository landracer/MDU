/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "UnifiedDataStore.h"
#include <QCoreApplication>
#include <QDebug>

UnifiedDataStore& UnifiedDataStore::instance() {
    static UnifiedDataStore s_instance;
    return s_instance;
}

UnifiedDataStore::UnifiedDataStore(QObject* parent)
    : QObject(parent)
    , m_refreshTimer(new QTimer(this))
{
    m_refreshTimer->setInterval(33); // ~30 Hz refresh for UI
    connect(m_refreshTimer, &QTimer::timeout, this, &UnifiedDataStore::refreshTick);
    m_refreshTimer->start();

    initializeDefaultPoints();
}

void UnifiedDataStore::initializeDefaultPoints() {
    // Register all known data points so we have entries for them
    // This allows iteration and validation without dynamic allocation
    QHash<DataPointID, const char*> pointList;
    
    // Engine/ECU
    pointList[DataPointID::RPM] = "RPM";
    pointList[DataPointID::CoolantTemp] = "CoolantTemp";
    pointList[DataPointID::OilPressure] = "OilPressure";
    pointList[DataPointID::IntakeTemp] = "IntakeTemp";
    pointList[DataPointID::ThrottlePosition] = "ThrottlePosition";
    pointList[DataPointID::MapPressure] = "MapPressure";
    pointList[DataPointID::BoostPressure] = "BoostPressure";
    pointList[DataPointID::FuelPressure] = "FuelPressure";
    pointList[DataPointID::TimingAdvance] = "TimingAdvance";
    pointList[DataPointID::InjectorPulseWidth] = "InjectorPulseWidth";
    pointList[DataPointID::MAF] = "MAF";
    pointList[DataPointID::Lambda] = "Lambda";
    pointList[DataPointID::AFR] = "AFR";
    pointList[DataPointID::EGT1] = "EGT1";
    pointList[DataPointID::EGT2] = "EGT2";
    pointList[DataPointID::EGT3] = "EGT3";
    pointList[DataPointID::EGT4] = "EGT4";
    pointList[DataPointID::EGT5] = "EGT5";
    pointList[DataPointID::EGT6] = "EGT6";
    pointList[DataPointID::EGT7] = "EGT7";
    pointList[DataPointID::EGT8] = "EGT8";
    pointList[DataPointID::BatteryVoltage] = "BatteryVoltage";
    pointList[DataPointID::VehicleSpeed] = "VehicleSpeed";
    pointList[DataPointID::Gear] = "Gear";
    pointList[DataPointID::EFRSpeed] = "EFRSpeed";
    pointList[DataPointID::N75DutyCycle] = "N75DutyCycle";
    pointList[DataPointID::N75Setpoint] = "N75Setpoint";

    // GPS/Navigation
    pointList[DataPointID::GPSSpeed] = "GPSSpeed";
    pointList[DataPointID::GPSHeading] = "GPSHeading";
    pointList[DataPointID::GPSLatitude] = "GPSLatitude";
    pointList[DataPointID::GPSLongitude] = "GPSLongitude";
    pointList[DataPointID::GPSAltitude] = "GPSAltitude";
    pointList[DataPointID::GPSSatellites] = "GPSSatellites";
    pointList[DataPointID::GPSFixQuality] = "GPSFixQuality";
    pointList[DataPointID::GPSCourse] = "GPSCourse";

    // IMU
    pointList[DataPointID::IMUPitch] = "IMUPitch";
    pointList[DataPointID::IMURoll] = "IMURoll";
    pointList[DataPointID::IMUYaw] = "IMUYaw";
    pointList[DataPointID::IMUAx] = "IMUAx";
    pointList[DataPointID::IMUAy] = "IMUAy";
    pointList[DataPointID::IMUAz] = "IMUAz";

    // BMS
    pointList[DataPointID::BMSPackVoltage] = "BMSPackVoltage";
    pointList[DataPointID::BMSPackCurrent] = "BMSPackCurrent";
    pointList[DataPointID::BMSSOC] = "BMSSOC";
    pointList[DataPointID::BMSTemp] = "BMSTemp";
    pointList[DataPointID::BMSMinCellVoltage] = "BMSMinCellVoltage";
    pointList[DataPointID::BMSMaxCellVoltage] = "BMSMaxCellVoltage";
    pointList[DataPointID::BMSCellDelta] = "BMSCellDelta";
    pointList[DataPointID::BMSPackPower] = "BMSPackPower";
    pointList[DataPointID::BMSStackVoltage] = "BMSStackVoltage";
    pointList[DataPointID::BMSSOH] = "BMSSOH";
    
    // Cell voltages 1-16 (full 16S support)
    for (int i = 0; i < 16; i++) {
        DataPointID cellId = static_cast<DataPointID>(static_cast<uint32_t>(DataPointID::BMSCell1) + i);
        pointList[cellId] = QString("BMSCell%1").arg(i + 1).toStdString().c_str();
    }
    
    // BMS Temperature sensors
    pointList[DataPointID::BMSTemp1] = "BMSTemp1";
    pointList[DataPointID::BMSTemp2] = "BMSTemp2";
    pointList[DataPointID::BMSTemp3] = "BMSTemp3";
    
    // BMS Status Registers
    pointList[DataPointID::BMSFETStatus] = "BMSFETStatus";
    pointList[DataPointID::BMSSafetyA] = "BMSSafetyA";
    pointList[DataPointID::BMSSafetyB] = "BMSSafetyB";
    pointList[DataPointID::BMSSafetyC] = "BMSSafetyC";
    pointList[DataPointID::BMSErrorFlags] = "BMSErrorFlags";
    pointList[DataPointID::BMSBalancingStatus] = "BMSBalancingStatus";
    
    // VESC/Drivetrain
    pointList[DataPointID::VESCERPM] = "VESCERPM";
    pointList[DataPointID::VESCMotorCurrent] = "VESCMotorCurrent";
    pointList[DataPointID::VESCInputCurrent] = "VESCInputCurrent";
    pointList[DataPointID::VESCDutyCycle] = "VESCDutyCycle";
    pointList[DataPointID::VESCMOSFETTemp] = "VESCMOSFETTemp";
    pointList[DataPointID::VESCMotorTemp] = "VESCMotorTemp";
    pointList[DataPointID::VESCInputVoltage] = "VESCInputVoltage";
    pointList[DataPointID::VESCPowerIn] = "VESCPowerIn";
    pointList[DataPointID::VESCRPM] = "VESCRPM";
    pointList[DataPointID::VESCFaultCode] = "VESCFaultCode";
    pointList[DataPointID::WheelRPMFL] = "WheelRPMFL";
    pointList[DataPointID::WheelRPMFR] = "WheelRPMFR";
    pointList[DataPointID::WheelRPMRL] = "WheelRPMRL";
    pointList[DataPointID::WheelRPMRR] = "WheelRPMRR";
    pointList[DataPointID::WheelRPMAvg] = "WheelRPMAvg";

    // Pre-allocate all entries
    QWriteLocker locker(&m_lock);
    for (auto it = pointList.begin(); it != pointList.end(); ++it) {
        m_data[it.key()] = DataPointEntry{};
    }
}

void UnifiedDataStore::setDataPoint(DataPointID id, double value, const QString& source) {
    uint32_t val = static_cast<uint32_t>(id);
    
    {
        QWriteLocker locker(&m_lock);
        auto it = m_data.find(id);
        if (it == m_data.end()) {
            // Dynamic addition for unknown data points
            qDebug() << "[UnifiedDataStore] Creating new entry for ID 0x" << QString("%1").arg(static_cast<uint32_t>(id), 4, 16, QChar('0'));
            m_data[id] = DataPointEntry{value, QDateTime::currentDateTime(), source, true};
        } else {
            qDebug() << "[UnifiedDataStore] Updating existing entry for ID 0x" << QString("%1").arg(static_cast<uint32_t>(id), 4, 16, QChar('0'));
            it.value().value = value;
            it.value().timestamp = QDateTime::currentDateTime();
            if (!source.isEmpty()) {
                it.value().source = source;
            }
            it.value().valid = true;
        }
    }

    emit dataPointUpdated(id, value, source);
}

double UnifiedDataStore::getDataPoint(DataPointID id, bool* valid) const {
    QReadLocker locker(&m_lock);
    auto it = m_data.find(id);
    if (it != m_data.end()) {
        if (valid) *valid = it.value().valid;
        return it.value().value;
    }
    if (valid) *valid = false;
    return 0.0;
}

QDateTime UnifiedDataStore::getTimestamp(DataPointID id) const {
    QReadLocker locker(&m_lock);
    auto it = m_data.find(id);
    if (it != m_data.end()) {
        return it.value().timestamp;
    }
    return QDateTime{};
}

bool UnifiedDataStore::isValid(DataPointID id) const {
    QReadLocker locker(&m_lock);
    auto it = m_data.find(id);
    if (it != m_data.end()) {
        return it.value().valid;
    }
    return false;
}

void UnifiedDataStore::resetValidity() {
    QWriteLocker locker(&m_lock);
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        it.value().valid = false;
    }
}

QVector<DataPointID> UnifiedDataStore::getValidPoints() const {
    QReadLocker locker(&m_lock);
    QVector<DataPointID> result;
    for (auto it = m_data.begin(); it != m_data.end(); ++it) {
        if (it.value().valid) {
            result.append(it.key());
        }
    }
    return result;
}

QString UnifiedDataStore::getSource(DataPointID id) const {
    QReadLocker locker(&m_lock);
    auto it = m_data.find(id);
    if (it != m_data.end()) {
        return it.value().source;
    }
    return QString();
}
