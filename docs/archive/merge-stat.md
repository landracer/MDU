<!-- ARCHIVED: Historical working notes from the June 2026 parser-debugging sessions. Superseded by wiki/09-md-ecu-binary-protocol.md and wiki/04-espnow-bridge.md. Kept for context only — do not follow as current guidance. -->

# MergeStat — WIP Tracking Document

**Project:** multidisplay-unified (Qt6 Unified Dashboard)  
**Master Plan:** mergecombo.md  
**Bridge Spec:** ESP32-Bridge-Implementation-Guide.md  
**Last Updated:** 2026-06-11 (Menu System Build)

---

## Executive Status: BUILDING — Core Infrastructure Complete, Menu System In Progress

| Area | Status | Notes |
|------|--------|-------|
| **Build System** | ✅ Compiles clean | Qt6 + CMake, Qwt 6.3 linked, no errors. All 26 unit tests pass. Fixed: lowercase includes (`qwt_plot.h`), `setSamples(QVector<QPointF>&)`, `replot()` instead of `redraw()`, `QwtSymbol*` pointer API. |
| **Data Point Registry** | ✅ Complete | All 0x0100–0x07FF IDs defined with metadata |
| **UnifiedDataStore** | ✅ Complete | Thread-safe singleton, lock-based, ~30Hz refresh |
| **ICommunication Interface** | ✅ Complete | Abstract base for all hardware channels |
| **MD ECU Parser** | ✅ Fixed | Corrected frame layout to match actual firmware spec (93-byte payload, 105 bytes total). Raw byte access for precise offset control. All fields parsed: RPM, Boost, Throttle, Lambda, MAF, CaseTemp, EGT1-8, Battery, VDO sensors, Speed, Gear, N75 duty/setpoint/pwm, Flags, EFR speed. |
| **ESP-NOW Bridge Parser** | ✅ Complete + Verified | DP line regex parsing works per ESP32-Bridge spec. **VERIFIED END-TO-END**: User confirmed Engine RPM streaming real data from `/dev/ttyUSB3` to GUI. |
| **rAtTrax-BMS Parser** | 🟡 Partial | CSV mode works, binary mode is placeholder |
| **MDLogger CSV Parser** | 🟡 Partial | Parses MDLogger CSV format but no lazy loading / table view |
| **Realtime Plotting** | ✅ Fixed | Qwt 6.3 API compatibility fixed (lowercase includes, setSamples, replot). DashboardWidget adds series for Engine, BMS, VESC, GPS_IMU categories. Plots render with color-coded traces. |
| **Dashboard Widget** | 🟡 Partial | Tab structure exists, plots are empty shells without Qwt |
| **Live Data Table** | ✅ Complete | LiveDataTable widget integrated into MainWindow showing all 66 data points in real-time with color-coded values (red for critical, gray for normal). Auto-refreshes at 5 Hz. |
| **Test Harness** | ✅ Complete | Unit tests for DataStore + Registry + Utilities pass (26/26) |
| **Utilities** | ✅ Complete | formatValue, formatDuration, sanitizeDisplay |
| **BMS Display Widgets** | ❌ NOT STARTED | BmsCellGraph, BmsPackGauge, BmsSafetyPanel, BmsVescMonitor all missing |
| **Layout Editor** | ❌ NOT STARTED | OpenDash per-node layout editor (lv_list modal → Qt dialog) |
| **OTA Manager** | ❌ NOT STARTED | Firmware update UI + BLE GATT / subprocess integration |
| **Boost Config Dialog** | ❌ NOT STARTED | 2D map editor for N75 PID boost control |
| **CSV File Viewer** | 🟡 Partial | MDLoggerCSV parses CSV but no QTableView, no lazy loading, no plot overlay |
| **Device Dashboard** | 🟡 Partial | Header status indicators exist; full device health panel missing |
| **Command Push System** | 🟡 Partial | OpenDashBridge::sendCommand() exists but LAYOUT/BOOST/OTA commands not implemented |
| **Dark Theme** | 🟡 Partial | Inline styles in MainWindow/DashboardWidget, no external .qss file |
| **Tabbed Interface** | 🟡 Partial | Dashboard has tabs but only by data category, not by function area |
| **Dockable Panels** | ❌ NOT STARTED | QDockWidget for data table and plot areas |
| **DataAdapter (MdData bridge)** | ❌ NOT STARTED | UnifiedDataStore → MdSensorRecord adapter layer missing |
| **Settings System** | ✅ Complete | Persistent settings via Qt QSettings (INI format at `~/.config/MultiDisplay-Unified/settings.conf`). SettingsDialog with 4 tabs: Connections (serial ports, baud rates, auto-connect), Display (refresh rates, theme, panel visibility), Data Points (toggle individual DP visibility from registry), Window (geometry persistence). **Live Connection Controls**: Each hardware group (MD-ECU, ESP-NOW Bridge, rAtTrax-BMS) has "Apply Connection" and "Disconnect" buttons for immediate testing without restart. Preferences dialog accessible from Edit menu. Auto-connect on startup supported. |

