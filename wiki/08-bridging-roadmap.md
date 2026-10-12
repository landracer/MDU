# Bridging the Gap: MultiDisplay → MultiDisplay Unified

**Date:** June 11, 2026  
**Version:** v0.1.0 Foundation → Feature-Complete Rewrite

---

## Executive Summary

**Yes, this is a ground-up rewrite.** The new `multidisplay-unified` app shares **zero UI features** with the original `multidisplay-app`. What exists today is the data ingestion infrastructure — four hardware parsers, a unified data store, and basic value displays. The original app had rich interactive dashboards, evaluation windows, spectrograms, map editors, analog gauges, and mobile-specific UIs.

This document provides:
1. Honest assessment of what's missing
2. Feature comparison table (old vs new)
3. Architecture mapping (how old components map to new ones)
4. Phased migration plan with specific tasks and effort estimates

---

## 1. Current State Assessment

### What We Have (Foundation Layer)

| Component | Status | Description |
|-----------|--------|-------------|
| **Data Ingestion** | ✅ Complete | 4 hardware parsers (MultiDisplay ECU, rAtTrax-BMS, OpenDash Mesh, MDLogger CSV) |
| **Data Store** | ✅ Complete | Thread-safe singleton with signals at ~30Hz |
| **Data Point Registry** | ✅ Complete | 66+ registered data points across 7 categories |
| **Basic UI** | ⚠️ Minimal | Tabbed value displays, simple scrolling plot |
| **Evaluation Tools** | ❌ Missing | No spectrograms, no boost/lambda evaluation windows |
| **Interactive Widgets** | ❌ Missing | No analog gauges, no bar graphs, no real-time visualizations |
| **Map Editors** | ❌ Missing | No boost map editor, no N75 curve editor |
| **Data Replay** | ❌ Missing | No save/open/replay CSV functionality |
| **Mobile UI** | ❌ Missing | No Android/Maemo specific interfaces |
| **Configuration Dialogs** | ❌ Missing | No N75 setup, gear settings, serial options dialogs |

### What the Original App Had (~60 source files)

The original `multidisplay-app` is a feature-rich application with:

- **Desktop UI:** `DigifantApplicationWindow`, `MultidisplayUIMainWindow` — full desktop dashboard
- **Mobile UI:** 30+ mobile-specific files for Android/Maemo (touch-optimized gauges, swipe gestures)
- **Evaluation Windows:** 3 evaluation modes (Boost/Lambda plot, RPM/Boost plot, Boost/Lambda spectrogram)
- **Real-Time Widgets:** Analog gauges (`GLGauge`), bar graphs (`BarGraphWidget`), measurement widgets
- **Plotting System:** Qwt-based plots with zoom/pan (`MdPlot`, `MdPlotZoomer`, `MdPlotPicker`)
- **Map Editors:** 16x1 map editor (`Map16x1`), boost transfer functions, N75 duty maps
- **Data Management:** Save/open data files, CSV export, data replay with speed control
- **Configuration:** N75 PID settings, gear settings, serial options, visualization config
- **Communication Layer:** 8+ communication backends (USB/serial, Bluetooth Classic, BLE, QextSerial)
- **Thread System:** Replay worker threads, job runner framework

---

## 2. Feature Comparison Table

### 2.1 Core Features

| Feature | Old App (`multidisplay-app`) | New App (`multidisplay-unified`) | Gap |
|---------|------------------------------|----------------------------------|-----|
| MultiDisplay ECU support | ✅ Binary protocol | ✅ Binary parser (rewritten) | **CLOSED** — foundation exists |
| rAtTrax-BMS support | ❌ Not supported | ✅ CSV + binary parser | **N/A** — new feature |
| OpenDash Mesh support | ❌ Not supported | ✅ ESP-NOW USB bridge | **N/A** — new feature |
| MDLogger CSV import | ❌ Not supported | ✅ CSV file import | **N/A** — new feature |
| Data point unification | ❌ Separate data structures | ✅ Unified 66+ point ID space | **N/A** — new architecture |

### 2.2 UI/Visualization Features

