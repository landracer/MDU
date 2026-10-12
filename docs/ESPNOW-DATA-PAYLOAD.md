# ESP-NOW Data Payload Format - Unified Integration Guide

## Overview

This document describes the ESP-NOW data payload format used by the OpenDashBridge component to integrate ESPNOW data into the MultiDisplay-Unified system.

## Data Flow Architecture

```
┌─────────────────┐     ESPN-NOW      ┌──────────────────────┐
│   BMS / VESC    │ ───────────────>  │    ESP32 Bridge      │
│   (rAtTrax)     │                   │   (/dev/ttyUSB1)     │
└─────────────────┘                   └──────────────────────┘
                                              │ USB CDC-ACM
                                              │ @ 921600 baud
                                              ▼
                                    ┌──────────────────────┐
                                    │  OpenDashBridge      │
                                    │  (OpenDashBridge.cpp)│
                                    └──────────────────────┘
                                              │ frameParsed()
                                              ▼
                                    ┌──────────────────────┐
                                    │ UnifiedDataStore     │
                                    │ ( setDataPoint() )   │
                                    └──────────────────────┘
                                              │
                                              ├──> LiveDataTable (visible/hidden)
                                              ├──> DashboardWidget (plots)
                                              └──> DataPointRegistry (metadata)
```

## Payload Format

### Text-Based Format (Primary)

ESPNOW data points are sent as text lines in the format:
```
DP:0xXXXX:YYYY.YY
```

Where:
- `DP:` - Prefix indicating a data point update
- `0xXXXX` - 4-digit hexadecimal DataPointID (16-bit)
- `YYYY.YY` - Floating-point value

Examples:
```
DP:0x0100:3250.00    # Engine RPM = 3250
DP:0x0400:48.50      # BMS Pack Voltage = 48.5V
DP:0x0410:3.95       # BMS Cell 1 Voltage = 3.95V
DP:0x060C:2450.00    # VESC eRPM = 2450
```

### Binary Format (Fallback)

When text parsing fails, the system attempts to parse binary ESP-NOW packets:

```
┌─────────┬───────┬───────┬─────────┬─────────┬─────────┬────────┐
│ SYNC    │ CMD   │ LEN   │ DP_ID   │ VALUE   │ CHECKSUM│
│ 0xAA    │ 0x81  │ 0x06  │ HI:LO   │ FLOAT32 │         │
└─────────┴───────┴───────┴─────────┴─────────┴─────────┴────────┘
  Byte 0    Byte 1  Byte 2   Bytes 3-4  Bytes 5-8   Byte 9

Total: 10 bytes
```

Where:
- **SYNC** (0xAA) - Packet synchronization byte
- **CMD** (0x81) - Data response command
- **LEN** (0x06) - Payload length (dp_id hi:lo + float = 6 bytes)
- **DP_ID** (hi:lo, big-endian) - Data point ID (16-bit)
- **VALUE** (4 bytes, little-endian float32) - Sensor value
- **CHECKSUM** (1 byte) - Simple sum checksum

## Supported Data Point IDs

### Engine/ECU (0x0100–0x01FF)

| ID | Name | Unit | Range |
|----|------|------|-------|
| 0x0100 | RPM | RPM | 0-15000 |
| 0x0101 | VehicleSpeed | km/h | 0-300 |
| 0x0102 | OilPressure | kPa | 0-600 |
| 0x0103 | IntakeTemp | °C | -40 to 150 |
| 0x0104 | ThrottlePosition | % | 0-100 |
| 0x0105 | MapPressure | kPa | 0-300 |
| 0x0106 | BoostPressure | kPa | -100 to 300 |
| 0x0107 | FuelPressure | kPa | 0-800 |
| 0x010D | BatteryVoltage | V | 0-30 |
| 0x0110 | FuelLevel | % | 0-100 |
| 0x010E | Gear | gear | 0-6 |

### GPS/Navigation (0x0200–0x02FF)

| ID | Name | Unit | Range |
|----|------|------|-------|
| 0x0200 | GPSSpeed | km/h | 0-500 |
| 0x0201 | GPSHeading | ° | 0-360 |
| 0x0202 | GPSLatitude | deg (×1e6) | -90 to +90 |
| 0x0203 | GPSLongitude | deg (×1e6) | -180 to +180 |
| 0x0204 | GPSAltitude | m | -500 to 9000 |
| 0x0205 | GPSSatellites | count | 0-32 |