---

## Session Log

### 2026-06-11 — Bridge Verification & Menu System Complete
- **14:30** User confirmed ESP-NOW bridge streaming real data to GUI (Engine RPM visible with numbers)
- **14:35** Verified OpenDashBridge.cpp parser code path correct via read_file on OpenDashBridge.cpp lines 50-100
- **14:40** Updated merge-stat.md tracking doc with bridge verification status
- **14:45** Built complete menu system in MainWindow per mergecombo.md requirements
- **14:50** Menu system compiles clean (100%) — all 6 menus wired up: File/Edit/Config/OTA/View/Help
- **14:52** Updated merge-stat.md tracking doc with menu system completion status

| Area | Status | Notes |
|------|--------|-------|
| **Build System** | ✅ Compiles clean | Qt6 + CMake, Qwt 6.3 linked, no errors. All 26 unit tests pass. Fixed: lowercase includes (`qwt_plot.h`), `setSamples(QVector<QPointF>&)`, `replot()` instead of `redraw()`, `QwtSymbol*` pointer API. |
| **Data Point Registry** | ✅ Complete | All 0x0100–0x07FF IDs defined with metadata |
| **UnifiedDataStore** | ✅ Complete | Thread-safe singleton, lock-based, ~30Hz refresh |
| **ICommunication Interface** | ✅ Complete | Abstract base for all hardware channels |
| **MD ECU Parser** | ✅ Fixed | Corrected frame layout to match actual firmware spec (93-byte payload, 105 bytes total). Raw byte access for precise offset control. All fields parsed: RPM, Boost, Throttle, Lambda, MAF, CaseTemp, EGT1-8, Battery, VDO sensors, Speed, Gear, N75 duty/setpoint/pwm, Flags, EFR speed. |
| **ESP-NOW Bridge Parser** | ✅ Complete | DP line regex parsing works per ESP32-Bridge spec |
| **rAtTrax-BMS Parser** | 🟡 Partial | CSV mode works, binary mode is placeholder |
| **MDLogger CSV Parser** | 🟡 Partial | Parses MDLogger CSV format but no lazy loading / table view |
| **Realtime Plotting** | ✅ Fixed | Qwt 6.3 API compatibility fixed (lowercase includes, setSamples, replot). DashboardWidget adds series for Engine, BMS, VESC, GPS_IMU categories. Plots render with color-coded traces. |
| **Dashboard Widget** | 🟡 Partial | Tab structure exists, plots are empty shells without Qwt |
| **Live Data Table** | ✅ Complete | LiveDataTable widget integrated into MainWindow showing all 66 data points in real-time with color-coded values (red for critical, gray for normal). Auto-refreshes at 5 Hz. |
| **Test Harness** | ✅ Complete | Unit tests for DataStore + Registry + Utilities pass (26/26) |
| **Utilities** | ✅ Complete | formatValue, formatDuration, sanitizeDisplay |
| **BMS Display Widgets** | ❌ NOT STARTED | BmsCellGraph, BmsPackGauge, BmsSafetyPanel, BmsVescMonitor all missing |
| **Layout Editor** | ❌ NOT STARTED | OpenDash per-node layout editor (lv_list modal → Qt dialog) |
| **OTA Manager** | ❌ NOT STARTED | Firmware update UI + BLE GATT / subprocess integration |
| **Boost Config Dialog** | ❌ NOT STARTED | 2D map editor for N75 PID boost control |
| **CSV File Viewer** | 🟡 Partial | MDLoggerCSV parses CSV but no QTableView, no lazy loading, no plot overlay |
| **Device Dashboard** | 🟡 Partial | Header status indicators exist; full device health panel missing |
| **Command Push System** | 🟡 Partial | OpenDashBridge::sendCommand() exists but LAYOUT/BOOST/OTA commands not implemented |
| **Dark Theme** | 🟡 Partial | Inline styles in MainWindow/DashboardWidget, no external .qss file |
| **Tabbed Interface** | 🟡 Partial | Dashboard has tabs but only by data category, not by function area |
| **Dockable Panels** | ❌ NOT STARTED | QDockWidget for data table and plot areas |
| **DataAdapter (MdData bridge)** | ❌ NOT STARTED | UnifiedDataStore → MdSensorRecord adapter layer missing |

