/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */
#ifndef UNIFIEDDATAPoint_H
#define UNIFIEDDATAPoint_H

#include <cstdint>
#include <QString>
#include <QDateTime>
#include <QReadWriteLock>
#include <QHash>
#include <QVector>
#include <functional>

/**
 * @brief Unified Data Point ID space — matches OpenDash DP IDs (0x0100–0x07FF)
 * 
 * This is the single source of truth for ALL data points across all 4 hardware systems.
 * Every system's data gets mapped into this unified namespace.
 */
enum class DataPointID : uint32_t {
    // ─── ENGINE / ECU (0x0100–0x01FF) ──────────────────────────────
    // Aligned to opendash_dp_catalog.c (authoritative wire namespace)
    RPM = 0x0100,                    // Engine RPM
    VehicleSpeed = 0x0101,           // km/h
    CoolantTemp = 0x0102,            // °C
    IntakeTemp = 0x0103,             // °C
    EngineLoad = 0x0104,             // %
    ThrottlePosition = 0x0105,       // %
    BoostPressure = 0x0106,          // kPa
    OilTemp = 0x0107,                // °C
    OilPressure = 0x0108,            // kPa
    FuelPressure = 0x0109,           // kPa
    AFR = 0x010A,                    // Air-fuel ratio
    Lambda = 0x010B,                 // λ
    EGT = 0x010C,                    // °C (single probe)
    BatteryVoltage = 0x010D,         // V
    TimingAdvance = 0x010E,          // degrees
    MAF = 0x010F,                    // g/s
    FuelLevel = 0x0110,              // %
    TransTemp = 0x0111,              // °C
    EGT1 = 0x0112,                   // Exhaust Gas Temp cylinder 1
    EGT2 = 0x0113,
    EGT3 = 0x0114,
    EGT4 = 0x0115,
    O2Lambda = 0x0116,               // λ (secondary O2)
    MDRPM = 0x0117,                  // RPM (MultiDisplay-sourced)
    EGT5 = 0x0118,
    EGT6 = 0x0119,
    EGT7 = 0x011A,
    EGT8 = 0x011B,
    OBD2Flags = 0x011C,              // bitmask
    MILOn = 0x011D,                  // 0/1
    DTCCount = 0x011E,               // count
    // MD-Unified local extensions (0x0130+, NOT part of the ESP-NOW catalog)
    Gear = 0x0130,                   // 1–6, 0 = neutral
    EFRSpeed = 0x0131,               // RPM (EFR pulse width)
    InjectorPulseWidth = 0x0132,     // ms
    MapPressure = 0x0133,            // kPa
    N75DutyCycle = 0x0134,           // % (N75 valve duty cycle)
    N75Setpoint = 0x0135,            // kPa (boost setpoint)
    Knock = 0x0136,                  // Knock sensor input (raw uint16 counts)
    VdoPressure1 = 0x0137,           // VDO pressure sensor 1 (raw)
    VdoPressure2 = 0x0138,
    VdoPressure3 = 0x0139,
    VdoTemp1 = 0x013A,               // VDO temperature sensor 1 (raw)
    VdoTemp2 = 0x013B,
    VdoTemp3 = 0x013C,
    N75Pwm = 0x013D,                 // N75 requested boost PWM (raw 0-255)
    EcuFlags = 0x013E,               // ECU status flags (bitmask byte)

