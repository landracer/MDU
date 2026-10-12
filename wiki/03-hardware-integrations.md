# Hardware Integrations

## Overview

MultiDisplay Unified supports **four distinct hardware systems**, each with its own communication protocol, data format, and connection method. All systems feed into a unified data model using OpenDash-compatible Data Point IDs (0x0100–0x07FF).

---

## 1. MultiDisplay ECU (ATmega2560)

### Hardware Description

The original MultiDisplay ECU based on the ATmega2560 microcontroller. This is the core engine management system that provides:
- Engine RPM, boost pressure, lambda, EGT readings
- Digifant K-line OBD2 communication
- N75 boost valve control
- Gear position detection
- Battery voltage monitoring

### Connection

| Parameter | Value |
|-----------|-------|
| Interface | USB Serial (CDC-ACM) |
| Baud Rate | 115200 |
| Port Type | `/dev/ttyUSB*` (Linux), `COMx` (Windows) |
| Data Format | Custom binary protocol |

### Binary Protocol — CRITICAL IMPLEMENTATION NOTES

**⚠️ READ THIS FIRST: The TAG byte value IS the total frame length in bytes.** Main sensor frame tag `0x5F` = 95 decimal → a 95-byte frame (STX + TAG + 92-byte payload + ETX).

The parser counts bytes until it has received `TAG` bytes and only then *validates* that the last byte is ETX (`0x03`). A `0x03` byte **inside the payload does not end the frame** — framing ends by position, not by the first ETX-like byte.

```
┌──────┬───────┬──────────────────┬──────┐
│ STX  │ TAG   │ DATA PAYLOAD     │ ETX  │
│ 0x02 │ byte  │ variable length  │ 0x03 │
└──────┴───────┴──────────────────┴──────┘
```

| Field | Value | Description |
|-------|-------|-------------|
| **STX** | `0x02` | Start byte (fixed) |
| **TAG** | Varies | Data type identifier — **its decimal value is the total frame length** (0x5F = 95 bytes) |
| **DATA** | Variable | Sensor values in fixed-point format |
| **ETX** | `0x03` | End byte (fixed) |

### Frame Types and Sizes

| TAG (Hex) | TAG (Dec) | Description | Total Size | Payload Size | Notes |
|-----------|-----------|-------------|------------|--------------|-------|
| `0x5F` | 95 | **Main sensor data** | **95 bytes** | **92 bytes** | Full engine telemetry — this is the primary frame |
| `0x16` | 22 | N75 duty map | 22 bytes | 19 bytes | Control frame |
| `0x26` | 38 | N75 setpoint map | 38 bytes | 35 bytes | Control frame |
| `0x04` | 4 | ACK frame | 4 bytes | 1 byte | Acknowledgment |
| `0x17` | 23 | N75 params | 23 bytes | 20 bytes | Configuration |
| `0x11` | 17 | Gear ratio (6G) | 17 bytes | 14 bytes | Configuration |
| `0x45` | 69 | Boost PID | 69 bytes | 66 bytes | PID tuning data |

**Key insight:** every TAG value equals that frame type's total byte count (22-byte duty-map frame has TAG 22, etc.). The receiver buffers TAG bytes, verifies the last is ETX, and dispatches by tag. Frames with TAG < 95 bytes are control/config frames — skip them when parsing main sensor data.

### Main Sensor Frame Payload Layout (TAG = 0x5F, 92 bytes payload)

This is the complete engine telemetry frame. All multi-byte values are Little Endian.

| Offset | Size | Type | Field | Scaling | DataPointID |
|--------|------|------|-------|---------|-------------|
| 0-3 | 4B | uint32 LE | timestamp | millis() (not mapped) | — |
| 4-5 | 2B | int16 LE | calRPM | Raw RPM | `DataPointID::RPM` |
| 6-7 | 2B | uint16 LE | calAbsoluteBoost | /100 → Bar | `DataPointID::BoostPressure` |
| 8 | 1B | uint8 | throttle | /255 × 100 → % | `DataPointID::ThrottlePosition` |
| 9-10 | 2B | uint16 LE | calLambdaF | /100 → ratio | `DataPointID::Lambda` |
| 11-12 | 2B | uint16 LE | calLMM | /100 → g/s | `DataPointID::MAF` |
| 13-14 | 2B | uint16 LE | caseTemp | /100 → °C | `DataPointID::CoolantTemp` |
| 15-30 | 16B | uint16×8 LE | calEgt[0..7] | Raw °C | `EGT1` through `EGT8` |
| 31-32 | 2B | uint16 LE | batVolt | /100 → V | `DataPointID::BatteryVoltage` |
| 33-44 | 12B | uint16×6 LE | vdoPres[1..3], vdoTemp[1..3] | Raw values | Not mapped (reserved) |
| 45-46 | 2B | uint16 LE | speedF | /100 → km/h | `DataPointID::VehicleSpeed` |
| 47 | 1B | uint8 | gear | 0=neutral, 1-6=gear | `DataPointID::Gear` |
| 48 | 1B | uint8 | n75_duty | Raw 0-255 | `DataPointID::N75DutyCycle` |
| 49-50 | 2B | uint16 LE | n75_req_boost | /100 → Bar | `DataPointID::N75Setpoint` |
| 51 | 1B | uint8 | n75_req_boost_pwm | Raw | Not mapped |
| 52 | 1B | uint8 | flags | Status bitmask | Not mapped |
| 53-54 | 2B | uint16 LE | efr_speed_reading | 40,000,000/raw → RPM | `DataPointID::EFRSpeed` |

