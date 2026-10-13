# API Reference

## Overview

This page documents the public APIs of MultiDisplay Unified's core components. All classes inherit from `QObject` and use Qt's signal/slot mechanism for communication.

---

## ICommunication Interface

Abstract base class for all hardware parsers. Provides a uniform interface regardless of data source.

### Header
```cpp
#include "ICommunication.h"
```

### Class Definition

```cpp
class ICommunication : public QObject {
    Q_OBJECT
public:
    explicit ICommunication(const QString& name, QObject* parent = nullptr);
    virtual ~ICommunication() = default;

    // Connection management (override in subclasses)
    virtual bool open() = 0;           // Connect to hardware
    virtual void close() = 0;          // Disconnect from hardware
    virtual bool isConnected() const;  // Check connection state

    // Identity
    QString name() const;              // Get channel name
    void setName(const QString& name); // Set channel name

signals:
    void frameParsed(const QHash<DataPointID, double>& values);
        // Emitted when a complete frame of data is parsed
        // Values map DataPointID → double (SI units)
    
    void connectionStateChanged(bool connected);
        // Emitted when connection state changes
    
    void errorOccurred(const QString& error);
        // Emitted on errors (connection, parse, etc.)
};
```

### Implementations

| Class | File | Hardware |
|-------|------|----------|
| `MdSerialComUnified` | [`src/MdSerialComUnified.cpp`](../src/MdSerialComUnified.cpp) | MultiDisplay ECU |
| `RAtTraxBMS` | [`src/RAtTraxBMS.cpp`](../src/RAtTraxBMS.cpp) | rAtTrax-BMS |
| `OpenDashBridge` | [`src/OpenDashBridge.cpp`](../src/OpenDashBridge.cpp) | ESP-NOW Bridge |
| `MDLoggerCSV` | [`src/MDLoggerCSV.cpp`](../src/MDLoggerCSV.cpp) | CSV file import |

---

## UnifiedDataStore

Thread-safe singleton that holds the current state of all data points.

### Header
```cpp
#include "UnifiedDataStore.h"
```

### Class Definition

```cpp
class UnifiedDataStore : public QObject {
    Q_OBJECT
public:
    // Singleton access
    static UnifiedDataStore& instance();

    // Data point operations
    void setDataPoint(DataPointID id, double value, const QString& source);
        // Set a data point value (called by parsers)
    
    double getDataPoint(DataPointID id, bool* valid = nullptr) const;
        // Get current value. Returns 0.0 if invalid.
        // If valid pointer is provided, sets it to true/false
    
    bool isValid(DataPointID id) const;
        // Check if this data point has been set at least once
    
    QDateTime getTimestamp(DataPointID id) const;
        // Get timestamp of last update

signals:
    void dataPointUpdated(DataPointID id, double value, const QString& source);
        // Emitted whenever a data point is updated
        // UI components connect to this signal for real-time updates
};
```

### Usage Example

```cpp
// Get singleton instance
auto& store = UnifiedDataStore::instance();

// Write (from parser)
store.setDataPoint(DataPointID::RPM, 3200.0, "ESP-NOW-Bridge");

// Read (from UI)
bool valid = false;
double rpm = store.getDataPoint(DataPointID::RPM, &valid);
if (valid) {
    qDebug() << "RPM:" << rpm;
}

// Check validity
if (store.isValid(DataPointID::BoostPressure)) {
    // Boost pressure has been received at least once
}

// Get timestamp
QDateTime ts = store.getTimestamp(DataPointID::RPM);
```

### Thread Safety

- Uses `QReadWriteLock` for concurrent access
- Multiple readers can read simultaneously without blocking
- Writer (parser threads) gets exclusive access when updating
- Signal-based updates are thread-safe via Qt's queued connections

---

## DataPointRegistry

Central metadata registry for all registered data points.

### Header
```cpp
#include "DataPointRegistry.h"
```

### Class Definition

```cpp
class DataPointRegistry {
public:
    static DataPointRegistry& instance();

    // Validation
    bool isValid(DataPointID id) const;
        // Returns true if this DP ID is registered

    // Metadata lookup
    struct PointInfo {
        DataPointID id;
        const char* name;       // Human-readable name
        const char* unit;       // Unit string (e.g., "RPM", "kPa")
        const char* category;   // Category (e.g., "ENGINE/ECU")
        double rawToSI = 1.0;   // Scale factor for raw → SI conversion
        double siOffset = 0.0;  // Offset to apply after multiplication
        bool signedValue = true;
    };

    PointInfo getInfo(DataPointID id) const;
        // Returns metadata for a data point

    // Unit conversion
    double toSI(DataPointID id, int16_t raw) const;
    double toSI(DataPointID id, uint16_t raw) const;
    double toSI(DataPointID id, int32_t raw) const;
    double toSI(DataPointID id, float raw) const;
    double toSI(DataPointID id, double raw) const;
        // Convert raw hardware value to SI units

    int16_t fromSI(DataPointID id, double si) const;
    uint16_t fromSI_signed(DataPointID id, double si) const;
        // Convert SI value back to raw hardware format

    // Category queries
    QVector<DataPointID> allPoints() const;
        // Returns all registered data point IDs
    
    QVector<DataPointID> pointsInCategory(const QString& category) const;
        // Returns data point IDs in a specific category

    // Description
    QString description(DataPointID id) const;
        // Returns human-readable description

    // Type checks
    bool isNumeric(DataPointID id) const;
    bool isTemperature(DataPointID id) const;
    bool isVoltage(DataPointID id) const;
};
```