| Feature | Old App | New App | Gap Size | Priority |
|---------|---------|---------|----------|----------|
| **Analog Gauges** (`GLGauge`, `MeasurementWidget`) | ✅ Color-gradient circular gauges | ❌ Missing | **LARGE** | P0 |
| **Bar Graph Thermo Widgets** (`BarGraphWidget`, `QwtThermo`) | ✅ Vertical bar graphs with alarm thresholds | ❌ Missing | **LARGE** | P0 |
| **Real-Time Visualizations** (`RealTimeVis`) | ✅ Multi-panel real-time display | ❌ Missing | **LARGE** | P0 |
| **2D Plotting** (`MdPlot`, `MdPlotData`) | ✅ Qwt plots with zoom/pan/picker | ⚠️ Stub only (Qwt unavailable) | **MEDIUM** | P1 |
| **Evaluation Windows** (3 types: Boost/Lambda, RPM/Boost, Spectrogram) | ✅ Full evaluation suite | ❌ Missing | **LARGE** | P1 |
| **Spectrogram Plots** (`EvalSpectrogramPlot`, `MdSpectrogramData`) | ✅ Heat map visualization | ❌ Missing | **LARGE** | P1 |
| **Power Curve Plot** (`PowerPlot`) | ✅ Power vs RPM curve | ❌ Missing | **MEDIUM** | P2 |
| **Boost Map Editor** (`Map16x1`) | ✅ 16x16 map editor with heat map | ❌ Missing | **LARGE** | P1 |
| **Transfer Function Editor** (`TransferFunction`, `BoostPlot`) | ✅ Pressure sensor calibration curves | ❌ Missing | **MEDIUM** | P2 |

### 2.3 Data Management Features

| Feature | Old App | New App | Gap Size | Priority |
|---------|---------|---------|----------|----------|
| **Save Raw Data** (`saveData()`) | ✅ Binary data file save | ❌ Missing | **LARGE** | P1 |
| **Open Data Files** (`openData()`) | ✅ Load saved binary data | ❌ Missing | **LARGE** | P1 |
| **Data Replay** (`replayData()`, `ReplayWorker`) | ✅ Playback with speed control | ⚠️ CSV import only | **MEDIUM** | P1 |
| **CSV Export** (`saveDataAsCSV()`) | ✅ Export to CSV | ❌ Missing | **MEDIUM** | P1 |
| **Data Window Scrolling** (`DataViewSlider`, `changeDataWinMark()`) | ✅ Scroll through recorded data | ❌ Missing | **MEDIUM** | P2 |

### 2.4 Configuration & Settings

| Feature | Old App | New App | Gap Size | Priority |
|---------|---------|---------|----------|----------|
| **Serial Options Dialog** (`SerialOptionsDialog`) | ✅ Port, baud rate, protocol config | ❌ Missing | **MEDIUM** | P1 |
| **N75 Options Dialog** (`N75OptionsDialog`) | ✅ N75 valve configuration | ❌ Missing | **MEDIUM** | P2 |
| **N75 PID Settings Widget** (`N75PidSettingsWidget`) | ✅ PID tuning interface | ❌ Missing | **MEDIUM** | P2 |
| **Gear Settings Dialog** (`GearSettingsDialog`) | ✅ Gear ratio configuration | ❌ Missing | **SMALL** | P3 |
| **V2 Settings Dialog** (`V2SettingsDialog`) | ✅ VR6 specific settings | ❌ Missing | **SMALL** | P3 |
| **V2 N75 Setup Dialog** (`V2N75SetupDialog`) | ✅ VR6 N75 calibration | ❌ Missing | **SMALL** | P3 |
| **Visualization Config** (`VisConfigDialog`) | ✅ Customize real-time display | ❌ Missing | **MEDIUM** | P2 |
| **Data Table Config** (`DataTableConfigDialog`) | ✅ Configure data table columns | ❌ Missing | **SMALL** | P3 |

### 2.5 Mobile Features

| Feature | Old App | New App | Gap Size | Priority |
|---------|---------|---------|----------|----------|
| **Android MainWindow** (`AndroidMainWindow`) | ✅ Touch-optimized dashboard | ❌ Missing | **LARGE** | P2 |
| **Android Dashboard Dialog** (`AndroidDashboardDialog`) | ✅ Android-specific dashboard | ❌ Missing | **LARGE** | P2 |
| **Mobile Boost PID Window** (`MobileBoostPidWindow`) | ✅ Mobile N75 tuning | ❌ Missing | **MEDIUM** | P3 |
| **Mobile Visualization** (`MobileVis1MainWindow`) | ✅ Mobile real-time display | ❌ Missing | **LARGE** | P2 |
| **Mobile Command Window** (`MobileCommandWindow`) | ✅ Mobile command input | ❌ Missing | **SMALL** | P3 |
| **Mobile Evaluation Dialog** (`MobileEvaluationDialog`) | ✅ Mobile evaluation views | ❌ Missing | **MEDIUM** | P3 |
| **Android N75 Dialog** (`AndroidN75Dialog`) | ✅ Android N75 config | ❌ Missing | **SMALL** | P3 |
| **Swipe Gestures** (`SwipeGestureRecognizer`) | ✅ Touch gesture support | ❌ Missing | **MEDIUM** | P2 |
| **Maemo Support** | ✅ Maemo/MeeGo platform | ❌ Not planned | **N/A** | Deprecated |

