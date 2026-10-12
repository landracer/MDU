<!-- Canonical protocol reference. Ground truth: ancestor app ../multidisplay-app/src/MdSerialComBinary.cpp (convertReceivedMd2Frame + frame dispatcher). -->

# MD-ECU Binary Protocol (ATmega2560 → USB Serial)

**Transport:** USB serial, **115200 baud**, 8N1. Little-endian for the ATmega2560 (AVR) main-frame
fields; the Digifant add-on block keeps its native 68HC11 **big-endian** byte order (decoded from
the ancestor app's field order — its high byte position is preserved as raw counts, not rescaled).

## Framing — THE ONE RULE

**The TAG byte's decimal value IS the total frame length in bytes.** There are no length-prefix
and no length-suffix bytes: a frame with TAG `0x5F` (= 95 decimal) is exactly 95 bytes long.

```
┌───────┬─────────────┬──────────────────────┬───────┐
│ STX   │ TAG         │ PAYLOAD (TAG-3 bytes)│ ETX   │
│ 0x02  │ = frame len │  sensor readings     │ 0x03  │
└───────┴─────────────┴──────────────────────┴───────┘
```

Receiver state machine (`src/MdSerialComUnified.cpp`):

1. `FRAME_COMPLETE/FRAMEERROR` → wait for STX `0x02`
2. `WAITING_FOR_TAG` → accept only known tags (95, 69, 22, 38, 23, 17, 4); store byte, `frameLength = tag`, go `RECEIVING`
3. `RECEIVING` → buffer bytes. When `index == frameLength − 1`: the byte **must** be ETX `0x03`
   → dispatch by tag; otherwise count as discarded and reset.
4. A payload byte equal to `0x03` mid-frame is data, **not** an end marker — framing ends by
   *position*, never by first-ETX scan.

### Frame inventory (tag = total bytes)

| TAG | Dec | Payload | Frame | Purpose | Decode status |
|-----|-----|---------|-------|---------|---------------|
| `0x5F` | **95** | 92 B | main sensor frame | full engine telemetry | ✅ decoded → DPs |
| `0x45` | 69 | 66 B | boost PID frame | PID tuning data | framed, decode pending |
| `0x26` | 38 | 35 B | N75 setpoint map | 16×16 map + gear + mode + serial | framed, decode pending |
| `0x17` | 23 | 20 B | N75 params | PID params + serial | framed, decode pending |
| `0x16` | 22 | 19 B | N75 duty map | 16×16 duty map + serial | framed, decode pending |
| `0x11` | 17 | 14 B | gear ratio frame | serial + gear + 6 ratios | framed, decode pending |
| `0x04` | 4 | 1 B | ACK (frame# echo) | round-trip latency probe | logged only |

> **Correction (Oct 2026):** earlier unified docs claimed frames were ETX-terminated variable-length
> blobs with tags like `0x1A`(26)/`0x1E`(30)/`0x4F`(79 OBD-II). That was wrong on every point: the
> tag set is {95, 69, 22, 38, 23, 17, 4}, every tag equals its frame's byte count, and there is no
> OBD-II tagged frame.

## Main sensor frame payload (92 bytes)

Offsets are payload-relative (frame byte 2 = payload offset 0). AVR little-endian pairs unless noted.

| Payload offset | Field | Raw → physical | DataPointID |
|---|---|---|---|
| 0–3 | millis timestamp | not mapped | — |
| 4–5 | calRPM | raw int16 LE | `RPM` (0x0100) |
| 6–7 | calAbsoluteBoost | uint16 LE /100 (absolute; ancestor subtracted 1.0 as bar-gauge — **unified stores raw/100; gauge-vs-absolute bench check pending**) | `BoostPressure` (0x0106) |
| 8 | throttle | raw /255 → % | `ThrottlePosition` (0x0105) |
| 9–10 | calLambdaF | uint16 LE /100 | `Lambda` (0x010B) |
| 11–12 | calLMM (MAF) | uint16 LE /100 g/s | `MAF` (0x010F) |
| 13–14 | caseTemp | uint16 LE /100 °C | `CoolantTemp` (0x0102) |
| 15–16 … 29–30 | calEgt[0..7] | uint16 LE raw counts ×8 (successive 2-byte pairs) | `EGT1..EGT8` |
| 31–32 | batVolt | uint16 LE /100 → V | `BatteryVoltage` (0x010D) |
| 33–38 | vdoPres 1–3 | raw | not mapped |
| 39–44 | vdoTemp 1–3 | raw | not mapped |
| 45–46 | speedF | uint16 LE /100 → km/h | `VehicleSpeed` (0x0101) |
| 47 | gear | 0 = neutral, 1–6 | `Gear` (0x0130) |
| 48 | n75 duty | raw 0–255 | `N75DutyCycle` (0x0134) |
| 49–50 | n75 req boost | uint16 LE /100 | `N75Setpoint` (0x0135) |
| 51 | n75 req boost PWM | raw | not mapped |
| 52 | status flags | bitmask | not mapped |
| 53–54 | EFR speed | `raw == 0xFFFF → 0`, else `40,000,000/raw` → RPM | `EFRSpeed` (0x0131) |
| 55–56 | knock | raw | not mapped |
| 57–91 | **Digifant block** (see below) | mixed | not mapped (roadmap) |

## Digifant block (payload 57–91, 68HC11-derived)

Parsed in the ancestor app, intentionally unmapped in unified until its DPs are defined:

| Offset | Field | Decode |
|---|---|---|
| 57 | df_boost_raw | raw |
| 58 | df_lambda | raw |
| 59 | df_raw_knock | raw |
| 60 / 61 | df_ect / df_iat | via digifant ECT/IAT maps |
| 62 | df_co_poti | raw |
| 63 | df_flags | bitmask |
| 64 | df_ign | `(raw × −0.351563) + 73.9` deg |
| 65 | df_lc_flags | bitmask |
| 66–73 | cyl1–4 knock retard/decay | each ×0.351563; sum = total retard |
| 74 | df_voltage_raw | via voltage map |
| 75–76 | df_inj_time | **BE** uint16 ×2 (µs) |
| 77–85 | enrichment bytes | raw per-cause enrichment |
| 86 | df_isv | raw |
| 87–88 | df_rpm_delta_hall | raw; `30000000/raw` = computed RPM |
| 89–90 | df_freq (K-line) | LE uint16 |
| 91 | df_active_frame | `< 255` ⇒ Digifant connected |

## Gotchas

1. **Never** end a frame on a payload byte equal to `0x03` — position is the only end condition.
2. Frames can span `readyRead()` chunk boundaries; keep the state machine member state across calls.
3. EGT/knock/VDO raw scales are unverified — the ancestor logged them raw; keep them raw until bench calibration.
4. `0xFFFF` in the EFR field means "no sensor fitted" → emit 0, not garbage.
