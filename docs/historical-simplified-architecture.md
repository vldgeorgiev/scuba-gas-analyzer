# Historical: simplified firmware architecture

Status: historical design draft, superseded by [the updated improvement plan](improvement-plan.md).
The partial redesign was reverted on 2026-09-15. Component names and consolidation instructions
below refer to that discarded work, not the current source tree. Do not implement this draft
without reconciling it with the current plan.

The goal is to retain the correctness improvements of the redesign while making the firmware
appropriate for a small ESP32 application: few owners, few messages, concrete components, bounded
memory, and tests around behavior rather than around infrastructure.

## Design principles

- Use two application tasks: one UI task and one analyzer task.
- Give each mutable resource one owner. The UI task owns LVGL; the analyzer task owns sensors,
  calibration, effective settings, persistence, and sensor power.
- Exchange fixed-size values through small FreeRTOS queues. Do not share LVGL objects, ADC objects,
  calibration buffers, or mutable settings across tasks.
- Keep mathematical and state-transition code independent of Arduino, FreeRTOS, Preferences, and
  LVGL so it can be tested natively.
- Prefer concrete classes. Add an interface only at a hardware or operating-system boundary that a
  host test must replace.
- Support only one user operation at a time. The product does not need a general multi-operation
  transaction framework.
- Use fixed-capacity storage throughout. Avoid heap allocation in normal acquisition, calibration,
  logging, and messaging paths.

## System view

```text
                         fixed-size Command
  +----------------+  ------------------------>  +---------------------+
  |    UI task     |                              |    Analyzer task    |
  |                |  <------------------------  |                     |
  | LVGL 9.1       |   AppEvent / latest sample  | AnalyzerService     |
  | UI adapter     |                              | CalibrationSession  |
  | draft settings |                              | MeasurementEngine   |
  +----------------+                              +----------+----------+
                                                             |
                                          +------------------+------------------+
                                          |                  |                  |
                                   +------v------+    +------v------+    +------v------+
                                   | ADS1115     |    | Settings    |    | Sensor power |
                                   | devices     |    | store/NVS   |    | GPIO          |
                                   +-------------+    +-------------+    +-------------+
```

There is no third application manager task. Arduino startup composes the concrete components,
creates the queues, and starts the two owners. The normal Arduino `loop()` is blocked if it is not
used as the UI task.

## Task ownership

### UI task

The UI task alone may call LVGL or use generated UI object pointers. This includes initialization,
`lv_timer_handler()`, widget updates, chart updates, screen changes, and event callbacks. LVGL 9.1
is not thread-safe; keeping all calls on one task avoids a GUI mutex and makes the future LVGL
Editor integration predictable.

The UI owns editable draft settings. Applying a draft submits one complete settings candidate.
Until the analyzer acknowledges it, the UI continues to distinguish the draft from the effective
settings. UI callbacks submit work and return promptly; they do not read sensors or write NVS.

### Analyzer task

`AnalyzerService` is the sole owner of:

- both ADS1115 devices and their readiness/recovery state;
- sensor enable outputs and the fixed 3,000 ms CO startup interval;
- acquisition timing and the latest measurement generation;
- the active calibration session and its sample history;
- effective settings and persistent settings writes;
- preparation and acknowledgement for sleep.

It runs one explicit state machine. Each pass handles a bounded amount of command work, advances a
calibration or sleep transition when active, and performs acquisition when due. No operation uses a
long blocking delay.

## Communication

Use three bounded paths:

1. `measurementQueue`: depth one, overwrite with the newest immutable `Measurement`.
2. `commandQueue`: a small FIFO, normally depth four, from UI to analyzer.
3. `eventQueue`: a small FIFO for accepted/rejected/completed/failed/cancelled events.

Calibration graph updates are progress, not terminal results. Keep the latest progress snapshot in
one overwrite slot or coalesce it in the UI-facing state. A slow UI may skip graph frames, but it
must not affect stability detection or the final calibration result.

