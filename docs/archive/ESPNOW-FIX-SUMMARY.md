<!-- ARCHIVED: Historical working notes from the June 2026 parser-debugging sessions. Superseded by wiki/09-md-ecu-binary-protocol.md and wiki/04-espnow-bridge.md. Kept for context only — do not follow as current guidance. -->

# ESPNOW/OpenDash Fix Summary

## What Was Done

### 1. Hex Dump Diagnostic Enhancement

Added raw hex dump output to `OpenDashBridge::onReadyRead()`:

```cpp
qDebug() << "[ESP-NOW-Bridge] Raw bytes received:" << data.length() << "bytes";
qDebug() << "[ESP-NOW-Bridge] Hex dump:" << data.toHex(' ').data();
```

**Purpose:** See exactly what bytes arrive, just like the MD ECU fix

### 2. Buffer Reset on Parse Error

Added buffer reset when DP line parsing fails:

```cpp
if (!match.hasMatch()) {
    qWarning() << "[ESP-NOW-Bridge] Failed to parse DP line:" << line;
    
    // Buffer reset on error (matching MD ECU pattern)
    m_receiveBuffer.clear();
    qDebug() << "[ESP-NOW-Bridge] Buffer cleared due to parse error";
    return;
}
```

**Purpose:** Prevent cascading failures from one bad line corrupting all subsequent data

### 3. Line Validation

Added length check before parsing:

```cpp
if (line.length() < 8) {  // Minimum: DP:x:0\n = 8 chars
    qWarning() << "[ESP-NOW-Bridge] Line too short:" << line;
    return;
}
```

### 4. ID Range Validation

Added explicit range check for DP IDs:

```cpp
if (val < 0x0100 || val > 0x07FF) {
    qWarning() << "[ESP-NOW-Bridge] DP ID out of range [0x0100-0x07FF]:"
               << QString("0x%1").arg(dpId, 4, 16, QChar('0'));
    return;
}
```

## Files Modified

| File | Changes |
|------|---------|
| `src/OpenDashBridge.cpp` | Hex dump, buffer reset on error, line validation |
| `docs/ESPNOW-OPENASH-BATTLE-REPORT.md` | Comprehensive analysis document |

## How to Test

### 1. Run with Hex Dump

```bash
cd /home/sysadmin/Documents/multidisplay-app/multidisplay-unified
./build_debug/multidisplay-unified --bridge /dev/ttyACM7 2>&1 | grep -E "(Raw bytes|Hex dump|Clean line)"
```

### 2. Look For

**Good data:**
```
[ESP-NOW-Bridge] Raw bytes received: 28 bytes
[ESP-NOW-Bridge] Hex dump: 44 50 3a 30 78 30 31 30 30 3a 32 35 2e 35 0a
[ESP-NOW-Bridge] Clean line: DP:0x0100:25.5
```

**Problems to identify:**
- Lines concatenated without newlines (no `0a` between lines)
- ESP32 debug output mixed in (non-DP lines)
- Partial lines (incomplete hex sequences)

### 3. Capture Raw Data

```bash
# Save raw data for analysis
timeout 10s ./build_debug/multidisplay-unified --bridge /dev/ttyACM7 2>&1 | tee capture.log

# Extract hex dump section
grep "Hex dump" capture.log > hex_dump.txt
```

## Expected Outcomes

### If ESP32 Firmware is Correct

You should see clean output like:
```
[ESP-NOW-Bridge] Raw bytes received: 28 bytes
[ESP-NOW-Bridge] Hex dump: 44 50 3a 30 78 30 31 30 30 3a 32 35 2e 35 0a
[ESP-NOW-Bridge] Clean line: DP:0x0100:25.5
[ESP-NOW-Bridge] Parsed DP line: DP:0x0100:25.5 ID:0x0100 Value:25.5
```

### If There Are Issues

You'll see one of these patterns:

**Multiple lines without separators:**
```
[ESP-NOW-Bridge] Raw bytes received: 56 bytes
[ESP-NOW-Bridge] Hex dump: 44 50 3a 30 78 30 31 30 30 3a 32 35 2e 35 44 50 3a 30 78 30 31 30 36 3a 31 34 35 2e 35 0a
[ESP-NOW-Bridge] Clean line: DP:0x0100:25.5DP:0x0106:145.5 (MISSING NEWLINE!)
```

**Debug output mixed in:**
```
[ESP-NOW-Bridge] Raw bytes received: 89 bytes
[ESP-NOW-Bridge] Hex dump: 49 20 28 31 32 33 34 35 29 20 63 70 75 5f 73 74 61 72 74 3a 20 49 6e 69 74 20 64 6f 6e 65 0a 44 50 3a 30 78 30 31 30 30 3a 32 35 2e 35 0a
[ESP-NOW-Bridge] Unknown message: I (12345) cpu_start: Init done
```

## Next Steps Based on Results

### If Data Looks Clean

The fixes should improve reliability. Monitor for:
- Buffer overflow errors
- Parse failures
- Missing data points

### If Problems Persist

Check ESP32 firmware:
1. Ensure `Serial.println()` is used (not `Serial.print()`)
2. Disable debug logs: `esp_log_level_set("*", ESP_LOG_ERROR)`
3. Check buffer flushing - data should be sent immediately

### If You See Concatenated Lines

The ESP32 might be batching multiple DP lines without proper separation:

**Firmware fix:**
```cpp
// Send each line separately with flush
Serial.println("DP:0x0100:25.5");
Serial.flush();  // Force immediate transmission
```

## Comparison to MD ECU Fix

| Aspect | MD ECU | ESPNOW/OpenDash |
|--------|--------|-----------------|
| **Frame boundary** | ETX byte (0x03) | Newline (\n) |
| **Data format** | Binary | Text |
| **State machine** | Byte-by-byte | Line-by-line |
| **Buffer reset on error** | ✓ Yes | ✓ Now added |
| **Hex dump debugging** | ✓ Yes | ✓ Now added |

## Files to Review

1. `docs/ESPNOW-OPENASH-BATTLE-REPORT.md` - Full battle report with analysis
2. `src/OpenDashBridge.cpp` - Current implementation with fixes
3. `docs/data-ESPNOW-usbnode.md` - ESPNOW protocol specification

## Summary

The OpenDashBridge now has:
1. **Hex dump debugging** to see raw bytes (like MD ECU fix)
2. **Buffer reset on error** to prevent cascading failures
3. **Line validation** to catch malformed data early

Run the application and capture hex output to identify if there are issues with how the ESP32 bridge is sending data.