### 2.6 Communication Layer

| Feature | Old App | New App | Gap Size | Priority |
|---------|---------|---------|----------|----------|
| **USB/Serial (Qt SerialPort)** | ✅ `MdQSerialPortCom` | ✅ `MdSerialComUnified` | **CLOSED** — rewritten |
| **USB/Serial (QextSerial)** | ✅ `MdQextSerialCom` | ❌ Not included | **SMALL** | P3 (legacy) |
| **Bluetooth Classic** | ✅ `MdBluetoothCom` | ❌ Missing | **MEDIUM** | P2 |
| **Bluetooth Low Energy** | ✅ `MdBluetoothLECom` | ❌ Missing | **MEDIUM** | P2 |
| **Bluetooth Wrapper** | ✅ `MdBluetoothWrapper` | ❌ Missing | **SMALL** | P2 |
| **ESP-NOW USB Bridge** | ❌ Not supported | ✅ `OpenDashBridge` | **N/A** — new feature |
| **Binary Protocol Parser** | ✅ `MdBinaryProtocol` | ✅ Built into `MdSerialComUnified` | **CLOSED** — refactored |

---

## 3. Architecture Mapping

### 3.1 Old → New Component Mapping

```
┌─────────────────────────────────────────────────────────────────┐
│                     OLD APP ARCHITECTURE                        │
│                                                                 │
│  AppEngine (central controller)                                 │
│  ├── MdData (data storage)                                      │
│  ├── MdBinaryProtocol (parser)                                  │
│  ├── MdAbstractCom + backends (serial/Bluetooth)                │
│  ├── EvaluationWindow x3 (evaluation UI)                        │
│  ├── RealTimeVis (real-time display)                            │
│  ├── MultidisplayUIMainWindow (desktop UI)                      │
│  ├── MobileMainWindow + Android variants (mobile UI)            │
│  └── ReplayWorker (data replay)                                 │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                    NEW APP ARCHITECTURE                         │
│                                                                 │
│  UnifiedDataStore (thread-safe singleton) ← ✅ NEW              │
│  ├── MdSerialComUnified (MultiDisplay parser) ← REWRITTEN      │
│  ├── RAtTraxBMS (BMS parser) ← NEW                             │
│  ├── OpenDashBridge (ESP-NOW bridge) ← NEW                     │
│  ├── MDLoggerCSV (CSV import) ← NEW                            │
│  ├── DashboardWidget (basic value display) ← STUB              │
│  ├── RealtimePlot (scrolling plot) ← STUB                      │
│  └── MainWindow (hardware management) ← BASIC                  │
└─────────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────────┐
│                   MIGRATION TARGET ARCHITECTURE                 │
│                                                                 │
│  UnifiedAppEngine (central controller) ← PORT FROM OLD          │
│  ├── UnifiedDataStore (thread-safe singleton) ← KEEP           │
│  │   ├── Signals for all 66+ data points                       │
│  │   └── Historical data buffer for replay                     │
│  ├── Hardware Parsers (keep existing) ← KEEP                   │
│  │   ├── MdSerialComUnified                                    │
│  │   ├── RAtTraxBMS                                            │
│  │   ├── OpenDashBridge                                        │
│  │   └── MDLoggerCSV                                           │
│  ├── Communication Backends ← PORT FROM OLD                    │
│  │   ├── SerialCom (Qt SerialPort)                             │
│  │   ├── BluetoothCom (Classic)                                │
│  │   └── BLECom                                                │
│  ├── Dashboard Widgets ← PORT/REWRITE FROM OLD                 │
│  │   ├── AnalogGauge (GLGauge)                                 │
│  │   ├── BarGraphWidget (QwtThermo)                            │
│  │   ├── MeasurementWidget                                     │
│  │   └── RealTimeVis (multi-panel)                             │
│  ├── Evaluation Suite ← PORT FROM OLD                          │
│  │   ├── BoostLambdaEvalWindow                                 │
│  │   ├── RPMBoostEvalWindow                                    │
│  │   └── SpectrogramWindow                                     │
│  ├── Plotting System ← PORT FROM OLD                           │
│  │   ├── MdPlot (Qwt plot with zoom/pan)                       │
│  │   ├── MdPlotData (plot data storage)                        │
│  │   ├── EvalPlot (evaluation plots)                           │
│  │   └── EvalSpectrogramPlot (heat maps)                       │
│  ├── Map Editors ← PORT FROM OLD                               │
│  │   ├── Map16x1 (16x16 map editor)                            │
│  │   ├── BoostMapEditor (boost curve editor)                   │
│  │   └── TransferFunction (pressure calibration)               │
│  ├── Data Management ← PORT FROM OLD                           │
│  │   ├── DataSaver (binary file save)                          │
│  │   ├── DataLoader (binary file load)                         │
│  │   ├── CSVExporter                                           │
│  │   └── ReplayPlayer (speed-controlled playback)              │
│  ├── Configuration Dialogs ← PORT FROM OLD                     │
│  │   ├── SerialOptionsDialog                                   │
│  │   ├── N75OptionsDialog                                      │
│  │   ├── GearSettingsDialog                                    │
│  │   └── VisConfigDialog                                       │
│  ├── Mobile UI ← PORT/REWRITE FROM OLD                         │
│  │   ├── AndroidDashboard                                      │
│  │   ├── MobileRealTimeVis                                     │
│  │   └── MobileEvaluation                                      │
│  └── Layout System ← NEW (combines old flexibility)            │
│      ├── Configurable widget positions                         │
│      ├── Preset layouts (Racing, Diagnostic, Telemetry)        │
│      └── Save/load custom layouts                              │
└─────────────────────────────────────────────────────────────────┘
```