### State Machine Implementation

The parser uses a byte-by-byte state machine:

```
FRAME_COMPLETE ──STX──→ WAITING_FOR_TAG ──valid TAG──→ RECEIVING
     ↑                                                        │
     └───────FRAMEERROR / next STX ←──ETX(0x03)──────────────┘
```

- **FRAME_COMPLETE**: Waiting for STX (0x02)
- **WAITING_FOR_TAG**: Received STX, checking if TAG is valid
- **RECEIVING**: Collecting bytes until byte position (TAG − 1); that final byte must be ETX (0x03) or the frame is discarded
- **FRAMEERROR**: Corruption detected, reset and wait for next STX

### Common Pitfalls

1. **❌ WRONG:** Ending a frame at the *first* `0x03` byte seen (payload bytes can be 0x03!)
2. **✅ CORRECT:** `framelength = tag_value`; validate ETX only at position `framelength − 1`
3. **❌ WRONG:** Expecting all frames to be the same size
4. **✅ CORRECT:** Only process frames with ≥92 bytes payload for main sensor data
5. **❌ WRONG:** Assuming one frame per `readAll()` call
6. **✅ CORRECT:** Process byte-by-byte; frames can span chunks or multiple frames fit in one chunk

### Command Line Usage

```bash
./multidisplay-unified --md-ecu /dev/ttyACM7
```

### Implementation Details

- **Parser class**: `MdSerialComUnified` (inherits from `ICommunication`)
- **File**: [`src/MdSerialComUnified.cpp`](../src/MdSerialComUnified.cpp)
- **Header**: [`include/MdSerialComUnified.h`](../include/MdSerialComUnified.h)
- **Key features**: ETX-based frame detection, variable-length frame handling, fixed-point conversion
- **Full documentation**: See [wiki/09-md-ecu-binary-protocol.md](./09-md-ecu-binary-protocol.md) for complete protocol reference

---

## 2. rAtTrax-BMS (ESP32-DOIT + BQ76952 + MPU6050 + VESC)

### Hardware Description

The rAtTrax-BMS system is a multi-sensor platform based on the ESP32-DOIT board that aggregates data from:
- **BQ76952 6S BMS** — Battery management via I2C @ 400kHz
- **MPU6050 DMP IMU** — Inertial measurement (pitch, roll, yaw, acceleration)
- **VESC motor controller** — Via CAN bus @ 500kbps
- **Reyax RYS352A GPS** — GPS module @ 115200 baud

### Connection

| Parameter | Value |
|-----------|-------|
| Interface | USB CDC-ACM or Bluetooth SPP |
| Baud Rate | 115200 |
| Port Type | `/dev/ttyACM*` (Linux), `COMx` (Windows) |
| Data Format | CSV text lines or binary protocol |

### CSV Data Format

The rAtTrax-BMS firmware outputs comma-separated values:

```
Timestamp,GPS_Lat,GPS_Lng,GPS_Alt,GPS_Speed_kmh,GPS_Sats,
BMS_SOC,BMS_Voltage,BMS_Current,BMS_Temp,
IMU_Pitch,IMU_Roll,IMU_Yaw,
Speed_Avg_mph,RPM_FL,RPM_FR,RPM_RL,RPM_RR,
VESC_Voltage,VESC_Motor_Temp,VESC_Current,VESC_Duty,
VESC_RPM,VESC_Fault
```

**Field mapping (0-indexed):**

