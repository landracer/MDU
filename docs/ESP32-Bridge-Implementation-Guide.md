# ESP-NOW USB Bridge — Implementation Guide for Junior Developers

## TL;DR (5-second summary)

Your ESP32 receives ESP-NOW frames from OpenDash devices, **parses them**, and sends the data over USB as **plain text lines** like this:

```
DP:0x0400:25.20
DP:0x0100:3200.0
DP:0x0402:78.50
```

That's it. One line per data point. Newline at the end. No binary. No hex dumps. No raw bytes.

---

## 1. What This Device Does

```
[OpenDash Device] --ESP-NOW--> [ESP32-VROOM-32] --USB CDC-ACM--> [Linux PC]
                                    |                                |
                              Parses &                          multidisplay-unified
                              formats as                         (dashboard app)
                              ASCII text
```

The ESP32 is a **bridge**. It sits between wireless OpenDash devices and the host PC.

---

## 2. What You MUST NOT Do

| ❌ DON'T | ✅ DO INSTEAD |
|----------|---------------|
| `Serial.write(rawBytes, len)` — send raw binary | Parse the frame first, then `snprintf()` + `Serial.write(text)` |
| Send hex dumps like `02 0A 39 41...` | Send ASCII text like `DP:0x0400:25.20` |
| Send ESP32 boot messages or panic backtraces | Disable debug logging or redirect to a different UART |
| Send data without a newline at the end | Always end each line with `\n` (0x0A) |

**If you see binary garbage on the USB port, you're doing it wrong.**

---

## 3. Exact Output Format

### 3.1 Data Point Lines (the only thing that matters)

```
DP:<4-digit-hex-id>:<float-value>\n
```

| Part | Rules | Example |
|------|-------|---------|
| `DP:` | Uppercase, case-sensitive, followed by colon | `DP:` |
| `<hex-id>` | Exactly 4 hex digits, uppercase, with `0x` prefix | `0x0400`, `0x0100`, `0x06FF` |
| `:` | Single colon separator | `:` |
| `<float-value>` | Decimal number, up to 2 decimal places is fine | `25.20`, `3200.0`, `-12500.0` |
| `\n` | Newline character (0x0A) at the end | (newline) |

### 3.2 Concrete Examples

```
DP:0x0400:25.20
DP:0x0100:3200.0
DP:0x0402:78.50
DP:0x0606:-12500.0
DP:0x0202:37.7749
DP:0x0301:-2.80
```

### 3.3 Invalid Examples (will be rejected)

```
dp:0x0100:3200.0        ← lowercase "dp" — WRONG
DP:100:3200.0           ← missing "0x" prefix — WRONG
DP:0X0100:3200.0        ← uppercase "X" — WRONG (must be lowercase "x")
DP:0x0100 3200.0        ← space instead of colon — WRONG
DP:0x0100               ← missing value — WRONG
DP::3200.0              ← missing ID — WRONG
```

---

## 4. Data Point ID Reference (Copy This Table)

These are the IDs your firmware might send. Only send data for points you actually have sensors for.

### ENGINE/ECU (0x0100–0x01FF)

| Hex | Name | Unit | Example Value |
|-----|------|------|---------------|
| `0x0100` | Engine RPM | RPM | `DP:0x0100:3200.0` |
| `0x0101` | Coolant Temp | °C | `DP:0x0101:95.00` |
| `0x0102` | Oil Pressure | kPa | `DP:0x0102:450.00` |
| `0x0103` | Intake Temp | °C | `DP:0x0103:35.50` |
| `0x0104` | Throttle Position | % | `DP:0x0104:65.00` |
| `0x0105` | MAP Pressure | kPa | `DP:0x0105:102.00` |
| `0x0106` | Boost Pressure | kPa | `DP:0x0106:145.50` |
| `0x0107` | Fuel Pressure | kPa | `DP:0x0107:380.00` |
| `0x0108` | Timing Advance | degrees | `DP:0x0108:12.50` |
| `0x0109` | Injector Pulse Width | ms | `DP:0x0109:3.25` |
| `0x010A` | MAF | g/s | `DP:0x010A:18.50` |
| `0x010B` | Lambda | λ | `DP:0x010B:1.23` |
| `0x010C` | AFR | AFR | `DP:0x010C:14.70` |
| `0x010D`–`0x0114` | EGT 1–8 | °C | `DP:0x010D:650.00` |
| `0x0115` | Battery Voltage | V | `DP:0x0115:12.60` |
| `0x0116` | Vehicle Speed | km/h | `DP:0x0116:85.00` |
| `0x0117` | Gear | gear | `DP:0x0117:4.00` |
| `0x0118` | EFR Speed | RPM | `DP:0x0118:2800.00` |
| `0x0119` | N75 Duty Cycle | % | `DP:0x0119:45.00` |
| `0x011A` | N75 Setpoint | kPa | `DP:0x011A:120.00` |

