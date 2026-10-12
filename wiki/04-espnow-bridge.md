# ESP-NOW Bridge Guide

## Overview

The **ESP-NOW USB Bridge** is a firmware implementation for an ESP32 device that receives ESP-NOW frames from OpenDash-compatible wireless dashboard nodes and forwards parsed data to a host PC via USB CDC-ACM serial connection.

This bridge enables the MultiDisplay Unified application to receive real-time sensor data from multiple wireless OpenDash devices through a single USB connection.

---

## Architecture

```
┌─────────────┐    ┌─────────────┐    ┌─────────────┐
│ OpenDash    │    │ OpenDash    │    │ OpenDash    │
│ Node 1      │    │ Node 2      │    │ Node N      │
│ (ESP32-S3)  │    │ (ESP32-S3)  │    │ (ESP32-S3)  │
└──────┬──────┘    └──────┬──────┘    └──────┬──────┘
       │                  │                  │
       │     ESP-NOW      │     ESP-NOW      │
       │   (wireless)     │   (wireless)     │
       └────────┬─────────┴────────┬─────────┘
                │                  │
          ┌─────▼──────────────────▼─────┐
          │  ESP32-VROOM-32              │
          │  USB Bridge Node             │
          │  (receives all ESP-NOW)      │
          │  parses & formats as text    │
          └──────────┬───────────────────┘
                     │
              USB CDC-ACM
              /dev/ttyUSB*
                     │
               ┌─────▼─────┐
               │ MultiDisplay│
               │ Unified App│
               └───────────┘
```

---

## Protocol Specification

### Message Format

All messages are **newline-delimited UTF-8 text**. Each message is a single line terminated by `\n` (0x0A).

### Data Point Messages (`DP:`)

**Format:**
```
DP:<hex_id>:<float_value>\n
```

| Component | Description | Example |
|-----------|-------------|---------|
| `DP:` | Message type prefix (case-sensitive, uppercase) | `DP:` |
| `<hex_id>` | Data Point ID as 4-digit uppercase hex with `0x` prefix | `0x0100`, `0x0400` |
| `:` | Separator colon | `:` |
| `<float_value>` | IEEE 754 float, up to 6 decimal places | `3200.0`, `14.285` |
| `\n` | Line terminator (newline) | (newline) |

**Valid Examples:**
```
DP:0x0100:3200.0
DP:0x0106:145.50
DP:0x0400:25.20
DP:0x0402:78.50
DP:0x0606:-12500.00
```

**Invalid Examples (will be rejected):**
```
dp:0x0100:3200.0        ← lowercase "dp"
DP:100:3200.0           ← missing "0x" prefix
DP:0X0100:3200.0        ← uppercase "X" (must be lowercase)
DP:0x0100 3200.0        ← space instead of colon
DP:0x0100               ← missing value
```

### Other Message Types

| Prefix | Purpose | Example |
|--------|---------|---------|
| `CFG:` | Configuration/status notification | `CFG:BOOT:v1.0.0` |
| `OTA:` | Over-the-air update status | `OTA:START`, `OTA:COMPLETE` |
| `ERR:` | Error/warning report | `ERR:CRC_FAIL:frame_id:0x1234` |

---

## Data Point ID Reference

### ENGINE/ECU (0x0100–0x01FF)

| Hex | Name | Unit | Example |
|-----|------|------|---------|
| 0x0100 | Engine RPM | RPM | `DP:0x0100:3200.0` |
| 0x0101 | Vehicle Speed | km/h | `DP:0x0101:95.00` |
| 0x0106 | Boost Pressure | kPa | `DP:0x0106:145.50` |
| 0x010B | Lambda | λ | `DP:0x010B:1.23` |
| 0x0112–0x0115, 0x0118–0x011B | EGT 1–8 | °C | `DP:0x0112:650.00` |
| 0x010D | Battery Voltage | V | `DP:0x010D:12.60` |

### GPS/NAV (0x0200–0x02FF)

