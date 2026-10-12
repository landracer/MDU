/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#include "DataPointRegistry.h"
#include <algorithm>  // For std::sort

DataPointRegistry& DataPointRegistry::instance() {
    static DataPointRegistry s_instance;
    return s_instance;
}

DataPointRegistry::DataPointRegistry() {
    initializeRegistry();
}

void DataPointRegistry::initializeRegistry() {
    // ─── ENGINE / ECU (0x0100–0x01FF) ──────────────────────────────
    m_registry[DataPointID::RPM] = {
        DataPointID::RPM, "Engine RPM", "RPM", "ENGINE/ECU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::CoolantTemp] = {
        DataPointID::CoolantTemp, "Coolant Temperature", "°C", "ENGINE/ECU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::OilPressure] = {
        DataPointID::OilPressure, "Oil Pressure", "kPa", "ENGINE/ECU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::IntakeTemp] = {
        DataPointID::IntakeTemp, "Intake Air Temperature", "°C", "ENGINE/ECU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::ThrottlePosition] = {
        DataPointID::ThrottlePosition, "Throttle Position", "%", "ENGINE/ECU",
        0.01, 0.0, false
    };
    m_registry[DataPointID::MapPressure] = {
        DataPointID::MapPressure, "MAP Pressure", "kPa", "ENGINE/ECU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::BoostPressure] = {
        DataPointID::BoostPressure, "Boost Pressure", "kPa", "ENGINE/ECU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::FuelPressure] = {
        DataPointID::FuelPressure, "Fuel Pressure", "kPa", "ENGINE/ECU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::TimingAdvance] = {
        DataPointID::TimingAdvance, "Timing Advance", "degrees", "ENGINE/ECU",
        0.1, 0.0, true
    };
    m_registry[DataPointID::InjectorPulseWidth] = {
        DataPointID::InjectorPulseWidth, "Injector Pulse Width", "ms", "ENGINE/ECU",
        0.01, 0.0, false
    };
    m_registry[DataPointID::MAF] = {
        DataPointID::MAF, "Mass Air Flow", "g/s", "ENGINE/ECU",
        0.1, 0.0, false
    };
    m_registry[DataPointID::Lambda] = {
        DataPointID::Lambda, "Lambda (Air-Fuel Ratio)", "λ", "ENGINE/ECU",
        0.01, 0.0, false
    };
    m_registry[DataPointID::AFR] = {
        DataPointID::AFR, "Air-Fuel Ratio", "AFR", "ENGINE/ECU",
        0.01, 0.0, false
    };
    
    // EGTs (1-8)
    for (int i = 0; i < 8; i++) {
        DataPointID egtId = static_cast<DataPointID>(static_cast<uint32_t>(DataPointID::EGT1) + i);
        m_registry[egtId] = {
            egtId, QString("Exhaust Gas Temp %1").arg(i + 1),
            "°C", "ENGINE/ECU", 1.0, 0.0, false
        };
    }
    
    m_registry[DataPointID::BatteryVoltage] = {
        DataPointID::BatteryVoltage, "Battery Voltage", "V", "ENGINE/ECU",
        0.01, 0.0, false
    };
    m_registry[DataPointID::VehicleSpeed] = {
        DataPointID::VehicleSpeed, "Vehicle Speed (ECU-derived)", "km/h", "ENGINE/ECU",
        0.01, 0.0, false
    };
    m_registry[DataPointID::Gear] = {
        DataPointID::Gear, "Gear Position", "gear", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::EFRSpeed] = {
        DataPointID::EFRSpeed, "EFR Speed (pulse width)", "RPM", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::N75DutyCycle] = {
        DataPointID::N75DutyCycle, "N75 Boost Valve Duty Cycle", "%", "ENGINE/ECU",
        0.1, 0.0, false
    };
    m_registry[DataPointID::N75Setpoint] = {
        DataPointID::N75Setpoint, "N75 Boost Setpoint", "kPa", "ENGINE/ECU",
        0.1, 0.0, false
    };
    // Catalog-aligned OBD2/engine points (arrive as SI floats over ESP-NOW)
    m_registry[DataPointID::EngineLoad] = {
        DataPointID::EngineLoad, "Engine Load", "%", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::OilTemp] = {
        DataPointID::OilTemp, "Oil Temperature", "°C", "ENGINE/ECU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::EGT] = {
        DataPointID::EGT, "EGT (single probe)", "°C", "ENGINE/ECU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::FuelLevel] = {
        DataPointID::FuelLevel, "Fuel Level", "%", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::TransTemp] = {
        DataPointID::TransTemp, "Transmission Temperature", "°C", "ENGINE/ECU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::O2Lambda] = {
        DataPointID::O2Lambda, "O2 Lambda (secondary)", "λ", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::MDRPM] = {
        DataPointID::MDRPM, "MD RPM (MultiDisplay)", "RPM", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::OBD2Flags] = {
        DataPointID::OBD2Flags, "OBD2 Flags", "bitmask", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::MILOn] = {
        DataPointID::MILOn, "MIL On", "0/1", "ENGINE/ECU",
        1.0, 0.0, false
    };
    m_registry[DataPointID::DTCCount] = {
        DataPointID::DTCCount, "DTC Count", "count", "ENGINE/ECU",
        1.0, 0.0, false
    };

    // ─── GPS / NAVIGATION (0x0200–0x02FF) ──────────────────────────
    m_registry[DataPointID::GPSSpeed] = {
        DataPointID::GPSSpeed, "GPS Ground Speed", "km/h", "GPS/NAV",
        0.01, 0.0, false
    };
    m_registry[DataPointID::GPSHeading] = {
        DataPointID::GPSHeading, "GPS Heading", "degrees", "GPS/NAV",
        0.01, 0.0, false
    };
    m_registry[DataPointID::GPSLatitude] = {
        DataPointID::GPSLatitude, "GPS Latitude", "degrees", "GPS/NAV",
        1.0, 0.0, true
    };
    m_registry[DataPointID::GPSLongitude] = {
        DataPointID::GPSLongitude, "GPS Longitude", "degrees", "GPS/NAV",
        1.0, 0.0, true
    };
    m_registry[DataPointID::GPSAltitude] = {
        DataPointID::GPSAltitude, "GPS Altitude", "meters", "GPS/NAV",
        0.01, 0.0, true
    };
    m_registry[DataPointID::GPSSatellites] = {
        DataPointID::GPSSatellites, "GPS Satellite Count", "sats", "GPS/NAV",
        1.0, 0.0, false
    };
    m_registry[DataPointID::GPSFixQuality] = {
        DataPointID::GPSFixQuality, "GPS Fix Quality", "quality", "GPS/NAV",
        1.0, 0.0, false
    };
    m_registry[DataPointID::GPSCourse] = {
        DataPointID::GPSCourse, "GPS Course", "degrees", "GPS/NAV",
        0.01, 0.0, false
    };
    m_registry[DataPointID::LapTime] = {
        DataPointID::LapTime, "Lap Time", "ms", "GPS/NAV",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BestLap] = {
        DataPointID::BestLap, "Best Lap", "ms", "GPS/NAV",
        1.0, 0.0, false
    };
    m_registry[DataPointID::LapDelta] = {
        DataPointID::LapDelta, "Lap Delta", "ms", "GPS/NAV",
        1.0, 0.0, true
    };
    m_registry[DataPointID::SectorTime] = {
        DataPointID::SectorTime, "Sector Time", "ms", "GPS/NAV",
        1.0, 0.0, false
    };
    m_registry[DataPointID::PredictiveLap] = {
        DataPointID::PredictiveLap, "Predictive Lap", "ms", "GPS/NAV",
        1.0, 0.0, false
    };
    m_registry[DataPointID::GPSFix] = {
        DataPointID::GPSFix, "GPS Fix", "fix", "GPS/NAV",
        1.0, 0.0, false
    };

    // ─── IMU (0x0300–0x03FF) ──────────────────────────────
    m_registry[DataPointID::GForceLat] = {
        DataPointID::GForceLat, "G-Force Lateral", "g", "IMU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::GForceLong] = {
        DataPointID::GForceLong, "G-Force Longitudinal", "g", "IMU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::GForceVert] = {
        DataPointID::GForceVert, "G-Force Vertical", "g", "IMU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::YawRate] = {
        DataPointID::YawRate, "Yaw Rate", "°/s", "IMU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::PitchRate] = {
        DataPointID::PitchRate, "Pitch Rate", "°/s", "IMU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::RollRate] = {
        DataPointID::RollRate, "Roll Rate", "°/s", "IMU",
        1.0, 0.0, true
    };
    m_registry[DataPointID::IMUPitch] = {
        DataPointID::IMUPitch, "IMU Pitch Angle", "degrees", "IMU",
        0.1, 0.0, true
    };
    m_registry[DataPointID::IMURoll] = {
        DataPointID::IMURoll, "IMU Roll Angle", "degrees", "IMU",
        0.1, 0.0, true
    };
    m_registry[DataPointID::IMUYaw] = {
        DataPointID::IMUYaw, "IMU Yaw Angle", "degrees", "IMU",
        0.1, 0.0, true
    };
    m_registry[DataPointID::IMUAx] = {
        DataPointID::IMUAx, "IMU Acceleration X", "m/s²", "IMU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::IMUAy] = {
        DataPointID::IMUAy, "IMU Acceleration Y", "m/s²", "IMU",
        0.01, 0.0, true
    };
    m_registry[DataPointID::IMUAz] = {
        DataPointID::IMUAz, "IMU Acceleration Z", "m/s²", "IMU",
        0.01, 0.0, true
    };

    // ─── BATTERY / BMS (0x0400–0x04FF) ────────────────────────────
    m_registry[DataPointID::BMSPackVoltage] = {
        DataPointID::BMSPackVoltage, "BMS Pack Voltage", "V", "BMS",
        0.1, 0.0, false
    };
    m_registry[DataPointID::BMSPackCurrent] = {
        DataPointID::BMSPackCurrent, "BMS Pack Current", "A", "BMS",
        0.1, 0.0, true
    };
    m_registry[DataPointID::BMSSOC] = {
        DataPointID::BMSSOC, "BMS State of Charge", "%", "BMS",
        0.1, 0.0, false
    };
    m_registry[DataPointID::BMSTemp] = {
        DataPointID::BMSTemp, "BMS Maximum Cell Temperature", "°C", "BMS",
        0.1, 0.0, true
    };
    m_registry[DataPointID::BMSMinCellVoltage] = {
        DataPointID::BMSMinCellVoltage, "BMS Minimum Cell Voltage", "V", "BMS",
        0.001, 0.0, false
    };
    m_registry[DataPointID::BMSMaxCellVoltage] = {
        DataPointID::BMSMaxCellVoltage, "BMS Maximum Cell Voltage", "V", "BMS",
        0.001, 0.0, false
    };
    m_registry[DataPointID::BMSCellDelta] = {
        DataPointID::BMSCellDelta, "BMS Cell Voltage Delta", "mV", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSPackPower] = {
        DataPointID::BMSPackPower, "BMS Pack Power", "W", "BMS",
        1.0, 0.0, true
    };
    m_registry[DataPointID::BMSStackVoltage] = {
        DataPointID::BMSStackVoltage, "BMS Stack Voltage", "V", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSSOH] = {
        DataPointID::BMSSOH, "BMS State of Health", "%", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSTempIC] = {
        DataPointID::BMSTempIC, "BMS Temp IC", "°C", "BMS",
        1.0, 0.0, true
    };
    m_registry[DataPointID::BMSBalanceActive] = {
        DataPointID::BMSBalanceActive, "BMS Balance Active", "0/1", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSCharging] = {
        DataPointID::BMSCharging, "BMS Charging", "0/1", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSEnergyCharged] = {
        DataPointID::BMSEnergyCharged, "BMS Energy Charged", "Wh", "BMS",
        1.0, 0.0, false
    };
    
    // Cell voltages - Full 16S support (0x0410-0x041F)
    for (int i = 0; i < 16; i++) {
        DataPointID cellId = static_cast<DataPointID>(static_cast<uint32_t>(DataPointID::BMSCell1) + i);
        m_registry[cellId] = {
            cellId, QString("BMS Cell %1 Voltage").arg(i + 1),
            "V", "BMS", 0.001, 0.0, false
        };
    }
    
    // BMS Temperature sensors (TS1, TS2, TS3 from BQ76952)
    m_registry[DataPointID::BMSTemp1] = {
        DataPointID::BMSTemp1, "BMS Temperature Sensor 1 (TS1)", "°C", "BMS",
        0.1, 0.0, true
    };
    m_registry[DataPointID::BMSTemp2] = {
        DataPointID::BMSTemp2, "BMS Temperature Sensor 2 (TS2)", "°C", "BMS",
        0.1, 0.0, true
    };
    m_registry[DataPointID::BMSTemp3] = {
        DataPointID::BMSTemp3, "BMS Temperature Sensor 3 (TS3)", "°C", "BMS",
        0.1, 0.0, true
    };
    
    // BMS Status Registers
    m_registry[DataPointID::BMSFETStatus] = {
        DataPointID::BMSFETStatus, "BMS FET Control Status", "bitmask", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSSafetyA] = {
        DataPointID::BMSSafetyA, "BMS Safety Register A", "bitmask", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSSafetyB] = {
        DataPointID::BMSSafetyB, "BMS Safety Register B", "bitmask", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSSafetyC] = {
        DataPointID::BMSSafetyC, "BMS Safety Register C", "bitmask", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSErrorFlags] = {
        DataPointID::BMSErrorFlags, "BMS Error Flags", "bitmask", "BMS",
        1.0, 0.0, false
    };
    m_registry[DataPointID::BMSBalancingStatus] = {
        DataPointID::BMSBalancingStatus, "BMS Balancing Status", "bitmask", "BMS",
        1.0, 0.0, false
    };
    
    // ESP-NOW BMS cell voltage IDs (separate from UART to avoid conflicts)
    // ESPNOW uses range 0x1410-0x141F for cells 1-16
    m_registry[static_cast<DataPointID>(0x1410)] = {
        static_cast<DataPointID>(0x1410), "BMS Cell 1 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1411)] = {
        static_cast<DataPointID>(0x1411), "BMS Cell 2 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1412)] = {
        static_cast<DataPointID>(0x1412), "BMS Cell 3 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1413)] = {
        static_cast<DataPointID>(0x1413), "BMS Cell 4 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1414)] = {
        static_cast<DataPointID>(0x1414), "BMS Cell 5 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1415)] = {
        static_cast<DataPointID>(0x1415), "BMS Cell 6 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1416)] = {
        static_cast<DataPointID>(0x1416), "BMS Cell 7 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1417)] = {
        static_cast<DataPointID>(0x1417), "BMS Cell 8 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1418)] = {
        static_cast<DataPointID>(0x1418), "BMS Cell 9 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x1419)] = {
        static_cast<DataPointID>(0x1419), "BMS Cell 10 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x141A)] = {
        static_cast<DataPointID>(0x141A), "BMS Cell 11 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x141B)] = {
        static_cast<DataPointID>(0x141B), "BMS Cell 12 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x141C)] = {
        static_cast<DataPointID>(0x141C), "BMS Cell 13 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x141D)] = {
        static_cast<DataPointID>(0x141D), "BMS Cell 14 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x141E)] = {
        static_cast<DataPointID>(0x141E), "BMS Cell 15 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    m_registry[static_cast<DataPointID>(0x141F)] = {
        static_cast<DataPointID>(0x141F), "BMS Cell 16 Voltage (ESPNOW)",
        "V", "BMS", 0.001, 0.0, false
    };
    // ─── SYSTEM (0x0500–0x05FF) ───────────────────────────
    m_registry[DataPointID::SysCPUTemp] = {
        DataPointID::SysCPUTemp, "System CPU Temperature", "°C", "SYSTEM",
        1.0, 0.0, true
    };
    m_registry[DataPointID::SysFreeHeap] = {
        DataPointID::SysFreeHeap, "System Free Heap", "KB", "SYSTEM",
        1.0, 0.0, false
    };
    m_registry[DataPointID::SysWiFiRSSI] = {
        DataPointID::SysWiFiRSSI, "WiFi RSSI", "dBm", "SYSTEM",
        1.0, 0.0, true
    };
    m_registry[DataPointID::SysUptime] = {
        DataPointID::SysUptime, "System Uptime", "s", "SYSTEM",
        1.0, 0.0, false
    };
    m_registry[DataPointID::SysSDFree] = {
        DataPointID::SysSDFree, "SD Card Free", "MB", "SYSTEM",
        1.0, 0.0, false
    };
    m_registry[DataPointID::SysLogSession] = {
        DataPointID::SysLogSession, "Log Session", "count", "SYSTEM",
        1.0, 0.0, false
    };
    // ─── VESC / DRIVETRAIN (0x0600–0x06FF) ────────────────────────
    // Aligned to opendash catalog / rAtTrax ESP-NOW broadcast
    m_registry[DataPointID::VESCERPM] = {
        DataPointID::VESCERPM, "VESC eRPM", "eRPM", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCMotorCurrent] = {
        DataPointID::VESCMotorCurrent, "VESC Motor Current", "A", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCDutyCycle] = {
        DataPointID::VESCDutyCycle, "VESC Duty Cycle", "duty", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCInputCurrent] = {
        DataPointID::VESCInputCurrent, "VESC Input Current", "A", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCInputVoltage] = {
        DataPointID::VESCInputVoltage, "VESC Input Voltage", "V", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCPowerIn] = {
        DataPointID::VESCPowerIn, "VESC Power In", "W", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCMOSFETTemp] = {
        DataPointID::VESCMOSFETTemp, "VESC MOSFET Temperature", "°C", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCMotorTemp] = {
        DataPointID::VESCMotorTemp, "VESC Motor Temperature", "°C", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCRPM] = {
        DataPointID::VESCRPM, "VESC RPM", "RPM", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCFaultCode] = {
        DataPointID::VESCFaultCode, "VESC Fault Code", "code", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };    m_registry[DataPointID::VESCAhUsed] = {
        DataPointID::VESCAhUsed, "VESC Ah Used", "Ah", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCAhCharged] = {
        DataPointID::VESCAhCharged, "VESC Ah Charged", "Ah", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCWhUsed] = {
        DataPointID::VESCWhUsed, "VESC Wh Used", "Wh", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCWhCharged] = {
        DataPointID::VESCWhCharged, "VESC Wh Charged", "Wh", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCPIDPos] = {
        DataPointID::VESCPIDPos, "VESC PID Position", "degrees", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCTacho] = {
        DataPointID::VESCTacho, "VESC Tachometer", "count", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };
    m_registry[DataPointID::VESCADC1] = {
        DataPointID::VESCADC1, "VESC ADC 1", "V", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCADC2] = {
        DataPointID::VESCADC2, "VESC ADC 2", "V", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCADC3] = {
        DataPointID::VESCADC3, "VESC ADC 3", "V", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCPPM] = {
        DataPointID::VESCPPM, "VESC PPM", "0–1", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::VESCPowerMotor] = {
        DataPointID::VESCPowerMotor, "VESC Power Motor", "W", "VESC/DRIVETRAIN",
        1.0, 0.0, true
    };    
    // Wheel RPMs
    m_registry[DataPointID::WheelRPMFL] = {
        DataPointID::WheelRPMFL, "Wheel RPM Front-Left", "RPM", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::WheelRPMFR] = {
        DataPointID::WheelRPMFR, "Wheel RPM Front-Right", "RPM", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::WheelRPMRL] = {
        DataPointID::WheelRPMRL, "Wheel RPM Rear-Left", "RPM", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::WheelRPMRR] = {
        DataPointID::WheelRPMRR, "Wheel RPM Rear-Right", "RPM", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };
    m_registry[DataPointID::WheelRPMAvg] = {
        DataPointID::WheelRPMAvg, "Average Speed", "mph", "VESC/DRIVETRAIN",
        1.0, 0.0, false
    };

    // Build category index
    for (auto it = m_registry.begin(); it != m_registry.end(); ++it) {
        m_categories[it.value().category].append(it.key());
    }
}