### Usage Example

```cpp
auto& registry = DataPointRegistry::instance();

// Check if valid
if (registry.isValid(DataPointID::RPM)) {
    auto info = registry.getInfo(DataPointID::RPM);
    qDebug() << info.name << "—" << info.unit << "in category" << info.category;
}

// Convert raw to SI
int16_t rawCoolant = 9500;
double coolantSI = registry.toSI(DataPointID::CoolantTemp, rawCoolant);
// Result: 95.00°C (raw × 0.01)

// Get all points in a category
auto enginePoints = registry.pointsInCategory("ENGINE/ECU");
qDebug() << "Engine has" << enginePoints.size() << "data points";
```

---

## DataPointID Enum

Defined in [`include/UnifiedDataPoint.h`](../include/UnifiedDataPoint.h).

### Helper Functions

```cpp
// Get category string for a data point
const char* DataPointCategory(DataPointID id);
// Returns: "ENGINE/ECU", "GPS/NAV", "IMU", "BMS", "VESC/DRIVETRAIN"

// Get human-readable name
const char* DataPointName(DataPointID id);
// Returns: "Engine RPM", "Boost Pressure", "GPS Speed", etc.

// Get unit string
const char* DataPointUnit(DataPointID id);
// Returns: "RPM", "kPa", "km/h", "°C", "V", "A", "%"
```

### Usage Example

```cpp
DataPointID id = DataPointID::BoostPressure;
qDebug() << DataPointName(id)   // "Boost Pressure"
         << DataPointUnit(id)   // "kPa"
         << DataPointCategory(id); // "ENGINE/ECU"
```

---

## RealtimePlot

Qwt-based scrolling plot with auto-scaling and color-coded traces. Falls back to value display when Qwt is unavailable.

### Header
```cpp
#include "RealtimePlot.h"
```

### Class Definition

```cpp
class RealtimePlot : public /* QwtPlot or QWidget */ {
    Q_OBJECT
public:
    explicit RealtimePlot(QWidget* parent = nullptr);
    ~RealtimePlot() override;

    // Series management
    bool addSeries(DataPointID id, const QColor& color = Qt::white, 
                   const QString& source = QString());
        // Add a data series to the plot
    
    void removeSeries(DataPointID id);
        // Remove a data series
    
    bool hasSeries(DataPointID id) const;
        // Check if a series is plotted

    // Time window
    void setTimeWindow(double seconds);
        // Set scroll range in seconds (1.0 to 60.0)
    
    double timeWindow() const;
        // Get current time window

    // Data updates
    void updateValue(DataPointID id, double value, 
                     const QDateTime& timestamp);
        // Update with a new data point value
    
    void clear();
        // Clear all data and reset axes

    // Styling
    void setTitle(const QString& title);
    void setYLabel(const QString& label);

signals:
    void viewChanged();
        // Emitted when user zooms/panes the plot

private slots:
    void onCleanupTimer();
        // Periodic cleanup of old data points (every 1 second)
};
```

### Usage Example

```cpp
RealtimePlot* plot = new RealtimePlot(parent);
plot->setTimeWindow(30.0);  // 30-second scroll window
plot->addSeries(DataPointID::RPM, Qt::green);
plot->addSeries(DataPointID::BoostPressure, Qt::yellow);

// Connect to data store updates
connect(&UnifiedDataStore::instance(), &UnifiedDataStore::dataPointUpdated,
        plot, [plot](DataPointID id, double value, 
                     const QString&, const QDateTime& ts) {
    if (plot->hasSeries(id)) {
        plot->updateValue(id, value, ts);
    }
});
```

---

## DashboardWidget

Tabbed dashboard UI with category-based tabs.

### Header
```cpp
#include "DashboardWidget.h"
```

### Class Definition

```cpp
class DashboardWidget : public QWidget {
    Q_OBJECT
public:
    explicit DashboardWidget(QWidget* parent = nullptr);

    // Tab management
    void addSeries(DataPointID id, const QColor& color, 
                   const QString& source);
        // Add a data series to the appropriate tab
    
    void removeSeries(DataPointID id);
        // Remove a data series

private:
    QTabWidget* m_tabs;
    // Internal state for tab management
};
```

### Tabs

| Tab | Category | Data Points |
|-----|----------|-------------|
| Engine | ENGINE/ECU | RPM, Boost, Lambda, EGTs, Battery, etc. |
| GPS_IMU | GPS/NAV + IMU | GPS Speed, Lat/Lon, IMU Pitch/Roll/Yaw |
| BMS | BMS | Pack V/A/SOC, Cell Voltages, Temps |
| VESC | VESC/DRIVETRAIN | Motor V/A/RPM, Wheel RPMs, Faults |
| AllData | All | Complete data point listing |

