# MultiDisplay Unified — Wiki

**Version: v0.2.0** (see [`../CHANGELOG.md`](../CHANGELOG.md)). User & developer documentation.

## Pages

| Page | Contents |
|------|----------|
| [01-architecture.md](01-architecture.md) | Store → parsers → UI architecture, signal flow |
| [02-data-points.md](02-data-points.md) | DataPointID space (tables generated from code) |
| [03-hardware-integrations.md](03-hardware-integrations.md) | All 4 sources, comparison matrix |
| [04-espnow-bridge.md](04-espnow-bridge.md) | ESP-NOW bridge (DP: lines @ 921600) |
| [05-building.md](05-building.md) | Build & run instructions |
| [06-api-reference.md](06-api-reference.md) | Class-by-class API reference |
| [07-roadmap.md](07-roadmap.md) | Roadmap (living) |
| [08-bridging-roadmap.md](08-bridging-roadmap.md) | Historical foundation-era gap analysis (superseded by 07 + A-class.md) |
| [09-md-ecu-binary-protocol.md](09-md-ecu-binary-protocol.md) | **MD-ECU wire truth** (canonical) |
| [10-rattrax-bms-integration.md](10-rattrax-bms-integration.md) | rAtTrax-BMS line protocol |

## Authoritative sources of truth

1. **Data points:** `include/UnifiedDataPoint.h` + `src/DataPointRegistry.cpp` — always.
2. **MD-ECU wire protocol:** wiki/09 (derived from ancestor `MdSerialComBinary.cpp`).
3. **ESP-NOW wire format:** `docs/ESPNOW-DATA-PAYLOAD.md` (firmware `DP:0x%04X:%.2f` @ 921600).
4. **Version:** `CMakeLists.txt` `project(... VERSION ...)` — the only place it is written.

## Status snapshot

- ✅ Thread-safe UnifiedDataStore (~30 Hz, QReadWriteLock)
- ✅ 4 parsers; MD-ECU framing corrected (TAG = frame length; ETX checked at position)
- ✅ RealtimePlot (Qwt 6.3-Qt6), clickable legend, markers, pause/review, Show-All
- ✅ LiveDataTable, UnitConverter, Settings persistence
- ✅ 13 test groups / 34 assertions — all passing (`--unit-test`)
- ⏳ Digifant-block & VDO/knock DP mapping; bench verification of boost semantics

## License

Sovereign Individual License v1.0 — see [`../LICENSE`](../LICENSE). Not open source; no AI/ML
text-and-data training use.
