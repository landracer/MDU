# A-Class Game Plan — Path to Professional-Grade Quality

**Status: LIVING DOCUMENT — the master plan. Update as phases complete.**

"A-class" means: an automotive-grade telemetry suite that a stranger could clone, build, trust,
and extend — correct parsers, deterministic tests, professional visualization, packaged releases.

---

## Pillar 1 — Data correctness (P0, mostly done Oct 2026)

- [x] MD-ECU framing corrected (TAG = total frame length; ETX validated at position)
- [x] All 8 tag constants aligned to ancestor firmware contract; fabricated OBD-II tag removed
- [x] Main-frame payload offsets corrected (no more reads past the 92-byte payload)
- [x] `testSingleton()` wired into `runAllTests()` (now 20 groups / 49 assertions)
- [ ] **Bench-verify** main-frame boost semantics (absolute kPa vs gauge; ancestor subtracted 1.0)
- [x] Digifant block (payload 57–91) + VDO sensors + knock mapped to DPs (0x0136–0x0160) and
      fixture-tested; protocol layer split into pure `MdFrameDecoder` + thin serial transport
- [x] Golden-fixture decoder tests (synthetic 92-byte payload, exact-value asserts, corrupt-frame
      discard/resync, seeded fuzz)
- [ ] Replay the original app's real logged captures (data_extraction_logs/) through the decoder
      as additional fixtures once live ECU hardware is on the bench

## Pillar 2 — Testing & CI

- [x] Test-suite wiring decision: **keep TestHarness** (stable output, already aggregated) and
      wrap it in CTest instead of porting to QTest — `add_test(NAME unit_tests COMMAND
      multidisplay-unified --unit-test)` with `PASS_REGULAR_EXPRESSION "ALL TESTS PASSED"`.
      (Per-test QTest data tables remain an optional upgrade, not a blocker.)
- [x] CTest integration: `ctest --output-on-failure` runs the suite (offscreen Qt)
- [x] CI: GitHub Actions on ubuntu-24.04 — apt Qt6 + **Qwt 6.3.0 built from source**
      (Ubuntu Noble ships no Qt6-Qwt package) — configure, build, ctest, scoped to master
- [x] Windows + macOS CI: owner doesn't run Windows/Apple, so this uses the ecosystem-
      standard setup (MSVC + `aqtinstall` Qt 6.8.2 + windeployqt zip; clang/make
      + macdeployqt dmg; Qwt 6.3 source-built there too) so any beta tester on those platforms
      can grab the CI artifact; these jobs are community-iterated, not owner-maintained
- [x] Decoder fuzz/robustness: seeded 50k random-byte fuzz must not crash or wedge the state
      machine; golden frame still decodes exactly afterwards

## Pillar 3 — Visualization (core done Oct 2026; matplot++ engine = future)

The reference libraries are now vendored for study/borrowing (licenses checked):

| Reference | License | Verdict for Qt app |
|---|---|---|
| matplotplusplus | MIT | **Best future engine** — publish-quality 2D/3D; backend is gnuplot/OpenGL (no Qt canvas → needs a thin Qt-image backend) |
| implot / implot3d | MIT | Immediate-mode **ImGui** widgets — wrong paradigm for this QWidget app; study their decimation/perf ideas only |
| mathplot (MathGL) | LGPL-3.0 | Heavy dep; skip unless 3D needed fast |
| cxxplot | see vendored dir | Async C++ scientific plotting for the export/report subsystem |
| graphing-calculator | — | UX reference: adaptive axis labels, pinch/zoom behavior |

Plan: keep Qwt (current) as the default engine; port `RealtimePlot` behind a `PlotEngine`
interface so matplot++ (Qt-image backend) can slot in later; adopt decimation + dirty-rect
replot tricks (already batched at 20Hz). 3D attitude display via matplot++ 3D or OpenGL canvas —
never ImGui.

- [x] `PlotEngine` interface (addSeries/updateValue/pause/markers) extracted from RealtimePlot
      — RealtimePlot is now one swappable implementation over the pure PlotDataModel
- [x] Downsample visible window: min/max-per-bucket decimation sized to canvas width, so
      8h/30Hz sessions render ≤ ~2 points per pixel column instead of every sample
- [x] XY-mode: toolbar X-axis selector plots any DP against any DP (RPM vs boost, EGT vs
      lambda), not just vs time; pairing + interpolation live in unit-tested PlotDataModel

## Pillar 4 — Session recording & replay (done Oct 2026 — the telemetry product core)

- [x] Binary session log (`SessionLog`, `.mdsn`): framing-preserving fixed-record blocks,
      footer timestamp index (`t0RelUs → offset`) for seekable replay; corrupt/truncated
      files rejected outright, never half-parsed; round-trip + truncation fuzz-tested
- [x] Replay UI unified on one session model: replay feeds the same `UnifiedDataStore`
      signal path as live sources (`--session file.mdsn`, File menu); CSV import remains a
      source into the same store
- [x] Export: CSV/JSON with per-DP name + unit metadata from the registry
- [ ] Report images via cxxplot/matplot++ (deferred — export covers data portability)
- [ ] Time-paced live playback of recordings (current replay dumps full state instantly,
      like CSV import); scrubbing UI is a follow-on

## Pillar 5 — Productization

- [x] Mobile/Android decision **made** (wiki/12): LAN JSON API + bundled web dashboard —
      the browser is the universal thin client; no Google dev-platform/AKG commitment; Qt-for-
      Android stays parked behind the existing `android/` placeholder
- [ ] Headless `--serve` web dashboard (implements the mobile path); AppImage/NSIS polish
- [ ] Settings profiles per vehicle; language/translation readiness
- [ ] Crash-safe logging (rotating file, rate-capped), log level CLI flag
- [ ] README screenshots/GIF (real offscreen captures), in-app first-run tour

## Pillar 6 — Repo hygiene (done)

- [x] git initialized (first commit: corrected parsers + docs overhaul)
- [x] LICENSE (Sovereign Individual License v1.0 — ecosystem-wide proprietary license)
- [x] CHANGELOG.md / CONTRIBUTING.md; per-file license headers on all 43 sources
- [x] Session notes moved to docs/archive/ with superseded banners; stray PNG deleted
- [x] Version single-sourced to CMake; qDebug console handler (Qt >= 6.8 journald trap)
- [x] GitHub private repo remote (landracer/MDU) + Actions CI green (pillar 2)

---

## Definition of done (v1.0)

1. Every wire byte is explained: each frame field maps to a DP or a documented gap
2. Every parser has a fixture test; `--unit-test` green in CI on two platforms (ubuntu green; Windows pending pinning)
3. Plots stay smooth at 8h/30Hz/50-series sessions with full review scrubbing
4. A new user can connect hardware and see correct curves in < 5 minutes