### GPS/NAV (0x0200–0x02FF)

| Hex | Name | Unit | Example Value |
|-----|------|------|---------------|
| `0x0200` | GPS Speed | km/h | `DP:0x0200:95.00` |
| `0x0201` | GPS Heading | degrees | `DP:0x0201:180.50` |
| `0x0202` | GPS Latitude | degrees | `DP:0x0202:37.7749` |
| `0x0203` | GPS Longitude | degrees | `DP:0x0203:-122.4194` |
| `0x0204` | GPS Altitude | meters | `DP:0x0204:15.50` |
| `0x0205` | GPS Satellites | count | `DP:0x0205:12.00` |
| `0x0206` | GPS Fix Quality | code | `DP:0x0206:3.00` |
| `0x0207` | GPS Course | degrees | `DP:0x0207:175.00` |

### IMU (0x0300–0x03FF)

| Hex | Name | Unit | Example Value |
|-----|------|------|---------------|
| `0x0300` | IMU Pitch | degrees | `DP:0x0300:5.20` |
| `0x0301` | IMU Roll | degrees | `DP:0x0301:-2.80` |
| `0x0302` | IMU Yaw | degrees | `DP:0x0302:180.50` |
| `0x0303` | IMU Accel X | m/s² | `DP:0x0303:0.15` |
| `0x0304` | IMU Accel Y | m/s² | `DP:0x0304:-0.02` |
| `0x0305` | IMU Accel Z | m/s² | `DP:0x0305:9.81` |

### BMS (0x0400–0x04FF)

| Hex | Name | Unit | Example Value |
|-----|------|------|---------------|
| `0x0400` | Pack Voltage | V | `DP:0x0400:25.20` |
| `0x0401` | Pack Current | A | `DP:0x0401:85.50` |
| `0x0402` | State of Charge | % | `DP:0x0402:78.50` |
| `0x0403` | BMS Temp | °C | `DP:0x0403:32.10` |
| `0x0404` | Min Cell Voltage | V | `DP:0x0404:3.350` |
| `0x0405` | Max Cell Voltage | V | `DP:0x0405:3.365` |
| `0x0406`–`0x040B` | Cell 1–6 Voltage | V | `DP:0x0406:3.352` |

### VESC/DRIVETRAIN (0x0600–0x06FF)

| Hex | Name | Unit | Example Value |
|-----|------|------|---------------|
| `0x0600` | Motor Voltage | V | `DP:0x0600:42.30` |
| `0x0601` | Motor Current | A | `DP:0x0601:85.50` |
| `0x0602` | Avg Current | A | `DP:0x0602:72.00` |
| `0x0603` | Duty Cycle | % | `DP:0x0603:45.67` |
| `0x0604` | MOSFET Temp | °C | `DP:0x0604:55.20` |
| `0x0605` | Motor Temp | °C | `DP:0x0605:62.80` |
| `0x0606` | eRPM | eRPM | `DP:0x0606:-12500.00` |
| `0x0607` | Fault Code | bitmask | `DP:0x0607:0.00` |
| `0x0608`–`0x060B` | Wheel RPM FL/FR/RL/RR | RPM | `DP:0x0608:1200.00` |

---

## 5. Arduino Code Example (Copy-Paste Ready)

```cpp
#include <esp_now.h>
#include <WiFi.h>
#include <Arduino.h>

// ── Your ESP-NOW receive callback ──
void onDataRecv(const uint8_t *mac_addr, const esp_now_recv_info_t *recvInfo, 
                const uint8_t *data, int len) {
    
    // data[] contains the ESP-NOW payload from the OpenDash device.
    // The EXACT layout of data[] depends on what your OpenDash devices send.
    // You need to parse it into (dpId, value) pairs.
    
    // ── EXAMPLE: if your OpenDash frame is [2 bytes ID][4 bytes float] ──
    for (int i = 0; i + 6 <= len; i += 6) {
        uint16_t dpId = (data[i] << 8) | data[i + 1];
        float value;
        memcpy(&value, &data[i + 2], sizeof(float));
        
        sendDP(dpId, value);  // Send as formatted text over USB/Serial
    }
}

// ── This is the function that formats and sends data ──
void sendDP(uint16_t dpId, float value) {
    char buf[32];
    int len = snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dpId, value);
    Serial.write(buf, len);  // Send ASCII text over USB CDC-ACM
}

// ── Setup ──
void setup() {
    Serial.begin(115200);  // USB CDC-ACM at 115200 baud
    
    // Disable ESP32 debug output (prevents boot messages from mixing with data)
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
    // Nothing needed here — data arrives via callback
}
```

