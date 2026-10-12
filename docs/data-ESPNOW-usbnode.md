# ESP-NOW USB Bridge Node — Data Format Specification

## Overview

This document specifies the exact serial data format expected by **OpenDashBridge** (the multidisplay-unified application) from an ESP32 USB bridge node forwarding ESP-NOW data. The bridge receives ESP-NOW frames from OpenDash-compatible devices and forwards parsed data over USB CDC-ACM to the host PC.

---

## 1. Physical Layer

| Parameter | Value |
|-----------|-------|
| Interface | USB CDC-ACM (virtual serial port) |
| Port | `/dev/ttyACM*` (Linux), `COMx` (Windows), `/dev/cu.usbmodem*` (macOS) |
| Baud Rate | **115200** |
| Data Bits | 8 |
| Parity | None |
| Stop Bits | 1 |
| Flow Control | None |

---

## 2. Message Protocol

All messages are **newline-delimited UTF-8 text**. Each message is a single line terminated by `\n` (0x0A). No carriage return (`\r`) required but tolerated.

### 2.1 Message Types

| Prefix | Direction | Purpose |
|--------|-----------|---------|
| `DP:` | Bridge → Host | Data Point value update |
| `CFG:` | Bridge → Host | Configuration/status notification |
| `OTA:` | Bridge → Host | Over-the-air update status |
| `ERR:` | Bridge → Host | Error/warning report |

**Only `DP:` messages carry sensor data.** The others are informational.

---

## 3. Data Point Messages (`DP:`)

### 3.1 Format

```
DP:<hex_id>:<float_value>\n
```

### 3.2 Components

| Component | Description | Example |
|-----------|-------------|---------|
| `DP:` | Message type prefix (case-sensitive, uppercase) | `DP:` |
| `<hex_id>` | Data Point ID as **4-digit uppercase hex** with `0x` prefix | `0x0100`, `0x0400`, `0x06FF` |
| `:` | Separator (colon) | `:` |
| `<float_value>` | IEEE 754 single-precision float, **up to 6 decimal places**, no trailing zeros required | `3200.0`, `14.285`, `-5.3`, `0` |
| `\n` | Line terminator | (newline) |

### 3.3 Valid Ranges for `<hex_id>`

| Range | Category | Points |
|-------|----------|--------|
| `0x0100`–`0x01FF` | ENGINE/ECU | RPM, CoolantTemp, OilPressure, IntakeTemp, ThrottlePosition, MapPressure, BoostPressure, FuelPressure, TimingAdvance, InjectorPulseWidth, MAF, Lambda, AFR, EGT1-8, BatteryVoltage, VehicleSpeed, Gear, EFRSpeed, N75DutyCycle, N75Setpoint |
| `0x0200`–`0x02FF` | GPS/NAV | GPSSpeed, GPSHeading, GPSLatitude, GPSLongitude, GPSAltitude, GPSSatellites, GPSFixQuality, GPSCourse |
| `0x0300`–`0x03FF` | IMU | IMUPitch, IMURoll, IMUYaw, IMUAx, IMUAy, IMUAz |
| `0x0400`–`0x04FF` | BMS | BMSPackVoltage, BMSPackCurrent, BMSSOC, BMSTemp, BMSMinCellVoltage, BMSMaxCellVoltage, BMSCell1-6 |
| `0x0500`–`0x05FF` | RESERVED | — |
| `0x0600`–`0x06FF` | VESC/DRIVETRAIN | VESCMotorVoltage, VESCMotorCurrent, VESCAverageCurrent, VESCDutyCycle, VESCMOSFETTemp, VESCMotorTemp, VESCRPM, VESCFaultCode, WheelRPMFL, WheelRPMFR, WheelRPMRL, WheelRPMRR |
| `0x0700`–`0x07FF` | SYSTEM | SystemStatus, Heartbeat |

### 3.4 Complete Data Point ID Reference

