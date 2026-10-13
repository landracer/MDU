# Architecture Overview

## System Design Philosophy

MultiDisplay Unified is built around a **unified data model** — all four hardware systems feed into a single thread-safe data store using OpenDash-compatible Data Point IDs (0x0100–0x07FF). This means the UI doesn't care where data comes from; it just receives parsed `DataPointID → double` mappings.

## High-Level Architecture

```
┌─────────────────────────────────────────────────────────────────────┐
│                     MultiDisplay Unified                            │
│                        (Qt6 Application)                            │
├─────────────────────────────────────────────────────────────────────┤
│                                                                     │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐              │
│  │ MdSerialCom  │  │ RAtTraxBMS   │  │ OpenDash     │              │
│  │ Unified      │  │              │  │ Bridge       │              │
│  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘              │
│         │                 │                  │                       │
│  ┌──────┴─────────────────┴──────────────────┴───────┐              │
│  │           ICommunication Interface                │              │
│  │  (open/close/connectionStateChanged/frameParsed)  │              │
│  └────────────────────┬──────────────────────────────┘              │
│                       │                                              │
│              ┌────────▼────────┐                                    │
│              │ UnifiedDataStore│                                    │
│              │  (Thread-Safe)  │                                    │
│              │                 │                                    │
│              │ QReadWriteLock  │                                    │
│              │ Signal-based    │                                    │
│              │ updates @ ~30Hz │                                    │
│              └────────┬────────┘                                    │
│                       │                                              │
│        ┌──────────────┼──────────────┐                              │
│        │              │              │                                │
│  ┌─────▼─────┐ ┌─────▼─────┐ ┌─────▼─────┐                         │
│  │ Realtime  │ │Dashboard  │ │  Status   │                         │
│  │  Plot     │ │  Widget   │ │  Bar      │                         │
│  └───────────┘ └───────────┘ └───────────┘                         │
│                                                                     │
├─────────────────────────────────────────────────────────────────────┤
│  MDLoggerCSV (File Import)                                          │
│  ┌──────────────┐                                                   │
│  │ CSV Parser   │ ──► UnifiedDataStore                             │
│  └──────────────┘                                                   │
└─────────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. ICommunication Interface

Abstract base class that all hardware parsers implement. Provides a uniform API regardless of data source:

```cpp
class ICommunication : public QObject {
    virtual bool open() = 0;      // Connect to hardware
    virtual void close() = 0;     // Disconnect from hardware
    virtual bool isConnected() const = 0;
    
signals:
    void frameParsed(const QHash<DataPointID, double>& values);
    void connectionStateChanged(bool connected);
    void errorOccurred(const QString& error);
};
```

**Implementations:**
- `MdSerialComUnified` — MultiDisplay ECU binary frames (STX/TAG/ETX)
- `RAtTraxBMS` — rAtTrax-BMS CSV and binary protocols
- `OpenDashBridge` — ESP-NOW USB bridge (DP: line format)
- `MDLoggerCSV` — CSV file import with playback controls

### 2. UnifiedDataStore

Thread-safe singleton that holds the current state of all data points. Uses `QReadWriteLock` for concurrent read access and signal-based updates at ~30Hz.

```cpp
class UnifiedDataStore {
    static UnifiedDataStore& instance();  // Singleton
    
    void setDataPoint(DataPointID id, double value, const QString& source);
    double getDataPoint(DataPointID id, bool* valid = nullptr) const;
    bool isValid(DataPointID id) const;
    QDateTime getTimestamp(DataPointID id) const;
    
signals:
    void dataPointUpdated(DataPointID id, double value, const QString& source);
};
```

**Key Design Decisions:**
- Singleton pattern ensures single source of truth
- `QReadWriteLock` allows multiple readers without blocking
- Signal-based updates decouple data from UI
- Thread-safe for concurrent hardware parser access

### 3. DataPointRegistry

Central metadata registry for all 66+ registered data points. Provides:
- Validation (is this a known DP ID?)
- Metadata lookup (name, unit, category, scaling factor)
- Unit conversion (raw ↔ SI units)
- Category indexing (ENGINE/ECU, GPS/NAV, IMU, BMS, VESC/DRIVETRAIN)

```cpp
class DataPointRegistry {
    bool isValid(DataPointID id) const;
    PointInfo getInfo(DataPointID id) const;
    double toSI(DataPointID id, int16_t raw) const;
    int16_t fromSI(DataPointID id, double si) const;
    QVector<DataPointID> pointsInCategory(const QString& category) const;
};
```

### 4. RealtimePlot

Qwt-based scrolling plot with auto-scaling and color-coded traces. When Qwt is unavailable, falls back to a simple value display widget.

**Features:**
- Automatic Y-axis scaling (min/max tracking)
- Configurable time window (1s to 60s scroll)
- Multiple color-coded curves per data source
- Thread-safe updates via signal/slot
- Periodic cleanup of old data points

### 5. DashboardWidget

QTabWidget with category-based tabs:
- **Engine** — RPM, Boost, Lambda, EGTs, Battery
- **GPS_IMU** — GPS Speed, Lat/Lon, IMU Pitch/Roll/Yaw
- **BMS** — Pack V/A/SOC, Cell Voltages, Temps
- **VESC** — Motor V/A/RPM, Wheel RPMs, Faults
- **AllData** — Complete data point listing

### 6. MainWindow

Main application window with:
- Hardware connection management (connect/disconnect buttons)
- Status bar showing connection states and warnings
- Integration with DashboardWidget and RealtimePlot
- Command-line argument parsing for hardware ports

## Data Flow

```
Hardware Device
       │
       ▼