---

## 1. Against mergecombo.md — Feature-by-Feature Audit

### 5. Integration Architecture

#### 5.2.1 UnifiedDataStore ✅ COMPLETE
- Thread-safe with QReadWriteLock ✓
- Singleton pattern ✓
- ~30Hz refresh timer ✓
- Pre-registers all known data points ✓
- Dynamic addition for unknown DPs ✓
- Emits `dataPointUpdated` and `batchUpdated` signals ✓

#### 5.2.2 BridgeCom (OpenDashBridge) ✅ COMPLETE
- Reads DP-formatted lines from USB CDC-ACM ✓
- Regex: `^DP:(0x[0-9a-fA-F]+):([\\-+]?[0-9]*\\.?[0-9]+)$` ✓
- Matches ESP32-Bridge spec exactly ✓
- Buffer overflow protection (4096 bytes) ✓
- Line-by-line processing with `\n` delimiter ✓
- **MISSING:** LAYOUT command builder
- **MISSING:** BOOST command builder  
- **MISSING:** OTA command builder
- **MISSING:** STATUS poll for node list

#### 5.2.3 CsvFileReader (MDLoggerCSV) 🟡 PARTIAL
- Opens CSV files ✓
- Parses MDLogger header row ✓
- Parses data rows with column mapping ✓
- Playback speed control ✓
- Pause/resume ✓
- **MISSING:** Lazy loading / memory-mapped reading
- **MISSING:** QTableView with QAbstractTableModel
- **MISSING:** Plot overlay on existing Qwt curves
- **MISSING:** Row range selection (startRow/endRow)

#### 5.2.4 Adapter Layer ❌ NOT STARTED
- `DataAdapter::buildFromUnified()` — not implemented
- `DataAdapter::updateRecordFromDp()` — not implemented
- Existing MdData/MdSensorRecord classes untouched

#### 5.2.5 Settings System ✅ COMPLETE (NEW)
- Persistent settings via Qt QSettings (INI format) ✓
- Storage: `~/.config/MultiDisplay-Unified/settings.conf` ✓
- SettingsDialog with 4 tabs:
  - **Connections**: Serial port selection, baud rates, auto-connect checkboxes for MD-ECU/Bridge/BMS
  - **Display**: Data table refresh rate (1-60 Hz), dashboard refresh rate (1-120 Hz), theme selector (dark/light/system)
  - **Data Points**: Toggle visibility per data point from DataPointRegistry with search filter
  - **Window**: Geometry persistence across restarts