```
ENGINE/ECU (0x0100-0x01FF):
  0x0100  RPM                          — Engine RPM (raw: ×1.0)
  0x0101  CoolantTemp                  — °C (raw: ×0.01, offset 0)
  0x0102  OilPressure                  — kPa (raw: ×0.01)
  0x0103  IntakeTemp                   — °C (raw: ×0.01)
  0x0104  ThrottlePosition             — % (raw: ×0.01)
  0x0105  MapPressure                  — kPa (raw: ×0.01)
  0x0106  BoostPressure                — kPa (raw: ×0.01)
  0x0107  FuelPressure                 — kPa (raw: ×0.01)
  0x0108  TimingAdvance                — degrees (raw: ×0.1)
  0x0109  InjectorPulseWidth           — ms (raw: ×0.01)
  0x010A  MAF                          — g/s (raw: ×0.1)
  0x010B  Lambda                       — Air-Fuel Ratio λ (raw: ×0.01)
  0x010C  AFR                          — Air-Fuel Ratio (raw: ×0.01)
  0x010D  EGT1                         — °C (raw: ×1.0)
  0x010E  EGT2                         — °C (raw: ×1.0)
  0x010F  EGT3                         — °C (raw: ×1.0)
  0x0110  EGT4                         — °C (raw: ×1.0)
  0x0111  EGT5                         — °C (raw: ×1.0)
  0x0112  EGT6                         — °C (raw: ×1.0)
  0x0113  EGT7                         — °C (raw: ×1.0)
  0x0114  EGT8                         — °C (raw: ×1.0)
  0x0115  BatteryVoltage               — V (raw: ×0.01)
  0x0116  VehicleSpeed                 — km/h (raw: ×0.01)
  0x0117  Gear                         — gear position (raw: ×1.0)
  0x0118  EFRSpeed                     — RPM equivalent (raw: ×1.0)
  0x0119  N75DutyCycle                 — % (raw: ×0.1)
  0x011A  N75Setpoint                  — kPa (raw: ×0.1)

GPS/NAV (0x0200-0x02FF):
  0x0200  GPSSpeed                     — km/h (raw: ×0.01)
  0x0201  GPSHeading                   — degrees (raw: ×0.01)
  0x0202  GPSLatitude                  — degrees (raw: ×1.0)
  0x0203  GPSLongitude                 — degrees (raw: ×1.0)
  0x0204  GPSAltitude                  — meters (raw: ×0.01)
  0x0205  GPSSatellites                — count (raw: ×1.0)
  0x0206  GPSFixQuality                — quality code (raw: ×1.0)
  0x0207  GPSCourse                    — degrees (raw: ×0.01)

IMU (0x0300-0x03FF):
  0x0300  IMUPitch                     — degrees (raw: ×0.1)
  0x0301  IMURoll                      — degrees (raw: ×0.1)
  0x0302  IMUYaw                       — degrees (raw: ×0.1)
  0x0303  IMUAx                        — m/s² (raw: ×0.01)
  0x0304  IMUAy                        — m/s² (raw: ×0.01)
  0x0305  IMUAz                        — m/s² (raw: ×0.01)

BMS (0x0400-0x04FF):
  0x0400  BMSPackVoltage               — V (raw: ×0.1)
  0x0401  BMSPackCurrent               — A (raw: ×0.1, signed)
  0x0402  BMSSOC                       — % (raw: ×0.1)
  0x0403  BMSTemp                      — °C (raw: ×0.1)
  0x0404  BMSMinCellVoltage            — V (raw: ×0.001)
  0x0405  BMSMaxCellVoltage            — V (raw: ×0.001)
  0x0406  BMSCell1                     — V (raw: ×0.001)
  0x0407  BMSCell2                     — V (raw: ×0.001)
  0x0408  BMSCell3                     — V (raw: ×0.001)
  0x0409  BMSCell4                     — V (raw: ×0.001)
  0x040A  BMSCell5                     — V (raw: ×0.001)
  0x040B  BMSCell6                     — V (raw: ×0.001)

VESC/DRIVETRAIN (0x0600-0x06FF):
  0x0600  VESCMotorVoltage             — V (raw: ×0.1)
  0x0601  VESCMotorCurrent             — A (raw: ×0.1, signed)
  0x0602  VESCAverageCurrent           — A (raw: ×0.1, signed)
  0x0603  VESCDutyCycle                — % (raw: ×0.01)
  0x0604  VESCMOSFETTemp               — °C (raw: ×0.1)
  0x0605  VESCMotorTemp                — °C (raw: ×0.1)
  0x0606  VESCRPM                      — eRPM (raw: ×1.0)
  0x0607  VESCFaultCode                — bitmask (raw: ×1.0)
  0x0608  WheelRPMFL                   — RPM (raw: ×1.0)
  0x0609  WheelRPMFR                   — RPM (raw: ×1.0)
  0x060A  WheelRPMRL                   — RPM (raw: ×1.0)
  0x060B  WheelRPMRR                   — RPM (raw: ×1.0)

SYSTEM (0x0700-0x07FF):
  0x0700  SystemStatus                 — status code (raw: ×1.0)
  0x0701  Heartbeat                    — counter (raw: ×1.0)
```