### 3.2 Data Flow Comparison

**Old App Data Flow:**
```
Hardware → MdAbstractCom → MdBinaryProtocol → MdData → UI Widgets
```
- Tight coupling: UI widgets directly access `MdData` pointers
- Single-threaded data processing
- Manual data management (save/open/replay)

**New App Data Flow (Current):**
```
Hardware → ICommunication → Parser → UnifiedDataStore → DashboardWidget
```
- Clean abstraction: `ICommunication` interface
- Thread-safe singleton with signals
- No historical data storage yet

**Target Data Flow:**
```
Hardware → ICommunication → Parser → UnifiedDataStore → UI Widgets
                                              ↓
                                       Historical Buffer
                                              ↓
                                    ReplayPlayer → EvalWindows
```
- Keep clean abstraction from new app
- Add historical buffer to `UnifiedDataStore`
- Replay feeds into evaluation windows
- UI widgets connect to `UnifiedDataStore` signals

---

## 4. Phased Migration Plan

### Phase 1: Core UI Widgets (P0 — 3-4 weeks)

**Goal:** Replace basic value displays with rich interactive widgets from the old app.

#### Task 1.1: Analog Gauge Widget
- **Source:** `widgets/rtwidget.h` → `GLGauge`, `MeasurementWidget`
- **Effort:** 3-5 days
- **Dependencies:** Qwt installed (or create software fallback)
- **Tasks:**
  - Port `GLGauge` OpenGL-based circular gauge to Qt6
  - Port `MeasurementWidget` with color-gradient rendering
  - Create `BarGraphWidget` with `QwtThermo` vertical bar
  - Connect to `UnifiedDataStore` signals (RPM, Boost, Lambda, etc.)
  - Add alarm threshold visualization
- **Key Files:**
  - `include/AnalogGauge.h/.cpp`
  - `include/BarGraphWidget.h/.cpp`
  - `include/MeasurementWidget.h/.cpp`

#### Task 1.2: Real-Time Visualization Panel
- **Source:** `widgets/realtimevis.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Task 1.1 (gauge widgets)
- **Tasks:**
  - Port multi-panel real-time display from `RealTimeVis`
  - Arrange gauges in configurable layout
  - Add auto-sizing for different screen resolutions
  - Support multiple preset layouts (Racing, Diagnostic, Telemetry)
- **Key Files:**
  - `include/RealTimeVis.h/.cpp`

#### Task 1.3: Qwt Plotting System
- **Source:** `MdPlot.h/.cpp`, `MdPlotData.h/.cpp`, `MdPlotZoomer.h/.cpp`, `MdPlotPicker.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Qwt 6.2+ installed
- **Tasks:**
  - Port Qwt plot components with zoom/pan functionality
  - Create `UnifiedPlot` base class (replaces `RealtimePlot` stub)
  - Connect to `UnifiedDataStore` signals for real-time updates
  - Add cursor crosshair with data point readout