The UI permits one outstanding state-changing operation. It disables or marks the relevant action
busy until a terminal event arrives. Therefore the firmware does not need terminal-capacity
reservations, a general result retry collection, or several simultaneous operation records.

Commands should be product-specific and small, for example:

- apply a complete settings candidate;
- start O2-air, O2-100%, or He calibration;
- cancel calibration;
- reset O2-air or He calibration;
- clear the optional O2-100% calibration;
- prepare for sleep.

Events contain the command identifier, outcome, effective settings when relevant, and a compact
reason code. A monotonic settings generation is retained only because it prevents a measurement
captured under old coefficients from being presented after new settings are acknowledged.

## Main components

### `AnalyzerService`

This is the application coordinator, not a generic framework. It owns the current mode
(`Measuring`, `Calibrating`, or `PreparingSleep`) and delegates focused work to the components
below. It should depend on a grouped hardware object, the settings store, and the concrete queue
adapter rather than receiving a long list of individual interfaces.

### `MeasurementEngine`

Performs one bounded acquisition cycle and returns a fully initialized `Measurement`. Each channel
contains its value, raw millivolts, timestamp, and status such as disabled, warming, valid, invalid,
or unavailable. Pure conversion and validation functions remain separate and host-testable.

### `AdcDevice`

Use the unmodified Adafruit ADS1X15 library for register access, gain, data rate, and voltage scaling.
Keep only a small wrapper for a conversion polling deadline, readiness, and recovery backoff.
Use the library's non-blocking conversion API and configure the ESP32 Wire timeout. Do not add a
custom register driver, transport hierarchy, or dependency patch.

The stock library does not expose individual I2C transfer failures. Initialization failure and a
conversion that never becomes ready can be reported, but plausible results from failed exchanges
cannot always be detected. This limitation is accepted in favor of simpler maintenance.
Native tests substitute the library's public API; target builds and hardware checks validate the
real integration. Adafruit may allocate internally during initialization and recovery.

### `CalibrationSession`

Contains the calibration state machine and fixed-capacity rolling sample window. It:

- accepts only fresh timestamped samples;
- evaluates spread, drift, minimum coverage, sample gaps, dwell time, and overall timeout;
- emits immutable progress snapshots for the future graph;
- returns a candidate coefficient without modifying effective settings;
- preserves the previous calibration on cancellation, timeout, invalid input, or storage failure.

After stability qualifies, `AnalyzerService` validates the whole candidate settings value, stores
it, applies it, increments the settings generation when measurement semantics changed, and reports
success. Automatic cold-boot calibration uses the same session. A confirmed wake from application
sleep skips automatic startup calibration.

### `SettingsStore`

Keep one `Settings` value with defaults, explicit optional O2-100% presence, whole-value validation,
and a schema version. Load it once during startup and hold it in RAM.

Keep the existing individual Preferences keys. Load them once at startup into the RAM value and
write only fields that changed after the complete candidate has passed validation. Persistence is
best-effort: if a write reports failure, keep the previous RAM value and report the operation as
failed.

This unreleased application does not require migration, schema compatibility, atomic multi-key
updates, CRCs, redundant slots, sequence counters, or recovery from power loss during a settings
write. The settings are non-critical and documented defaults are acceptable if stored values are
missing or invalid.

References:

- https://docs.espressif.com/projects/esp-idf/en/release-v4.2/esp32/api-reference/storage/nvs_flash.html
- https://docs.lvgl.io/9.1/overview/os.html

### `DisplayModel` and UI adapter

`DisplayModel` remains a pure conversion from the latest measurement and effective settings to
what the screen should show, including stale and unavailable states. The handwritten UI adapter
maps that model and application events to generated widgets.

Generated filenames and object names are deliberately absent from this architecture. The adapter
is rewritten against the user's eventual LVGL Editor export without changing analyzer code.

### `EventLog`

