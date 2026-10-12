# Development Roadmap

## Current Status: v0.2.0 — Feature Complete (in progress) ✅

**Version:** 0.2.0 — see [`../CHANGELOG.md`](../CHANGELOG.md) for the full history.

All four hardware parsers are implemented and the MD-ECU framing now matches the ancestor
firmment contract exactly. Boost/N75 configuration dialogs, live data table, unit toggling,
pause/review and markers shipped in v0.2.0. Remaining phases below; the master game plan is
[`../A-class.md`](../A-class.md).

---

## Completed Features (v0.1.0)

### ✅ Core Infrastructure
- [x] UnifiedDataStore thread-safe singleton with signal-based updates at ~30Hz
- [x] DataPointRegistry with 66+ registered data points across 7 categories
- [x] ICommunication abstract interface for all hardware sources
- [x] Thread-safe concurrent access via QReadWriteLock

### ✅ Hardware Parsers
- [x] MdSerialComUnified — MultiDisplay ECU binary frame parser (STX/TAG/ETX)
- [x] RAtTraxBMS — rAtTrax-BMS CSV and binary protocol handler
- [x] OpenDashBridge — ESP-NOW USB bridge (DP: line format) ✅ **Live verified**
- [x] MDLoggerCSV — CSV file import with playback controls

### ✅ UI Components
- [x] RealtimePlot — Qwt-based scrolling plot with stub mode for offline builds
- [x] DashboardWidget — Tabbed dashboard with 5 category tabs
- [x] MainWindow — Hardware connection management and status bar

### ✅ Utilities & Testing
- [x] Utilities.h/.cpp — Endianness, checksums, angle conversions
- [x] TestHarness — 13 test groups / 34 assertions (all passing)
- [x] DataPointID enum with helper functions (name, unit, category)

### ✅ Build System
- [x] CMakeLists.txt with Qt6 dependencies and optional Qwt support
- [x] Platform-specific settings for Linux/Windows/Android
- [x] build.sh convenience script
- [x] Android configuration (build.gradle, AndroidManifest.xml)

### ✅ Documentation
- [x] README.md — Project overview
- [x] DEVELOPMENT.md — Contributor guide
- [x] PROJECT_STATUS.md — Status report
- [x] data-ESPNOW-usbnode.md — ESP-NOW USB bridge spec
- [x] ESP32-Bridge-Implementation-Guide.md — Junior dev guide
- [x] wiki/ — Complete wiki documentation (7 pages)

---

## Phase 2: UI Polish & User Experience

**Priority:** High  
**Estimated effort:** 2–3 weeks  
**Dependencies:** none — builds on the v0.1.0 foundation (see wiki/08)

### 2.1 Real-Time Dashboard Widget

**Current state:** DashboardWidget shows static value displays when Qwt is unavailable. Values update but styling needs work.

**Tasks:**
- [ ] Improve value display formatting (units, precision, color coding)
- [ ] Add sparkline mini-charts for trend visualization
- [ ] Implement responsive layout that adapts to window size
- [ ] Add configurable widget sizing and positioning
- [ ] Support dark/light theme switching

**Acceptance criteria:**
- Dashboard looks polished and professional
- Values are easy to read at a glance
- Layout works on various screen sizes (desktop, tablet)

### 2.2 Layout Editor Dialog

**Current state:** No layout customization — fixed tab structure.

**Tasks:**
- [ ] Create LayoutEditorDialog widget
- [ ] Allow users to add/remove data point widgets per tab
- [ ] Support drag-and-drop reordering of widgets
- [ ] Save/load custom layouts as JSON files
- [ ] Provide preset layouts (Racing, Diagnostic, Telemetry)

**Acceptance criteria:**
- Users can create custom dashboard layouts
- Layouts persist between sessions
- Preset layouts provide good starting points

### 2.3 Boost Map Editor

**Current state:** No boost map editing capability.

**Tasks:**
- [ ] Create BoostMapEditor widget with 2D heat map display
- [ ] Support N75 duty cycle and boost setpoint maps
- [ ] Allow point-by-point or curve-based editing
- [ ] Import/export map data as JSON or CSV
- [ ] Overlay multiple maps for comparison

**Acceptance criteria:**
- Visual map editor is intuitive and responsive
- Maps can be saved, loaded, and compared
- Works with MultiDisplay ECU N75 data format

---

## Phase 3: Advanced Features

**Priority:** Medium  
**Estimated effort:** 3–4 weeks  
**Dependencies:** Phase 2 UI components

### 3.1 Over-the-Air (OTA) Firmware Updates

**Current state:** No OTA support.

**Tasks:**
- [ ] Implement OTA client in OpenDashBridge
- [ ] Add OTA message handling (CFG:, OTA: prefixes)
- [ ] Create firmware update dialog with progress indicator
- [ ] Support rollback on failed updates
- [ ] Verify firmware integrity before flashing

**Acceptance criteria:**
- Users can push firmware updates from the dashboard
- Updates are verified and rollback-safe
- Progress is visible and informative

### 3.2 Data Logging & Export

**Current state:** MDLoggerCSV supports file import only.

**Tasks:**
- [ ] Implement real-time data logging to CSV/JSON files
- [ ] Configurable log interval (10ms to 1s)
- [ ] Auto-rotation based on file size or time
- [ ] Export selected data points as CSV
- [ ] Playback controls for logged data

**Acceptance criteria:**
- Logging doesn't impact real-time performance
- Log files are well-formatted and easy to analyze
- Playback matches original timing

### 3.3 Multi-Source Support

