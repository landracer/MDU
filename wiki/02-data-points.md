# Data Point System

> **These tables are generated from and subordinate to the code** — `include/UnifiedDataPoint.h` +
> `src/DataPointRegistry.cpp`. If a table here disagrees with the enum, **the enum wins** — fix the table.

## Overview

MultiDisplay Unified uses a **unified data point ID space** modeled on OpenDash's DP catalog
(ranges 0x0100–0x07FF, plus the ESP-NOW-only remap range 0x1410–0x141F). This is the single
source of truth for all sensor data across all four hardware systems. Every parser maps its raw
data into this namespace, and every UI component reads from it.

## Data Point Categories

| Range | Category | Description |
|-------|----------|-------------|
| 0x0100–0x011E | ENGINE/ECU | Engine sensors, EGT1–8, OBD2 (aligned to opendash catalog) |
| 0x0130–0x0135 | ENGINE/ECU (local) | MD-Unified extensions: Gear, EFR speed, N75, MAP, injector PW |
| 0x0200–0x020D | GPS/NAV | GPS position, speed, heading, altitude, sats, fix, lap timing |
| 0x0300–0x030B | IMU | G-forces, yaw/pitch/roll rates, MPU6050 DMP attitude, accel |
| 0x0400–0x042C | BMS | BQ76952: pack V/A/SOC, 16 cells, status registers, thermistors |
| 0x0500–0x0505 | SYSTEM | CPU temp, heap, WiFi RSSI, uptime, SD free, log session |
| 0x0600–0x0614 | VESC | VESC motor controller telemetry (aligned to opendash catalog) |
| 0x0620–0x0624 | DRIVETRAIN | Wheel RPMs FL/FR/RL/RR + average |
| 0x0700–0x0743 | RELAY/MOS | Relay and MOSFET channel states (handled by range) |
| 0x1410–0x141F | BMS (ESP-NOW) | Remapped ESP-NOW cell voltages ("BMS Cell N (ESP-NOW)") |

The 0x0100–0x07FF wire IDs are aligned to the **opendash catalog** (`common/src/opendash_dp_catalog.c`), which is the authoritative namespace for everything arriving over the ESP-NOW bridge.

## Complete Data Point Reference

### ENGINE/ECU (0x0100–0x011E, catalog-aligned)

| ID | Name | Unit |
|----|------|------|
| 0x0100 | Engine RPM | RPM |
| 0x0101 | Vehicle Speed | km/h |
| 0x0102 | Coolant Temperature | °C |
| 0x0103 | Intake Air Temperature | °C |
| 0x0104 | Engine Load | % |
| 0x0105 | Throttle Position | % |
| 0x0106 | Boost Pressure | kPa |
| 0x0107 | Oil Temperature | °C |
| 0x0108 | Oil Pressure | kPa |
| 0x0109 | Fuel Pressure | kPa |
| 0x010A | Air-Fuel Ratio | AFR |
| 0x010B | Lambda | λ |
| 0x010C | EGT (single probe) | °C |
| 0x010D | Battery Voltage | V |
| 0x010E | Timing Advance | degrees |
| 0x010F | Mass Air Flow | g/s |
| 0x0110 | Fuel Level | % |
| 0x0111 | Transmission Temperature | °C |
| 0x0112–0x0115 | EGT 1–4 | °C |
| 0x0116 | O2 Lambda (secondary) | λ |
| 0x0117 | MD RPM (MultiDisplay-sourced) | RPM |
| 0x0118–0x011B | EGT 5–8 | °C |
| 0x011C | OBD2 Flags | bitmask |
| 0x011D | MIL On | 0/1 |
| 0x011E | DTC Count | count |

### ENGINE/ECU local extensions (0x0130+, not on the ESP-NOW wire)

