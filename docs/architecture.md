# Current firmware architecture

Updated 2026-09-15 after step 3d. Scope: [improvement plan](improvement-plan.md).
Verification and historical increments: [progress](progress.md).
This describes the current implementation; the final section identifies work still planned.

## Two application tasks

```text
UI task (core 0)                      Analyzer task (core 1)
LVGL / EEZ, input, display            ADCs, sensor power, calibration
draft settings, effective copy        effective settings, Preferences
               command queue ------>
               <------ result queue
               <------ latest measurement queue
```

`setup()` creates the three queues and the tasks. The analyzer task creates its own `SettingsStore`,
`SensorManager`, and concrete `app::Analyzer` locally; UI callbacks cannot access those instances.
The UI task owns `DisplayManager`, EEZ globals, widget pointers, and its copy of effective settings.
The Arduino loop blocks indefinitely. There is no extra manager/logger/battery/monitor task.

| Task | Work and timing | Allocated stack |
| --- | --- | --- |
| UI | Initialize display/input/generated UI, drain results/readings, render, delay 5 ms | 10 KB |
| Analyzer | Load settings, initialize sensors/power, handle commands, measure, delay 10 ms | 4 KB |

Normal measurement scheduling waits at least 500 ms from the end of the last cycle; there is no
catch-up loop. Processing a command makes a fresh cycle due. UI presentation checks run around
100 ms apart, with immediate invalidation on settings acknowledgement. These are code intervals,
not measured response bounds; bus, flash, rendering, and scheduling add time.

The old `Task_Screen_Update`, GUI mutex, `configOpen`, `SensorAccess`, and performance-monitor
task were removed. UI initialization is ordered, not delayed by an arbitrary second. Installed
EEZ `ui_init()` calls `eez_flow_init()`, which loads assets, starts flow/global variables, creates
screens, and selects the initial screen before returning. UI then seeds settings and blank readings.
LVGL MCP was consulted for single-task ownership; APIs remain compatible with pinned LVGL 9.1.0.

## Message contract

The message definitions and concrete owner are in `src/app/Analyzer.h` and `.cpp`.
No interfaces, promises/futures, general event bus, terminal reservations, or multi-operation scheduler.
Command/result structs are statically checked as trivially copyable for FreeRTOS queue copying.

| Queue | Capacity and semantics |
| --- | --- |
| Measurement | One snapshot, overwritten without waiting; old samples may be skipped |
| Command | One fixed-size command; zero-wait submission; full/busy means no operation started |
| Result | One fixed-size result; separate from telemetry; zero-wait send with one owner-retained pending result |

UI admission starts busy until the analyzer's Startup result arrives. Only one state-changing user
operation is outstanding. A submitted command receives a nonzero ID; UI stays busy until the matching
terminal result is consumed, including failures. Sleep preparation retains that ID until sleep entry
or an acknowledged resume; it does not free ordinary admission on preparation success.
Unknown/duplicate results do not clear another
operation. Startup has its own result type and is accepted once. IDs skip zero on wrap.

Commands: apply editable settings, calibrate O2-air/O2-pure/He, reset air/He, clear optional
pure-O2 calibration, PrepareSleep, and Resume. Calibration cancellation/progress are not added yet.
Every result carries the request type/ID, failure reason, effective settings, measurement generation,
and calibration value where relevant. Startup also carries the independent ADC initialization report.

`Analyzer::service()` first retries its pending result; if delivery remains blocked, it does not
take another command. The task still calls `measure()` when due. Slow UI consumption cannot discard
an outcome or stop ordinary measurement publication unless sleep preparation deliberately suspends it.
Normal calibration temporarily occupies the
analyzer until its fixed sampling finishes; UI rendering continues. No progress/cancel UI is claimed.

A missing startup or terminal result does not cause UI to clear busy on a timer. Doing that could
admit another operation while a supposedly timed-out command still owns the hardware. Queue/task
allocation failures are logged; failed analyzer creation leaves commands unavailable. A permanently
stuck task requires explicit recovery policy later, not silent readmission.

## Sleep preparation and resume

Step 3c adds the handshake on the existing queues, without a shared sensor-access flag or a new
task. No inactivity trigger or deep-sleep call exists yet, so normal use cannot initiate preparation
in this increment. The backend and UI timeout/resume pump are ready for that next integration.