**Current state:** Only one hardware source can be active at a time.

**Tasks:**
- [ ] Enable simultaneous connection to multiple hardware sources
- [ ] Implement data point conflict resolution (last writer wins, priority-based)
- [ ] Show source indicator on each data point display
- [ ] Allow per-source configuration

**Acceptance criteria:**
- Can connect MD-ECU + BMS + OpenDash simultaneously
- Data from all sources merges correctly in UnifiedDataStore
- UI shows which source provided each value

---

## Phase 4: Platform Expansion

**Priority:** Low-Medium  
**Estimated effort:** 2–3 weeks per platform  
**Dependencies:** Phase 3 multi-source support

### 4.1 macOS Support

**Current state:** Not tested — pending debug hardware acquisition.

**Tasks:**
- [ ] Set up macOS development environment
- [ ] Fix any macOS-specific Qt issues
- [ ] Test with all four hardware sources
- [ ] Create macOS installer (DMG)
- [ ] Code signing and notarization

### 4.2 Android Improvements

**Current state:** Configuration exists but not fully tested.

**Tasks:**
- [ ] Complete Android build pipeline
- [ ] Test Bluetooth SPP connection for BMS
- [ ] Optimize UI for touch screens
- [ ] Create Android-specific layouts
- [ ] Google Play Store deployment preparation

---

## Phase 5: Community & Ecosystem

**Priority:** Ongoing  
**Estimated effort:** Continuous

### 5.1 OpenDash Integration

**Tasks:**
- [ ] Publish DP ID mapping to OpenDash project
- [ ] Create OpenDash device profile for MultiDisplay Unified
- [ ] Contribute ESP-NOW bridge firmware to OpenDash ecosystem
- [ ] Document integration steps for OpenDash users

### 5.2 Sample Data & Testing

**Tasks:**
- [ ] Create sample data files for each hardware type
- [ ] Build automated test suite with simulated hardware
- [ ] Record demo videos showing each feature
- [ ] Create CI/CD pipeline for automated testing

### 5.3 Community Building

**Tasks:**
- [ ] Set up issue templates and contribution guidelines
- [ ] Create Discord/Matrix channel for community chat
- [ ] Document coding standards and review process
- [ ] Plan regular community sync meetings

---

## Known Issues & Technical Debt

### Critical (Must Fix)

| Issue | Impact | Priority |
|-------|--------|----------|
| Only one hardware source active at a time | Limits multi-system use | High |
| Dashboard widget needs UI polish | Looks unfinished | Medium |
| No layout persistence | Users lose customizations | Medium |

### Important (Should Fix)

| Issue | Impact | Priority |
|-------|--------|----------|
| Qwt dependency blocks full plotting | Development friction | Low |
| No data logging capability | Can't record sessions | Medium |
| Android not tested on real device | Deployment uncertainty | Medium |

### Nice to Have

| Issue | Impact | Priority |
|-------|--------|----------|
| macOS support pending | Platform gap | Low |
| No automated CI/CD | Manual testing only | Low |
| Limited documentation for advanced users | Learning curve | Low |

---

## Release Planning

### v0.2.0 — UI Polish (Target: July 2026)
- Dashboard widget improvements
- Layout editor basic functionality
- Theme support (dark/light)
- Better error messages and status indicators

### v0.3.0 — Advanced Features (Target: August 2026)
- Boost map editor
- Data logging/export
- Multi-source support
- OTA firmware update framework

### v0.4.0 — Platform Expansion (Target: September 2026)
- macOS support
- Android testing and fixes
- Installer packages for all platforms

### v1.0.0 — Stable Release (Target: October 2026)
- All Phase 2–3 features complete
- Comprehensive testing across platforms
- Full documentation
- Community release announcement

---

## Contributing

We welcome contributions from the community! Here's how to get involved:

### For Developers

1. **Pick an issue** from the GitHub issues list
2. **Fork the repository** and create a feature branch
3. **Write code** following the coding standards in DEVELOPMENT.md
4. **Add tests** for new functionality
5. **Submit a pull request** with a clear description

### For Hardware Testers

1. **Connect your hardware** using the command line options
2. **Report data quality** issues (missing points, wrong values, etc.)
3. **Share your setup** (hardware list, firmware versions, connections)
4. **Help others** in the community by answering questions

### For Documentation

1. **Improve wiki pages** with clearer explanations or examples
2. **Add screenshots** of working setups
3. **Translate documentation** into other languages
4. **Create video tutorials** for getting started

---

## Getting Started Contributing

### Development Setup

```bash
# 1. Clone the repository
cd /home/sysadmin/Documents/multidisplay-app/multidisplay-unified

# 2. Create build directory
mkdir -p build && cd build

# 3. Configure (without Qwt for faster builds)
cmake .. -DENABLE_QWT=OFF

# 4. Build
make -j$(nproc)

# 5. Run tests
./multidisplay-unified --unit-test

# 6. Test with hardware
./multidisplay-unified --bridge /dev/ttyUSB3
```

### Coding Standards

- **C++17** standard
- **Qt6** conventions (signals/slots, QObject hierarchy)
- **4-space indentation** (no tabs)
- **Meaningful variable names** (no single-letter vars except loop counters)
- **Comment complex logic**, not obvious code
- **Write tests** for new features

### Commit Message Format

```
<type>: <short description>

Detailed explanation of changes, if needed.
Fixes #123  (if applicable)
```

**Types:** feat, fix, docs, style, refactor, test, chore

---

*Last updated: June 11, 2026*  
*Next release target: v0.2.0 — July 2026*