    // ─── MD-ECU Digifant block (0x0140+, main frame payload 57-91) ──
    DFBoost = 0x0140,                // Digifant boost/raw MAP input (raw)
    DFLambda = 0x0141,               // Digifant lambda sensor input (raw)
    DFKnock = 0x0142,                // Digifant knock input (raw)
    DFEct = 0x0143,                  // Digifant ECT via NTC map (°C)
    DFIat = 0x0144,                  // Digifant IAT via NTC map (°C)
    DFThrottlePoti = 0x0145,         // Digifant throttle potentiometer (raw)
    DFFlags = 0x0146,                // Digifant status flags (bitmask byte)
    DFIgnition = 0x0147,             // °: raw × −0.351563 + 73.9
    DFLcFlags = 0x0148,              // Digifant lambda-check flags (bitmask byte)
    DFCyl1KnockRetard = 0x0149,      // °: raw × 0.351563
    DFCyl2KnockRetard = 0x014A,
    DFCyl3KnockRetard = 0x014B,
    DFCyl4KnockRetard = 0x014C,
    DFCyl1KnockDecay = 0x014D,       // raw counts
    DFCyl2KnockDecay = 0x014E,
    DFCyl3KnockDecay = 0x014F,
    DFCyl4KnockDecay = 0x0150,
    DFVoltage = 0x0151,              // V via Digifant voltage map
    DFInjectPulse = 0x0152,          // µs: BE uint16 × 2
    DFColdEnrich = 0x0153,           // raw enrichment bytes
    DFWarmEnrich = 0x0154,
    DFEctEnrich = 0x0155,
    DFAccelEnrich = 0x0156,
    DFCounterEnrich = 0x0157,
    DFIatEnrich = 0x0158,
    DFIgnCounter = 0x0159,           // raw
    DFIgnAddon = 0x015A,             // raw
    DFEctAddon = 0x015B,             // raw
    DFISV = 0x015C,                  // idle spark value (raw; ancestor ISV map formula unported)
    DFRpmDeltaHall = 0x015D,         // BE uint16 raw (µs hall delta)
    DFComputedRpm = 0x015E,          // 30,000,000 / deltaHall
    DFKlineFreq = 0x015F,            // K-line bit rate (raw uint16 LE)
    DFActiveFrame = 0x0160,          // 255 = no Digifant link
    
    // ─── GPS / NAVIGATION (0x0200–0x02FF) ─────────────────────────
    GPSSpeed = 0x0200,               // km/h from GPS
    GPSHeading = 0x0201,             // degrees
    GPSLatitude = 0x0202,            // decimal degrees
    GPSLongitude = 0x0203,           // decimal degrees
    GPSAltitude = 0x0204,            // meters
    GPSSatellites = 0x0205,          // count
    GPSFixQuality = 0x0206,          // rAtTrax: fix type / opendash catalog: HDOP
    GPSCourse = 0x0207,              // rAtTrax: course / opendash catalog: lap number
    LapTime = 0x0208,                // ms
    BestLap = 0x0209,                // ms
    LapDelta = 0x020A,               // ms
    SectorTime = 0x020B,             // ms
    PredictiveLap = 0x020C,          // ms
    GPSFix = 0x020D,                 // 0–3
    
    // ─── IMU (0x0300–0x03FF) ──────────────────────────────────────
    GForceLat = 0x0300,              // g
    GForceLong = 0x0301,             // g
    GForceVert = 0x0302,             // g
    YawRate = 0x0303,                // °/s
    PitchRate = 0x0304,              // °/s
    RollRate = 0x0305,               // °/s
    IMUPitch = 0x0306,               // degrees (DMP)
    IMURoll = 0x0307,                // degrees (DMP)
    IMUYaw = 0x0308,                 // degrees (extension)
    IMUAx = 0x0309,                  // m/s²
    IMUAy = 0x030A,
    IMUAz = 0x030B,
    
    // ─── BATTERY / BMS (0x0400–0x04FF) ────────────────────────────
    // Aligned to rAtTrax-BMS ESP-NOW broadcast map
    BMSPackVoltage = 0x0400,         // V
    BMSPackCurrent = 0x0401,         // A (signed)
    BMSSOC = 0x0402,                 // % state of charge
    BMSMinCellVoltage = 0x0403,      // V
    BMSMaxCellVoltage = 0x0404,      // V
    BMSCellDelta = 0x0405,           // mV (max-min)×1000
    BMSTemp = 0x0406,                // °C (BQ76952 die)
    BMSPackPower = 0x0407,           // W (V×I)
    BMSStackVoltage = 0x0408,        // V (sum of cells)
    BMSSOH = 0x0409,                 // % state of health
    BMSTempIC = 0x040A,              // °C (opendash catalog)
    BMSBalanceActive = 0x040B,       // 0/1
    BMSCharging = 0x040C,            // 0/1
    BMSEnergyCharged = 0x040D,       // Wh
    