| Index | Field | Data Point ID | Unit |
|-------|-------|---------------|------|
| 0 | Timestamp | — | — |
| 1 | GPS_Lat | GPSLatitude | degrees |
| 2 | GPS_Lng | GPSLongitude | degrees |
| 3 | GPS_Alt | GPSAltitude | meters |
| 4 | GPS_Speed_kmh | GPSSpeed | km/h |
| 5 | GPS_Sats | GPSSatellites | count |
| 6 | BMS_SOC | BMSSOC | % |
| 7 | BMS_Voltage | BMSPackVoltage | V |
| 8 | BMS_Current | BMSPackCurrent | A |
| 9 | BMS_Temp | BMSTemp | °C |
| 10 | IMU_Pitch | IMUPitch | degrees |
| 11 | IMU_Roll | IMURoll | degrees |
| 12 | IMU_Yaw | IMUYaw | degrees |
| 13 | Speed_Avg_mph | WheelRPMAvg | mph |
| 14–17 | Wheel RPMs | WheelRPMFL/FR/RL/RR | RPM |
| 18 | VESC_Voltage | VESCMotorVoltage | V |
| 19 | VESC_Motor_Temp | VESCMotorTemp | °C |
| 20 | VESC_Current | VESCMotorCurrent | A |
| 21 | VESC_Duty | VESCDutyCycle | % |
| 22 | VESC_RPM | VESCRPM | eRPM |
| 23 | VESC_Fault | VESCFaultCode | bitmask |

### Safety Features

The rAtTrax-BMS handler monitors for:
- **Battery critical**: SOC < 5% or sudden drop > 10% → emits `batteryCritical()` signal
- **Thermal warning**: BMS temp > 60°C → emits `thermalWarning()` signal
- **Fault detection**: VESC fault code ≠ 0 → emits `faultDetected()` signal

### Command Line Usage

```bash
./multidisplay-unified --bms /dev/ttyACM0
```

### Implementation Details

- **Parser class**: `RAtTraxBMS` (inherits from `ICommunication`)
- **File**: [`src/RAtTraxBMS.cpp`](../src/RAtTraxBMS.cpp)
- **Modes**: CSV mode (default) or binary mode
- **Buffer management**: 4KB receive buffer with overflow protection

---

## 3. OpenDash Mesh (ESP32-S3 Fleet)

### Hardware Description

The OpenDash Mesh system consists of multiple ESP32-S3 devices that communicate wirelessly via ESP-NOW protocol. Each device acts as a dashboard node displaying sensor data from various sources. The **USB bridge node** receives these ESP-NOW frames and forwards parsed data to the host PC via USB CDC-ACM.

### Connection

| Parameter | Value |
|-----------|-------|
| Interface | USB CDC-ACM (bridge node) |
| Baud Rate | 921600 (fixed by openDstream firmware `UART_BAUD_RATE`) |
| Port Type | `/dev/ttyUSB*` or `/dev/ttyACM*` |
| Data Format | ASCII text lines (`DP:` prefix) |

### USB Bridge Protocol

The bridge node forwards parsed data as newline-delimited ASCII text:

```
DP:<hex_id>:<float_value>\n
```

**Examples:**
```
DP:0x0400:25.20
DP:0x0100:3200.0
DP:0x0306:-1.08
DP:0x0200:0.86
```

### Message Types

| Prefix | Direction | Purpose |
|--------|-----------|---------|
| `DP:` | Bridge → Host | Data Point value update |
| `CFG:` | Bridge → Host | Configuration/status notification |
| `OTA:` | Bridge → Host | Over-the-air update status |
| `ERR:` | Bridge → Host | Error/warning report |

### Implementation Details

- **Parser class**: `OpenDashBridge` (inherits from `ICommunication`)
- **File**: [`src/OpenDashBridge.cpp`](../src/OpenDashBridge.cpp)
- **Regex parser**: Validates DP format with `^DP:(0x[0-9a-fA-F]+):([\\-+]?[0-9]*\\.?[0-9]+)$`
- **ID validation**: Accepts IDs in range 0x0100–0x07FF

### Firmware Requirements

The bridge firmware must:
1. Receive ESP-NOW frames from OpenDash devices
2. Parse the frame payload to extract (dpId, value) pairs
3. Format as `DP:0x<hex_id>:<value>\n` ASCII text
4. Send via Serial.write() over USB CDC-ACM

See [ESP-NOW Bridge Guide](04-espnow-bridge.md) for full implementation details.

### Command Line Usage

```bash
./multidisplay-unified --bridge /dev/ttyUSB3
```

### Verified Working

✅ Live data confirmed on ttyUSB3:
```
DP:0x0306:-1.08    (IMU Pitch)
DP:0x0200:0.00     (GPS Speed)
DP:0x0307:-2.17    (IMU Roll)
DP:0x0200:0.86     (GPS Speed)
```

---

## 4. MDLogger (ATmega328P)

### Hardware Description

The MDLogger is a data logging system based on the ATmega328P microcontroller that writes CSV log files to an SD card. These files can be imported into MultiDisplay Unified for playback and analysis.

### Connection

| Parameter | Value |
|-----------|-------|
| Interface | File import (SD card → PC) |
| Data Format | Standard CSV text file |
| File Extension | `.CSV` |

