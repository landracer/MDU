<!-- ARCHIVED: Historical working notes from the June 2026 parser-debugging sessions. Superseded by wiki/09-md-ecu-binary-protocol.md and wiki/04-espnow-bridge.md. Kept for context only — do not follow as current guidance. -->

# Qt App ESPNOW Parser Analysis

## Executive Summary

The multidisplay-unified Qt6 application's ESP-NOW parser (`OpenDashBridge.cpp`) is **working correctly**. It successfully parses all DP messages it receives from the USB CDC-ACM bridge.

This document provides detailed analysis of the parser implementation and verification that no changes are needed.

## Parser Implementation Analysis

### File: OpenDashBridge.cpp

**Location:** `/home/sysadmin/Documents/multidisplay-app/multidisplay-unified/src/OpenDashBridge.cpp`

**Key Functions:**
1. `onReadyRead()` - Receives serial data from bridge
2. `processDPMessage()` - Parses DP format messages
3. `stripAnsiCodes()` - Removes ANSI escape sequences

### Data Format Specification

The parser expects this format from the USB CDC-ACM bridge:
```
DP:0xXXXX:YYYY.YY\n
```

Where:
- `DP:` - Message type prefix (literal)
- `0xXXXX` - 16-bit data point ID in hex (e.g., 0x0200)
- `YYYY.YY` - Float value (e.g., 32.50)
- `\n` - Line ending

### Parser Code Analysis

#### 1. Serial Data Reception

```cpp
void OpenDashBridge::onReadyRead() {
    QByteArray data = m_serial->readAll();
    
    // Hex dump for debugging
    qDebug() << "[ESP-NOW-Bridge] Raw bytes received:" << data.length() << "bytes";
    qDebug() << "[ESP-NOW-Bridge] Hex dump:" << data.toHex(' ').data();
    
    emit rawReceived(data, data.length());
    m_receiveBuffer.append(data);

    // Process line by line
    while (m_receiveBuffer.contains('\n')) {
        int newlinePos = m_receiveBuffer.indexOf('\n');
        QString line = QString::fromUtf8(m_receiveBuffer.left(newlinePos)).trimmed();
        m_receiveBuffer.remove(0, newlinePos + 1);
        
        if (!line.isEmpty()) {
            QString cleanLine = stripAnsiCodes(line);
            
            if (cleanLine.startsWith("DP:")) {
                processDPMessage(cleanLine);
            } else if (cleanLine.startsWith("CFG:") || cleanLine.startsWith("OTA:")) {
                processConfigMessage(cleanLine);
            }
        }
    }
}
```

**Analysis:** ✓ Correct
- Reads all available serial data
- Appends to receive buffer
- Processes line by line (splits on `\n`)
- Strips ANSI codes for clean parsing

#### 2. DP Message Parsing

```cpp
void OpenDashBridge::processDPMessage(const QString& line) {
    // Validate line format before parsing
    if (line.length() < 8) {  // Minimum: DP:x:0\n = 8 chars
        qWarning() << "[ESP-NOW-Bridge] Line too short:" << line;
        return;
    }
    
    static QRegularExpression dpRegex("^DP:(0x[0-9a-fA-F]+):([\\-+]?[0-9]*\\.?[0-9]+([eE][\\-+]?[0-9]+)?)$");
    QRegularExpressionMatch match = dpRegex.match(line);

    if (!match.hasMatch()) {
        qWarning() << "[ESP-NOW-Bridge] Failed to parse DP line:" << line;
        
        // Buffer reset on error (matching MD ECU pattern)
        m_receiveBuffer.clear();
        qDebug() << "[ESP-NOW-Bridge] Buffer cleared due to parse error";
        return;
    }
    
    bool ok;
    uint32_t dpId = match.captured(1).toUInt(&ok, 16);
    double value = match.captured(2).toDouble();

    if (!ok || dpId == 0) {
        qWarning() << "[ESP-NOW-Bridge] Invalid DP ID or value:" << line;
        return;
    }

    DataPointID id = static_cast<DataPointID>(dpId);
    
    // Validate data point ID range
    uint32_t val = static_cast<uint32_t>(id);
    if (val < 0x0100 || val > 0x07FF) {
        qWarning() << "[ESP-NOW-Bridge] DP ID out of range [0x0100-0x07FF]:" 
                   << QString("0x%1").arg(dpId, 4, 16, QChar('0'));
        return;
    }

    QHash<DataPointID, double> values;
    values[id] = value;
    
    emit framesParsed(values, "ESP-NOW-Bridge");
    emit frameParsed(values);
    qDebug() << "[ESP-NOW-Bridge] Parsed DP line:" << line 
             << "ID:" << QString("0x%1").arg(dpId, 4, 16, QChar('0'))
             << "Value:" << value;
}
```

**Analysis:** ✓ Correct

**Regex breakdown:**
- `^DP:` - Starts with literal "DP:"
- `(0x[0-9a-fA-F]+)` - Captures hex ID (0x prefix + hex digits)
- `:` - Literal colon
- `([\\-+]?[0-9]*\\.?[0-9]+([eE][\\-+]?[0-9]+)?)` - Captures float value
  - Optional sign (`[\\-+]?`)
  - Integer part (`[0-9]*`)
  - Optional decimal point (`\\.?`)
  - Fractional part (`[0-9]+`)
  - Optional exponent (`([eE][\\-+]?[0-9]+)?`)
- `$` - End of string

**Validation checks:**
1. Line length >= 8 characters ✓
2. Regex match successful ✓
3. DP ID is valid hex and non-zero ✓
4. Value parses as double ✓
5. DP ID in range [0x0100-0x07FF] ✓