- **Key Files:**
  - `include/UnifiedPlot.h/.cpp`
  - `include/UnifiedPlotData.h/.cpp`
  - `include/UnifiedPlotZoomer.h/.cpp`

**Acceptance Criteria for Phase 1:**
- [ ] Dashboard shows analog gauges for RPM, Boost, Lambda, Battery
- [ ] Gauges update in real-time from all 4 hardware sources
- [ ] Qwt plots display scrolling data with zoom/pan
- [ ] Real-time visualization panel arranges widgets in layout

---

### Phase 2: Evaluation & Analysis (P1 — 3-4 weeks)

**Goal:** Port the evaluation window suite for post-run analysis.

#### Task 2.1: Boost/Lambda Evaluation Window
- **Source:** `evaluation/evaluationwindow.h/.cpp`, `evaluation/EvalPlot.h/.cpp`
- **Effort:** 3-5 days
- **Dependencies:** Phase 1 (Qwt plotting), Data save/load
- **Tasks:**
  - Port `EvaluationWindow` with Boost/Lambda scatter plot
  - Connect to historical data buffer in `UnifiedDataStore`
  - Add axis range configuration
  - Support loading saved data files
- **Key Files:**
  - `include/EvaluationWindow.h/.cpp`
  - `include/BoostLambdaPlot.h/.cpp`

#### Task 2.2: RPM/Boost Evaluation Window
- **Source:** `evaluation/evaluationwindow.h/.cpp` (RPMBoost type)
- **Effort:** 2-3 days
- **Dependencies:** Task 2.1
- **Tasks:**
  - Port RPM vs Boost 2D plot
  - Add color overlay for throttle position
  - Reuse `EvaluationWindow` base class

#### Task 2.3: Spectrogram Visualization
- **Source:** `evaluation/EvalSpectrogramPlot.h/.cpp`, `evaluation/MdSpectrogramData.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Phase 1 (Qwt), Data save/load
- **Tasks:**
  - Port spectrogram heat map plot
  - Implement `SpectrogramData` storage for 2D histograms
  - Add color map configuration (heat, ice, grayscale)
  - Support time-axis scrolling
- **Key Files:**
  - `include/SpectrogramPlot.h/.cpp`
  - `include/SpectrogramData.h/.cpp`

#### Task 2.4: Data Save/Load Infrastructure
- **Source:** `AppEngine.cpp` → `saveData()`, `openData()`, `saveDataAsCSV()`
- **Effort:** 5-7 days
- **Dependencies:** None
- **Tasks:**
  - Extend `UnifiedDataStore` with circular buffer (last N seconds)
  - Implement binary data file format (compatible with old app or new)
  - Implement CSV export with configurable columns
  - Add "Record" button to MainWindow status bar
  - Implement file open dialog for saved data
- **Key Files:**
  - `src/UnifiedDataStore.cpp` (extend with buffer)
  - `include/DataSaver.h/.cpp`
  - `include/DataLoader.h/.cpp`
  - `include/CSVExporter.h/.cpp`

**Acceptance Criteria for Phase 2:**
- [ ] Evaluation windows display saved data from all 3 types
- [ ] Spectrogram shows heat map of Boost/Lambda correlation
- [ ] User can record data during runtime
- [ ] User can load saved data files and replay analysis
- [ ] CSV export works with configurable columns

---

### Phase 3: Map Editors & Configuration (P1-P2 — 2-3 weeks)

**Goal:** Port the boost map editor and configuration dialogs.

#### Task 3.1: 16x16 Map Editor
- **Source:** `Map16x1.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Phase 1 (Qwt plotting)
- **Tasks:**
  - Port 16x16 grid editor with heat map visualization
  - Support N75 duty cycle maps and boost setpoint maps
  - Add point-by-point editing with keyboard navigation
  - Implement import/export (JSON/CSV)
  - Overlay comparison mode (load 2 maps, show delta)
- **Key Files:**
  - `include/MapEditor.h/.cpp`
  - `include/MapHeatMap.h/.cpp`

#### Task 3.2: Transfer Function Editor
- **Source:** `TransferFunction.h/.cpp`, `BoostPlot.h/.cpp`
- **Effort:** 3-5 days
- **Dependencies:** Phase 1 (Qwt plotting)
- **Tasks:**
  - Port pressure sensor transfer function calibration
  - Support multiple sensor types (400kPa, 250kPa, 200kPa, 100kPa)
  - Allow custom calibration curve definition
  - Visualize raw ADC vs calibrated pressure
- **Key Files:**
  - `include/TransferFunction.h/.cpp`
  - `include/PressureCalibration.h/.cpp`