bool DataPointRegistry::isValid(DataPointID id) const {
    return m_registry.contains(id);
}

QVector<DataPointID> DataPointRegistry::allPoints() const {
    QVector<DataPointID> result;
    result.reserve(m_registry.size());
    for (auto it = m_registry.begin(); it != m_registry.end(); ++it) {
        result.append(it.key());
    }
    return result;
}

QVector<DataPointID> DataPointRegistry::pointsInCategory(const QString& category) const {
    auto it = m_categories.find(category);
    if (it != m_categories.end()) {
        return it.value();
    }
    return QVector<DataPointID>();
}

QVector<DataPointID> DataPointRegistry::allPointsSorted() const {
    QVector<DataPointID> result = allPoints();
    std::sort(result.begin(), result.end(), [this](DataPointID a, DataPointID b) {
        const PointInfo& infoA = m_registry[a];
        const PointInfo& infoB = m_registry[b];
        // First sort by category
        if (infoA.category != infoB.category) {
            return infoA.category < infoB.category;
        }
        // Then sort by name within category
        return infoA.name < infoB.name;
    });
    return result;
}

QVector<DataPointID> DataPointRegistry::pointsInCategorySorted(const QString& category) const {
    QVector<DataPointID> result = pointsInCategory(category);
    std::sort(result.begin(), result.end(), [this, category](DataPointID a, DataPointID b) {
        const PointInfo& infoA = m_registry[a];
        const PointInfo& infoB = m_registry[b];
        // Sort by name within category
        return infoA.name < infoB.name;
    });
    return result;
}