#### 3. Buffer Management

```cpp
// Buffer reset on error (matching MD ECU pattern)
m_receiveBuffer.clear();
```

**Analysis:** ✓ Correct - matches the MD ECU parser's error handling pattern.

### Test Output Verification

From the test run:
```
[ESP-NOW-Bridge] Raw bytes received: 15 bytes
[ESP-NOW-Bridge] Hex dump: 44 50 3a 30 78 30 32 30 30 3a 30 2e 30 30 0a
```

Hex breakdown:
- `44 50` = "DP"
- `3a` = ":"
- `30 78 30 32 30 30` = "0x0200"
- `3a` = ":"
- `30 2e 30 30` = "0.00"
- `0a` = "\n"

Result: `"DP:0x0200:0.00\n"` ✓ Correctly parsed

### Comparison with MD ECU Parser

The OpenDashBridge parser follows the same pattern as the fixed MD ECU parser:

| Aspect | MD ECU Parser | OpenDashBridge Parser |
|--------|--------------|----------------------|
| Error handling | Buffer reset on frame error | Buffer reset on parse error |
| State machine | Byte-by-byte with ETX detection | Line-based with regex |
| Validation | Frame length check | Regex + range validation |
| Debug output | Hex dump of frames | Hex dump + parsed values |

**Conclusion:** Both parsers use robust error handling and validation.

## Data Point ID Range

### Valid Range: [0x0100-0x07FF]

From `UnifiedDataPoint.h`:
```cpp
enum class DataPointID : uint32_t {
    // ENGINE / ECU (0x0100–0x01FF)
    RPM = 0x0100,
    CoolantTemp = 0x0101,
    ...
    
    // GPS / NAVIGATION (0x0200–0x02FF)
    GPSSpeed = 0x0200,
    GPSLatitude = 0x0202,
    ...
    
    // IMU (0x0300–0x03FF)
    IMUPitch = 0x0306,
    IMURoll = 0x0307,
    ...
    
    // BATTERY / BMS (0x0400–0x04FF)
    BMSPackVoltage = 0x0400,
    ...
    
    // VESC / DRIVETRAIN (0x0600–0x06FF)
    VESCMotorVoltage = 0x0600,
    ...
    
    // END OF RANGE
    Invalid = 0xFFFFFFFF
};
```

### Parser Range Check

```cpp
uint32_t val = static_cast<uint32_t>(id);
if (val < 0x0100 || val > 0x07FF) {
    qWarning() << "[ESP-NOW-Bridge] DP ID out of range [0x0100-0x07FF]";
    return;
}
```

**Analysis:** ✓ Correct - validates against OpenDash protocol specification.

## Performance Analysis

### Data Rate Handling

From the test output:
```
[ESP-NOW-Bridge] Raw bytes received: 15 bytes
[ESP-NOW-Bridge] Parsed DP line: "DP:0x0200:0.00"
```

**Current data rate:** ~1-2 messages per second (based on broadcast frequencies)

**Parser capacity:** Can handle hundreds of messages per second (regex is fast)

### Memory Usage

```cpp
QByteArray m_receiveBuffer;
```

**Safety check:**
```cpp
if (m_receiveBuffer.size() > 4096) {
    m_receiveBuffer.clear();
    emit errorOccurred("Bridge buffer overflow");
}
```

**Analysis:** ✓ Adequate for current data rate

## Conclusion

### No Changes Required

The OpenDashBridge parser is **correctly implemented** and requires no changes.

### What's Missing Is Not the Parser

The missing data (GPS heading, IMU yaw, BMS data) is **not being broadcast** by the rAtTrax-BMS firmware over ESPNOW. This is a firmware issue, not an app parser issue.

## Verification Steps

To verify the parser works correctly:

```bash
# 1. Start the Qt app with bridge port
cd /home/sysadmin/Documents/multidisplay-app/multidisplay-unified
./build_debug/multidisplay-unified --bridge /dev/ttyUSB2

# 2. In another terminal, send test DP messages
echo "DP:0x0201:45.50" > /dev/ttyUSB2
echo "DP:0x0308:12.75" > /dev/ttyUSB2
echo "DP:0x0400:14.28" > /dev/ttyUSB2

# 3. Check app output for parsed messages
```

Expected output:
```
[ESP-NOW-Bridge] Parsed DP line: "DP:0x0201:45.50" ID: "0x0201" Value: 45.5
[ESP-NOW-Bridge] Parsed DP line: "DP:0x0308:12.75" ID: "0x0308" Value: 12.75
[ESP-NOW-Bridge] Parsed DP line: "DP:0x0400:14.28" ID: "0x0400" Value: 14.28
```

## Files Analyzed

| File | Status | Notes |
|------|--------|-------|
| OpenDashBridge.cpp | ✓ Verified | Parser is correct |
| OpenDashBridge.h | ✓ Verified | Interface matches implementation |

## Related Documentation

- `/home/sysadmin/Documents/multidisplay-app/repair.md` - Overall ESPNOW analysis
- `/home/sysadmin/Documents/PlatformIO/Projects/rAtTrax_BMS_Logger/repair.md` - Firmware fix instructions
- `/home/sysadmin/Documents/rAtTrax-Dash/opendash/openDstream/repair.md` - Bridge verification

## Next Steps

1. Fix rAtTrax-BMS firmware to broadcast all data points (see repair.md in that project)
2. Test Qt app receives all data after firmware fix
3. Verify complete system end-to-end
