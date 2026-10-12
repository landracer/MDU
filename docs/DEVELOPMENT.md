# MultiDisplay Unified — Development Guide

## Quick Start for Contributors

### 1. Build the Project

```bash
cd multidisplay-unified
./build.sh release
```

Or manually:
```bash
mkdir build && cd build
cmake .. -DENABLE_QWT=ON
make -j$(nproc)
```

### 2. Run Unit Tests

```bash
./multidisplay-unified --unit-test
```

Expected output:
```
========================================
  MultiDisplay Unified — Test Suite
========================================
  ✓ testDataPointSetGet : RPM: 3200 (expected 3200)
  ✓ testDataPointSetGet_boost : Boost: 145.5 (expected 145.5)
  ...
========================================
  Results: 34/34 passed
  ALL TESTS PASSED
========================================
```

### 3. Run in Test Mode (Simulated Data)

```bash
./multidisplay-unified --test
```

This generates realistic simulated data for all 4 hardware systems at 30Hz. You'll see:
- Engine RPM, Boost, Lambda, EGTs scrolling on the plot
- BMS Pack Voltage/Current/SOC updating
- VESC Motor RPM/Voltage/Amps displayed
- Wheel RPMs for all 4 corners
- IMU Pitch/Roll/Yaw values

### 4. Connect Real Hardware

```bash
# Single device
./multidisplay-unified --md-ecu /dev/ttyUSB0

# Multiple devices
./multidisplay-unified \
  --md-ecu /dev/ttyUSB0 \
  --bms /dev/ttyUSB1 \
  --bridge /dev/ttyUSB2

# CSV replay
./multidisplay-unified --csv MD0001.CSV
```

---

## Architecture Overview

### Core Components

```
┌─────────────────────────────────────────────────────┐
│                   MainWindow                         │
│  (UI shell, hardware connection management)         │
├─────────────────────────────────────────────────────┤
│                                                     │
│  ┌──────────────┐  ┌──────────────┐                │
│  │ MdSerialCom  │  │ RAtTraxBMS   │                │
│  │ Unified      │  │              │                │
│  └──────┬───────┘  └──────┬───────┘                │
│         │                 │                        │
│  ┌──────┴───────┐  ┌──────┴───────┐               │
│  │ OpenDash     │  │ MDLoggerCSV  │               │
│  │ Bridge       │  │              │               │
│  └──────┬───────┘  └──────┬───────┘               │
│         │                 │                        │
│         └────────┬────────┘                        │
│                  │                                 │
│         ┌────────▼────────┐                       │
│         │ UnifiedDataStore│ ← SINGLETON           │
│         │ (Thread-Safe)   │                       │
│         └────────┬────────┘                       │
│                  │ emits dataPointUpdated()        │
│         ┌────────▼────────┐                       │
│         │ DashboardWidget │                       │
│         │ (QTabWidget +   │                       │
│         │  RealtimePlot)  │                       │
│         └─────────────────┘                       │
│                                                     │
├─────────────────────────────────────────────────────┤
│              DataPointRegistry                      │
│  (Metadata, scaling, validation — static singleton) │
└─────────────────────────────────────────────────────┘
```

### Data Flow

1. **Hardware** → Serial/USB/Bluetooth
2. **ICommunication subclass** parses raw bytes → `QHash<DataPointID, double>`
3. **MainWindow** receives parsed hash → calls `UnifiedDataStore::setDataPoint()`
4. **UnifiedDataStore** stores value with timestamp → emits `dataPointUpdated()` signal
5. **DashboardWidget** receives signal → updates corresponding `RealtimePlot`
6. **RealtimePlot** appends point to curve → triggers Qwt redraw

### Key Design Principles

1. **Single Source of Truth**: UnifiedDataStore is the ONLY place data lives
2. **Loose Coupling**: Parsers don't know about UI, UI doesn't know about hardware
3. **Thread Safety**: QReadWriteLock protects all data store access
4. **Extensible**: New data points = enum entry + registry entry, no code changes elsewhere
5. **Testable**: Every component can be tested in isolation

---

## Adding a New Data Point

### Step 1: Add to Enum (include/UnifiedDataPoint.h)

```cpp
enum class DataPointID : uint32_t {
    // ... existing points ...
    
    // Add your new point in the appropriate range
    MyNewSensor = 0x0120,  // Engine/ECU range
};
```

### Step 2: Add Helper Strings (include/UnifiedDataPoint.h)