#### Task 3.3: Configuration Dialogs
- **Source:** `N75OptionsDialog`, `GearSettingsDialog`, `SerialOptionsDialog`, `VisConfigDialog`
- **Effort:** 5-7 days
- **Dependencies:** None
- **Tasks:**
  - Port all configuration dialogs to Qt6
  - Integrate with `QSettings` for persistence
  - Add validation for parameter ranges
  - Create unified "Settings" menu in MainWindow
- **Key Files:**
  - `include/SerialOptionsDialog.h/.cpp`
  - `include/N75OptionsDialog.h/.cpp`
  - `include/GearSettingsDialog.h/.cpp`
  - `include/VisConfigDialog.h/.cpp`

**Acceptance Criteria for Phase 3:**
- [ ] Map editor displays 16x16 heat map with editable cells
- [ ] Transfer function editor calibrates pressure sensors
- [ ] All configuration dialogs save settings between sessions
- [ ] Settings menu provides access to all configuration options

---

### Phase 4: Communication Layer Expansion (P2 — 2 weeks)

**Goal:** Add Bluetooth and legacy serial backends.

#### Task 4.1: Bluetooth Classic Support
- **Source:** `com/MdBluetoothCom.h/.cpp`, `com/MdBluetoothWrapper.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Qt Bluetooth module
- **Tasks:**
  - Port Bluetooth communication backend
  - Implement device discovery and pairing
  - Create Bluetooth connection dialog
  - Test with MultiDisplay hardware over Bluetooth
- **Key Files:**
  - `include/BluetoothCom.h/.cpp`

#### Task 4.2: BLE Support
- **Source:** `com/MdBluetoothLECom.h/.cpp`
- **Effort:** 3-5 days
- **Dependencies:** Qt Bluetooth module (BLE)
- **Tasks:**
  - Port BLE communication backend
  - Implement GATT service discovery
  - Support Nordic nRF52-based hardware
- **Key Files:**
  - `include/BLECom.h/.cpp`

#### Task 4.3: Legacy QextSerial Support
- **Source:** `com/MdQextSerialCom.h/.cpp`
- **Effort:** 2-3 days
- **Dependencies:** qextserialport library
- **Tasks:**
  - Add optional QextSerialPort backend (for legacy hardware)
  - Make optional in CMakeLists.txt
  - Provide fallback to Qt SerialPort if unavailable
- **Key Files:**
  - `include/QextSerialCom.h/.cpp`

**Acceptance Criteria for Phase 4:**
- [ ] Bluetooth Classic connection works with MultiDisplay
- [ ] BLE connection works with nRF52-based hardware
- [ ] QextSerialPort available if library installed
- [ ] Connection dialog supports all backends

---

### Phase 5: Mobile UI (P2 — 3-4 weeks)

**Goal:** Port Android-specific UI for mobile deployment.

#### Task 5.1: Android Dashboard
- **Source:** `mobile/AndroidMainWindow.h/.cpp`, `mobile/AndroidDashboardDialog.h/.cpp`
- **Effort:** 7-10 days
- **Dependencies:** Phase 1 (gauge widgets)
- **Tasks:**
  - Port touch-optimized Android dashboard
  - Implement large gauge widgets for in-car viewing
  - Add swipe gesture navigation between pages
  - Optimize layout for tablet and phone screens
- **Key Files:**
  - `include/AndroidDashboard.h/.cpp`

#### Task 5.2: Mobile Real-Time Visualization
- **Source:** `mobile/MobileVis1MainWindow.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Phase 1 (RealTimeVis)
- **Tasks:**
  - Port mobile real-time display
  - Implement swipe between different gauge combinations
  - Add auto-rotate for portrait/landscape
- **Key Files:**
  - `include/MobileRealTimeVis.h/.cpp`