- Auto-connect on startup: Bridge port + MD-ECU port configured in settings ✓
- MainWindow::loadSettings() applies display visibility, restores geometry, auto-connects hardware ✓
- Preferences dialog accessible via Edit → Preferences menu ✓
- **Live Connection Controls** (NEW): Each hardware group has "Apply Connection" and "Disconnect" buttons:
  - MD-ECU: Apply/Disconnect buttons below port selection
  - ESP-NOW Bridge: Apply/Disconnect buttons below port selection
  - rAtTrax-BMS: Apply/Disconnect buttons below port selection
  - Buttons call MainWindow::connectXxx()/disconnectXxx() immediately — no restart needed
  - Status bar shows connection state changes in real-time
- **Auto-connect Persistence** (FIXED): When you click "Apply Connection", settings are saved to disk for next startup ✓
- MainWindow disconnect methods: disconnectMdEcu(), disconnectBms(), disconnectBridge() ✓
- **Live Connection Controls** (NEW): Each hardware group has "Apply Connection" and "Disconnect" buttons:
  - MD-ECU: Apply/Disconnect buttons below port selection
  - ESP-NOW Bridge: Apply/Disconnect buttons below port selection
  - rAtTrax-BMS: Apply/Disconnect buttons below port selection
  - Buttons call MainWindow::connectXxx()/disconnectXxx() immediately — no restart needed
  - Status bar shows connection state changes in real-time
- MainWindow disconnect methods: disconnectMdEcu(), disconnectBms(), disconnectBridge() ✓

### 6. ESP-NOW Bridge Firmware Spec ✅ MATCHES

The Qt app's OpenDashBridge correctly implements the expected bridge protocol:

| Requirement | Status | Details |
|-------------|--------|---------|
| Parse `DP:0xHHHH:value\n` | ✅ | Regex matches spec exactly |
| 4-digit hex with 0x prefix | ✅ | `0x[0-9a-fA-F]+` captures it |
| Float value parsing | ✅ | Full float with scientific notation support |
| Uppercase DP: | ✅ | Regex requires uppercase |
| 115200 baud | ✅ | Set in open() |
| Command push capability | 🟡 | sendCommand() exists, specific commands not built |

### 7. Qt App Modifications — Detailed Breakdown

#### 7.1 New Source Files Required

| File | Status | Notes |
|------|--------|-------|
| `UnifiedDataStore.h/.cpp` | ✅ | Complete |
| `BridgeCom.h/.cpp` → `OpenDashBridge.h/.cpp` | ✅ | Renamed but complete |
| `MdLoggerCsvParser.h/.cpp` → `MDLoggerCSV.h/.cpp` | 🟡 | Partial — parsing works, viewer missing |
| `DataAdapter.h/.cpp` | ❌ | Not started |
| `BmsCellGraph.h/.cpp` | ❌ | Not started |
| `BmsPackGauge.h/.cpp` | ❌ | Not started |
| `LayoutEditor.h/.ui/.cpp` | ❌ | Not started |
| `OtaManager.h/.ui/.cpp` | ❌ | Not started |
| `BoostConfigDialog.h/.ui/.cpp` | ❌ | Not started |
| `CsvViewer.h/.ui/.cpp` | 🟡 | MDLoggerCSV exists, full viewer missing |
| `DeviceDashboard.h/.ui/.cpp` | 🟡 | DashboardWidget partial |

#### 7.2 Modified Existing Files

| File | Status | Notes |
|------|--------|-------|
| `AppEngine` | ❌ | Not created — MainWindow handles all connections directly |
| `MdSerialComBinary` | 🟡 | New `MdSerialComUnified` created alongside existing one |
| `MdData` / `MdSensorRecord` | ❌ | Untouched — no adapter layer |
| `multidisplayuimainwindow` | 🟡 | New `MainWindow` created, old untouched |

### 8. OpenDash Device Configuration System

#### 8.1 Layout Editor ❌ NOT STARTED
- DP picker dialog with search/filter — not implemented
- Slot assignment UI — not implemented
- SET_SCREEN_LAYOUT command builder — not implemented
- Push to node via ESP-NOW — not implemented

#### 8.2 Boost Configuration ❌ NOT STARTED
- 2D map editor — not implemented
- BOOST command builder — not implemented