```cpp
inline const char* DataPointName(DataPointID id) {
    switch (id) {
        // ... existing cases ...
        case DataPointID::MyNewSensor: return "My New Sensor";
        // ...
    }
}

inline const char* DataPointUnit(DataPointID id) {
    switch (id) {
        // ... existing cases ...
        case DataPointID::MyNewSensor: return "units";
        // ...
    }
}
```

### Step 3: Register in Registry (src/DataPointRegistry.cpp)

```cpp
m_registry[DataPointID::MyNewSensor] = {
    DataPointID::MyNewSensor,
    "My New Sensor",
    "units",
    "ENGINE/ECU",
    0.01,     // rawToSI multiplier
    0.0,      // siOffset
    false     // signedValue
};
```

### Step 4: Parse in Your Communication Handler

```cpp
// In your ICommunication subclass
QHash<DataPointID, double> values;
values[DataPointID::MyNewSensor] = parseRawValue(rawBytes);
emit framesParsed(values, "MySource");
```

### Step 5: That's It!

The data point will automatically appear in:
- DashboardWidget tabs (based on category range)
- RealtimePlot legends
- Status displays
- Unit tests (if you add them)

---

## Adding a New Communication Channel

### Step 1: Create Header (include/MyDevice.h)

```cpp
#ifndef MYDEVICE_H
#define MYDEVICE_H

#include "ICommunication.h"

class MyDevice : public ICommunication {
    Q_OBJECT
public:
    explicit MyDevice(const QString& portName, QObject* parent = nullptr);
    
    bool connect() override;
    void disconnect() override;
    
public slots:
    void onReadyRead();
    
signals:
    void frameParsed(const QHash<DataPointID, double>& values);
};

#endif // MYDEVICE_H
```

### Step 2: Implement (src/MyDevice.cpp)

```cpp
#include "MyDevice.h"

MyDevice::MyDevice(const QString& portName, QObject* parent)
    : ICommunication("MyDevice", parent)
{
    // Setup serial port, timers, etc.
}

bool MyDevice::connect() {
    // Open connection
    m_connected = true;
    emit connectionStateChanged(true);
    return true;
}

void MyDevice::onReadyRead() {
    QByteArray data = m_serial->readAll();
    emit rawReceived(data, data.length());
    
    // Parse data → QHash<DataPointID, double>
    QHash<DataPointID, double> values = parseFrame(data);
    
    if (!values.isEmpty()) {
        emit framesParsed(values, "MyDevice");
    }
}
```

### Step 3: Connect in MainWindow (src/MainWindow.cpp)

```cpp
void MainWindow::connectMyDevice(const QString& portName) {
    m_myDevice = new MyDevice(portName, this);
    
    connect(m_myDevice, &MyDevice::frameParsed,
            this, [this](const QHash<DataPointID, double>& values) {
        for (auto it = values.begin(); it != values.end(); ++it) {
            UnifiedDataStore::instance().setDataPoint(
                it.key(), it.value(), "MyDevice"
            );
        }
    });
    
    m_myDevice->connect();
}
```

---

## Coding Standards

### C++17 Features to Use

- `std::optional` for nullable values
- `std::variant` for discriminated unions
- Structured bindings: `auto [key, value] = pair;`
- `if constexpr` for compile-time branching
- `std::string_view` for read-only string parameters

### Qt Conventions

- Use `QString` for text (not `std::string`)
- Use `qDebug()` for logging (not `printf`)
- Use signals/slots for all cross-component communication
- Use `QReadWriteLock` for shared data protection
- Prefer `QPointer<T>` for weak references to QObject-derived classes

### Naming Conventions

| Type | Convention | Example |
|------|-----------|---------|
| Classes | PascalCase | `UnifiedDataStore` |
| Functions | camelCase | `setDataPoint()` |
| Variables | camelCase | `m_portName` |
| Constants | UPPER_SNAKE | `MAX_BUFFER_SIZE` |
| Enums | PascalCase + members camelCase | `DataPointID::RPM` |

### File Organization

```
include/ClassName.h    ← Public API, no implementation
src/ClassName.cpp      ← Implementation
```

- Headers should be self-contained (include their own dependencies)
- Use include guards (`#ifndef CLASSNAME_H`)
- Keep headers minimal — forward declare when possible

---

## Testing Guidelines

### Unit Tests

Place tests in `src/TestHarness.cpp` or create new test files.

Test everything that:
- Converts raw bytes to SI values
- Parses protocol frames
- Validates data point ranges
- Handles edge cases (overflow, underflow, NaN)

### Integration Tests