#### Task 5.3: Mobile Evaluation & Commands
- **Source:** `mobile/MobileEvaluationDialog.h/.cpp`, `mobile/MobileCommandWindow.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Phase 2 (evaluation windows)
- **Tasks:**
  - Port mobile evaluation views
  - Implement command input window for N75 control
  - Add touch-friendly buttons and sliders
- **Key Files:**
  - `include/MobileEvaluation.h/.cpp`
  - `include/MobileCommandWindow.h/.cpp`

**Acceptance Criteria for Phase 5:**
- [ ] Android dashboard displays gauges on phone and tablet
- [ ] Swipe gestures navigate between dashboard pages
- [ ] Mobile evaluation views show saved data
- [ ] Command window controls N75 valve position

---

### Phase 6: Data Replay & Advanced Features (P2-P3 — 2 weeks)

**Goal:** Implement data replay with speed control and advanced analysis.

#### Task 6.1: Replay Player
- **Source:** `thread/replayworker.h/.cpp`, `thread/jobrunnerthread.h/.cpp`
- **Effort:** 5-7 days
- **Dependencies:** Phase 2 (data save/load)
- **Tasks:**
  - Port replay worker with speed control (0.5x to 32x)
  - Implement time-scrubbing on loaded data
  - Add pause/play/step controls
  - Synchronize replay with evaluation windows
- **Key Files:**
  - `include/ReplayPlayer.h/.cpp`

#### Task 6.2: Power Curve Analysis
- **Source:** `PowerPlot.h/.cpp`
- **Effort:** 3-5 days
- **Dependencies:** Phase 2 (data save/load), Phase 1 (Qwt)
- **Tasks:**
  - Implement power vs RPM curve calculation
  - Calculate torque from RPM and boost
  - Display on Qwt plot with fit curve
- **Key Files:**
  - `include/PowerCurvePlot.h/.cpp`

#### Task 6.3: WOT Events Detection
- **Source:** `WotEventsDialog.h/.cpp`
- **Effort:** 3-5 days
- **Dependencies:** Phase 2 (data save/load)
- **Tasks:**
  - Detect Wide Open Throttle events in recorded data
  - Display WOT segments in data viewer
  - Extract peak values during WOT events
- **Key Files:**
  - `include/WOTEventsDialog.h/.cpp`

**Acceptance Criteria for Phase 6:**
- [ ] Replay player loads saved data and plays back at variable speed
- [ ] Time scrubbing jumps to any point in recorded data
- [ ] Power curve calculated and displayed from saved data
- [ ] WOT events detected and highlighted

---

## 5. Effort Summary

| Phase | Description | Effort | Priority | Blocks |
|-------|-------------|--------|----------|--------|
| **1** | Core UI Widgets (gauges, plots, real-time vis) | 3-4 weeks | P0 | All UI work |
| **2** | Evaluation & Analysis (windows, spectrograms, save/load) | 3-4 weeks | P1 | Analysis features |
| **3** | Map Editors & Configuration (map editor, dialogs) | 2-3 weeks | P1-P2 | Calibration tools |
| **4** | Communication Layer (Bluetooth, BLE, legacy serial) | 2 weeks | P2 | Mobile connectivity |
| **5** | Mobile UI (Android dashboard, touch gestures) | 3-4 weeks | P2 | Android deployment |
| **6** | Data Replay & Advanced Features (replay, power curve, WOT) | 2 weeks | P2-P3 | Post-run analysis |
| | **TOTAL** | **15-21 weeks** | | |

---

## 6. Quick Win Opportunities

While the full migration is underway, these features can be added quickly for immediate value:

### QW-1: Improve Dashboard Widget Styling (1-2 days)
- Add unit labels, color coding, alarm thresholds
- Make value displays larger and more readable
- **Impact:** App looks polished immediately

### QW-2: Add Sparkline Mini-Charts (2-3 days)
- Show trend history next to each value display
- Use simple QPainter (no Qwt required)
- **Impact:** Dashboard becomes more informative

### QW-3: Add Settings Menu with Basic Options (1-2 days)
- Serial port selection dropdown in MainWindow
- Save/load app settings with QSettings
- **Impact:** User can configure connections without code changes

### QW-4: Implement CSV Export (2-3 days)
- Export current data buffer to CSV file
- Configurable column selection
- **Impact:** Users can analyze data in Excel/spreadsheets

### QW-5: Add Data Point Selector (3-5 days)
- Allow users to choose which data points appear on dashboard
- Save custom widget layouts
- **Impact:** Dashboard becomes customizable

---

## 7. Build System Considerations

### Qwt Dependency Strategy

The old app requires Qwt 6.x for most UI features. The new app currently uses stub classes when Qwt is unavailable. Recommended approach:

**Option A: Make Qwt Optional (Current Approach)**
- ✅ Builds without Qwt (stub widgets)
- ⚠️ Limited functionality without Qwt (no gauges, no plots)
- ✅ Good for CI/testing environments

**Option B: Require Qwt for Full Features**
- ❌ Fails to build without Qwt installed
- ✅ All features work out of the box
- ✅ Simpler codebase (no stub classes)

**Option C: Hybrid Approach (Recommended)**
- Build with stubs by default (`-DENABLE_QWT=OFF`)
- Enable full widgets when Qwt found (`-DENABLE_QWT=ON`)
- Dashboard works always, gauges/plots require Qwt
- **Implementation:** Add `#ifdef ENABLE_QWT_PLOTTING` guards around gauge/plot code

