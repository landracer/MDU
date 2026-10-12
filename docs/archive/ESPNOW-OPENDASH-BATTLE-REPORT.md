<!-- ARCHIVED: Historical working notes from the June 2026 parser-debugging sessions. Superseded by wiki/09-md-ecu-binary-protocol.md and wiki/04-espnow-bridge.md. Kept for context only — do not follow as current guidance. -->

# ESPNOW/OpenDash Frame Parser — Battle Report

## The Problem

The multidisplay-unified Qt6 application receives **some IMU data** from the ESP-NOW bridge, but **missing most of the ESPNOW payload**. Similar to the MD ECU issue, we suspect frame boundary detection is not working properly.

## Phase 1: Understanding the Architecture

### Current Implementation (OpenDashBridge.cpp)

```cpp
void OpenDashBridge::onReadyRead() {
    QByteArray data = m_serial->readAll();
    
    emit rawReceived(data, data.length());
    m_receiveBuffer.append(data);

    // Process line by line
    while (m_receiveBuffer.contains('\n')) {
        int newlinePos = m_receiveBuffer.indexOf('\n');
        QString line = QString::fromUtf8(m_receiveBuffer.left(newlinePos)).trimmed();
        m_receiveBuffer.remove(0, newlinePos + 1);
        
        if (cleanLine.startsWith("DP:")) {
            processDPMessage(cleanLine);
        }
    }
}
```

**Assumptions:**
- Data arrives as newline-delimited text lines
- Each `DP:` line is a complete message
- No binary framing needed

### MD ECU Comparison (Fixed)

```cpp
void MdSerialComUnified::onReadyRead() {
    // Process byte by byte using state machine
    for (int i = 0; i < data.size(); i++) {
        quint8 d = static_cast<quint8>(data[i]);

        switch (m_status) {
        case MD_STATUS_RECEIVING:
            if (d == 0x03) {  // ETX byte marks frame end
                m_status = MD_STATUS_FRAME_COMPLETE;
                int actualFramelength = m_index + 1;
                QByteArray payload(reinterpret_cast<char*>(rcvData) + 2, actualFramelength - 3);
                processMainData(payload);
            }
        }
    }
}
```

**Key Difference:**
- MD ECU uses **ETX byte (0x03)** to detect frame boundaries
- OpenDashBridge assumes **newline (\n)** is sufficient

## Phase 2: The Risk Analysis

### What Could Go Wrong?

| Issue | MD ECU (Fixed) | ESPNOW/OpenDash (Risk) |
|-------|---------------|------------------------|
| **Multiple frames in one readAll()** | ✓ Seen (142 bytes = 2 frames) | ⚠️ Possible if ESP32 batches DP lines |
| **Partial lines across reads** | N/A (binary stream) | ⚠️ Lines split by chunk boundaries |
| **Garbage/debug output mixed in** | ✓ Seen between frames | ⚠️ ESP32 debug logs contaminating data |
| **Buffer not resetting on error** | ✓ Fixed (memset pattern) | ⚠️ No buffer reset on parse failure |

### The Critical Question

**Is the ESP32 bridge sending pure text lines, or is there binary framing?**

If the ESP32 firmware does ANY of these, we have a problem:
- Sends multiple `DP:` lines in one USB packet without proper line separation
- Has debug logs mixed with DP data
- Uses different line endings (\r\n vs \n)
- Sends partial lines due to buffer overflow

## Phase 3: Diagnostic Enhancement (Applied)

Added hex dump output to see exactly what bytes arrive:

```cpp
void OpenDashBridge::onReadyRead() {
    QByteArray data = m_serial->readAll();
    
    // Hex dump for debugging (similar to MD ECU fix)
    qDebug() << "[ESP-NOW-Bridge] Raw bytes received:" << data.length() << "bytes";
    qDebug() << "[ESP-NOW-Bridge] Hex dump:" << data.toHex(' ').data();
    
    emit rawReceived(data, data.length());
    m_receiveBuffer.append(data);
    // ... rest of processing
}
```

## Phase 4: What to Look For

### Expected (Good) Data

```
Raw bytes received: 28 bytes
Hex dump: 44 50 3a 30 78 30 31 30 30 3a 33 32 30 30 2e 30 0a
Clean line: DP:0x0100:3200.0
```

### Problem Indicators

#### 1. Multiple Lines Without Proper Separation
```
Raw bytes received: 56 bytes  
Hex dump: 44 50 3a 30 78 30 31 30 30 3a 33 32 30 30 2e 30 44 50 3a 30 78 30 31 30 36 3a 31 34 35 2e 35 0a
Clean line: DP:0x0100:3200.0DP:0x0106:145.5 (MISSING NEWLINE!)
```

#### 2. Partial Lines
```
Raw bytes received: 14 bytes
Hex dump: 44 50 3a 30 78 30 31 30 30 3a 33 32 30 30 (NO NEWLINE!)
Clean line: DP:0x0100:3200 (INCOMPLETE - waiting for next chunk)
```

#### 3. ESP32 Debug Output Mixed In
```
Raw bytes received: 89 bytes
Hex dump: 49 20 28 31 32 33 34 35 29 20 63 70 75 5f 73 74 61 72 74 3a 20 49 6e 69 74 20 64 6f 6e 65 0a 44 50 3a 30 78 30 31 30 30 3a 33 32 30 30 2e 30 0a
Clean line: I (12345) cpu_start: Init done (ignored)
Next line: DP:0x0100:3200.0
```

