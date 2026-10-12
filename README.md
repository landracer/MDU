# MultiDisplay Unified

**A Qt6 application that unifies 4 hardware systems into a single real-time dashboard.**

> **License notice:** this project is distributed under the **Sovereign Individual License v1.0**
> (see [`LICENSE`](LICENSE)). This is a *restricted private license*, not an open-source license.
> Personal, non-commercial, educational use only; no commercial use; no AI/ML training on this
> code. Rights not expressly granted are reserved.

## Hardware Systems Supported

| System | MCU | Data Source | Connection |
|--------|-----|-------------|------------|
| **MultiDisplay ECU** | ATmega2560 | Engine sensors, OBD2/Digifant K-line, N75 boost | USB Serial @ 115200 |
| **rAtTrax-BMS** | ESP32-DOIT | BQ76952 6S BMS, MPU6050 IMU, VESC CAN, Reyax GPS | USB CDC @ 115200 |
| **OpenDash Mesh** | ESP32-S3 fleet | Wireless dashboard nodes | ESP-NOW Bridge (openDstream) → USB UART @ **921600** |
| **MDLogger** | ATmega328P | CSV log files from SD card | File import (replay) |

## Architecture

All parsers implement `ICommunication` and emit `frameParsed(QHash<DataPointID,double>)`;
everything funnels into the thread-safe `UnifiedDataStore` singleton, which emits
`dataPointUpdated(id, value, source)` to the UI (LiveDataTable + tabbed `RealtimePlot`s) at ~30Hz.

```
Hardware ──▶ Parser (ICommunication) ──▶ UnifiedDataStore ──▶ LiveDataTable / RealtimePlot tabs
```

## Data Point ID Space

The **single source of truth for data points is the code**: [`include/UnifiedDataPoint.h`](include/UnifiedDataPoint.h)
(enum) + [`src/DataPointRegistry.cpp`](src/DataPointRegistry.cpp) (metadata: name, unit, category, scale, offset).
Category ranges (see also [wiki/02-data-points.md](wiki/02-data-points.md)):

| Range | Category | Notes |
|-------|----------|-------|
| 0x0100–0x01FF | Engine/ECU | Catalog-aligned wire IDs + local extensions (0x0130+: Gear, EFR, N75, MAP, inj PW) |
| 0x0200–0x02FF | GPS/NAV | Speed, lat/lon, altitude, sats, lap timing |
| 0x0300–0x03FF | IMU | G-forces, rates, DMP attitude |
| 0x0400–0x04FF | BMS | Pack V/A/SOC, 16 cells (0x0410+), status/thermistors |
| 0x0500–0x05FF | SYSTEM | CPU temp, heap, RSSI, uptime, SD free |
| 0x0600–0x06FF | VESC/Drivetrain | Motor V/A/RPM, wheel RPMs, faults |
| 0x0700–0x07FF | RELAY/MOS | Relay/MOSFET channel states |
| 0x1410–0x141F | BMS (ESP-NOW) | Remapped ESP-NOW cells — the bridge remaps wire `0x041X` → `0x141X` so USB-BMS and ESP-NOW data never collide |

## Building

### Prerequisites

- **Qt6** (6.2+) with modules: Core, Gui, SerialPort, Widgets
- **CMake** 3.16+ and a **C++17** compiler
- **Qwt 6.2+ built against Qt6** (optional but recommended — real plots instead of the value-display stub; on Fedora: `qwt-qt6-devel`, on Debian/Ubuntu: `libqwt-qt6-dev`)

### Linux / macOS

```bash
cd multidisplay-unified
cmake -S . -B build            # options: -DENABLE_QWT=ON|OFF (default ON)
cmake --build build -j$(nproc)
./build/multidisplay-unified --help
```

> Note (Qt ≥ 6.8): systemd-distro Qt silently reroutes `qDebug` output to journald when
> stderr is not a TTY. `main()` installs a console message handler so test/parser output is
> always visible in your terminal.

### Android (stubs — see A-class.md roadmap)

`android/` contains placeholder `build.gradle` / `AndroidManifest.xml`; Android packaging is not
functional yet and is tracked as a roadmap item.

## Usage

```bash
./multidisplay-unified --md-ecu /dev/ttyUSB0      # MultiDisplay ECU binary stream
./multidisplay-unified --bms /dev/ttyUSB1         # rAtTrax-BMS USB CDC
./multidisplay-unified --bridge /dev/ttyUSB2      # openDstream ESP-NOW bridge (921600)
./multidisplay-unified --csv MD0001.CSV           # MDLogger CSV replay
./multidisplay-unified --md-ecu /dev/ttyUSB0 --bms /dev/ttyUSB1 --bridge /dev/ttyUSB2 --csv x.CSV  # all at once
./multidisplay-unified --test                     # simulated data @30Hz (no hardware needed)
./multidisplay-unified --unit-test                # run the 34-assertion test suite and exit
```

| Option | Description |
|--------|-------------|
| `--md-ecu, -m <port>` | MultiDisplay ECU serial port (`/dev/ttyUSB0` or `COM3`) |
| `--bms, -b <port>` | rAtTrax-BMS serial port |
| `--bridge, -g <port>` | ESP-NOW bridge (openDstream) serial port @ 921600 |
| `--csv, -c <file>` | MDLogger CSV file to replay (timestamp-paced, speed multiplier, pause/resume) |
| `--test` | Simulated data for all systems at 30Hz |
| `--unit-test` | Run unit tests and exit |
| `--help, -h` / `--version` | Standard options (version from CMake `project VERSION`) |

All sources can be connected **simultaneously** — each parser writes to the same store.

## Project Structure

```
multidisplay-unified/
├── CMakeLists.txt              # Build configuration (version is defined here, once)
├── README.md                   # This file
├── LICENSE                     # Sovereign Individual License v1.0
├── CHANGELOG.md  CONTRIBUTING.md
├── A-class.md                  # Professional-grade improvement game plan (living plan)
├── build.sh                    # Convenience build script
├── include/  src/              # Headers / implementation (parser, store, UI, tests)
├── resources/                  # app-icon.png + vendored plotting references under resources/plotting/
├── android/                    # Android packaging stubs (non-functional yet)
├── docs/                       # DEVELOPMENT.md + ESP-NOW protocol specs + archive/ (superseded session notes)
└── wiki/                       # 11-page user/dev wiki (01-architecture … 10-rattrax-bms)
```

## Development Status

**Current: v0.2.0** — see [`CHANGELOG.md`](CHANGELOG.md). Foundation + boost/N75 configuration,
live table, unit toggling, pause/review and markers are in. Remaining gaps and the professional-
grade roadmap live in [`A-class.md`](A-class.md).

- ✅ Thread-safe `UnifiedDataStore` (~30Hz, `QReadWriteLock`)
- ✅ 4 hardware parsers (MD-ECU framing now matches the ancestor firmware contract)
- ✅ Qwt real-time plots, clickable legends, markers, review mode
- ✅ 13 unit-test groups / 34 assertions, all passing (`--unit-test`)
- ⏳ Digifant-block payload fields (EGT/VDO/knock semantics) mapping — bench verification pending
- 🔲 Session save/load/replay, export, Android, OTA — see A-class.md

## Contributing

See [`CONTRIBUTING.md`](CONTRIBUTING.md). In-house contributors only — this is **not** an
open-source public project.

## License

Sovereign Individual License v1.0 — see [`LICENSE`](LICENSE).

---

**Built by the MultiDisplay/rAtTrax in-house team.**