Use one fixed-size chronological ring plus an active-fault bitmask. Historical error messages do
not keep the current fault icon active after a condition clears. Logging does not need a separate
polymorphic store and sink hierarchy; a small concrete service with task-safe entry points is
sufficient.

## Sleep

The persisted timeout belongs to `Settings`. The UI task measures user inactivity using LVGL's
display inactivity state and submits `PrepareSleep` only when no UI operation is active.

The analyzer then stops accepting ordinary commands, cancels or rejects sleep if calibration is
active according to policy, deasserts sensor outputs, and returns a bounded acknowledgement. Only
after that acknowledgement does the UI turn off the backlight/display/touch, configure active-low
GPIO14 wake, and enter deep sleep.

On wake, startup loads persistent settings, recognizes a validated application-sleep marker,
restores accepted calibration without automatic recalibration, and restarts the CO warm-up interval.
Button release/debounce prevents the wake press from activating the UI or immediately sleeping
again.

## Proposed source layout

Paths are responsibility names, not a contract with the future generated UI:

```text
src/
  app/
    AnalyzerService.h
    AnalyzerService.cpp
    AppMessages.h
    FreeRtosBus.h
  sensors/
    AdcDevice.h
    AdcDevice.cpp
    MeasurementEngine.h
    MeasurementEngine.cpp
    Measurement.h
    Conversions.h
    Conversions.cpp
    CalibrationSession.h
    CalibrationSession.cpp
  settings/
    Settings.h
    Settings.cpp
    SettingsStore.h
    SettingsStore.cpp       # individual Preferences keys, cached in RAM
  presentation/
    DisplayModel.h
    DisplayModel.cpp
  log/
    EventLog.h
    EventLog.cpp
  ui_adapter/
    UiAdapter.h
    UiAdapter.cpp
```

## What to consolidate from the current branch

- Replace `control/Channels`, `FreeRtosChannels`, `TerminalBudget`, `Submitter`, and `ResultSender`
  with `AppMessages` and one concrete `FreeRtosBus`.
- Replace the large acquisition owner constructor with `AnalyzerService(Hardware&, SettingsStore&,
  FreeRtosBus&)` or equivalent grouped concrete dependencies.
- Remove `AdcTransport`, `ArduinoAdc`, and copied ADS1115 constants. Keep `AdcDevice` as a thin
  wrapper around unmodified Adafruit ADS1X15; no custom register or voltage-scaling implementation.
- Keep the pure acquisition and derivation tests, but test through `MeasurementEngine` rather than
  preserving every current class boundary.
- Replace the record serializer, CRC, dual-slot store, and migration code with one small
  Preferences-backed store using the existing individual keys.
- Preserve the useful behaviors already implemented: initialized snapshots, bounded reads,
  independent ADC recovery, fixed CO warm-up, stale-value suppression, acknowledged operations,
  calibration rollback, and active-fault tracking.
- Reduce comments that repeat implementation details. Retain comments for hardware constraints,
  units, non-obvious invariants, and reasons behind decisions.

## Verification boundary

Host tests should cover conversions, complete measurement initialization, ADC timeout/recovery,
settings validation and write failures, calibration stability traces and rollback, command state transitions,
stale presentation, time wrap, and sleep preparation.

Target builds prove compilation and size only. Physical checks remain required for ADC behavior,
sensor power timing, actual calibration traces and thresholds, UI responsiveness, NVS write time,
deep-sleep wake on GPIO14, retained settings, post-wake CO warm-up, current consumption, and repeated
sleep/wake cycles.

## Decision before further implementation

Revise OpenSpec changes 3 to 5 against this target before continuing implementation. Reuse existing
behavioral tests where they express product requirements, but do not preserve an abstraction only
because a test was written around it. The simplification is complete when the same safety behavior
is expressed with fewer owners, fewer queue mechanisms, fewer public types, and a source layout a
single maintainer can understand without reconstructing a framework.