#### 4. Wrong Line Endings
```
Raw bytes received: 29 bytes
Hex dump: 44 50 3a 30 78 30 31 30 30 3a 33 32 30 30 2e 30 0d 0a
Clean line: DP:0x0100:3200.0 (with \r, might cause issues)
```

## Phase 5: Recommended Fixes

### Fix #1: Buffer Reset on Parse Failure

```cpp
void OpenDashBridge::processDPMessage(const QString& line) {
    static QRegularExpression dpRegex("^DP:(0x[0-9a-fA-F]+):([\\-+]?[0-9]*\\.?[0-9]+([eE][\\-+]?[0-9]+)?)$");
    QRegularExpressionMatch match = dpRegex.match(line);

    if (!match.hasMatch()) {
        qWarning() << "[ESP-NOW-Bridge] Failed to parse DP line:" << line;
        
        // Buffer reset on error (matching MD ECU pattern)
        m_receiveBuffer.clear();
        qDebug() << "[ESP-NOW-Bridge] Buffer cleared due to parse error";
        return;
    }
    
    // ... rest of parsing
}
```

### Fix #2: Handle Multiple Lines in One Chunk

```cpp
void OpenDashBridge::onReadyRead() {
    QByteArray data = m_serial->readAll();
    if (data.isEmpty()) return;

    emit rawReceived(data, data.length());
    m_receiveBuffer.append(data);

    // Process ALL complete lines in buffer
    int processedLines = 0;
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
        
        processedLines++;
    }
    
    // Log if we processed multiple lines in one chunk
    if (processedLines > 1) {
        qDebug() << "[ESP-NOW-Bridge] Processed" << processedLines << "lines in single readAll()";
    }

    // Safety: clear buffer if it grows too large
    if (m_receiveBuffer.size() > 4096) {
        m_receiveBuffer.clear();
        emit errorOccurred("Bridge buffer overflow");
    }
}
```

### Fix #3: Add Line Validation

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
        
        // Buffer reset on error
        m_receiveBuffer.clear();
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
}
```

## Phase 6: Testing Strategy

### Step 1: Run with Hex Dump

```bash
./multidisplay-unified --bridge /dev/ttyACM7 2>&1 | grep -E "(Raw bytes|Hex dump|Clean line)"
```

**Look for:**
- Are all lines properly newline-terminated?
- Are there multiple DP lines in one raw chunk?
- Is there ESP32 debug output mixed in?

### Step 2: Check Raw Hex

If you see issues, look at the hex dump:

```bash
# Expected: Each line ends with 0a (newline)
# Problem: Lines concatenated without 0a between them
```

### Step 3: Verify ESP32 Firmware

Check your ESP32 bridge firmware for:
- `Serial.println()` vs `Serial.print()` - must use `println()` for newlines
- Debug log filtering - disable INFO/DEBUG level logs
- Buffer flushing - ensure data is sent immediately

## Phase 7: Root Cause Hypotheses

### Hypothesis A: ESP32 Firmware Not Sending Newlines

**Symptom:** Lines appear concatenated in hex dump

**Fix:** Ensure firmware uses `Serial.println()` not `Serial.print()`

```cpp
// Wrong:
Serial.print("DP:0x0100:3200.0");

// Right:
Serial.println("DP:0x0100:3200.0");
```

### Hypothesis B: ESP32 Debug Logs Mixed In

**Symptom:** Non-DP lines in output (I (12345), E (1234), etc.)

**Fix:** Filter ESP32 logs in firmware:

```cpp
esp_log_level_set("*", ESP_LOG_ERROR);  // Only show errors
```

### Hypothesis C: Buffer Not Resetting on Error

**Symptom:** After one parse failure, all subsequent data is corrupted

**Fix:** Apply Fix #1 above (buffer reset on error)

## Phase 8: Comparison to MD ECU Fix

| Aspect | MD ECU | ESPNOW/OpenDash |
|--------|--------|-----------------|
| **Frame boundary** | ETX byte (0x03) | Newline (\n) |
| **Data format** | Binary | Text |
| **State machine** | Byte-by-byte | Line-by-line |
| **Buffer reset on error** | ✓ Yes (memset) | ⚠️ No (needs fix) |
| **Multiple frames/chunk** | ✓ Handled by state machine | ⚠️ Should work but verify |
| **Garbage detection** | ✓ Non-STX bytes skipped | ⚠️ Unknown messages logged |

## Phase 9: Next Steps

1. **Run the application with hex dump enabled**
2. **Capture raw data from ESPNOW bridge**
3. **Analyze hex output for patterns**
4. **Apply fixes based on findings**

### Immediate Action Items

- [ ] Test with hex dump enabled
- [ ] Capture sample data showing missing payloads
- [ ] Analyze hex output to identify pattern
- [ ] Apply buffer reset on error (Fix #1)
- [ ] Add line validation (Fix #3)

## Lessons Learned from MD ECU Fix

1. **Never trust documentation comments over actual data** - The original app's comment said "93-byte payload" but real hardware showed 92 bytes
2. **Hex dumps are your best friend** - Couldn't solve this without seeing raw bytes
3. **The original app isn't always right** - Just because it works doesn't mean its logic is correct
4. **State machines handle variable data naturally** - Byte-by-byte processing works regardless of chunk boundaries

## Conclusion

The OpenDashBridge implementation assumes text-line-based communication, which should work if the ESP32 firmware sends proper newline-terminated lines. However, without seeing the actual raw hex data, we can't confirm:

1. Are newlines present between DP lines?
2. Is there debug output mixed in?
3. Are lines being split across reads?

**The hex dump enhancement I added will reveal exactly what's happening at the byte level**, just like it did for the MD ECU fix.