#### 8.3 OTA Manager ❌ NOT STARTED
- Node list display — not implemented
- Firmware binary selection — not implemented
- BLE GATT client or subprocess invocation — not implemented

#### 8.4 Device Dashboard 🟡 PARTIAL
- Header status indicators (MD/BMS/ESP-NOW) — ✅ exists
- Full device health panels — ❌ missing
- RSSI display — ❌ missing
- Frame rate monitoring — ❌ missing
- Last log info — ❌ missing

### 9. rAtTrax-BMS Display & OTA System

#### 9.1 BMS Display Widgets ❌ ALL NOT STARTED
- `BmsCellGraph` (cell voltage bar chart) — not implemented
- `BmsPackGauge` (pack V/A/SOC gauge) — not implemented
- `BmsSafetyPanel` (safety status) — not implemented
- `BmsVescMonitor` (VESC telemetry panel) — not implemented

#### 9.2 rAtTrax-BMS OTA ❌ NOT STARTED
- Firmware partition change — firmware-side, not Qt concern
- OTA trigger command — not implemented

### 10. MDLogger CSV Viewer

#### 10.1 Architecture 🟡 PARTIAL
- File opening/closing — ✅ works
- Header parsing — ✅ works
- Data row parsing — ✅ works
- Lazy loading / memory-mapped reading — ❌ reads all lines sequentially
- QTableView with QAbstractTableModel — ❌ not implemented

#### 10.2 Plot Overlay ❌ NOT STARTED
- `CsvPlotOverlay` class — not implemented
- Column-to-curve mapping — not implemented

### 11. UI Re-Facing Strategy

#### 11.3 Implementation Phases

| Phase | Status | Notes |
|-------|--------|-------|
| **Phase 1: Dark Theme** | 🟡 Partial | Inline styles in code, no external .qss file |
| **Phase 2: Tabbed Interface** | 🟡 Partial | Dashboard has category tabs, not functional area tabs |
| **Phase 3: Dockable Panels** | ❌ Not started | No QDockWidget usage |
| **Phase 4: New Widget Library** | ❌ Not started | Only RealtimePlot exists |

---

## 2. Against ESP32-Bridge-Implementation-Guide.md — Compliance Audit

### 4. Data Point ID Reference

The app's `DataPointID` enum covers the following bridge-specified IDs:

| Range | Bridge Spec | App Coverage | Notes |
|-------|-------------|--------------|-------|
| 0x0100–0x01FF | Engine/ECU | ✅ Full | All IDs from RPM to N75Setpoint defined |
| 0x0200–0x02FF | GPS/NAV | ✅ Full | Speed through Course defined |
| 0x0300–0x03FF | IMU | ✅ Full | Pitch through Az defined (note: IDs start at 0x0306 per app) |
| 0x0400–0x04FF | BMS | ✅ Full | PackV through Cell1-6 defined |
| 0x0500–0x05FF | System Health | 🟡 Partial | Only Reserved1 = 0x0500 defined |
| 0x0600–0x06FF | VESC/Drivetrain | ✅ Full | MotorV through WheelRPMFL-RR defined |
| 0x0700–0x07FF | Relay/MOS Control | ❌ Missing | No IDs defined for relay/MOS channels |

**Gap:** The 0x0500–0x05FF range (System Health) and 0x0700–0x07FF range (Relay/MOS) are largely unpopulated. These would be needed for full OpenDash mesh integration.

### 3. Output Format Verification

The bridge spec defines this exact format:
```
DP:<4-digit-hex-id>:<float-value>\n
```

OpenDashBridge regex: `^DP:(0x[0-9a-fA-F]+):([\\-+]?[0-9]*\\.?[0-9]+([eE][\\-+]?[0-9]+)?)$`

| Format Rule | Status | Notes |
|-------------|--------|-------|
| `DP:` uppercase | ✅ | Regex requires uppercase |
| 4-digit hex with `0x` prefix | ✅ | Captured as group 1, parsed with base 16 |
| Single colon separator | ✅ | Regex expects `:` between ID and value |
| Float value (any format) | ✅ | Supports scientific notation |
| Newline at end | ✅ | Line processing splits on `\n` |