    // Cell voltages - Full 16S support (0x0410–0x041F)
    BMSCell1 = 0x0410,
    BMSCell2 = 0x0411,
    BMSCell3 = 0x0412,
    BMSCell4 = 0x0413,
    BMSCell5 = 0x0414,
    BMSCell6 = 0x0415,
    BMSCell7 = 0x0416,
    BMSCell8 = 0x0417,
    BMSCell9 = 0x0418,
    BMSCell10 = 0x0419,
    BMSCell11 = 0x041A,
    BMSCell12 = 0x041B,
    BMSCell13 = 0x041C,
    BMSCell14 = 0x041D,
    BMSCell15 = 0x041E,
    BMSCell16 = 0x041F,
    
    // BMS Status Registers (rAtTrax ESP-NOW 0x0420–0x0425)
    BMSFETStatus = 0x0420,           // FET control status
    BMSSafetyA = 0x0421,             // Safety register A
    BMSSafetyB = 0x0422,             // Safety register B
    BMSSafetyC = 0x0423,             // Safety register C
    BMSErrorFlags = 0x0424,          // Error flags
    BMSBalancingStatus = 0x0425,     // Balancing status bitmask
    
    // Local extensions: BQ76952 thermistors (not on the wire)
    BMSTemp1 = 0x042A,               // °C (thermistor TS1)
    BMSTemp2 = 0x042B,               // °C (thermistor TS2)
    BMSTemp3 = 0x042C,               // °C (thermistor TS3)
    
    // ─── SYSTEM (0x0500–0x05FF) ───────────────────────────────────
    SysCPUTemp = 0x0500,             // °C
    SysFreeHeap = 0x0501,            // KB
    SysWiFiRSSI = 0x0502,            // dBm
    SysUptime = 0x0503,              // s
    SysSDFree = 0x0504,              // MB
    SysLogSession = 0x0505,          // count
    
    // ─── VESC / DRIVETRAIN (0x0600–0x06FF) ────────────────────────
    // Aligned to opendash catalog / rAtTrax VESC broadcast
    VESCERPM = 0x0600,               // eRPM (motor)
    VESCMotorCurrent = 0x0601,       // A (signed)
    VESCDutyCycle = 0x0602,          // duty (0–1 rAtTrax / % opendash)
    VESCAhUsed = 0x0603,             // Ah
    VESCAhCharged = 0x0604,          // Ah
    VESCWhUsed = 0x0605,             // Wh
    VESCWhCharged = 0x0606,          // Wh
    VESCMOSFETTemp = 0x0607,         // °C
    VESCMotorTemp = 0x0608,          // °C
    VESCInputCurrent = 0x0609,       // A
    VESCPIDPos = 0x060A,             // degrees
    VESCTacho = 0x060B,              // count
    VESCInputVoltage = 0x060C,       // V
    VESCADC1 = 0x060D,               // V
    VESCADC2 = 0x060E,               // V
    VESCADC3 = 0x060F,               // V
    VESCPPM = 0x0610,                // 0–1
    VESCRPM = 0x0611,                // RPM
    VESCPowerIn = 0x0612,            // W
    VESCPowerMotor = 0x0613,         // W
    VESCFaultCode = 0x0614,          // fault enum
    
    // Wheel RPM (from VESC encoders)
    WheelRPMFL = 0x0620,             // Front-left wheel RPM
    WheelRPMFR = 0x0621,             // Front-right wheel RPM
    WheelRPMRL = 0x0622,             // Rear-left wheel RPM
    WheelRPMRR = 0x0623,             // Rear-right wheel RPM
    WheelRPMAvg = 0x0624,            // mph average speed
    
    // ─── RELAY / MOS (0x0700–0x0743) — handled by range in helpers ─
    
    // ─── END OF RANGE ─────────────────────────────────────────────
    Invalid = 0xFFFFFFFF
};

// Helper: get category string for a data point
inline const char* DataPointCategory(DataPointID id) {
    uint32_t val = static_cast<uint32_t>(id);
    if (val >= 0x1410 && val <= 0x141F) return "BMS";   // ESP-NOW cell aliases
    if (val >= 0x0100 && val <= 0x01FF) return "ENGINE/ECU";
    if (val >= 0x0200 && val <= 0x02FF) return "GPS/NAV";
    if (val >= 0x0300 && val <= 0x03FF) return "IMU";
    if (val >= 0x0400 && val <= 0x04FF) return "BMS";
    if (val >= 0x0500 && val <= 0x05FF) return "SYSTEM";
    if (val >= 0x0600 && val <= 0x06FF) return "VESC/DRIVETRAIN";
    if (val >= 0x0700 && val <= 0x07FF) return "RELAY/MOS";
    return "UNKNOWN";
}