| ID | Name | Unit |
|----|------|------|
| 0x0130 | Gear Position | gear |
| 0x0131 | EFR Speed (pulse width) | RPM |
| 0x0132 | Injector Pulse Width | ms |
| 0x0133 | MAP Pressure | kPa |
| 0x0134 | N75 Boost Valve Duty Cycle | % |
| 0x0135 | N75 Boost Setpoint | kPa |

### GPS/NAVIGATION (0x0200–0x020D)

| ID | Name | Unit | Source |
|----|------|------|--------|
| 0x0200 | GPS Ground Speed | km/h | rAtTrax-BMS, OpenDash, MDLogger CSV |
| 0x0201 | GPS Heading | degrees | rAtTrax-BMS, OpenDash |
| 0x0202 | GPS Latitude | degrees | rAtTrax-BMS, OpenDash, MDLogger CSV |
| 0x0203 | GPS Longitude | degrees | rAtTrax-BMS, OpenDash, MDLogger CSV |
| 0x0204 | GPS Altitude | meters | rAtTrax-BMS, OpenDash |
| 0x0205 | GPS Satellite Count | sats | rAtTrax-BMS, OpenDash, MDLogger CSV |
| 0x0206 | GPS Fix Quality (catalog: HDOP) | quality | rAtTrax-BMS, MDLogger CSV |
| 0x0207 | GPS Course (catalog: lap number) | degrees | rAtTrax-BMS, MDLogger CSV |
| 0x0208 | Lap Time | ms | OpenDash |
| 0x0209 | Best Lap | ms | OpenDash |
| 0x020A | Lap Delta | ms | OpenDash |
| 0x020B | Sector Time | ms | OpenDash |
| 0x020C | Predictive Lap | ms | OpenDash |
| 0x020D | GPS Fix (0–3) | fix | OpenDash |

### IMU (0x0300–0x030B)

| ID | Name | Unit |
|----|------|------|
| 0x0300 | G-Force Lateral | g |
| 0x0301 | G-Force Longitudinal | g |
| 0x0302 | G-Force Vertical | g |
| 0x0303 | Yaw Rate | °/s |
| 0x0304 | Pitch Rate | °/s |
| 0x0305 | Roll Rate | °/s |
| 0x0306 | IMU Pitch Angle (DMP) | degrees |
| 0x0307 | IMU Roll Angle (DMP) | degrees |
| 0x0308 | IMU Yaw Angle | degrees |
| 0x0309–0x030B | IMU Acceleration X/Y/Z | m/s² |

### BATTERY/BMS (0x0400–0x042C)

| ID | Name | Unit |
|----|------|------|
| 0x0400 | BMS Pack Voltage | V |
| 0x0401 | BMS Pack Current (signed) | A |
| 0x0402 | BMS State of Charge | % |
| 0x0403 | BMS Min Cell Voltage | V |
| 0x0404 | BMS Max Cell Voltage | V |
| 0x0405 | BMS Cell Delta | mV |
| 0x0406 | BMS Temperature (die) | °C |
| 0x0407 | BMS Pack Power | W |
| 0x0408 | BMS Stack Voltage | V |
| 0x0409 | BMS State of Health | % |
| 0x040A | BMS Temp IC | °C |
| 0x040B | BMS Balance Active | 0/1 |
| 0x040C | BMS Charging | 0/1 |
| 0x040D | BMS Energy Charged | Wh |
| 0x0410–0x041F | BMS Cell 1–16 Voltage | V |
| 0x0420 | BMS FET Status | bitmask |
| 0x0421–0x0423 | BMS Safety A/B/C | bitmask |
| 0x0424 | BMS Error Flags | bitmask |
| 0x0425 | BMS Balancing Status | bitmask |
| 0x042A–0x042C | BMS Thermistor 1–3 (local) | °C |
| 0x1410–0x141F | BMS Cell 1–16 (ESP-NOW remap) | V |

### SYSTEM (0x0500–0x0505)