### 3.5 Examples of Valid `DP:` Messages

```
DP:0x0100:3200.0
DP:0x0106:145.5
DP:0x010B:1.23
DP:0x0400:25.2
DP:0x0402:78.5
DP:0x0403:32.1
DP:0x0606:-12500.0
DP:0x0202:37.7749
DP:0x0203:-122.4194
DP:0x0300:5.2
DP:0x0301:-2.8
DP:0x0600:42.3
DP:0x0601:85.5
DP:0x0603:45.67
DP:0x011D:0
DP:0x011E:0
DP:0x011F:0
```

### 3.6 Invalid Formats (will be rejected)

```
dp:0x0100:3200.0        ← lowercase prefix
DP:100:3200.0           ← missing 0x prefix
DP:0X0100:3200.0        ← uppercase X (must be lowercase x)
DP:0x0100 3200.0        ← space instead of colon
DP:0x0100:              ← missing value
DP::3200.0              ← missing ID
DP:0x0100:abc           ← non-numeric value
DP:0x0100:3200.0:extra  ← extra fields
```

---

## 4. Configuration Messages (`CFG:`)

### 4.1 Format

```
CFG:<type>:<key>:<value>\n
```

### 4.2 Types

| Type | Description | Example |
|------|-------------|---------|
| `LAYOUT` | Dashboard layout change | `CFG:LAYOUT:LEFT:OK` |
| `PARAM` | Parameter update confirmation | `CFG:PARAM:refresh_rate:30` |
| `BOOT` | Bridge boot notification | `CFG:BOOT:v1.2.3` |

### 4.3 Examples

```
CFG:BOOT:v1.0.0
CFG:LAYOUT:LEFT:OK
CFG:PARAM:refresh_rate:30
```

---

## 5. OTA Messages (`OTA:`)

### 5.1 Format

```
OTA:<status>\n
```

### 5.2 Status Values

| Status | Description |
|--------|-------------|
| `START` | OTA update started |
| `PROGRESS:<percent>` | Update in progress (0-100) |
| `COMPLETE` | Update successful |
| `FAILED` | Update failed |
| `ROLLBACK` | Rolled back to previous version |

### 5.3 Examples

```
OTA:START
OTA:PROGRESS:45
OTA:PROGRESS:89
OTA:COMPLETE
```

---

## 6. Error Messages (`ERR:`)

### 6.1 Format

```
ERR:<code>:<message>\n
```

### 6.2 Error Codes

| Code | Description |
|------|-------------|
| `ESP_NOW_ERROR` | ESP-NOW protocol error |
| `BUFFER_OVERFLOW` | Internal buffer full |
| `PARSE_ERROR` | Failed to parse incoming frame |
| `SIGNAL_LOSS` | Lost connection to sender |
| `CRC_FAIL` | Checksum mismatch |

### 6.3 Examples

```
ERR:ESP_NOW_ERROR:no_peers_found
ERR:CRC_FAIL:frame_id:0x1234
ERR:SIGNAL_LOSS:peer_mac:A4:C1:3E:xx:xx:xx
```

---

## 7. Performance Requirements

| Parameter | Requirement |
|-----------|-------------|
| **Minimum baud rate** | 115200 (230400 recommended for high-frequency data) |
| **Max message length** | 128 bytes per line |
| **Max messages/second** | 200 (to avoid buffer overflow) |
| **Line terminator** | `\n` only (no `\r\n` required, but tolerated) |
| **Character encoding** | UTF-8 (ASCII subset sufficient) |
| **Empty lines** | Silently ignored |
| **Unknown prefixes** | Logged as warnings, not errors |

