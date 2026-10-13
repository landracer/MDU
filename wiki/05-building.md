# Building & Installation

## Prerequisites

### Required

- **Qt6** (6.2+) with modules: Core, Gui, SerialPort, Widgets
- **CMake** 3.16+
- **C++17** compiler (GCC 9+, Clang 10+, MSVC 2019+)
- **make** or **ninja** build tool

### Optional

- **Qwt** 6.2+ — For full-featured real-time plotting (optional, stub mode available)
- **Android NDK** — For Android builds
- **Qt for Android** — For Android deployment

---

## Linux / macOS Build

### Step 1: Clone the repository

```bash
cd /home/sysadmin/Documents/multidisplay-app
# Repository should already be cloned as multidisplay-unified/
```

### Step 2: Configure with CMake

```bash
cd multidisplay-unified
mkdir -p build && cd build

# Without Qwt (recommended for development)
cmake .. -DENABLE_QWT=OFF

# With Qwt (if you have it installed)
cmake .. -DENABLE_QWT=ON
```

### Step 3: Build

```bash
make -j$(nproc)
```

### Step 4: Run

```bash
./multidisplay-unified --help
```

### Using the build script

A convenience build script is provided:

```bash
cd /home/sysadmin/Documents/multidisplay-app/multidisplay-unified
./build.sh
```

---

## Windows Build (MSVC)

### Step 1: Install prerequisites

- Visual Studio 2019 or 2022 with C++ desktop development workload
- Qt6 for MSVC (download from qt.io or use vcpkg)
- CMake 3.16+

### Step 2: Configure and build

```cmd
cd multidisplay-unified
mkdir build && cd build

cmake .. -G "Visual Studio 17 2022" -DENABLE_QWT=ON
cmake --build . --config Release
```

### Using Qt Creator (GUI)

1. Open Qt Creator
2. File → Open File or Project → Select `CMakeLists.txt`
3. Configure kit (MSVC or MinGW)
4. Build → Build All
5. Run → Start Debugging / Start Without Debugging

---

## Android Build

### Prerequisites

- Qt for Android (qt-android package)
- Android NDK r25+
- Android SDK API 21+
- Java JDK 11+

### Configuration

The Android configuration is in the `android/` directory:

**build.gradle:**
```groovy
android {
    defaultConfig {
        minSdkVersion 21
        targetSdkVersion 33
        ndk {
            abiFilters "armeabi-v7a", "arm64-v8a", "x86", "x86_64"
        }
    }
}
```

**AndroidManifest.xml:**
- USB host permission for serial connections
- Bluetooth permissions for BMS connection
- QtActivity configuration

### Build

```bash
cd multidisplay-unified
mkdir -p build-android && cd build-android

cmake .. -DCMAKE_TOOLCHAIN_FILE=$QT_ANDROID_CMAKE_SETTINGS \
         -DENABLE_QWT=OFF

make -j$(nproc)
```

---

## Qwt Integration

### When to use Qwt

- **With Qwt**: Full real-time scrolling plots with auto-scaling, grid overlay, and color-coded traces
- **Without Qwt**: Simple value display widget showing latest readings per data point

Qwt is **optional** — the application builds and runs without it. This is useful for:
- Development environments where Qwt isn't installed
- Minimal deployments
- Testing the fallback UI

### Installing Qwt on Linux

```bash
# Ubuntu/Debian
sudo apt install libqwt-dev

# Fedora
sudo dnf install qwt-devel

# Arch Linux
sudo pacman -S qwt-qt6

# macOS (Homebrew)
brew install qwt
```

### Building with Qwt

```bash
cd multidisplay-unified/build
cmake .. -DENABLE_QWT=ON
make -j$(nproc)
```

---

## Running the Application

### Command Line Options

```
Usage: ./multidisplay-unified [options]

Options:
  -h, --help           Displays help on commandline options.
  --help-all           Displays help, including generic Qt options.
  -v, --version        Displays version information.
  -m, --md-ecu <port>  MultiDisplay ECU serial port (e.g., /dev/ttyUSB0)
  -b, --bms <port>     rAtTrax-BMS serial port (e.g., /dev/ttyACM0)
  -g, --bridge <port>  OpenDash ESP-NOW Bridge serial port (e.g., /dev/ttyUSB3)
  -c, --csv <file>     MDLogger CSV file to import (e.g., MD0001.CSV)
  --test               Run in test mode with simulated data
  --unit-test          Run unit tests only and exit
```

