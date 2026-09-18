# Testing and CI

This is the maintained reference for automated checks. Update it when a test suite, PlatformIO test
environment, CI step, or validation boundary changes. Historical test results remain in
[progress.md](progress.md); they are not the source of truth for the current test layout.

## Quick commands

PlatformIO Core 6.2.0 is used by CI. If `pio` is not on `PATH`, use
`~/.platformio/penv/bin/platformio` in its place.

```sh
# Portable logic tests, also run by CI
pio test -e native

# Real LVGL host integration tests, currently local-only
pio test -e native-ui

# Complete host check
pio test -e native -e native-ui

# Firmware compile checks, also run by CI
pio run -e t-display-s3 -e t-display-s3-release
```

Run one portable suite with PlatformIO's test filter, for example:

```sh
pio test -e native -f test_conversions
pio test -e native -f test_sensor_reads
```

No command above uploads firmware, flashes a board, performs OTA, or validates physical sensors,
touch, display layout, power consumption, deep sleep, or wake behavior.

## PlatformIO test environments

The authoritative environment definitions are in [`platformio.ini`](../platformio.ini).

| Environment | Purpose | Test selection | Production sources compiled |
| --- | --- | --- | --- |
| `native` | Portable application, settings, conversion, acquisition, and sleep logic | All `test/test_*` suites except `test_exported_ui` | `sensors/conversions.cpp`, `sensors/sensors.cpp`, `app/Analyzer.cpp` |
| `native-ui` | Exported UI and adapter against real LVGL 9.5.0 | Only `test_exported_ui` | `display/UiAdapter.cpp`, `sensors/conversions.cpp` |

Both environments use PlatformIO's Unity test framework and C++17. `native` adds `-Wall -Wextra`.
The embedded environments inherit `test_ignore = test_*`, so firmware builds do not discover or run
host tests.

Host isolation is provided by [`test/fakes`](../test/fakes). These replacements model Arduino time,
GPIO, Preferences, FreeRTOS queues, ADS1115 devices, and related platform services. They permit
deterministic failure, timeout, clock-wrap, queue-full, and persistence scenarios without a board.
They are behavioral fakes, not hardware or scheduler emulation.

## Portable native suites

Current inventory: **115 tests**. The registration lists at the bottom of each suite source are the
authoritative individual test names.

| Suite | Tests | Coverage | Primary implementation references |
| --- | ---: | --- | --- |
| [`test_analyzer`](../test/test_analyzer/test_analyzer.cpp) | 51 | Command/result admission, effective settings, Preferences reconciliation, calibration acceptance and stability, startup behavior, measurement generations, CO warm-up, sleep prepare/resume, queue pressure, and settings editor acknowledgement | [`src/app/Analyzer.cpp`](../src/app/Analyzer.cpp), [`src/app/Analyzer.h`](../src/app/Analyzer.h), [`src/app/AnalyzerPolicy.h`](../src/app/AnalyzerPolicy.h), [`src/settings/SettingsStore.h`](../src/settings/SettingsStore.h) |
| [`test_sensor_reads`](../test/test_sensor_reads/test_sensor_reads.cpp) | 34 | One-conversion sensor reads, raw/derived values, ADC initialization and timeout handling, per-device recovery, channel states, latest-value queue semantics, freshness, clock wrap, calibration acquisition, and warm-up classification | [`src/sensors`](../src/sensors), [`src/sensors/sensors.cpp`](../src/sensors/sensors.cpp), [`src/sensors/conversions.cpp`](../src/sensors/conversions.cpp) |
| [`test_sleep`](../test/test_sleep/test_sleep.cpp) | 16 | Timeout persistence, inactivity policy, wake-button debounce, retained wake marker classification, prepare/resume interruption, and calibration behavior across cold boot and application wake | [`src/app/SleepPolicy.h`](../src/app/SleepPolicy.h), [`src/app/Analyzer.cpp`](../src/app/Analyzer.cpp), [`src/settings/SettingsStore.h`](../src/settings/SettingsStore.h) |
| [`test_conversions`](../test/test_conversions/test_conversions.cpp) | 14 | O2, He, CO, temperature, MOD and integer conversions; finite/range guards; presentation formatting; settings selection mapping | [`src/sensors/conversions.cpp`](../src/sensors/conversions.cpp), [`src/display/UiPresentation.h`](../src/display/UiPresentation.h) |

These suites compile selected production sources with the platform fakes. They do not compile the
entire firmware or the generated LVGL UI.

## LVGL host integration suite

[`test_exported_ui`](../test/test_exported_ui/test_exported_ui.cpp) contains **4 tests** and runs only
under `native-ui`. It initializes real LVGL 9.5.0 with a 320 x 170 RGB565 partial display buffer and
uses the actual exported UI library plus [`UiAdapter.cpp`](../src/display/UiAdapter.cpp).

It verifies:

- exported boot, nonblank rendering, image binding, screen navigation, and calibration states;
- settings synchronization, save/rejection behavior, brightness, and update-screen actions;
- reading/status text, units, warning styles, channel states, stale data, MOD, and diagnostics;
- repeated dynamic Settings creation/deletion without lost LVGL heap or memory corruption.

Events are injected directly into LVGL objects. This suite does not exercise the touch controller,
TFT driver, target memory limits, on-device layout, or generated XML in the Editor before export.
Because `native-ui` is not invoked by the workflow, it must be run locally whenever the UI export,
UI assets, LVGL configuration, or `UiAdapter` changes.

## Hosted CI

The workflow is [`.github/workflows/ci.yml`](../.github/workflows/ci.yml). It runs on every push and
pull request using `ubuntu-latest`:

1. Check out the repository.
2. Install Python 3.11.
3. Install PlatformIO 6.2.0.
4. Run `pio test -e native`.
5. Compile `t-display-s3` and `t-display-s3-release` in one PlatformIO invocation.

The two firmware builds catch target-only compilation and linking failures across debug and release
flags. They are compile checks, not tests, and do not execute firmware. CI currently does **not** run
`native-ui`, `esp32dev`, upload, OTA, or device validation.

## Choosing validation

Use the smallest relevant suite first, then broaden according to the affected surface:

| Change | Minimum focused validation | Before completion |
| --- | --- | --- |
| Conversion or presentation helper | Relevant `native` suite with `-f` | `pio test -e native` |
| Analyzer, settings, sensors, calibration, or sleep policy | Relevant `native` suite with `-f` | `pio test -e native` and both S3 builds |
| `UiAdapter`, UI export, fonts, images, or LVGL config | `pio test -e native-ui` | Both host environments and both S3 builds |
| Board pins, display/touch driver, deep sleep, wake, or sensor power | Closest host tests and both S3 builds | Explicit physical-device validation remains required |
| CI or PlatformIO configuration | The command(s) changed by the edit | Reproduce all affected workflow commands |

Add regression coverage to the existing owning suite. Create another suite only when it needs a
meaningfully different build environment or fixture boundary.

## Maintenance checklist

When tests or CI change:

1. Update the suite count and coverage row here when `RUN_TEST` registrations move or change.
2. Update environment selection and compiled-source notes when `platformio.ini` changes.
3. Update hosted steps and exclusions when `.github/workflows/ci.yml` changes.
4. Keep physical/device limitations explicit; host success must not be described as device validation.
5. Record dated execution results in [`progress.md`](progress.md), not as timeless claims here.

For application behavior behind the tests, consult [`architecture.md`](architecture.md). Agents
should begin with this document, the affected suite row, and the referenced implementation files;
a repository-wide scan should not be necessary for routine test or CI work.