---

## 6. How to Verify It Works

### Step 1: Flash the firmware and connect USB

Plug the ESP32 into your Linux PC. Find the port:
```bash
ls -la /dev/ttyUSB* /dev/ttyACM* | grep "Jun 11"
# Look for the most recent timestamp
```

### Step 2: Read the output

```bash
cat /dev/ttyUSB3   # Replace with your actual port
```

### Step 3: Check what you see

**✅ CORRECT — You should see clean text lines:**
```
DP:0x0400:25.20
DP:0x0100:3200.0
DP:0x0402:78.50
DP:0x0606:-12500.00
```

**❌ WRONG — If you see any of these, fix the firmware:**
- Binary garbage / hex dumps (`02 0A 39 41...`)
- ESP32 boot messages (`I (123) cpu_start: Starting app...`)
- Panic backtraces (`PC: 0x400d1234`)
- Lines without `DP:` prefix
- Empty output

### Step 4: Test with the dashboard app

```bash
cd /home/sysadmin/Documents/multidisplay-app/multidisplay-unified/build_test
./multidisplay-unified --help   # Check available options for connecting to a serial port
```

---

## 7. Common Mistakes (And How to Fix Them)

### Mistake 1: Sending raw binary
```cpp
// WRONG — this sends the raw ESP-NOW bytes over USB
Serial.write(data, len);

// CORRECT — parse first, then format as text
uint16_t dpId = (data[0] << 8) | data[1];
float value;
memcpy(&value, &data[2], sizeof(float));
char buf[32];
snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dpId, value);
Serial.write(buf);
```

### Mistake 2: ESP32 boot messages polluting the output
```cpp
// WRONG — boot messages mix with data
void setup() {
    Serial.begin(115200);
    // No log level control → boot spam appears on USB
}

// CORRECT — suppress debug logs
void setup() {
    Serial.begin(115200);
    esp_log_level_set("*", ESP_LOG_ERROR);  // Only show errors, not info/debug
}
```

### Mistake 3: Missing newline at end of line
```cpp
// WRONG — no newline, host can't parse lines
snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f", dpId, value);  // ← missing \n

// CORRECT — always include newline
snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dpId, value);
```

### Mistake 4: Wrong hex format
```cpp
// WRONG — lowercase x or wrong digit count
snprintf(buf, sizeof(buf), "dp:0x%02X:%.2f\n", dpId, value);  // ← "dp" not "DP", %02X not %04X

// CORRECT — uppercase DP, 4-digit hex with 0x prefix
snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dpId, value);
```

---

## 8. Quick Checklist Before Testing

- [ ] Firmware compiles and flashes successfully
- [ ] ESP32 connects to WiFi / initializes ESP-NOW without crashing
- [ ] `esp_log_level_set("*", ESP_LOG_ERROR)` is called in setup()
- [ ] Serial.begin(115200) is used (not 9600 or other baud rate)
- [ ] Every `DP:` line ends with `\n`
- [ ] Hex IDs are uppercase, 4 digits, with `0x` prefix (`0x0400`, not `0x400`)
- [ ] No raw binary is sent over Serial — only ASCII text
- [ ] The port you're reading from matches the one shown by `ls /dev/ttyUSB*`

---

## 9. Troubleshooting

### "I see nothing on the port"
1. Check if the port exists: `ls -la /dev/ttyUSB* /dev/ttyACM*`
2. Verify baud rate: `stty -F /dev/ttyUSB3 speed` (should be 115200)
3. Make sure the ESP32 is actually sending — add a test blink or heartbeat

### "I see binary garbage"
Your firmware is forwarding raw ESP-NOW bytes instead of parsing them. Find where `Serial.write(data, len)` appears and replace it with the parse + snprintf pattern shown above.

### "I see boot messages mixed in"
Add `esp_log_level_set("*", ESP_LOG_ERROR);` at the top of setup().

### "The app says 'Failed to parse DP line'"
Check your format carefully:
- Is it `DP:` (uppercase) or `dp:` (lowercase)? Must be uppercase.
- Is the hex ID 4 digits? `0x0100` not `0x100`.
- Is there a colon between ID and value? `DP:0x0100:3200.0` not `DP:0x0100 3200.0`.

---

*Document version: 1.0 — June 11, 2026*  
*For: multidisplay-unified OpenDash Bridge integration*