| Hex | Name | Unit | Example |
|-----|------|------|---------|
| 0x0200 | GPS Speed | km/h | `DP:0x0200:95.00` |
| 0x0202 | GPS Latitude | degrees | `DP:0x0202:37.7749` |
| 0x0203 | GPS Longitude | degrees | `DP:0x0203:-122.4194` |

### IMU (0x0300–0x03FF)

| Hex | Name | Unit | Example |
|-----|------|------|---------|
| 0x0306 | IMU Pitch | degrees | `DP:0x0306:5.20` |
| 0x0307 | IMU Roll | degrees | `DP:0x0307:-2.80` |

### BMS (0x0400–0x04FF)

| Hex | Name | Unit | Example |
|-----|------|------|---------|
| 0x0400 | Pack Voltage | V | `DP:0x0400:25.20` |
| 0x0401 | Pack Current | A | `DP:0x0401:85.50` |
| 0x0402 | State of Charge | % | `DP:0x0402:78.50` |

### VESC/DRIVETRAIN (0x0600–0x06FF)

| Hex | Name | Unit | Example |
|-----|------|------|---------|
| 0x0600 | Motor Voltage | V | `DP:0x0600:42.30` |
| 0x0601 | Motor Current | A | `DP:0x0601:85.50` |
| 0x0606 | eRPM | eRPM | `DP:0x0606:-12500.00` |

---

## Firmware Implementation

### Arduino Framework Example

```cpp
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>

// ── Send a single Data Point as formatted text ──
void sendDP(uint16_t dpId, float value) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dpId, value);
    Serial.write(buf, len);  // Send ASCII text over USB CDC-ACM
}

// ── ESP-NOW receive callback ──
void onDataRecv(const uint8_t *mac_addr, const esp_now_recv_info_t *recvInfo, 
                const uint8_t *data, int len) {
    
    // Parse incoming ESP-NOW frame into (dpId, value) pairs
    // The exact layout of 'data' depends on your OpenDash device protocol
    
    // Example: if frame is [2 bytes ID][4 bytes float] per point
    for (int i = 0; i + 6 <= len; i += 6) {
        uint16_t dpId = (data[i] << 8) | data[i + 1];
        float value;
        memcpy(&value, &data[i + 2], sizeof(float));
        
        sendDP(dpId, value);
    }
}

// ── Setup ──
void setup() {
    Serial.begin(921600);  // UART at 921600 baud (matches openDstream UART_BAUD_RATE)
    
    // Suppress ESP32 debug/boot messages
    esp_log_level_set("*", ESP_LOG_ERROR);
    
    // Initialize ESP-NOW
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("ESP-NOW init failed");
        while (1) yield();
    }
    
    esp_now_register_recv_cb(onDataRecv);
}

void loop() {
    // Nothing needed — data arrives via callback
}
```

### ESP-IDF Framework Example

```c
#include <esp_now.h>
#include <wifi_promiscuous.h>
#include <uart.h>

void send_dp(uint32_t dp_id, float value) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dp_id, value);
    uart_write_bytes(UART_NUM_1, buf, len);  // UART connected to USB
}

void on_data_recv(const uint8_t *mac_addr, const esp_now_recv_info_t *recvInfo, 
                  const uint8_t *data, int len) {
    
    // Parse ESP-NOW frame and send as DP: formatted text
    for (int i = 0; i + 6 <= len; i += 6) {
        uint32_t dp_id = (data[i] << 24) | (data[i+1] << 16) | 
                         (data[i+2] << 8) | data[i+3];
        float value;
        memcpy(&value, &data[i + 4], sizeof(float));
        
        send_dp(dp_id, value);
    }
}

void app_main(void) {
    // Configure UART for USB CDC-ACM
    uart_config_t uart_config = {
        .baud_rate = 921600,  // openDstream: #define UART_BAUD_RATE (921600)
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    uart_param_config(UART_NUM_1, &uart_config);
    
    // Initialize ESP-NOW and register callback
    esp_now_init();
    esp_now_register_recv_cb(on_data_recv);
}
```

