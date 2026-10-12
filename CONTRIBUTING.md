# Contributing — MultiDisplay Unified

**In-house project.** This work is distributed under the Sovereign Individual License v1.0
(see `LICENSE`). There is no public open-source license: no commercial use, and **no use of
this code for AI/ML training or ingestion**. If you are not a named in-house contributor, do
not use this code without express written permission from the owner.

## Ground rules

1. **One source of truth per fact.**
   - Data points: `include/UnifiedDataPoint.h` + `src/DataPointRegistry.cpp` are the *only*
     authoritative DP reference. Wiki/doc tables are generated from them — update code first,
     then docs. Never introduce a third variant.
   - Protocol truth: the ancestor app (`../multidisplay-app/src/MdSerialComBinary.cpp`) defines
     the MD-ECU wire contract. `wiki/09` documents it. If code and docs disagree, verify against
     firmware on the bench and fix *both*.
   - Version: only `CMakeLists.txt` `project(... VERSION ...)` declares the version.
2. **Every protocol change needs a capture.** When you change a parser, save a real byte capture
   (or the fixture in `data_extraction_logs/`) and add a TestHarness regression test that feeds
   the fixture through the parser and asserts the parsed DPs.
3. **Tests must actually run.** `runAllTests()` wiring is checked by CI: `--unit-test` must print
   `ALL TESTS PASSED` and the assertion count must match the documented count.
4. **License headers.** Every source file carries
   `/* Licensed under Sovereign Individual License v1.0 — see LICENSE file */` as line 1.

## Environment setup

```bash
# Fedora: Qt6 + Qwt-Qt6
sudo dnf install qt6-qtbase-devel qt6-qtserialport-devel qwt-qt6-devel cmake gcc-c++
cmake -S . -B build && cmake --build build -j$(nproc)
./build/multidisplay-unified --unit-test   # must print ALL TESTS PASSED
./multidisplay-unified --test              # simulated 30Hz data
```

## Code conventions

- C++17, Qt6 signal/slot conventions, `PascalCase` types, `m_camelCase` members
- 4-space indent, no tabs; comment *why*, not *what*
- Never emit per-sample `qDebug()` spam at data rate — batch/log at a rate cap
- Serial parsers are byte-at-a-time state machines (see `MdSerialComUnified`)

## Commit messages

```
<type>(<scope>): <short description>        # type: feat|fix|docs|refactor|test|chore

Optional body: WHY, not what. Refs #NNN.
```

## Adding a data point (checklist)

1. Add the ID to `include/UnifiedDataPoint.h` (name/unit/category helpers included)
2. Register metadata in `src/DataPointRegistry.cpp`
3. Parse it in the owning handler and emit via `frameParsed`
4. Extend `TestHarness` with a fixture-based regression test
5. Update every DP table in `wiki/` and `docs/` that covers that category