---

## MainWindow

Main application window with hardware connection management.

### Header
```cpp
#include "MainWindow.h"
```

### Class Definition

```cpp
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void connectMD_ECU(const QString& portName);
        // Connect MultiDisplay ECU to serial port
    
    void connectBMS(const QString& portName);
        // Connect rAtTrax-BMS to serial port
    
    void connectBridge(const QString& portName);
        // Connect ESP-NOW Bridge to serial port
    
    void openCsvFile(const QString& filePath);
        // Open MDLogger CSV file for import

private:
    // Hardware channels
    MdSerialComUnified* m_mdEcu = nullptr;
    RAtTraxBMS* m_bms = nullptr;
    OpenDashBridge* m_bridge = nullptr;
    MDLoggerCSV* m_csvParser = nullptr;
    
    // UI components
    DashboardWidget* m_dashboard = nullptr;
    RealtimePlot* m_plot = nullptr;
};
```

---

## Utilities

Helper functions for common operations.

### Header
```cpp
#include "Utilities.h"
```

### Functions

```cpp
// Endianness conversion
uint16_t swapEndian16(uint16_t value);
uint32_t swapEndian32(uint32_t value);

// Check byte order
bool isLittleEndian();
bool isBigEndian();

// CRC-16 calculation
uint16_t crc16(const QByteArray& data);
uint16_t crc16(const uint8_t* data, int length);

// XOR checksum
uint8_t xorChecksum(const QByteArray& data);

// Value clamping
double clamp(double value, double min, double max);

// Linear interpolation
double lerp(double a, double b, float t);

// Angle conversions
double degToRad(double degrees);
double radToDeg(double radians);
```

### Usage Example

```cpp
#include "Utilities.h"

// Swap bytes (big-endian to little-endian)
uint16_t raw = 0x1234;
uint16_t swapped = swapEndian16(raw);  // 0x3412

// CRC-16 of data
QByteArray frame = QByteArray::fromHex("025F41");
uint16_t crc = crc16(frame);

// Clamp value
double speed = clamp(150.0, 0.0, 100.0);  // Returns 100.0

// Convert degrees to radians
double rad = degToRad(180.0);  // Returns π
```

---

## TestHarness

Unit test framework for validating core functionality.

### Header
```cpp
#include "TestHarness.h"
```

### Class Definition

```cpp
class TestHarness : public QObject {
    Q_OBJECT
public:
    explicit TestHarness(QObject* parent = nullptr);

    bool runAllTests();
        // Run all test groups (43 assertions) and return pass/fail

private slots:
    void testSingleton();
    void testDataPointSetGet();
    void testDataPointValidity();
    void testDataPointTimestamps();
    void testRegistryValidation();
    void testRegistryMetadata();
    void testRegistryConversion();
    void testCategoryIndexing();
    void testEndianness();
    void testChecksums();
    void testClampLerp();
    void testAngleConversions();
    void testUnitConverter();
    void testMdDecoderMainFrame();     // golden 92-byte payload fixtures
    void testMdDecoderDiscardResync(); // bad-ETX discard + resync
    void testMdDecoderFuzz();          // 50k random bytes, no crash/wedge

private:
    void report(const QString& test, bool passed, const QString& detail = QString());
};
```

### Running Tests

```bash
./multidisplay-unified --unit-test
```

---

## MdFrameDecoder (pure protocol layer)

`include/MdFrameDecoder.h` — the MD-ECU wire protocol as a **pure state machine** (no QSerialPort),
so every framing rule is unit-testable:

```cpp
class MdFrameDecoder {
public:
    void reset();
    // One byte at a time; returns true exactly once per complete main frame
    bool feedByte(quint8 byte, QHash<DataPointID, double>& outValues);
    quint8 lastTag() const;
    int discardedCount() const;   // frames dropped (bad ETX at end position)
    int controlCount() const;     // framed-but-undecoded control frames
    static QHash<DataPointID, double> decodeMainFrame(const QByteArray& payload);
};
```

`MdSerialComUnified` is now a thin `QSerialPort` transport that pipes bytes into `feedByte()`.
Digifant NTC/voltage lookup curves (`DigifantMap`) are ported verbatim from the ancestor app.

---

## Signal/Slot Architecture

All components communicate via Qt signals and slots. The data flow is:

```
Parser (ICommunication subclass)
    │ frameParsed(values)
    ▼
UnifiedDataStore::setDataPoint()
    │ dataPointUpdated(id, value, source)
    ▼
┌───────────┬───────────┬───────────┐
│ Realtime  │ Dashboard │ Status    │
│  Plot     │  Widget   │  Bar      │
└───────────┴───────────┴───────────┘
```

### Connecting to Data Updates

```cpp
// Connect UI to data store updates
connect(&UnifiedDataStore::instance(), 
        &UnifiedDataStore::dataPointUpdated,
        this, [this](DataPointID id, double value, const QString& source) {
    // Update UI for this data point
    updateDisplay(id, value);
});
```

---

*Next: [Development Roadmap](07-roadmap.md)*