// Helper: get human-readable name for a data point.
// NEVER returns "Unknown" for in-range IDs — falls back to a generated label
// like "DP 0x0421" so unmapped streams remain identifiable and plottable.
inline QString DataPointName(DataPointID id) {
    uint32_t val = static_cast<uint32_t>(id);
    
    // ESP-NOW BMS cell aliases (remapped by OpenDashBridge from 0x0410+)
    if (val >= 0x1410 && val <= 0x141F) {
        return QStringLiteral("BMS Cell %1 (ESP-NOW)").arg(val - 0x1410 + 1);
    }
    // Relay / MOS channels (0x0700–0x0743)
    if (val >= 0x0700 && val <= 0x0703) return QStringLiteral("HD Relay CH%1").arg(val - 0x0700 + 1);
    if (val >= 0x0710 && val <= 0x0717) return QStringLiteral("Relay A CH%1").arg(val - 0x0710 + 1);
    if (val >= 0x0720 && val <= 0x0727) return QStringLiteral("Relay B CH%1").arg(val - 0x0720 + 1);
    if (val >= 0x0730 && val <= 0x0733) return QStringLiteral("MOS A CH%1").arg(val - 0x0730 + 1);
    if (val >= 0x0740 && val <= 0x0743) return QStringLiteral("MOS B CH%1").arg(val - 0x0740 + 1);
    
    switch (id) {
        case DataPointID::RPM:                    return "Engine RPM";
        case DataPointID::VehicleSpeed:           return "Vehicle Speed";
        case DataPointID::CoolantTemp:            return "Coolant Temp";
        case DataPointID::IntakeTemp:             return "Intake Temp";
        case DataPointID::EngineLoad:             return "Engine Load";
        case DataPointID::ThrottlePosition:       return "Throttle Position";
        case DataPointID::BoostPressure:          return "Boost Pressure";
        case DataPointID::OilTemp:                return "Oil Temp";
        case DataPointID::OilPressure:            return "Oil Pressure";
        case DataPointID::FuelPressure:           return "Fuel Pressure";
        case DataPointID::AFR:                    return "Air-Fuel Ratio";
        case DataPointID::Lambda:                 return "Lambda";
        case DataPointID::EGT:                    return "EGT";
        case DataPointID::BatteryVoltage:         return "Battery Voltage";
        case DataPointID::TimingAdvance:          return "Timing Advance";
        case DataPointID::MAF:                    return "MAF";
        case DataPointID::FuelLevel:              return "Fuel Level";
        case DataPointID::TransTemp:              return "Trans Temp";
        case DataPointID::EGT1:                   return "EGT 1";
        case DataPointID::EGT2:                   return "EGT 2";
        case DataPointID::EGT3:                   return "EGT 3";
        case DataPointID::EGT4:                   return "EGT 4";
        case DataPointID::O2Lambda:               return "O2 Lambda";
        case DataPointID::MDRPM:                  return "MD RPM";
        case DataPointID::EGT5:                   return "EGT 5";
        case DataPointID::EGT6:                   return "EGT 6";
        case DataPointID::EGT7:                   return "EGT 7";
        case DataPointID::EGT8:                   return "EGT 8";
        case DataPointID::OBD2Flags:              return "OBD2 Flags";
        case DataPointID::MILOn:                  return "MIL On";
        case DataPointID::DTCCount:               return "DTC Count";
        case DataPointID::Gear:                   return "Gear";
        case DataPointID::EFRSpeed:               return "EFR Speed";
        case DataPointID::InjectorPulseWidth:     return "Injector Pulse Width";
        case DataPointID::MapPressure:            return "MAP Pressure";
        case DataPointID::N75DutyCycle:           return "N75 Duty Cycle";
        case DataPointID::N75Setpoint:            return "N75 Setpoint";
        case DataPointID::GPSSpeed:               return "GPS Speed";
        case DataPointID::GPSHeading:             return "GPS Heading";
        case DataPointID::GPSLatitude:            return "GPS Latitude";
        case DataPointID::GPSLongitude:           return "GPS Longitude";
        case DataPointID::GPSAltitude:            return "GPS Altitude";
        case DataPointID::GPSSatellites:          return "GPS Satellites";
        case DataPointID::GPSFixQuality:          return "GPS Fix Quality";
        case DataPointID::GPSCourse:              return "GPS Course";
        case DataPointID::LapTime:                return "Lap Time";
        case DataPointID::BestLap:                return "Best Lap";
        case DataPointID::LapDelta:               return "Lap Delta";
        case DataPointID::SectorTime:             return "Sector Time";
        case DataPointID::PredictiveLap:          return "Predictive Lap";
        case DataPointID::GPSFix:                 return "GPS Fix";
        case DataPointID::GForceLat:              return "G-Force Lateral";
        case DataPointID::GForceLong:             return "G-Force Longitudinal";
        case DataPointID::GForceVert:             return "G-Force Vertical";
        case DataPointID::YawRate:                return "Yaw Rate";
        case DataPointID::PitchRate:              return "Pitch Rate";
        case DataPointID::RollRate:               return "Roll Rate";
        case DataPointID::IMUPitch:               return "IMU Pitch";
        case DataPointID::IMURoll:                return "IMU Roll";
        case DataPointID::IMUYaw:                 return "IMU Yaw";
        case DataPointID::IMUAx:                  return "IMU Ax";
        case DataPointID::IMUAy:                  return "IMU Ay";
        case DataPointID::IMUAz:                  return "IMU Az";
        case DataPointID::BMSPackVoltage:         return "BMS Pack Voltage";
        case DataPointID::BMSPackCurrent:         return "BMS Pack Current";
        case DataPointID::BMSSOC:                 return "BMS State of Charge";
        case DataPointID::BMSMinCellVoltage:      return "BMS Min Cell Voltage";
        case DataPointID::BMSMaxCellVoltage:      return "BMS Max Cell Voltage";
        case DataPointID::BMSCellDelta:           return "BMS Cell Delta";
        case DataPointID::BMSTemp:                return "BMS Temperature";
        case DataPointID::BMSPackPower:           return "BMS Pack Power";
        case DataPointID::BMSStackVoltage:        return "BMS Stack Voltage";
        case DataPointID::BMSSOH:                 return "BMS State of Health";
        case DataPointID::BMSTempIC:              return "BMS IC Temp";
        case DataPointID::BMSBalanceActive:       return "BMS Balance Active";
        case DataPointID::BMSCharging:            return "BMS Charging";
        case DataPointID::BMSEnergyCharged:       return "BMS Energy Charged";
        case DataPointID::BMSCell1:               return "BMS Cell 1";
        case DataPointID::BMSCell2:               return "BMS Cell 2";
        case DataPointID::BMSCell3:               return "BMS Cell 3";
        case DataPointID::BMSCell4:               return "BMS Cell 4";
        case DataPointID::BMSCell5:               return "BMS Cell 5";
        case DataPointID::BMSCell6:               return "BMS Cell 6";
        case DataPointID::BMSCell7:               return "BMS Cell 7";
        case DataPointID::BMSCell8:               return "BMS Cell 8";
        case DataPointID::BMSCell9:               return "BMS Cell 9";
        case DataPointID::BMSCell10:              return "BMS Cell 10";
        case DataPointID::BMSCell11:              return "BMS Cell 11";
        case DataPointID::BMSCell12:              return "BMS Cell 12";
        case DataPointID::BMSCell13:              return "BMS Cell 13";
        case DataPointID::BMSCell14:              return "BMS Cell 14";
        case DataPointID::BMSCell15:              return "BMS Cell 15";
        case DataPointID::BMSCell16:              return "BMS Cell 16";
        case DataPointID::BMSFETStatus:           return "BMS FET Status";
        case DataPointID::BMSSafetyA:             return "BMS Safety Register A";
        case DataPointID::BMSSafetyB:             return "BMS Safety Register B";
        case DataPointID::BMSSafetyC:             return "BMS Safety Register C";
        case DataPointID::BMSErrorFlags:          return "BMS Error Flags";
        case DataPointID::BMSBalancingStatus:     return "BMS Balancing Status";
        case DataPointID::BMSTemp1:               return "BMS Temp 1 (TS1)";
        case DataPointID::BMSTemp2:               return "BMS Temp 2 (TS2)";
        case DataPointID::BMSTemp3:               return "BMS Temp 3 (TS3)";
        case DataPointID::SysCPUTemp:             return "CPU Temp";
        case DataPointID::SysFreeHeap:            return "Free Heap";
        case DataPointID::SysWiFiRSSI:            return "WiFi RSSI";
        case DataPointID::SysUptime:              return "Uptime";
        case DataPointID::SysSDFree:              return "SD Free";
        case DataPointID::SysLogSession:          return "Log Session";
        case DataPointID::VESCERPM:               return "VESC eRPM";
        case DataPointID::VESCMotorCurrent:       return "VESC Motor Current";
        case DataPointID::VESCDutyCycle:          return "VESC Duty Cycle";
        case DataPointID::VESCAhUsed:             return "VESC Ah Used";
        case DataPointID::VESCAhCharged:          return "VESC Ah Charged";
        case DataPointID::VESCWhUsed:             return "VESC Wh Used";
        case DataPointID::VESCWhCharged:          return "VESC Wh Charged";
        case DataPointID::VESCMOSFETTemp:         return "VESC MOSFET Temp";
        case DataPointID::VESCMotorTemp:          return "VESC Motor Temp";
        case DataPointID::VESCInputCurrent:       return "VESC Input Current";
        case DataPointID::VESCPIDPos:             return "VESC PID Position";
        case DataPointID::VESCTacho:              return "VESC Tachometer";
        case DataPointID::VESCInputVoltage:       return "VESC Input Voltage";
        case DataPointID::VESCADC1:               return "VESC ADC 1";
        case DataPointID::VESCADC2:               return "VESC ADC 2";
        case DataPointID::VESCADC3:               return "VESC ADC 3";
        case DataPointID::VESCPPM:                return "VESC PPM";
        case DataPointID::VESCRPM:                return "VESC RPM";
        case DataPointID::VESCPowerIn:            return "VESC Power In";
        case DataPointID::VESCPowerMotor:         return "VESC Power Motor";
        case DataPointID::VESCFaultCode:          return "VESC Fault Code";
        case DataPointID::WheelRPMFL:             return "Wheel RPM FL";
        case DataPointID::WheelRPMFR:             return "Wheel RPM FR";
        case DataPointID::WheelRPMRL:             return "Wheel RPM RL";
        case DataPointID::WheelRPMRR:             return "Wheel RPM RR";
        case DataPointID::WheelRPMAvg:            return "Avg Speed (mph)";
        default:
            // Identifiable generated label instead of "Unknown"
            return QStringLiteral("DP 0x%1").arg(val, 4, 16, QLatin1Char('0')).toUpper();
    }
}