---

## 8. ESP32 Firmware Implementation Example (Arduino/ESP-IDF)

### 8.1 Arduino Framework

```cpp
// Send a single Data Point over serial
void sendDP(uint32_t dpId, float value) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "DP:0x%04X:%.6f\n", dpId, value);
    Serial.write(buf, len);
}

// Send multiple data points in a batch (recommended for efficiency)
void sendBatch() {
    char buf[256];
    int offset = 0;
    
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0100:%.1f\n", rpm);
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0106:%.2f\n", boostPressure);
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0400:%.1f\n", packVoltage);
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0402:%.1f\n", soc);
    
    Serial.write(buf, offset);
}

// Setup
void setup() {
    Serial.begin(115200);  // or 230400 for higher throughput
    while (!Serial) {}     // Wait for USB enumeration (native USB chips)
    
    // Initialize ESP-NOW here...
}
```

### 8.2 ESP-IDF Framework

```c
void send_dp(uint32_t dp_id, float value) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "DP:0x%04X:%.6f\n", dp_id, value);
    uart_write_bytes(UART_NUM_1, buf, len);  // UART connected to USB
}

void send_batch(float rpm, float boost, float voltage, float soc) {
    char buf[256];
    int offset = 0;
    
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0100:%.1f\n", rpm);
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0106:%.2f\n", boost);
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0400:%.1f\n", voltage);
    offset += snprintf(buf + offset, sizeof(buf) - offset, "DP:0x0402:%.1f\n", soc);
    
    uart_write_bytes(UART_NUM_1, buf, offset);
}

void app_main(void) {
    // Configure UART for USB CDC-ACM
    uart_config_t uart_config = {
        .baud_rate = 115200,  // or 230400
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    
    // Initialize ESP-NOW here...
}
```

---

## 9. Testing Your Bridge

### 9.1 Manual Test with `cat` or `minicom`

```bash
# Watch data from the bridge
cat /dev/ttyACM0

# Or with minicom (115200 8N1)
sudo minicom -D /dev/ttyACM0 -b 115200
```

### 9.2 Expected Output

You should see clean lines like:

```
DP:0x0100:3200.0
DP:0x0106:145.50
DP:0x0400:25.20
DP:0x0402:78.50
DP:0x0606:-12500.0
```

### 9.3 What NOT to See

- ❌ ESP32 panic backtraces (means firmware is crashing)
- ❌ `I (12345) cpu_start:` boot messages (enable log filtering)
- ❌ Raw binary data (must be text)
- ❌ Lines without `DP:`, `CFG:`, `OTA:`, or `ERR:` prefix

### 9.4 Suppress ESP32 Debug Output

In your firmware, redirect debug logs to a different UART or disable them:

```cpp
// Arduino
esp_log_level_set("*", ESP_LOG_ERROR);  // Only show errors

// ESP-IDF (sdkconfig)
CONFIG_LOG_DEFAULT_LEVEL_ERROR=y
CONFIG_LOG_MAXIMUM_LEVEL_INFO=y
```

---

## 10. Integration with OpenDash

This bridge is designed to work alongside the **OpenDash** ecosystem. The DP ID space (`0x0100`–`0x07FF`) matches OpenDash's Data Point numbering convention, ensuring compatibility with existing OpenDash device profiles and dashboards.

When integrating with OpenDash:
- Use the same DP IDs as defined in this spec
- ESP-NOW frames from OpenDash devices should be parsed and forwarded using the format above
- The bridge can act as a transparent forwarder — receive ESP-NOW, parse, reformat to `DP:` lines, send over USB

---

## 11. Quick Reference Card

```
Format:    DP:<hex_id>:<float_value>\n
Hex ID:    4-digit uppercase with 0x prefix (e.g., 0x0100)
Value:     Float, up to 6 decimal places
Baud:      115200 or 230400, 8N1
Lines:     Newline-terminated, no \r required
Max len:   128 bytes per line
```

---

*Document version: 1.0 — June 10, 2026*  
*Target application: multidisplay-unified (OpenDash Bridge)*