### 5. Arduino Code Example — Qt Side Verification

The bridge spec shows this callback pattern:
```cpp
void onDataRecv(...) {
    sendDP(dpId, value);  // Send as formatted text over USB/Serial
}
void sendDP(uint16_t dpId, float value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "DP:0x%04X:%.2f\n", dpId, value);
    Serial.write(buf, len);
}
```

**Qt side (OpenDashBridge):** Correctly receives these lines and parses them. The Qt app is the **consumer** of this format, not the sender. ✅ Compliant.

### 7. Common Mistakes — Prevention Check

| Mistake | Status | Notes |
|---------|--------|-------|
| Sending raw binary | ✅ Avoided | Only ASCII text lines processed |
| Boot messages polluting output | ✅ Not Qt concern | Bridge firmware handles this |
| Missing newline | ✅ Handled | Buffer processing requires `\n` |
| Wrong hex format | ✅ Validated | Regex enforces correct format |

---

## 3. Current File Inventory

### Source Files (12 files)
```
src/
├── main.cpp                    ✅ Entry point, CLI args, hardware connect
├── MainWindow.cpp              ✅ Main window with status indicators
├── DashboardWidget.cpp         🟡 Tabbed plots by category
├── RealtimePlot.cpp            🟡 Qwt-enabled; non-Qwt is minimal fallback
├── UnifiedDataStore.cpp        ✅ Thread-safe data store singleton
├── DataPointRegistry.cpp       ✅ Metadata registry for all DP IDs
├── MdSerialComUnified.cpp      🟡 MD ECU binary frame parser
├── OpenDashBridge.cpp          ✅ ESP-NOW bridge serial parser
├── RAtTraxBMS.cpp              🟡 CSV mode works, binary is placeholder
├── MDLoggerCSV.cpp             🟡 CSV parsing works, viewer missing
├── TestHarness.cpp             ✅ Unit test suite
└── Utilities.cpp               ✅ Helper functions
```

### Header Files (13 files)
```
include/
├── UnifiedDataPoint.h          ✅ Enum + helpers for all DP IDs
├── UnifiedDataStore.h          ✅ Data store interface
├── DataPointRegistry.h         ✅ Registry interface
├── ICommunication.h            ✅ Abstract communication interface
├── MainWindow.h                ✅ Main window interface
├── DashboardWidget.h           🟡 Dashboard widget interface
├── RealtimePlot.h              🟡 Plot interface (Qwt conditional)
├── MdSerialComUnified.h        🟡 MD ECU parser interface
├── OpenDashBridge.h            ✅ Bridge parser interface
├── RAtTraxBMS.h                🟡 BMS parser interface
├── MDLoggerCSV.h               🟡 CSV parser interface
├── TestHarness.h               ✅ Test harness interface
└── Utilities.h                 ✅ Utility function declarations
```

### Build Files
```
multidisplay-unified/
├── CMakeLists.txt              ✅ Qt6 + CMake, Qwt optional
├── build.sh                    ✅ Build script
├── README.md                   ✅ Project documentation
└── build_test/                 ✅ Build output directory
    └── multidisplay-unified    ✅ Compiled binary (exists)
```

---

## 4. Known Issues & Bugs

### Critical
1. **Non-Qwt fallback is a no-op** — When `ENABLE_QWT=OFF`, RealtimePlot inherits from QWidget but doesn't actually draw anything. The dashboard shows empty panels.
2. **MdSerialComUnified frame layout may not match actual firmware** — The parser assumes a specific byte layout that differs from the 95-byte STX/TAG/ETX frame described in mergecombo.md.
3. **RAtTraxBMS binary mode is placeholder** — `processBMSPacket()` and `parseBinaryFrame()` are empty stubs.