On PrepareSleep with a nonzero ID, the analyzer records the active sleep ID, advances measurement
generation, deasserts He/CO sensor power, clears the measurement queue, and only then produces its
acknowledgement. `measure()` publishes nothing while prepared. Ordinary settings/calibration/reset
commands are rejected Busy before touching storage or hardware. The one-outstanding UI policy
prevents submitting preparation while a calibration or settings operation is outstanding; this
does not preempt the analyzer's synchronous calibration routine.

Repeated preparation for the active ID is idempotent. Another ID is rejected while prepared.
Resume must match the active sleep ID before it can restore effective power enables and advance
generation again. An already-awake analyzer acknowledges any nonzero Resume harmlessly, without
changing power, generation, settings, or a queued reading. This covers an aborted preparation that
never took effect as well as repeated resumes. A mismatched ID cannot release another active sleep.
Sleep/resume never writes Preferences. Disabled sensors stay disabled.

UI uses four phases: Awake, Preparing, Prepared, Resuming. Preparation has a two-second elapsed
deadline. The UI checks expiry before draining results, so a prepare acknowledgement arriving after
timeout cannot authorize sleep. Abort switches to Resuming and submits Resume with the retained ID,
retrying a full command queue without clearing busy. A resume is enqueued once at a time. If its
result reports failure, it may be retried after one second; the UI remains busy until success.
Late prepare acknowledgements are consumed but ignored during resume. Timing is wrap-safe.

Prepared deliberately awaits a decision by the future power-down sequence, not another automatic
deadline: that sequence must enter deep sleep or request resume on abort/new activity. It is not yet
wired and there is no claim of an end-to-end sleep feature. A permanently stuck analyzer cannot be
recovered by forgetting the pending ID. Device checks must confirm publication and power restoration,
not infer physical recovery merely from host state-machine tests.

## Sleep setting and activity policy

Step 3d adds `sleepMinutes` to AnalyzerSettings and the individual `sleep_minutes` Preferences key.
Default is five minutes; supported values are Off (0), 1, 2, 5, 10, 30. One option array and one
default constant supply validation, dropdown labels, and index mapping. Missing values load as five;
an invalid stored timeout is replaced with five without discarding other settings. The substitution
is serial-logged and marks the store dirty, so the next successful save reconciles it. No boot-time
write or settings migration is added. Changing sleep timeout does not change measurement generation.

DisplayManager appends a plain Sleep label/dropdown group to the existing configuration flex layout
after generated UI initialization. It uses no EEZ variable and generated ticks do not overwrite it.
Opening/synchronizing settings selects the effective value; closing includes the selected timeout
in the existing ApplySettings command. Until acknowledgement, the control uses the previous effective
value, including on busy/rejected/write-failed requests. The runtime group is 116 by 62 pixels;
accessibility in the existing scrolling configuration layout requires a device check. No generated
UI file is edited, and this binding will be replaced with the user's future Editor export.

LVGL's display inactivity counter is the only inactivity time source. Touch input already updates
it. DisplayManager exposes its read/reset APIs; UI resets activity after initialization and whenever
an acknowledged operation leaves the app no longer busy. Thus startup, failed settings, calibration
completion, and successful resume each start a fresh inactivity interval. Measurement updates do
not reset it.

On the primary S3 target, UI polls GPIO14/button 2 with its input pull-up. `WakeButton` treats a held
press as activity and requires 50 ms of uninterrupted release before reporting released. Raw changes
and completion of the release interval count as activity. It starts in wait-for-release state, so
the press that will eventually cause wake cannot activate a UI control. Elapsed comparisons handle
clock wrap. The other experimental board profile does not yet have this wake-button integration.

`SleepPolicy.h` provides the tested `sleepDue` decision using selected minutes, LVGL inactivity,
an inhibition flag, and the debounced release state. It adds no second idle timer or task. It is
not yet invoked to submit PrepareSleep: automatic sleep remains disabled in this build, despite
the selectable/saved timeout. The next increment must connect it only with complete display/touch
shutdown, abort restoration, wake classification, and scan/OTA activity handling. Synchronous network
callbacks cannot sleep while running, but a fresh idle interval after they return still needs wiring.