### IMU (0x0300–0x03FF)

| ID | Name | Unit | Range |
|----|------|------|-------|
| 0x0306 | IMUPitch | ° | -180 to +180 |
| 0x0307 | IMURoll | ° | -180 to +180 |
| 0x0308 | IMUYaw | ° | 0-360 |
| 0x0309-0x030B | IMU Accel X/Y/Z | m/s² | ±20 |

### BMS / Battery (0x0400–0x04FF) - **Primary Target**

#### Core Metrics

| ID | Name | Unit | Range |
|----|------|------|-------|
| 0x0400 | BMSPackVoltage | V | 0-100 |
| 0x0401 | BMSPackCurrent | A (signed) | -500 to +500 |
| 0x0402 | BMSSOC | % | 0-100 |
| 0x0403 | BMSTemp | °C | -40 to 85 |

#### Cell Voltages - Full 16S Support

| ID | Name | Unit | Range | Notes |
|----|------|------|-------|-------|
| 0x0410-0x041F | BMSCell1-16 Voltage | V | 2.5-4.3 | Each cell |

**Cell voltage ranges:**
- `BMSCell1` (0x0410) to `BMSCell16` (0x041F)
- Each cell supports full 16S LiPo/Li-ion pack
- Nominal range: 2.8V - 4.2V per cell

#### Temperature Sensors (BQ76952)

| ID | Name | Unit | Range |
|----|------|------|-------|
| 0x0420 | BMSTemp1 | °C | TS1 thermistor input |
| 0x0421 | BMSTemp2 | °C | TS2 thermistor input |
| 0x0422 | BMSTemp3 | °C | TS3 thermistor input |

#### Status Registers

| ID | Name | Type | Description |
|----|------|------|-------------|
| 0x0430 | BMSFETStatus | uint8 | FET control status bitmask |
| 0x0431 | BMSSafetyA | uint16 | Safety register A |
| 0x0432 | BMSSafetyB | uint16 | Safety register B |
| 0x0433 | BMSafetyC | uint16 | Safety register C |
| 0x0434 | BMSErrorFlags | uint16 | Error/fault flags bitmask |
| 0x0435 | BMSBalancingStatus | uint16 | Cell balancing status |

### VESC / Drivetrain (0x0600–0x06FF)

| ID | Name | Unit | Range |
|----|------|------|-------|
| 0x0600 | VESCMotorVoltage | V | 0-100 |
| 0x0601 | VESCMotorCurrent | A (signed) | -500 to +500 |
| 0x0602 | VESCAverageCurrent | A | 0-500 |
| 0x0603 | VESCDutyCycle | % | 0-100 |
| 0x0607 | VESCMOSFETTemp | °C | -40 to 125 |
| 0x0608 | VESCMotorTemp | °C | -40 to 150 |
| 0x060C | VESCRPM | eRPM | 0-100000 |

#### Wheel RPM (VESC Encoders)

| ID | Name | Unit | Description |
|----|------|------|-------------|
| 0x0620 | WheelRPMFL | RPM | Front-left wheel |
| 0x0621 | WheelRPMFR | RPM | Front-right wheel |
| 0x0622 | WheelRPMRL | RPM | Rear-left wheel |
| 0x0623 | WheelRPMRR | RPM | Rear-right wheel |

### Special ESP-NOW Aliases (0x1X04 Range)

These are alternative ID mappings used by some firmware versions:

| ID | Name | Maps To |
|----|------|---------|
| 0x1004 | BMS Cell 1 Voltage (ESP-NOW) | BMSCell1 (0x0410) |
| 0x1104 | BMS Cell 2 Voltage (ESP-NOW) | BMSCell2 (0x0411) |
| 0x1204 | BMS Cell 3 Voltage (ESP-NOW) | BMSCell3 (0x0412) |
| 0x1304 | BMS Cell 4 Voltage (ESP-NOW) | BMSCell4 (0x0413) |
| 0x1404 | BMS Cell 5 Voltage (ESP-NOW) | BMSCell5 (0x0414) |
| 0x1504 | BMS Cell 6 Voltage (ESP-NOW) | BMSCell6 (0x0415) |

## Visibility Toggle Support

### Per-Data Point Visibility

Each data point can be toggled visible/hidden via Settings:

```cpp
// Check if a data point is visible
Settings::instance().isDataPointVisible(static_cast<quint16>(dataPointId));

// Set visibility for a data point
Settings::instance().setDataPointVisible(static_cast<quint16>(dataPointId), true/false);
```

### Storage Keys

Visibility settings are stored in Qt Settings:
- **Linux**: `~/.config/MultiDisplay-Unified/settings.conf`
- **Windows**: `HKEY_CURRENT_USER\Software\MultiDisplay-Unified\app\datapoints\0xXXXX`
- **macOS**: `~/Library/Preferences/MultiDisplay-Unified.plist`

### Example Settings File

```ini
[app]
display/showDataTable=true
display/showDashboard=true
display/dataTableRefreshRate=5

[app/datapoints]
0x410=true   # BMSCell1 - Visible
0x411=true   # BMSCell2 - Visible
0x416=false  # BMSCell7 - Hidden (8S BMS doesn't have cells 7-16)
0x420=true   # BMSTemp1 - Visible
```

## Implementation Notes

### OpenDashBridge.cpp Processing

1. **Text Parsing** (`processDPMessage()`):
   ```cpp
   // Regex: ^DP:(0x[0-9a-fA-F]+):([\-+]?[0-9]*\.?[0-9]+)$
   // Extracts ID and value from DP:0xXXXX:value format
   ```

2. **Binary Parsing** (`processBinaryPackets()`):
   - Scans for 0xAA sync byte
   - Validates CMD=0x81, LEN=6
   - Extracts big-endian DP_ID (hi:lo)
   - Extracts little-endian float32 value

3. **Emit frameParsed()**:
   ```cpp
   QHash<DataPointID, double> values;
   values[id] = value;
   emit frameParsed(values);
   ```

### UnifiedDataStore Integration

```cpp
connect(m_bridge, &OpenDashBridge::frameParsed,
        this, [this](const QHash<DataPointID, double>& values) {
    for (auto it = values.begin(); it != values.end(); ++it) {
        UnifiedDataStore::instance().setDataPoint(it.key(), it.value(), "ESP-NOW-Bridge");
    }
});
```

### Data Point Registry

All new ESPNOW IDs must be registered in:
- `UnifiedDataPoint.h` - ID enum definitions
- `UnifiedDataStore.cpp` - InitializeDefaultPoints()
- `DataPointRegistry.cpp` - Metadata (name, unit, category)

## Future Expansion

### Adding New Data Points

1. **Add to UnifiedDataPoint.h**:
   ```cpp
   enum class DataPointID : uint32_t {
       // ...
       NEWDP_ID = 0x07XX,  // Add in appropriate range
   };
   ```

2. **Update DataPointRegistry.cpp**:
   ```cpp
   m_registry[DataPointID::NEWDP_ID] = {
       DataPointID::NEWDP_ID, "New Data Point Name",
       "unit", "CATEGORY", 1.0, 0.0, false
   };
   ```

3. **Register in UnifiedDataStore.cpp**:
   ```cpp
   pointList[DataPointID::NEWDP_ID] = "NEWDP_ID";
   ```

### Supported Ranges

- Engine/ECU: `0x0100–0x01FF`
- GPS/NAV: `0x0200–0x02FF`
- IMU: `0x0300–0x03FF`
- BMS: `0x0400–0x04FF` (Primary expansion area)
- Vehicle: `0x0500–0x05FF`
- VESC/Drivetrain: `0x0600–0x06FF`

## Testing

### Verify ESPNOW Data Flow

```bash
# Monitor the bridge port for DP lines
cat /dev/ttyUSB1 | grep "DP:"

# Should see:
# DP:0x0400:48.50
# DP:0x0410:3.95
# DP:0x060C:2450.00
```

### Check Qt Application

1. Connect to `/dev/ttyUSB1` as bridge port
2. Open Data Point Visibility dialog
3. Verify all ESPNOW data points appear in list
4. Toggle individual points on/off
5. Confirm values update in real-time

## References

- `multidisplay-unified/src/OpenDashBridge.cpp` - ESP-NOW receiver
- `multidisplay-unified/include/UnifiedDataPoint.h` - ID definitions
- `multidisplay-unified/include/DataPointRegistry.h` - Metadata registry
- `multidisplay-unified/include/LiveDataTable.h` - UI visibility control