With hardware connected:
1. Verify each system reports correct values
2. Check update rates match specifications
3. Test disconnection/reconnection handling
4. Verify no data loss during high-rate updates

### Test Mode

When hardware is unavailable, use `--test` mode which generates:
- Realistic RPM curves (idle → rev → idle)
- Simulated BMS discharge curve
- VESC motor response to throttle
- GPS track simulation

---

## Common Tasks

### How to Add a New Plot Tab

```cpp
// In DashboardWidget::setupUI()
RealtimePlot* plot = new RealtimePlot(tabWidget);
plot->setTitle("My Custom Plot");
plot->setTimeWindow(60.0); // 60 second scroll window
layout->addWidget(plot, 1);
m_plots["Custom"] = plot;
```

### How to Subscribe to Data Updates

```cpp
connect(&UnifiedDataStore::instance(),
        &UnifiedDataStore::dataPointUpdated,
        this, [this](DataPointID id, double value, const QString& source) {
    if (id == DataPointID::RPM) {
        m_rpmLabel->setText(QString("%1 RPM").arg(value, 0, 'f', 0));
    }
});
```

### How to Log Debug Information

```cpp
qDebug() << "[MyComponent] Processing frame:" << frameSize << "bytes";
qWarning() << "[MyComponent] Invalid data point ID:" << dpId;
qCritical() << "[MyComponent] Failed to open port:" << errorString;
```

---

## Protocol References

### MultiDisplay ECU Binary Protocol

**⚠️ CRITICAL:** The TAG byte's decimal value IS the total frame length (main frame tag 0x5F = 95 → 95-byte frame). The parser counts bytes up to that length and only then validates the final byte is ETX (0x03). A 0x03 inside a payload never ends a frame.

- **Definitive reference:** [`wiki/09-md-ecu-binary-protocol.md`](../wiki/09-md-ecu-binary-protocol.md) — Complete protocol spec with payload layout, state machine, and common pitfalls
- **Hardware integrations:** [`wiki/03-hardware-integrations.md`](../wiki/03-hardware-integrations.md) — MD ECU section with frame types and data point mapping

### Key Implementation Notes

1. Main sensor frame: TAG=0x5F, 95 bytes total (92 bytes payload), contains all engine telemetry
2. Shorter control frames (N75 maps, gear ratio, ACK) carry their own tag/length values (22, 38, 23, 17, 4)
3. All multi-byte values are Little Endian with fixed-point scaling (/100 for most)
4. Process bytes one at a time through state machine; frames can span or combine in serial chunks

---

## Troubleshooting

### Build Errors

**"Qwt not found"**
```bash
# Install Qwt on Ubuntu/Debian
sudo apt install libqwt-dev

# On macOS with Homebrew
brew install qwt
```

**"Qt6 modules not found"**
```bash
# Ensure Qt6 is in PATH
export PATH=/path/to/qt6/bin:$PATH
cmake .. -DCMAKE_PREFIX_PATH=/path/to/qt6
```

### Runtime Issues

**"Cannot open port /dev/ttyUSB0"**
- Check permissions: `sudo usermod -aG dialout $USER`
- Check device exists: `ls -la /dev/ttyUSB*`
- Try different baud rate

**Plots not updating**
- Verify hardware is connected and sending data
- Check status bar for connection state
- Run with `--test` to verify UI works

**Memory leaks**
- All QObject-derived classes should have parent pointers
- Use `QPointer<T>` for observer patterns
- Run with Valgrind: `valgrind --leak-check=full ./multidisplay-unified`

---

## Next Steps for the Team

### Phase 1 — Foundation ✅ (v0.1.0)
- ✅ UnifiedDataStore
- ✅ DataPointRegistry
- ✅ Communication interfaces
- ✅ Basic UI shell
- ✅ Qwt plot integration (Qwt 6.3-Qt6; stub fallback when absent)

### Phase 2 — Feature Complete (v0.2.0)
- [x] Layout Editor dialog
- [x] Boost map configuration (BoostConfigDialog, N75 widgets)
- [x] Configuration persistence (QSettings)
- [ ] Session save/load/replay beyond CSV import
- [ ] OTA update support
- [ ] Multi-window support

### Phase 3 — Polish & Deploy
- [ ] Android deployment
- [ ] Windows installer
- [x] Documentation (wiki/ + docs/ restructured)
- [ ] Performance optimization
- [ ] CI/CD pipeline

See [`../A-class.md`](../A-class.md) for the full professional-grade game plan.

---

**Questions? Open an issue or ask in the group chat.**