LVGL MCP guidance and installed 9.1 headers confirm `lv_display_get_inactive_time`,
`lv_display_trigger_activity`, and dropdown selection APIs. Installed SDK headers also expose S3
`ESP_EXT1_WAKEUP_ANY_LOW` and GPIO hold APIs, and pinned TouchLib has a checked `enableSleep()` call.
These are integration evidence for the next step, not proof of physical wake/current behavior.

## UI and drafts

Settings are loaded on the analyzer and copied into `UiState::effective` through results.
UI refreshes never read Preferences. Opening settings seeds the existing EEZ controls from that
copy; closing builds a candidate and submits it. Until acknowledgement, controls revert to effective
values rather than claim the draft was applied. Brightness remains a UI-only preview while editing,
and is restored/applied from the outcome. Drafts open during another operation are not overwritten
when that operation finishes; startup synchronization is the exception.

Callbacks no longer calibrate, write NVS, or drive sensor power. They enqueue and return, or show
busy through the existing message box. Calibration success/failure is displayed when its result is
drained, not at callback return. Reset/clear outcomes synchronize effective settings; failures show
an error. Non-calibration errors no longer include meaningless `nan mv` text.

Acquisition continues while settings are open. Calibration alone pauses normal acquisition while
its fixed 100-sample sequence executes on the analyzer. Existing Wi-Fi scan and OTA callbacks still
block the UI and are explicitly outside this ownership change. The UI log is written from the UI
task (and setup before UI creation), using errors delivered in results or measurement snapshots;
sensor code only writes serial diagnostics. The old bounded-log/active-fault redesign remains later.

## Settings and application

`AnalyzerSettings` is one small RAM value with validation and measurement-semantic comparison.
The name avoids EEZ's own `Settings` type. `SettingsStore` has only initialization/load/save/readiness operations;
legacy UI-callable field setters were removed. The analyzer is its only runtime user.

Both headers live under `src/settings`: `Settings.h` contains the RAM data/defaults/validation;
`SettingsStore.h` persists those values using the SDK's `Preferences`. The NVS namespace remains
`config` for existing stored data; this naming cleanup does not change its keys or format.

Startup loads the individual Preferences keys once. Missing keys use defaults. The sleep-timeout
field has its own fallback as described above. Any other invalid loaded
set that fails validation is visibly replaced with defaults as a whole and startup calibration is
skipped for that boot. Field-specific sanitization and calibration-required presentation are step 4.
Normal cold boot still respects a valid calibrate-on-start preference. No wake suppression exists
yet because application deep sleep is not implemented.

Apply-settings commands validate brightness, pO2 ranges/relationship, and calibration relationships.
Draft calibration fields are replaced by current coefficients before validation, so an old settings
screen cannot undo a calibration. Calibration commands derive a candidate, validate, persist changed
keys, then accept it. Existing SensorManager calibration routines temporarily mutate internal
coefficients, but no acquisition runs concurrently; `apply()` always restores the final effective
settings before another measurement, including after storage or validation failure.

Writes are best-effort per-key. No blob, migration, schema, CRC, or redundant record. If a later
write fails, earlier keys may already be stored; runtime remains unchanged and failure is reported.
After a reported failure (or invalid load), one dirty flag forces the complete candidate to be
written on the next accepted save, preventing an unchanged retry from claiming an unreconciled
store is current. A power interruption before that retry may still leave mixed valid stored values.
No persistent rollback or power-loss transaction guarantee is made.

Reset and optional-clear use the same acknowledged path and now apply immediately. Clearing stores
NaN under the existing optional pure-O2 key. Resetting air also clears a pure-O2 point that no longer
exceeds it. Startup and manual calibration use the same owner validation/persistence path.

## Measurements and freshness

Sensor classes retain the existing conversion formulas in `src/sensors/conversions.cpp`. Normal
gas reads use one conversion, not 20-sample averaging. Calibration still uses five batches of 20
with 100 ms pauses; `RunningAverage` remains only for that calibration path until step 5.

`adc_read.h` is one shared function using unmodified Adafruit start/poll/result calls. The elapsed
deadline is 25 ms at 128 SPS; polls yield with `delay(1)`, and Wire has a 10 ms timeout. Completion
or fetching at/after the deadline is conservatively rejected. Synchronous library calls cannot be
interrupted by this check and may overrun it. Individual I2C failure status is hidden by Adafruit:
a failed exchange may return a plausible value. No register/transport driver is reimplemented.