DataPointRegistry::PointInfo DataPointRegistry::getInfo(DataPointID id) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return it.value();
    }
    // Not explicitly registered: fall back to the UnifiedDataPoint helpers so
    // every ID still gets a proper label/unit (never "Unknown")
    return {id, DataPointName(id), DataPointUnit(id), DataPointCategory(id), 1.0, 0.0, true};
}

double DataPointRegistry::toSI(DataPointID id, int16_t raw) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return raw * it.value().rawToSI + it.value().siOffset;
    }
    return static_cast<double>(raw);
}

double DataPointRegistry::toSI(DataPointID id, uint16_t raw) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return raw * it.value().rawToSI + it.value().siOffset;
    }
    return static_cast<double>(raw);
}

double DataPointRegistry::toSI(DataPointID id, int32_t raw) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return raw * it.value().rawToSI + it.value().siOffset;
    }
    return static_cast<double>(raw);
}

double DataPointRegistry::toSI(DataPointID id, uint32_t raw) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return raw * it.value().rawToSI + it.value().siOffset;
    }
    return static_cast<double>(raw);
}

double DataPointRegistry::toSI(DataPointID id, float raw) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return raw * static_cast<float>(it.value().rawToSI) + static_cast<float>(it.value().siOffset);
    }
    return static_cast<double>(raw);
}