┌──────────────┐     ┌──────────────┐
│ Serial/USB/  │────▶│ Parser/      │
│ Bluetooth/   │     │ Protocol     │
│ File         │     │ Handler      │
└──────────────┘     └──────┬───────┘
                            │
                    frameParsed(values)
                            │
                            ▼
                   ┌────────────────┐
                   │ UnifiedDataStore│
                   │ (QReadWriteLock)│
                   └────────┬───────┘
                            │
                    dataPointUpdated()
                            │
              ┌─────────────┼─────────────┐
              ▼             ▼             ▼
       ┌──────────┐  ┌──────────┐  ┌──────────┐
       │ Realtime │  │Dashboard │  │ Status   │
       │  Plot    │  │  Widget  │  │  Bar     │
       └──────────┘  └──────────┘  └──────────┘
```

## Communication Protocols

### MultiDisplay ECU (Binary)

Custom binary protocol with frame structure:
- **STX** (start byte) = `0x02`
- **TAG** (data type identifier) = varies
- **Data payload** = sensor values in fixed-point format
- **ETX** (end byte) = `0x03`

Supported data types include main sensor frames, N75 duty maps, setpoint maps, and gearbox settings.

### rAtTrax-BMS (CSV + Binary)

- **CSV mode**: Comma-separated values from ESP32 firmware logging
  - Format: `Timestamp,GPS_Lat,GPS_Lng,...,BMS_SOC,BMS_Voltage,...`
- **Binary mode**: Direct BQ76952 protocol via ESP32 bridge

### OpenDash Mesh (ESP-NOW → USB)

ESP-NOW frames received by ESP32-S3 bridge, parsed, and forwarded as ASCII text:
```
DP:0x0400:25.20
DP:0x0100:3200.0
```

See [ESP-NOW Bridge Guide](04-espnow-bridge.md) for full specification.

### MDLogger (CSV File Import)

Standard CSV files generated by ATmega328P logger on SD card:
```
timestamp,rpm,boost,egt1,egt2,...,lambda,throttle,speed,gear,...
```

Supports playback controls (speed multiplier, pause/resume).

## Platform Support

| Platform | Status | Notes |
|----------|--------|-------|
| Linux | ✅ Tested | Primary development platform |
| Windows | ✅ Buildable | MSVC + MinGW supported |
| Android | ✅ Configured | Requires Qt for Android + NDK |
| macOS | 🔲 Planned | Pending debug hardware acquisition |

## Build System

CMake-based build with optional Qwt support:

```bash
cmake .. -DENABLE_QWT=ON    # With Qwt plotting library
cmake .. -DENABLE_QWT=OFF   # Without Qwt (stub mode)
```

When Qwt is unavailable, RealtimePlot falls back to a simple value display widget. This allows development and testing without requiring the Qwt library installation.

## Testing

20 test groups / 49 assertions (run with `--unit-test`, wired into CTest) covering:
- DataStore set/get operations
- Data point validity tracking
- Timestamp management
- Registry validation and metadata
- Unit conversion (raw ↔ SI)
- Category indexing
- Endianness handling
- Checksum calculations (XOR, CRC-16)
- Clamp/lerp utilities
- Angle conversions (degrees ↔ radians)

Run with: `./multidisplay-unified --unit-test`

---

*Next: [Data Point System](02-data-points.md)*