SensorManager tracks readiness and retry time per ADC. Both initialize independently. Failed/timeout
devices retry no more than once per second when needed; explicit calibration may retry too. Timeouts
skip remaining channels on that ADC, not the other ADC. Invalid numerical data does not cause device
reinitialization. The analyzer owns all ADC work and power GPIO writes; retries do not cycle power.
CO power-start timing is implemented in the analyzer. It records the time only when the CO output
changes from off to on, including startup, re-enable, and resume after preparation. For 3,000 ms it
reports Warming and skips CO conversions entirely; raw mV and ppm remain NaN. Other enabled channels
continue. The first accepted CO read starts after the deadline and must still pass normal checks.
Warm-up is evaluated before the cycle, so a cycle starting just before expiry may defer CO until
the next cycle rather than sample early. ADC-only retry or unrelated settings do not restart the
timer. If only warming CO needs ADC2, no conversion or recovery attempt is made for it until due.

Every snapshot carries cycle-start timestamp, generation, initialized numeric fields, and channel
states for O2/CO/He/temperature: Disabled, Valid, Invalid, Unavailable, and Warming (CO).
UI derives Stale at 1,800 ms
using unsigned elapsed arithmetic. Disabled remains distinct. NaN represents unusable numeric fields;
CO stays float until checked integer display. MOD validates inputs and integer bounds. Raw He mV
is preserved separately from its corrected value; He derivation requires usable O2. Temperature is
sampled only when He is enabled. Existing finite O2 clamping/CO formula behavior remains unchanged.

The generation increments only for applied measurement-semantic changes, skipping zero on wrap.
UI accepts a snapshot only for its acknowledged generation; a future or old snapshot shows unavailable
until a matching result/sample arrives. No measurement numbers are presented before Startup readiness.
Result handling invalidates cached old-generation readings immediately, including MOD and raw mV.
Freshness is recomputed even with no new sample, so a stopped analyzer cannot keep old readings current
while the UI is progressing. A blocked OTA callback can still delay visible updates.

`ReadingStatus.h` remains a thin temporary adapter after generated ticks in the UI task. Invalid,
Unavailable, Warming, and Stale replace existing primary reading labels; disabled panels retain EEZ's hiding
policy. The large O2 font shrinks for text and restores for valid values. It assumes lifetime-stable
generated objects. Generated ticks compare actual text, so recovery restores text even at the same
numeric value. Repeated non-valid overrides can allocate/invalidate labels and need sustained device
testing. The replacement UI should bind states directly, not inherit this workaround. A false
CO-positive colour condition is not proof of a safe measurement.

## Verification boundary

The native suite uses real conversions, sensors, SettingsStore, Analyzer, and UiState with small library/queue
stand-ins. It checks sampling/validity/freshness, independent recovery, candidate application and
failure retention, changed-key writes, startup admission, matching IDs, queue-full refusal, result
backpressure with continued measurements, generation filtering, sleep-ID matching, late acknowledgements,
resume retries, queue clearing, power restoration, and CO warm-up boundaries/recovery. It does not execute actual UI
callbacks or FreeRTOS scheduling. Target debug/release builds use real libraries and generated assets.

Current measured test counts and binary sizes live in progress. Device checks remain required for startup, setting/calibration
navigation, latency, stack high-water marks, ADC fault behavior, status layout, and heap stability.

## Planned next

- Step 3: connect the implemented timeout setting, idle policy, PrepareSleep/Resume and CO timing
  to operation inhibition, display/touch shutdown, GPIO14 wake, and confirmed-wake calibration
  preservation. No automatic sleep is enabled yet.
- Step 4: finish field-specific invalid-load handling, calibration-required state, settings UX and
  reboot/failure acceptance. Do not repeat the ownership refactor already done here.
- Step 5: incremental cancellable stability-gated calibration with graph progress and a qualified
  final window. No general operation scheduler is needed.
- Step 6: replace EEZ after feature parity with the user's actual LVGL Editor export.
- Step 7: bounded chronological log, active faults, and measured efficiency cleanup.