// Helper: get unit string for a data point
inline QString DataPointUnit(DataPointID id) {
    uint32_t val = static_cast<uint32_t>(id);
    
    if (val >= 0x1410 && val <= 0x141F) return "V";     // ESP-NOW cell aliases
    if (val >= 0x0700 && val <= 0x0743) return "";      // relay/MOS on-off / %
    
    switch (id) {
        case DataPointID::RPM:
        case DataPointID::MDRPM:
        case DataPointID::EFRSpeed:               return "RPM";
        case DataPointID::VehicleSpeed:
        case DataPointID::GPSSpeed:               return "km/h";
        case DataPointID::CoolantTemp:
        case DataPointID::IntakeTemp:
        case DataPointID::OilTemp:
        case DataPointID::TransTemp:
        case DataPointID::EGT:
        case DataPointID::EGT1: case DataPointID::EGT2:
        case DataPointID::EGT3: case DataPointID::EGT4:
        case DataPointID::EGT5: case DataPointID::EGT6:
        case DataPointID::EGT7: case DataPointID::EGT8:
        case DataPointID::BMSTemp:
        case DataPointID::BMSTempIC:
        case DataPointID::BMSTemp1:
        case DataPointID::BMSTemp2:
        case DataPointID::BMSTemp3:
        case DataPointID::SysCPUTemp:
        case DataPointID::VESCMOSFETTemp:
        case DataPointID::VESCMotorTemp:          return "°C";
        case DataPointID::EngineLoad:
        case DataPointID::ThrottlePosition:
        case DataPointID::FuelLevel:
        case DataPointID::BMSSOC:
        case DataPointID::BMSSOH:
        case DataPointID::N75DutyCycle:           return "%";
        case DataPointID::BoostPressure:
        case DataPointID::OilPressure:
        case DataPointID::FuelPressure:
        case DataPointID::MapPressure:
        case DataPointID::N75Setpoint:            return "kPa";
        case DataPointID::AFR:                    return "AFR";
        case DataPointID::Lambda:
        case DataPointID::O2Lambda:               return "λ";
        case DataPointID::BatteryVoltage:
        case DataPointID::BMSPackVoltage:
        case DataPointID::BMSStackVoltage:
        case DataPointID::BMSMinCellVoltage:
        case DataPointID::BMSMaxCellVoltage:
        case DataPointID::BMSCell1: case DataPointID::BMSCell2:
        case DataPointID::BMSCell3: case DataPointID::BMSCell4:
        case DataPointID::BMSCell5: case DataPointID::BMSCell6:
        case DataPointID::BMSCell7: case DataPointID::BMSCell8:
        case DataPointID::BMSCell9: case DataPointID::BMSCell10:
        case DataPointID::BMSCell11: case DataPointID::BMSCell12:
        case DataPointID::BMSCell13: case DataPointID::BMSCell14:
        case DataPointID::BMSCell15: case DataPointID::BMSCell16:
        case DataPointID::VESCInputVoltage:
        case DataPointID::VESCADC1:
        case DataPointID::VESCADC2:
        case DataPointID::VESCADC3:               return "V";
        case DataPointID::TimingAdvance:
        case DataPointID::GPSHeading:
        case DataPointID::GPSCourse:
        case DataPointID::IMUPitch:
        case DataPointID::IMURoll:
        case DataPointID::IMUYaw:
        case DataPointID::VESCPIDPos:             return "°";
        case DataPointID::InjectorPulseWidth:     return "ms";
        case DataPointID::MAF:                    return "g/s";
        case DataPointID::Gear:                   return "gear";
        case DataPointID::GPSLatitude:
        case DataPointID::GPSLongitude:           return "deg";
        case DataPointID::GPSAltitude:            return "m";
        case DataPointID::GPSSatellites:          return "sats";
        case DataPointID::LapTime:
        case DataPointID::BestLap:
        case DataPointID::LapDelta:
        case DataPointID::SectorTime:
        case DataPointID::PredictiveLap:          return "ms";
        case DataPointID::GForceLat:
        case DataPointID::GForceLong:
        case DataPointID::GForceVert:             return "g";
        case DataPointID::YawRate:
        case DataPointID::PitchRate:
        case DataPointID::RollRate:               return "°/s";
        case DataPointID::IMUAx: case DataPointID::IMUAy: case DataPointID::IMUAz:
            return "m/s²";
        case DataPointID::BMSPackCurrent:
        case DataPointID::VESCMotorCurrent:
        case DataPointID::VESCInputCurrent:       return "A";
        case DataPointID::BMSCellDelta:           return "mV";
        case DataPointID::BMSPackPower:
        case DataPointID::VESCPowerIn:
        case DataPointID::VESCPowerMotor:         return "W";
        case DataPointID::BMSEnergyCharged:
        case DataPointID::VESCWhUsed:
        case DataPointID::VESCWhCharged:          return "Wh";
        case DataPointID::VESCAhUsed:
        case DataPointID::VESCAhCharged:          return "Ah";
        case DataPointID::SysFreeHeap:            return "KB";
        case DataPointID::SysWiFiRSSI:            return "dBm";
        case DataPointID::SysUptime:              return "s";
        case DataPointID::SysSDFree:              return "MB";
        case DataPointID::VESCDutyCycle:          return "duty";
        case DataPointID::VESCERPM:               return "eRPM";
        case DataPointID::VESCRPM:                return "RPM";
        case DataPointID::WheelRPMFL:
        case DataPointID::WheelRPMFR:
        case DataPointID::WheelRPMRL:
        case DataPointID::WheelRPMRR:             return "RPM";
        case DataPointID::WheelRPMAvg:            return "mph";
        default:                                  return "";
    }
}

#endif // UNIFIEDDATAPoint_H