### Example: Connect to ESP-NOW Bridge

```bash
./multidisplay-unified --bridge /dev/ttyUSB3
```

Expected output:
```
========================================
MultiDisplay Unified v0.2.0
========================================
[Main] Connecting ESP-NOW Bridge to: "/dev/ttyUSB3"
[ESP-NOW-Bridge] Connected to "/dev/ttyUSB3"
[RealtimePlot] Added display: GPS Speed
[Dashboard] Added series: GPS Speed to tab: "GPS_IMU" from source: ""
========================================
Application started successfully
========================================
```

### Example: Connect to Multiple Sources

All four sources can be connected simultaneously — every option simply wires another
parser into the shared store:

```bash
./multidisplay-unified --md-ecu /dev/ttyUSB0 --bms /dev/ttyUSB1 --bridge /dev/ttyUSB2 --csv MD0001.CSV
```

### Example: Test Mode (Simulated Data)

```bash
./multidisplay-unified --test
```

### Example: Run Unit Tests

```bash
./multidisplay-unified --unit-test
```

Expected output:
```
========================================
MultiDisplay Unified — Test Suite
========================================
  ✓ "testDataPointSetGet" : "RPM: 3200 (expected 3200)"
  ...
  Results:  43 / 43  passed
  ALL TESTS PASSED
========================================
```

---

## Troubleshooting

### Build Errors

**Problem:** `QwtPlot: No such file or directory`

**Solution:** Qwt is not installed. Build without it:
```bash
cmake .. -DENABLE_QWT=OFF
```

**Problem:** `Qt6::SerialPort not found`

**Solution:** Install Qt6 SerialPort module:
```bash
# Ubuntu/Debian
sudo apt install qtbase6-dev qtserialport6-dev

# Check if Qt6 is in PATH
which qmake6
qmake6 --version
```

**Problem:** `C++17 required`

**Solution:** Update your compiler or CMake version:
```bash
gcc --version  # Should be 9+
cmake --version  # Should be 3.16+
```

### Runtime Errors

**Problem:** `Failed to create wl_display (No such file or directory)`

**Solution:** This is a Wayland display server issue on Linux. Either:
- Run with XWayland: `QT_QPA_PLATFORM=xcb ./multidisplay-unified`
- Or start a Wayland session

**Problem:** `Could not load the Qt platform plugin "wayland"`

**Solution:** Force X11 backend:
```bash
export QT_QPA_PLATFORM=xcb
./multidisplay-unified --bridge /dev/ttyUSB3
```

**Problem:** `Permission denied: /dev/ttyUSB3`

**Solution:** Add user to dialout group:
```bash
sudo usermod -aG dialout $USER
# Log out and back in for changes to take effect
```

### Hardware Connection Issues

**Problem:** No data from serial port

**Solution:**
1. Check port exists: `ls -la /dev/ttyUSB* /dev/ttyACM*`
2. Verify baud rate: `stty -F /dev/ttyUSB3 speed` (should be 115200)
3. Check for data: `cat /dev/ttyUSB3 | head -10`
4. Ensure device is powered and firmware is running

**Problem:** Binary garbage on serial port

**Solution:** Firmware is sending raw bytes instead of formatted text. See [ESP-NOW Bridge Guide](04-espnow-bridge.md) for correct output format.

---

## Development Workflow

### Quick Build & Test Cycle

```bash
# 1. Make code changes
# ... edit files ...

# 2. Rebuild (incremental — only changed files compile)
cd build
make -j$(nproc)

# 3. Run tests
./multidisplay-unified --unit-test

# 4. Run with hardware
./multidisplay-unified --bridge /dev/ttyUSB3
```

### Clean Build

```bash
cd build
rm -rf *
cmake .. -DENABLE_QWT=OFF
make -j$(nproc)
```

### Enable Debug Logging

Set environment variable before running:
```bash
export QT_LOGGING_RULES="*.debug=true"
./multidisplay-unified --bridge /dev/ttyUSB3
```

---

*Next: [API Reference](06-api-reference.md)*
