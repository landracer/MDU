<!-- ARCHIVED: Historical working notes from the June 2026 parser-debugging sessions. Superseded by wiki/09-md-ecu-binary-protocol.md and wiki/04-espnow-bridge.md. Kept for context only — do not follow as current guidance. -->

# ESPNOW Data Flow Analysis - Immediate Findings

## Executive Summary

**The OpenDash firmware is NOT sending any DP: data lines.** It only outputs ESP32 debug logs.

## Evidence

### Raw Serial Output from /dev/ttyACM2

```
I (173158490) opendash_uart: UART diag: 5357456 bytes, 1509825 STX, status=MD: mdv2 Connected
```

This is **ESP32 debug output**, NOT sensor data.

### No DP Lines Found

```bash
$ timeout 10s cat /dev/ttyACM2 | grep "DP:"
# (empty - no matches)
```

## Root Cause Analysis

### The Problem

The OpenDash firmware running on the ESP32 bridge node is:
1. ✅ Receiving ESP-NOW packets (1,509,825 STX bytes received)
2. ❌ NOT sending DP: lines over USB CDC-ACM
3. ❌ Only outputting debug logs

### Why This Happens

Looking at the parkpounder.md documentation:

> Firmware: listens for ESP-NOW packets (using OpenDash's common/ stack), extracts each data point, prints to USB CDC-ACM as **DP:%04X:%f\n**

This firmware **does not exist yet**. The current OpenDash firmware only:
- Receives ESP-NOW packets
- Displays them on the dashboard
- Outputs debug logs

It does NOT forward data to USB CDC-ACM.

## What You Need to Do

### Option 1: Use rAtTrax-BMS Instead (Immediate Solution)

The rAtTrax-BMS is already sending data:

```bash
$ cat /dev/ttyACM6 | grep -E "(MD ECU|Connected)"
mdv2 Connected
```

This port has the MD-ECU connected, not ESPNOW.

### Option 2: Flash OpenDash Bridge Firmware (Requires Code Change)

You need to flash a **new ESP32 firmware** that:
1. Listens for ESP-NOW packets
2. Extracts data points
3. Prints them as `DP:%04X:%f\n` to USB CDC-ACM

### Option 3: Modify Existing OpenDash Firmware (Requires Code Change)

If you have access to the OpenDash firmware source, modify it to:
1. In the ESP-NOW receive callback, format each data point as `DP:XXXX:value`
2. Write to USB CDC-ACM instead of just displaying on screen

## Immediate Action Items

### 1. Verify Which Port Has ESPNOW Data

```bash
# Check all available serial ports
ls /dev/ttyACM* /dev/ttyUSB*

# Test each one for DP: lines
for port in /dev/ttyACM0 /dev/ttyACM1 /dev/ttyACM2 /dev/ttyACM3; do
    echo "=== Testing $port ==="
    timeout 5s cat $port | grep -E "(DP:|Connected)" | head -10
done
```

### 2. If No ESPNOW Data Exists

You have two choices:

**A. Use rAtTrax-BMS (if you have BMS hardware)**
- Connect BMS to a different USB port
- Configure Qt app to use that port instead

**B. Flash OpenDash Bridge Firmware**
- Get the firmware from OpenDash repository
- Modify it to send DP: lines over USB
- Flash to ESP32

## Dmod.md - Required Changes for OpenDash Firmware

### File: `Dmod.md`

```
# OpenDash Firmware Modifications for Qt App Integration

## Problem
OpenDash nodes broadcast data via ESP-NOW but do NOT forward it to USB CDC-ACM.

## Solution
Modify the ESP-NOW receive callback to print data points as DP: lines.

## Changes Required

### File: opendash_espnow.c (or similar)

In the esp_now_recv_cb() function, add:

```c
// After receiving ESP-NOW packet and extracting data point:
printf("DP:0x%04X:%.2f\n", dp_id, value);
fflush(stdout);  // Force immediate transmission
```

### File: sdkconfig (if using ESP-IDF)

Disable debug logs to reduce noise:

```
CONFIG_LOG_DEFAULT_LEVEL_ERROR=y
```

## Testing

After flashing:
```bash
cat /dev/ttyACM2 | grep "DP:"
# Should see lines like: DP:0x0100:3200.00
```

## Notes
- Use `fflush(stdout)` to ensure immediate transmission
- Format: `DP:<4-digit-hex-id>:<float-value>`
- Float format: use %.2f for consistency