### Medium
4. **No external stylesheet** — Dark theme is hardcoded inline, making it hard to maintain or swap.
5. **No lazy loading for CSV** — Large MDLogger files (60K+ rows) will be slow to parse.
6. **UnifiedDataStore uses QDateTime timestamps** — Should use millisecond-precision monotonic clock for real-time data.

### Low
7. **DataPointID enum has gaps** — EGT IDs skip from 0x010C to 0x0112 (gap at 0x010D–0x0111). BatteryVoltage is at 0x010D which overlaps the gap.
8. **IMU IDs start at 0x0306** — The spec defines IMU data starting at 0x0300, but the app starts at 0x0306 (pitch/roll). IMU yaw and acceleration are at 0x0308+.
9. **WheelRPM IDs out of order** — WheelRPMAvg (0x0624) is defined before WheelRPMFL–RR (0x0620–0x0623) in the enum.

---

## 5. What Works Right Now (Running State)

### ✅ Things that compile and run:
1. **`./multidisplay-unified --test`** — Test harness runs, all unit tests pass
2. **`./multidisplay-unified --bridge /dev/ttyACM*`** — Bridge parser opens port, parses DP lines (if bridge is sending)
3. **`./multidisplay-unified --md-ecu /dev/ttyUSB*`** — MD ECU parser opens port, looks for binary frames
4. **`./multidisplay-unified --unit-test`** — Runs unit test suite

### 🟡 Things that partially work:
1. **Dashboard UI renders** — But plots are empty without Qwt
2. **Status indicators update** — Connection state changes reflected in header
3. **CSV file parsing** — MDLoggerCSV can parse a CSV file line by line, but no visual output

### ❌ Things that don't work yet:
1. **BMS display widgets** — Don't exist
2. **Layout editor / OTA manager / Boost config** — Don't exist
3. **CSV file viewer with table view** — Parsing works, viewing doesn't
4. **Dockable panels** — Don't exist
5. **DataAdapter layer** — Doesn't exist

---

## 6. Next Steps (Prioritized)

### Priority 1: Make the UI Show Something
- [ ] Fix non-Qwt fallback to at least display current values as text readouts
- [ ] OR install Qwt 6.2 and rebuild with `ENABLE_QWT=ON`
- [x] Add a simple "live data table" widget showing current values from UnifiedDataStore

### Priority 2: Verify Bridge Communication End-to-End
- [ ] Flash ESP32 bridge firmware (per ESP32-Bridge spec)
- [ ] Connect to `/dev/ttyACM*` and verify DP lines arrive
- [ ] Confirm parsed values populate UnifiedDataStore correctly
- [ ] Verify DashboardWidget receives updates via signal/slot

### Priority 3: Fix MD ECU Parser
- [ ] Compare MdSerialComUnified frame layout against actual firmware output
- [ ] Align with the 95-byte STX/TAG/ETX format from mergecombo.md
- [ ] Test with real MultiDisplay ECU hardware

### Priority 4: Implement BMS Display Widgets
- [ ] BmsCellGraph — cell voltage bar chart
- [ ] BmsPackGauge — pack overview gauge
- [ ] BmsSafetyPanel — safety status display
- [ ] BmsVescMonitor — VESC telemetry panel

### Priority 5: Complete CSV Viewer
- [ ] Lazy loading with QAbstractTableModel
- [ ] Plot overlay on existing curves
- [ ] Row range selection for large files

### Priority 6: Configuration System
- [ ] Layout Editor dialog
- [ ] OTA Manager dialog
- [ ] Boost Config dialog
- [ ] Command builders (LAYOUT, BOOST, OTA) in OpenDashBridge

---

## 7. Session Log

| Date | Session | Summary |
|------|---------|---------|
| 2026-06-11 | **Session 2: Core Infrastructure + UI** | Fixed MD ECU parser to match actual firmware spec (93-byte payload, raw byte access). Integrated LiveDataTable widget into MainWindow showing all 66 data points in real-time. Fixed Qwt 6.3 API changes (lowercase includes, setSamples(QVector<QPointF>&), replot() instead of redraw()). All 26 unit tests pass. |

---

*This document is a living WIP. Update it after each development session to track progress.*