```cmake
# CMakeLists.txt
find_package(Qwt 6.2 QUIET)
if(QWT_FOUND)
    set(ENABLE_QWT ON CACHE BOOL "Enable Qwt plotting library")
    message(STATUS "Qwt found — full UI features enabled")
else()
    set(ENABLE_QWT OFF CACHE BOOL "Enable Qwt plotting library")
    message(WARNING "Qwt not found — using stub widgets (limited UI)")
endif()
```

---

## 8. Migration Execution Strategy

### Recommended Approach: Parallel Development

**Do NOT delete the old app.** Maintain both codebases in parallel:

```
multidisplay-app/          (old app — read-only reference)
multidisplay-unified/      (new app — active development)
```

### Workflow:

1. **Reference, Don't Copy:** Port logic from old app, rewrite for new architecture
2. **Test in Parallel:** Verify old and new produce same results for same input
3. **Feature Flags:** Use `#ifdef ENABLE_QWT_PLOTTING` for optional features
4. **Incremental Deployment:** Each phase should be a working, testable unit

### Branch Strategy:

```
main                       (production-ready, always builds)
├── feature/core-ui        (Phase 1: gauges, plots)
│   └── feature/gauges     (Task 1.1: analog gauges)
│   └── feature/plots      (Task 1.3: Qwt plotting)
├── feature/evaluation     (Phase 2: eval windows, spectrograms)
│   └── feature/save-load  (Task 2.4: data save/load)
├── feature/map-editors    (Phase 3: map editor, transfer functions)
├── feature/bluetooth      (Phase 4: Bluetooth backends)
├── feature/mobile-ui      (Phase 5: Android dashboard)
└── feature/replay         (Phase 6: data replay, power curve)
```

---

## 9. Risk Assessment

| Risk | Likelihood | Impact | Mitigation |
|------|-----------|--------|------------|
| **Qwt not installed** | High | Medium | Stub widgets work always; Qwt enhances |
| **OpenGL compatibility** (for GLGauge) | Medium | Medium | Software fallback using QPainter |
| **Bluetooth module unavailable** (Qt6) | Low | Medium | Make Bluetooth optional in CMake |
| **Android build complexity** | Medium | High | Start with desktop-only, add Android later |
| **Scope creep** | High | High | Stick to phased plan; defer P3 features |
| **Data format incompatibility** | Medium | Medium | Define clear binary file format early |

---

## 10. Success Criteria

### v0.2.0 (Core UI) — ~4 weeks
- [ ] Dashboard shows analog gauges for key parameters
- [ ] Qwt plots display real-time scrolling data
- [ ] Real-time visualization panel arranges widgets
- [ ] App looks professional and usable

### v0.3.0 (Evaluation Suite) — ~8 weeks total
- [ ] 3 evaluation windows work with saved data
- [ ] Spectrogram heat map visualizes parameter correlations
- [ ] Data recording and CSV export functional
- [ ] Full post-run analysis capability

### v0.4.0 (Configuration & Maps) — ~10 weeks total
- [ ] Map editor supports N75/boost map editing
- [ ] Transfer function editor calibrates sensors
- [ ] All configuration dialogs persistent
- [ ] Complete calibration toolkit

### v0.5.0 (Full Feature Parity + New Features) — ~16 weeks total
- [ ] Bluetooth connectivity works
- [ ] Android dashboard functional
- [ ] Data replay with speed control
- [ ] Power curve and WOT analysis
- [ ] **Feature-complete rewrite** with new unified architecture

---

## 11. Conclusion

This is a ground-up rewrite, and that's a good thing. The new architecture (unified data store, abstract communication interface, signal-based updates) is cleaner and more maintainable than the old app's tight coupling.

**The foundation is solid.** What exists today — four hardware parsers, thread-safe data store, 66+ registered data points, passing unit tests — is the hard part. The UI layer (Phases 1-6) is well-understood because we have the old app as a reference.

**Key principle:** Each phase should deliver a working, testable increment. Don't wait for all phases to complete before testing — validate each phase with real hardware data before moving on.

**Timeline estimate:** 15-21 weeks for full feature parity plus new capabilities (rAtTrax-BMS, OpenDash Mesh support). Quick wins (QW-1 through QW-5) can be delivered in 1-2 weeks for immediate user value.
