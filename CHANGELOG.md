# Changelog — MultiDisplay Unified

All notable changes. Dates are project milestones, not commit dates.

## v0.2.0 — Boost Config & N75 PID Control

**Added**
- `BoostConfigDialog` — N75 boost-control config (normal/race duty, persistent via `Settings`)
- `GearSettingsDialog` + `N75PidSettingsWidget` / `N75TableWidget` — gearbox and N75 PID configuration UI
- Dedicated **BoostPID** dashboard tab (Boost, N75 duty, N75 setpoint, throttle)
- `LiveDataTable` — real-time text table of every data point
- `UnitConverter` — click-to-toggle display units (°C/°F/K, kPa/PSI/bar, km/h/mph, λ/AFR, m/ft, g/m/s², mV/V), persisted via QSettings
- Pause/Review mode — freeze plots, wheel-zoom, middle-drag pan; recording continues in background
- Click-point markers with per-series interpolated readout; right-click removes last; "Show All" session view
- Per-stream auto colors (20-color palette); legend click renames/recolors a series (persisted)

**Fixed (Oct 2026 maintenance pass)**
- MD-ECU binary framing restored to the ancestor app's contract: TAG byte = total frame
  length; ETX validated at position (length − 1). The unified parser previously terminated a
  frame at the *first* 0x03 byte (corrupting every payload containing 0x03) and parsed fields
  at wrong offsets, reading past the 92-byte payload
- MD-ECU tag constants corrected to firmware values: main 95, boost-PID 69, N75 duty 22,
  N75 setpoint 38, N75 params 23, gear 17, ACK 4
- `runAllTests()` now actually runs `testSingleton()` (13/13 test groups wired; 34 assertions)
- Version string single-sourced: CMake `project(... VERSION)` → `APP_VERSION` → `main.cpp`
- Qt log output now always routed to the console (Fedora Qt 6.8+ silently redirects qDebug
  to journald when stderr is not a TTY — test output and parser diagnostics were invisible)

## v0.1.0 — Foundation (June 2026)

- Project structure, `UnifiedDataPoint` DP enum + helpers, thread-safe `UnifiedDataStore` singleton
- `DataPointRegistry` metadata with raw↔SI scaling; `ICommunication` abstract interface
- Four parsers: MD-ECU binary, rAtTrax-BMS CSV, OpenDash ESP-NOW bridge (DP: lines), MDLogger CSV
- `RealtimePlot` (Qwt, stub mode without Qwt), tabbed `DashboardWidget`, `TestHarness` unit tests
- CMake build with optional Qwt; Android config stubs
