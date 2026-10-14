# Bench Work Roadmap — closing out Pillar 1 by hand

The only Pillar 1 items left need physical hardware: a vehicle/bench ECU and a second
serial receiver. This page is the playbook so any test pilot can execute without
conversation-level context. Everything here ends as a golden fixture in `tests/fixtures/`
or a corrected constant in `MdFrameDecoder` — code never ships on speculation.

## Prerequisites

- USB↔serial adapter on the MD-ECU K-line/UART line (the same wiring the ancestor app used)
- `multidisplay-unified` built locally (`wiki/05-building.md`)
- A known-good reference gauge/multimeter for B1

## B1 — Verify boost semantics (absolute kPa vs gauge)

The ancestor firmware *subtracted 1.0* somewhere in the boost path; we must not guess.

1. Key-on, engine **off**. The MAP/boost sensor sees ambient pressure.
2. Launch: `./multidisplay-unified --md-ecu /dev/ttyUSB0`
3. Watch DP `BoostPressure` (see `wiki/09-md-ecu-binary-protocol.md`):
   - reads ≈ 100 kPa → the wire value is **absolute**; the decoder must NOT subtract
   - reads ≈ 0 → the wire value is **gauge**; keep the ancestor's offset semantics
4. Record it: File → **Record Session** → key-crank/first-start so you capture the whole
   ramp. Stop recording; File → **Export Session Log** → JSON. That JSON is the fixture.
5. If the verdict contradicts the current constant, fix the constant + update the golden
   fixture test in the same commit.

## B2 — Real logged captures as decoder fixtures

Synthetic 92-byte payloads already cover the decoder; real traffic closes the loop.

1. **Decoded-level capture (easy path):** File → Record Session while benching every
   frame type (main sensor frame, each control frame, knock lines...). Stop → Export →
   JSON. Commit the JSON under `tests/fixtures/bench/` and wire exact-value assertions.
2. **Raw wire capture (full fidelity):** the MD bus is a broadcast UART — a second
   receiver spliced to the same line sees every byte:
   `cat /dev/ttyUSB1 > captures/$(date +%Y%m%d-%H%M).bin` while the ECU transmits.
   The .bin becomes a byte-exact fixture: `MdFrameDecoder` must reproduce the same DP
   values the live app showed during the same capture (cross-check the paired .mdsn export).
3. Every fixture commit also gets a `TestHarness` group (decode → exact values, corrupt
   byte → discard/resync).

## Sign-off criteria

- [ ] Boost semantics decided from a meter, not from the ancestor's source comment
- [ ] At least one real capture per frame type, checked in under `tests/fixtures/bench/`
- [ ] Golden fixture tests decode byte-exact and assert exact values
- [ ] `--unit-test` stays green with the new fixtures

## Notes

- Sessions record decoded samples (dpId, value, timestamp); raw byte capture is B2-step-2
  and deliberately separate — decoded data can't prove framing, raw bytes can.
- Keep the recorder running during *all* bench work; a session log is cheap insurance and
  turns one-off bench time into permanent regression coverage.