### CSV Format

```
timestamp,rpm,boost,egt1,egt2,egt3,egt4,egt5,egt6,egt7,egt8,lambda,throttle,speed,gear,efr_speed,bat,latitude,longitude,speed_kmh,course,sats,fix
```

**Field mapping:**

| Index | Field | Data Point ID | Unit | Import Scaling |
|-------|-------|---------------|------|----------------|
| 0 | timestamp | — (drives playback pacing) | ms (millis() uptime) | — |
| 1 | rpm | RPM | RPM | none |
| 2 | boost | BoostPressure | kPa | none |
| 3–10 | egt1–egt8 | EGT1–EGT8 | °C | none |
| 11 | lambda | Lambda | λ | none |
| 12 | throttle | ThrottlePosition | % | none |
| 13 | speed | VehicleSpeed | km/h | none |
| 14 | gear | Gear | gear | none |
| 15 | efr_speed | EFRSpeed | RPM | none |
| 16 | bat | BatteryVoltage | V | none |
| 17 | latitude | GPSLatitude | degrees | ÷ 1e6 (stored as int × 1e6) |
| 18 | longitude | GPSLongitude | degrees | ÷ 1e6 (stored as int × 1e6) |
| 19 | speed_kmh | GPSSpeed | km/h | ÷ 100 (stored × 100) |
| 20 | course | GPSCourse | degrees | ÷ 100 (stored × 100) |
| 21 | sats | GPSSatellites | count | none |
| 22 | fix | GPSFixQuality | quality | none |

**Parser behavior:**
- Empty fields (from MDLogger PID masking, e.g. `801,,721`) are preserved for column indexing and simply skipped — they do not shift the remaining columns.
- GPS fields are skipped when latitude and longitude are both 0 (no fix yet).
- Rows with fewer than 23 columns are rejected with a warning.

### Playback Controls

The MDLogger CSV parser supports:
- **Timestamp-paced playback**: row intervals are derived from the CSV `timestamp` column, so playback matches the original logging cadence
- **Playback speed multiplier**: 1.0× (real-time) to 10.0× (fast forward), applied to the timestamp deltas
- **Pause/resume**: Toggle parsing without closing the file
- **File status**: Open/closed state tracking; `parsingComplete` is emitted at end of file

### Command Line Usage

```bash
./multidisplay-unified --csv /path/to/MD0001.CSV
```

### Implementation Details

- **Parser class**: `MDLoggerCSV` (inherits from `ICommunication`)
- **File**: [`src/MDLoggerCSV.cpp`](../src/MDLoggerCSV.cpp)
- **Text stream**: Uses QTextStream for UTF-8 encoded CSV reading
- **Buffer management**: Line-by-line parsing, buffered one row ahead; a single-shot QTimer schedules each row from the timestamp delta ÷ playback speed

---

## Comparison Matrix

| Feature | MD ECU | rAtTrax-BMS | OpenDash Mesh | MDLogger |
|---------|--------|-------------|---------------|----------|
| MCU | ATmega2560 | ESP32-DOIT | ESP32-S3 | ATmega328P |
| Interface | USB Serial | USB/Bluetooth | USB (bridge) | File import |
| Baud Rate | 115200 | 115200 | 921600 (firmware-fixed) | N/A |
| Data Format | Binary frames | CSV lines | ASCII DP: lines | CSV file |
| Real-time | ✅ Yes | ✅ Yes | ✅ Yes | ❌ No (file) |
| GPS | ❌ No | ✅ Reyax GPS | ✅ Via nodes | ✅ Via file |
| IMU | ❌ No | ✅ MPU6050 | ✅ Via nodes | ❌ No |
| BMS | ❌ No | ✅ BQ76952 6S | ❌ No | ❌ No |
| VESC | ❌ No | ✅ CAN @500k | ❌ No | ❌ No |
| Engine | ✅ Full | ❌ No | ✅ Via nodes | ✅ Partial |

---

## Adding New Hardware

To add support for a new hardware system:

1. **Create a parser class** inheriting from `ICommunication`:
   ```cpp
   class MyNewParser : public ICommunication {
       Q_OBJECT
   public:
       explicit MyNewParser(const QString& port, QObject* parent = nullptr);
       bool open() override;
       void close() override;
       bool isConnected() const override;
   private slots:
       void onReadyRead();
   };
   ```

2. **Parse incoming data** and emit `frameParsed()` signal with a `QHash<DataPointID, double>` mapping.

3. **Register in MainWindow** to add connection UI and command-line support.

4. **Add data points** to the registry if using new DP IDs.

---

*Next: [ESP-NOW Bridge Guide](04-espnow-bridge.md)*