---

## Verification

### Step 1: Check the USB port

```bash
ls -la /dev/ttyUSB* /dev/ttyACM* | grep "Jun 11"
# Look for the most recent timestamp
```

### Step 2: Read the output

```bash
cat /dev/ttyUSB3   # Replace with your actual port
```

### Step 3: Verify correct output

**✅ CORRECT — Clean text lines:**
```
DP:0x0400:25.20
DP:0x0100:3200.0
DP:0x0306:-1.08
DP:0x0200:0.86
```

**❌ WRONG — If you see any of these:**
- Binary garbage / hex dumps (`02 0A 39 41...`)
- ESP32 boot messages (`I (123) cpu_start:`)
- Panic backtraces (`PC: 0x400d1234`)
- Lines without `DP:` prefix

### Step 4: Test with the application

```bash
cd /home/sysadmin/Documents/multidisplay-app/multidisplay-unified/build_test
./multidisplay-unified --bridge /dev/ttyUSB3
```

Expected output:
```
[ESP-NOW-Bridge] Connected to "/dev/ttyUSB3"
[RealtimePlot] Added display: GPS Speed
[Dashboard] Added series: GPS Speed to tab: "GPS_IMU" from source: ""
```

---

## Common Issues & Fixes

### Issue 1: Raw binary output

**Symptom:** `cat /dev/ttyUSB3` shows hex dumps or garbage bytes.

**Cause:** Firmware is forwarding raw ESP-NOW bytes instead of parsing them.

**Fix:** Replace `Serial.write(data, len)` with parse + snprintf pattern shown above.

### Issue 2: Boot messages in output

**Symptom:** Output contains `I (123) cpu_start: Starting app...` mixed with DP lines.

**Cause:** ESP32 debug logging is enabled and going to the same UART as data.

**Fix:** Add at the top of setup():
```cpp
esp_log_level_set("*", ESP_LOG_ERROR);  // Only show errors, not info/debug
```

### Issue 3: No output at all

**Symptom:** `cat /dev/ttyUSB3` returns immediately with no data.

**Causes:**
- Device not flashed or crashed after flash
- Port number changed after flashing (check `ls -la /dev/tty*`)
- Baud rate mismatch (verify with `stty -F /dev/ttyUSB3 speed`)

**Fix:** 
1. Check port: `ls -la /dev/ttyUSB* /dev/ttyACM* | grep "Jun 11"`
2. Verify baud: bridge firmware uses 921600 (see openDstream `UART_BAUD_RATE`)
3. Re-flash if necessary

### Issue 4: App rejects DP lines

**Symptom:** Application logs `Failed to parse DP line: ...`

**Causes:**
- Lowercase `dp:` instead of uppercase `DP:`
- Wrong hex format (e.g., `0x100` instead of `0x0100`)
- Missing colon separator or value

**Fix:** Ensure format is exactly `DP:0x%04X:%.2f\n` with uppercase DP and 4-digit hex.

---

## Performance Guidelines

| Parameter | Recommendation |
|-----------|---------------|
| Baud rate | 921600 (fixed by openDstream firmware) |
| Max message length | 128 bytes per line |
| Max messages/second | 200 (to avoid buffer overflow) |
| Line terminator | `\n` only (no `\r\n` required, but tolerated) |
| Character encoding | UTF-8 (ASCII subset sufficient) |
| Empty lines | Silently ignored by parser |

---

## Integration with OpenDash

This bridge is designed to work alongside the **OpenDash** ecosystem. The DP ID space (0x0100–0x07FF) matches OpenDash's Data Point numbering convention, ensuring compatibility with existing OpenDash device profiles and dashboards.

When integrating with OpenDash:
- Use the same DP IDs as defined in this spec
- ESP-NOW frames from OpenDash devices should be parsed and forwarded using the format above
- The bridge can act as a transparent forwarder — receive ESP-NOW, parse, reformat to `DP:` lines, send over USB

---

*Next: [Building & Installation](05-building.md)*
