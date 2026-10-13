# A-Class Game Plan — Path to Professional-Grade Quality

**Status: LIVING DOCUMENT — the master plan. Update as phases complete.**

"A-class" means: an automotive-grade telemetry suite that a stranger could clone, build, trust,
and extend — correct parsers, deterministic tests, professional visualization, packaged releases.

---

## Pillar 1 — Data correctness (P0, mostly done Oct 2026)

- [x] MD-ECU framing corrected (TAG = total frame length; ETX validated at position)
- [x] All 8 tag constants aligned to ancestor firmware contract; fabricated OBD-II tag removed
- [x] Main-frame payload offsets corrected (no more reads past the 92-byte payload)
- [x] `testSingleton()` wired into `runAllTests()` (now 16 groups / 43 assertions)
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
- [x] CI: GitHub Actions (ubuntu-24.04 + apt Qt6/Qwt) — configure, build, ctest.
      Windows matrix entry pending agreed MSVC/CIQt pinning (Pillar 6)
- [x] Decoder fuzz/robustness: seeded 50k random-byte fuzz must not crash or wedge the state
      machine; golden frame still decodes exactly afterwards

## Pillar 3 — Visualization (uses vendored references in resources/plotting/)

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

- [ ] `PlotEngine` interface (addSeries/updateValue/pause/markers) extracted from RealtimePlot
- [ ] Downsample visible window (min/max decimation) so 8h sessions stay >=60 fps
- [ ] XY-mode: plot any DP against any DP (RPM vs boost, EGT vs lambda), not just vs time

## Pillar 4 — Session recording & replay (the telemetry product core)

- [ ] Binary session log (framing-preserving + DP-keyed store dump), indexed by timestamp
- [ ] Replay UI (already have CSV import; unify on one session model)
- [ ] Export: CSV/JSON with unit metadata; report images via cxxplot/matplot++

## Pillar 5 — Productization

- [ ] AppImage + Windows installer; Android decision (Qt for Android is heavy — may stay desktop)
- [ ] Settings profiles per vehicle; language/translation readiness
- [ ] Crash-safe logging (rotating file, rate-capped), log level CLI flag
- [ ] README screenshots/GIF (real offscreen captures), in-app first-run tour

## Pillar 6 — Repo hygiene (done)

- [x] git initialized (first commit: corrected parsers + docs overhaul)
- [x] LICENSE (Sovereign Individual License v1.0 — ecosystem-wide proprietary license)
- [x] CHANGELOG.md / CONTRIBUTING.md; per-file license headers on all 43 sources
- [x] Session notes moved to docs/archive/ with superseded banners; stray PNG deleted
- [x] Version single-sourced to CMake; qDebug console handler (Qt >= 6.8 journald trap)
- [ ] GitHub private repo remote + Actions CI (pillar 2)

---

## Definition of done (v1.0)

1. Every wire byte is explained: each frame field maps to a DP or a documented gap
2. Every parser has a fixture test; `--unit-test` green in CI on two platforms
3. Plots stay smooth at 8h/30Hz/50-series sessions with full review scrubbing
4. A new user can connect hardware and see correct curves in < 5 minutes
