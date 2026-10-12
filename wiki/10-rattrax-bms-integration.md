# rAtTrax-BMS & ESPNOW Integration Guide

## Overview

### Primary Connection: USB CDC-ACM
The **rAtTrax-BMS Logger** ESP32 firmware sends battery management data to the **multidisplay-unified** Qt application via USB CDC-ACM serial at 115200 baud.

### Architecture
```
┌─────────────────────┐           USB CDC-ACM          ┌──────────────────┐
│   rAtTrax-BMS       │         115200 baud            │ multidisplay-    │
│   ESP32-DOIT        │ ──────────────────────────────▶ │ unified Qt6 App  │
│   (Firmware v3.1)   │     /dev/ttyUSB0               │ (Qt6/C++)        │
└─────────────────────┘                                └──────────────────┘
```

## USB Serial Output Format

### BMS Data Packet
```
BMS:voltage,current,temp,stack,soc,min,max,delta,cell1,cell2,...,cellN
```

**Example (6S battery):**
```
BMS:0.12,0.00,28.7,23.6,78,3.944,3.962,18.0,3.956,3.958,3.962,3.960,3.944,3.956
```

**Fields:**
| Index | Field | Description | Unit |
|-------|-------|-------------|------|
| 0 | `BMS:` | Prefix (not a value) | - |
| 1 | voltage | Pack voltage | V |
| 2 | current | Pack current (discharge = positive) | A |
| 3 | temp | BMS temperature | °C |
| 4 | stack | Sum of all cell voltages | V |
| 5 | soc | State of Charge | % |
| 6 | min | Minimum cell voltage | V |
| 7 | max | Maximum cell voltage | V |
| 8 | delta | Voltage difference (max-min)*1000 | mV |
| 9+ | cell1,cell2,...,cellN | Individual cell voltages | V |

**Cell Count:** Auto-adjusts to `CONNECTED_CELLS` in config.h (2, 3, 6, 8, 12, or 16)

### VESC Data Packet
```
VESC:RPM,MotorCurrent,Duty%,InputVoltage,FaultCode
```

**Example:**
```
VESC:3200,45.5,75.2,52.3,0
```

### GPS Data Packet
```
GPS:Lat,Lng,Altitude,Speed_mph,Satellites
```

**Example:**
```
GPS:39.7392,-104.9887,1625.3,45.2,14
```

### Alternative: ESPNOW Bridge (Limited Data)
The **openDstream** ESP32 bridge node receives ESP-NOW frames from the OpenDash network (CENTER master broadcasting GPS/IMU data) and forwards them over USB to `/dev/ttyUSB1`.

**Important:** The rAtTrax-BMS firmware does NOT send BMS data via ESPNOW - it only sends via USB serial. To get BMS data over ESPNOW would require:
- Modifying the rAtTrax-BMS firmware to initialize ESP-NOW
- Sending BMS data points (0x0400-0x041F) as DATA_RESPONSE frames

## Data Point Mapping - USB CDC-ACM

| OpenDash DP ID | Name | BMS Field | Type |
|----------------|------|-----------|------|
| 0x0400 | BMSPackVoltage | voltage | float (V) |
| 0x0401 | BMSPackCurrent | current | float (A) |
| 0x0402 | BMSSOC | soc | float (%) |
| 0x0403 | BMSMinCellVoltage | min | float (V) |
| 0x0404 | BMSMaxCellVoltage | max | float (V) |
| 0x0405 | - | delta (computed) | float (mV) |
| 0x0406 | BMSTemp | temp | float (°C) |
| 0x0410-0x041F | BMSCell1-Cell16 | cellN | float (V) |

## ESPNOW Bridge Data - USB CDC-ACM (/dev/ttyUSB1)

The openDstream bridge forwards ESP-NOW frames from CENTER node:
- GPS Data Points (0x0200-0x0207): Speed, Heading, Lat/Lon, Altitude, Satellites
- IMU Data Points (0x0306-0x030B): Pitch, Roll, Yaw, Acceleration

**Note:** BMS data is NOT included in ESPNOW broadcasts - use USB CDC-ACM for full BMS data.

## Qt Application Integration

### Command Line Usage
```bash
# Connect to rAtTrax-BMS via USB CDC-ACM
./multidisplay-unified --bms /dev/ttyUSB0

# Connect all systems simultaneously
./multidisplay-unified \
  --md-ecu /dev/ttyUSB0 \
  --bms /dev/ttyUSB1 \
  --bridge /dev/ttyUSB2
```

### Application UI Integration

**MainWindow → connectBMS()**
```cpp
void MainWindow::connectBMS(const QString& portName) {
    m_bms = new RAtTraxBMS(portName, this);
    connect(m_bms, &RAtTraxBMS::frameParsed,
            this, &MainWindow::onDataPointUpdated);
    m_bms->open();
}
```

**Data Flow:**
1. `RAtTraxBMS` reads from `/dev/ttyUSB0`
2. CSV parser extracts data values
3. `frameParsed(values)` signal emitted
4. `UnifiedDataStore::setDataPoint()` receives and stores
5. `dataPointUpdated(id, value, source)` emitted to UI

### UI Components Displaying BMS Data

| Component | Displays |
|-----------|----------|
| **DashboardWidget** (BMS tab) | Pack V/A/SOC, Cell Min/Max |
| **RealtimePlot** | All data as scrolling plots |
| **LiveDataTable** | All values in text format |

## Troubleshooting

### No Data Showing
1. Verify USB connection: `ls /dev/ttyUSB*`
2. Check permissions: User must be in `dialout` group
3. Verify firmware is sending: `screen /dev/ttyUSB0 115200`
4. Check app console output for debug messages

### Cell Voltages Not Showing
- The firmware sends individual cell voltages based on `CONNECTED_CELLS` config
- Ensure your BMS has the correct number of cells connected
- Verify CSV output format includes cell values (9+ fields)

## Future Enhancements

| Feature | Status |
|---------|--------|
| Individual cell balancing status | Firmware pending |
| Coulomb counting SOC | Firmware pending |
| SOH tracking | Firmware pending |
| ESP-NOW direct bridge | Firmware in progress |

---

**Last Updated:** 2026-06-17  
**Integration Version:** v3.1 (BMS Logger) ↔ v0.2.0 (multidisplay-unified)