| ID | Name | Unit |
|----|------|------|
| 0x0500 | System CPU Temperature | °C |
| 0x0501 | System Free Heap | KB |
| 0x0502 | WiFi RSSI | dBm |
| 0x0503 | System Uptime | s |
| 0x0504 | SD Card Free | MB |
| 0x0505 | Log Session | count |

### VESC/DRIVETRAIN (0x0600–0x0624, catalog-aligned)

| ID | Name | Unit |
|----|------|------|
| 0x0600 | VESC eRPM | eRPM |
| 0x0601 | VESC Motor Current (signed) | A |
| 0x0602 | VESC Duty Cycle | duty |
| 0x0603 | VESC Ah Used | Ah |
| 0x0604 | VESC Ah Charged | Ah |
| 0x0605 | VESC Wh Used | Wh |
| 0x0606 | VESC Wh Charged | Wh |
| 0x0607 | VESC MOSFET Temperature | °C |
| 0x0608 | VESC Motor Temperature | °C |
| 0x0609 | VESC Input Current | A |
| 0x060A | VESC PID Position | degrees |
| 0x060B | VESC Tachometer | count |
| 0x060C | VESC Input Voltage | V |
| 0x060D–0x060F | VESC ADC 1–3 | V |
| 0x0610 | VESC PPM | 0–1 |
| 0x0611 | VESC RPM (mechanical) | RPM |
| 0x0612 | VESC Power In | W |
| 0x0613 | VESC Power Motor | W |
| 0x0614 | VESC Fault Code | enum |
| 0x0620–0x0623 | Wheel RPM FL/FR/RL/RR | RPM |
| 0x0624 | Wheel RPM Average Speed | mph |

### RELAY/MOS (0x0700–0x0743)

Relay and MOSFET channel states are handled by range in the helper functions — there are no individual enum members; labels are generated (e.g., "Relay Ch 3").

## Data Point ID Enum Definition

The enum is defined in [`include/UnifiedDataPoint.h`](../include/UnifiedDataPoint.h):

```cpp
enum class DataPointID : uint32_t {
    // ENGINE/ECU (0x0100–0x011E, catalog-aligned)
    RPM = 0x0100,
    VehicleSpeed = 0x0101,
    CoolantTemp = 0x0102,
    BoostPressure = 0x0106,
    EGT1 = 0x0112, /* … EGT4 = 0x0115 */
    EGT5 = 0x0118, /* … EGT8 = 0x011B */
    // MD-Unified local extensions (0x0130+)
    Gear = 0x0130,
    EFRSpeed = 0x0131,
    N75DutyCycle = 0x0134,
    
    // GPS/NAV (0x0200–0x020D)
    GPSSpeed = 0x0200,
    GPSLatitude = 0x0202,
    GPSFix = 0x020D,
    
    // IMU (0x0300–0x030B)
    GForceLat = 0x0300,
    IMUPitch = 0x0306,
    
    // BMS (0x0400–0x042C)
    BMSPackVoltage = 0x0400,
    BMSSOC = 0x0402,
    BMSCell1 = 0x0410, /* … BMSCell16 = 0x041F */
    BMSFETStatus = 0x0420, /* status registers … 0x0425 */
    
    // SYSTEM (0x0500–0x0505)
    SysCPUTemp = 0x0500,
    
    // VESC/DRIVETRAIN (0x0600–0x0624, catalog-aligned)
    VESCERPM = 0x0600,
    VESCInputVoltage = 0x060C,
    VESCRPM = 0x0611,
    VESCFaultCode = 0x0614,
    WheelRPMFL = 0x0620,
    
    Invalid = 0xFFFFFFFF
};
```

## Helper Functions

Three helper functions map DataPointID to human-readable metadata:

```cpp
// Get category string
const char* DataPointCategory(DataPointID id);
// Returns: "ENGINE/ECU", "GPS/NAV", "IMU", "BMS", "SYSTEM", "VESC/DRIVETRAIN", "RELAY/MOS"

// Get human-readable name (never "Unknown" — unmatched IDs get "DP 0xNNNN")
QString DataPointName(DataPointID id);
// Returns: "Engine RPM", "Boost Pressure", "BMS Cell 3 (ESP-NOW)", "Relay Ch 2", etc.

// Get unit string
QString DataPointUnit(DataPointID id);
// Returns: "RPM", "kPa", "km/h", "°C", "V", "A", "%"
```

## Unit Conversion

The DataPointRegistry handles conversion between raw hardware values and SI units:

```cpp
// Raw → SI (applies scale factor and offset)
double toSI(DataPointID id, int16_t raw) const;
double toSI(DataPointID id, uint16_t raw) const;
double toSI(DataPointID id, int32_t raw) const;

// SI → Raw (for sending commands back to hardware)
int16_t fromSI(DataPointID id, double si) const;
uint16_t fromSI_signed(DataPointID id, double si) const;
```

**Example conversions:**

| Data Point | Raw Value | Scale | Offset | SI Result |
|------------|-----------|-------|--------|-----------|
| CoolantTemp | 9500 | ×0.01 | +0 | 95.00°C |
| BatteryVoltage | 1260 | ×0.01 | +0 | 12.60V |
| BoostPressure | 14550 | ×0.01 | +0 | 145.50kPa |
| BMSPackVoltage | 252 | ×0.1 | +0 | 25.20V |
| IMUPitch | 52 | ×0.1 | +0 | 5.20° |

## Display Unit Toggling

On top of the raw→SI conversion above, `UnitConverter` (singleton) lets you toggle the *display* unit of any convertible value — click the unit cell in the Live Data table, or click a plot legend entry and choose **Switch Unit**. Data is always stored in canonical SI units; conversion happens only at display time. Selections persist via QSettings (`units/*` keys).

Available families:

| Canonical | Alternatives |
|-----------|--------------|
| °C | °F, K |
| km/h | mph, m/s |
| kPa | PSI, bar |
| λ | AFR (×14.7) |
| meters | ft |
| g | m/s² |
| ms | s |
| mV | V |

## Adding New Data Points

To add a new data point:

1. **Define the ID** in [`include/UnifiedDataPoint.h`](../include/UnifiedDataPoint.h):
   ```cpp
   MyNewSensor = 0x0136,  // Pick an unused ID in the appropriate range (local extensions: 0x0130+)
   ```

2. **Register metadata** in [`src/DataPointRegistry.cpp`](../src/DataPointRegistry.cpp):
   ```cpp
   m_registry[DataPointID::MyNewSensor] = {
       DataPointID::MyNewSensor, "My New Sensor", "units", "CATEGORY",
       0.01, 0.0, true  // scale, offset, signed
   };
   ```

3. **Implement parsing** in the appropriate communication handler to emit the new ID.

4. **Add to UI** by ensuring the DashboardWidget includes it in the appropriate tab.

## Data Store API

The UnifiedDataStore provides the runtime interface for reading/writing data points:

```cpp
// Get singleton instance
auto& store = UnifiedDataStore::instance();

// Write a data point (called by parsers)
store.setDataPoint(DataPointID::RPM, 3200.0, "ESP-NOW-Bridge");

// Read a data point (called by UI)
bool valid = false;
double rpm = store.getDataPoint(DataPointID::RPM, &valid);

// Check if a point has been updated
if (store.isValid(DataPointID::RPM)) {
    // RPM has been received at least once
}

// Get timestamp of last update
QDateTime ts = store.getTimestamp(DataPointID::RPM);
```

## Thread Safety

The UnifiedDataStore uses `QReadWriteLock` to protect concurrent access:
- **Multiple readers** can read data points simultaneously without blocking
- **Writer** (parser threads) gets exclusive access when updating
- **Signal-based updates** notify UI components asynchronously at ~30Hz

This design allows all four hardware parsers to write concurrently while the UI reads safely.

---

*Next: [Hardware Integrations](03-hardware-integrations.md)*