double DataPointRegistry::toSI(DataPointID id, double raw) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return raw * it.value().rawToSI + it.value().siOffset;
    }
    return raw;
}

int16_t DataPointRegistry::fromSI(DataPointID id, double si) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end() && it.value().rawToSI != 0.0) {
        return static_cast<int16_t>((si - it.value().siOffset) / it.value().rawToSI);
    }
    return static_cast<int16_t>(si);
}

uint16_t DataPointRegistry::fromSI_signed(DataPointID id, double si) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end() && it.value().rawToSI != 0.0) {
        return static_cast<uint16_t>((si - it.value().siOffset) / it.value().rawToSI);
    }
    return static_cast<uint16_t>(si);
}

QString DataPointRegistry::description(DataPointID id) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return QString("%1 (%2 %3)").arg(it.value().name, QString::number(static_cast<uint32_t>(id), 16).toUpper(), it.value().unit);
    }
    return QString("%1 (%2 %3)").arg(DataPointName(id),
        QString::number(static_cast<uint32_t>(id), 16).toUpper(), DataPointUnit(id));
}

bool DataPointRegistry::isNumeric(DataPointID id) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return QString(it.value().unit) != "bitmask" && QString(it.value().unit) != "quality" && QString(it.value().unit) != "gear";
    }
    return true;
}

bool DataPointRegistry::isTemperature(DataPointID id) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return QString(it.value().unit) == "°C";
    }
    return false;
}

bool DataPointRegistry::isVoltage(DataPointID id) const {
    auto it = m_registry.find(id);
    if (it != m_registry.end()) {
        return QString(it.value().unit) == "V";
    }
    return false;
}
