# Current firmware architecture

Updated 2026-09-18 after calibration, UI, warm-up, and firmware-update improvements. Scope: [improvement plan](improvement-plan.md).
Verification and historical increments: [progress](progress.md).
This describes the current implementation; the final section identifies work still planned.

## Application tasks

```text
UI task (core 0)                      Analyzer task (core 1)
LVGL export, input, display           ADCs, sensor power, calibration
draft settings, effective copy        effective settings, Preferences
               command queue ------>
               <------ result queue
               <------ latest measurement queue
```

`setup()` creates the three analyzer/UI queues and two persistent tasks. The analyzer task creates its own `SettingsStore`,
`SensorManager`, and concrete `app::Analyzer` locally; UI callbacks cannot access those instances.
The UI task owns `DisplayManager`, exported LVGL subjects, widget pointers, and its copy of effective settings.
The Arduino loop blocks indefinitely. There is no extra manager/logger/battery/monitor task. Wi-Fi
scan and firmware installation each use one temporary low-priority worker so network waits do not
block LVGL; a one-entry overwrite queue returns scan results, progress, and completion to the UI task.
`src/network/FirmwareUpdate.cpp` owns the worker, fixed-size event contract, and OTA transport.
`UiAdapter` owns all LVGL callbacks and presentation of those events; workers never call LVGL.

| Task | Work and timing | Allocated stack |
| --- | --- | --- |
| UI | Initialize display/input/generated UI, drain results/readings, render, delay 5 ms | 10 KB |
| Analyzer | Load settings, initialize sensors/power, handle commands, measure, delay 10 ms | 4 KB |
| Network worker | Temporary Wi-Fi scan or firmware download; never calls LVGL | 4 KB scan / 10 KB update |

Normal measurement scheduling waits at least 500 ms from the end of the last cycle; there is no
catch-up loop. Processing a command makes a fresh cycle due. UI presentation checks run around
100 ms apart, with immediate invalidation on settings acknowledgement. These are code intervals,
not measured response bounds; bus, flash, rendering, and scheduling add time.

The old `Task_Screen_Update`, GUI mutex, `configOpen`, `SensorAccess`, and performance-monitor
task were removed. UI initialization is ordered, not delayed by an arbitrary second. `ui::init()`
calls `lvgl_ui_project_init("")` after display/input setup, binds callbacks once permanent screens
exist, seeds unavailable readings, and loads Main. The UI loop calls `lv_timer_handler()` only,
with no generated UI tick. A handwritten adapter translates application data into exported string/int
subjects. LVGL is pinned to 9.5.0; runtime XML is disabled. The generated UI is compiled as a library
directly from `lvgl-ui-project` through its `library.json` manifest and a PlatformIO local symlink
dependency. No custom build script, staging copy, or source rewriting is involved.

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

Commands: apply editable settings, calibrate O2-air/O2-pure/He, cancel the active calibration,
reset air/He, clear optional pure-O2 calibration, PrepareSleep, and Resume. Every result carries the
request type/ID, failure reason, effective settings, measurement generation, and calibration value
where relevant. Calibration progress additionally carries raw millivolts, elapsed time, phase, and
a terminal marker. Startup also carries the independent ADC initialization report.

`Analyzer::service()` first retries its pending result; if delivery remains blocked, it does not
take another command. The task still calls `measure()` when due. Slow UI consumption cannot discard
an outcome or stop ordinary measurement publication unless sleep preparation deliberately suspends it.
Calibration temporarily occupies the analyzer while its timed session runs; UI rendering continues.
Progress is coalesced with `xQueueOverwrite`, while terminal completion remains owner-retained and
retried. Rendering cadence cannot delay sampling or alter calibration acceptance.

A missing startup or terminal result does not cause UI to clear busy on a timer. Doing that could
admit another operation while a supposedly timed-out command still owns the hardware. Queue/task
allocation failures are logged; failed analyzer creation leaves commands unavailable. A permanently
stuck task requires explicit recovery policy later, not silent readmission.

## Sleep preparation and resume

Step 3c added the handshake on the existing queues, without a shared sensor-access flag or a new
task. Step 3e connects it to automatic deep sleep on the primary T-Display-S3 target. The experimental
esp32dev profile does not automatically sleep or use this wake path.

On PrepareSleep with a nonzero ID, the analyzer records the active sleep ID, advances measurement
generation, deasserts He/CO sensor power, clears the measurement queue, and only then produces its
acknowledgement. `measure()` publishes nothing while prepared. Ordinary settings/calibration/reset
commands are rejected Busy before touching storage or hardware. The one-outstanding UI policy
prevents submitting preparation while a calibration or settings operation is outstanding.
An active calibration can be cancelled only by its matching command ID.

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

Prepared proceeds through the power-down sequence below or requests resume on abort/new activity.
A permanently stuck analyzer cannot be
recovered by forgetting the pending ID. Device checks must confirm publication and power restoration,
not infer physical recovery merely from host state-machine tests.

## Sleep setting and activity policy

Step 3d adds `sleepMinutes` to AnalyzerSettings and the individual `sleep_minutes` Preferences key.
Default is five minutes; supported values are Off (0), 1, 2, 5, 10, 30. One option array and one
default constant supply validation, dropdown labels, and index mapping. Missing values load as five;
an invalid stored timeout is replaced with five without discarding other settings. The substitution
is serial-logged and marks the store dirty, so the next successful save reconciles it. No boot-time
write or settings migration is added. Changing sleep timeout does not change measurement generation.

The exported Settings dropdown binds `settings_sleep_index`. Opening settings copies effective values
into subjects; closing maps the selected index through `app::SLEEP_OPTIONS` into the existing
ApplySettings command. Settings is dynamically created and deleted on exit. Subject synchronization
restores effective values on busy/rejected/write-failed requests. DisplayManager has no
UI-specific sleep controls or accessors.

LVGL's display inactivity counter is the only inactivity time source. Touch input already updates
it. DisplayManager exposes its read/reset APIs; UI resets activity after initialization and whenever
an acknowledged operation leaves the app no longer busy. Thus startup, failed settings, calibration
completion, and successful resume each start a fresh inactivity interval. Measurement updates do
not reset it.

On the primary S3 target, UI polls GPIO14/button 2 with its input pull-up. `WakeButton` treats a held
press as activity and requires 50 ms of uninterrupted release before reporting released. Raw changes
and completion of the release interval count as activity. It starts in wait-for-release state, so
the press that caused wake cannot activate a UI control. Button 1 also resets activity while pressed,
but is not a wake source. Elapsed comparisons handle
clock wrap. The other experimental board profile does not yet have this wake-button integration.

`SleepPolicy.h` provides the tested `sleepDue` decision using selected minutes, LVGL inactivity,
an inhibition flag, and the debounced release state. It adds no second idle timer or task. On S3,
the UI evaluates it after its normal input/render pass. Startup, pending operations, network work,
and an open settings editor inhibit entry. Keeping settings open is deliberately more conservative
than the original all-screens policy: an unsaved draft is not discarded by sleeping.

Scan/update callbacks mark network activity and start a temporary worker. The UI task drains its
one-entry event queue, owns all dropdown/subject updates, and displays coalesced download percentages.
Network work inhibits sleep and completion/failure starts a fresh idle interval. A download aborts
after 15 seconds without data and cleans up a partial `Update` session. Measurement
changes never reset inactivity. Off disables automatic entry. A failed submit resets inactivity
instead of immediately retrying sleep every UI tick.

LVGL MCP guidance and installed 9.1 headers confirm `lv_display_get_inactive_time`,
`lv_display_trigger_activity`, and dropdown selection APIs. Installed SDK headers also expose S3
`ESP_EXT1_WAKEUP_ANY_LOW` and GPIO hold APIs, and pinned TouchLib has a checked `enableSleep()` call.
These are integration evidence, not proof of physical wake/current behavior.

## S3 deep-sleep entry and wake

`DeviceSleep.h` contains S3-specific entry/wake functions; `DeviceSleep.cpp` defines the single
RTC-retained marker. Main consumes/classifies the marker before either application task starts.
No additional worker task or generic power-management framework is used.

1. UI submits PrepareSleep only when idle and button 2 has been released for at least 50 ms.
  Analyzer finishes its current bounded work, powers He/CO off, clears queued readings, and acks.
2. UI checks timeout/activity before accepting a late result and again after processing input.
  Touch or either physical button aborts preparation. The retained sleep ID drives Resume; new
  ordinary commands remain blocked until its acknowledgement. Timeout reports a visible dialog.
3. After acknowledgement, UI configures active-low EXT1 wake on GPIO14. RTC peripherals remain on
  for the internal RTC pull-up; the pulldown is disabled. Failure to configure wake aborts entry.
4. DisplayManager checks input, sends panel display-off/sleep-in, writes backlight duty zero,
  detaches PWM, and drives backlight low. After the 120 ms panel interval it checks input again,
  stops touch polling, resets the touch controller for 200 ms low/200 ms high, probes its address,
  rechecks input, and checks TouchLib's `enableSleep()` result. Reset/probe is shared with abort
  recovery; either probe or write failure aborts entry with a stage-specific message. This sequence
  passed the device first-sleep, button-wake, restored-touch, and subsequent-sleep check.
  In the pinned CST-self driver,
  this boolean is inverted: `writeRegister()` returns 0 on success and -1 on failure, and
  `enableSleep()` returns that integer as bool. The adapter treats false as success and true as
  failure. Recheck this contract if TouchLib changes; do not assume conventional boolean semantics.
5. Wi-Fi is stopped if active. The already-low sensor outputs and backlight, plus the board's
  existing power-on output level, are held using checked GPIO hold calls and deep-sleep hold.
  Buttons are checked again, the application marker is written, and `esp_deep_sleep_start()` runs.

No LVGL handler runs while the display/touch are shut down. The final touch check is immediately
before touch is put to sleep; touches after that point cannot cancel, and only GPIO14 can wake.
The short shutdown/rollback delays intentionally block the UI during entry, not during normal
acquisition or the inactivity countdown. Panel commands have no transport acknowledgement; only
touch, wake configuration, and hold APIs expose checked outcomes. Do not claim every hardware
shutdown failure is detectable.

On any returned entry attempt, release holds, disable EXT1, restore the previous Wi-Fi mode if it
was stopped, and restore the display/touch before requesting analyzer Resume. TouchLib's repeated
`init()` does not reset an initialized device, so abort recovery explicitly pulses its reset pin
for 200 ms low/200 ms high and probes the controller address. A failed probe keeps touch polling
disabled and reports a visible error; an ACK is not proof of complete functional recovery. Panel
sleep-out waits 120 ms before display-on and restores effective brightness. The normal brightness
minimum remains unchanged. Wi-Fi reconnection is not guaranteed by restoring its prior mode.

Wake is confirmed only for a deep-sleep reset, EXT1 cause, GPIO14 wake status, and matching RTC
marker. The marker is cleared when inspected, including wrong-cause/invalid-marker cases. Cold
boot and unrelated resets follow the original calibrate-on-start preference. GPIO hold release
follows the SDK's configure-known-output-level-before-release rule; levels are reasserted afterward.
Sensor/backlight outputs start low and the board power-on pin returns to its normal high level.
The wake pin leaves RTC mode before normal input polling. The board rail is not redesigned or
powered off speculatively; retained levels/current still need measurement on the actual board.

On confirmed application wake, the analyzer skips automatic calibration and reloads settings.
On both cold boot and wake, O2-air and He acceptance are independently determined from the raw stored
coefficient before RAM fallback repair. Missing, NaN, or numerically invalid coefficients are not
accepted. An unrelated repaired setting does not invalidate valid calibration. Missing O2 suppresses
O2 and dependent He percentages; missing He suppresses only He. Raw millivolts remain available.
Defaults remain RAM settings values but are not used for affected derived readings until successful
calibration or explicit reset accepts that channel. Reset deliberately accepts the documented default,
not a measured calibration; no extra provenance key is added. Acceptance is persisted even when the
value equals the fallback. An unavailable store reports Storage rather than only CalibrationRequired.
Results carry independent required flags and name enabled channels needing calibration, including
O2 needed for He correction. Pure-O2 calibration requires accepted O2 air. Disabled channels retain
their required flags so enabling one cannot silently accept a fallback.

Basic first sleep, button wake, restored touch and another sleep passed on the device. Step-4b
calibration persistence changes are native-tested/build-verified, not yet device-validated. See the
acceptance checklist in progress for held buttons, aborts, retained settings/pins, and repeated cycles.

## UI and drafts

Settings are loaded on the analyzer and copied into `UiState::effective` through results.
UI refreshes never read Preferences. Opening seeds exported settings subjects from effective settings.
Closing captures editable controls and submits ApplySettings, then restores effective subjects and
brightness while awaiting acknowledgement. A failure or busy/queue refusal shows an ordinary error
dialog and keeps the effective settings; rejected edits are not retained. Reopening always starts
from the latest effective values. There is no retained draft, discard action, or separate save ID.

One editing flag protects controls and brightness preview from asynchronous result synchronization
while the editor is open; startup synchronization remains the exception. Results still update
effective settings and measurement generation. Closing builds the candidate from current effective
settings plus editable controls; the analyzer also strips calibration fields from ApplySettings.
Duplicate close notifications outside an editing session are ignored. Closing an unchanged editor
still submits, allowing the existing dirty-store retry to reconcile a prior partial failure.

An open editor or pending operation inhibits sleep, but failed edits do not inhibit it after exit.
Closing starts a fresh inactivity interval, as does completion of an operation. Opening settings
during sleep preparation still requests Resume. No draft persistence or automatic retry is added.

Callbacks no longer calibrate, write NVS, or drive sensor power. They enqueue and return, or show
busy through the existing message box. Starting calibration immediately opens the generated run
screen; it shows raw mV history, elapsed time, settling/stable state, and Cancel. Saved appears only
after persistence succeeds. Terminal state freezes the graph until Done. Reset/clear outcomes
synchronize effective settings; failures show an error. Non-calibration errors no longer include
meaningless `nan mv` text.

Acquisition continues while settings are open. Calibration alone pauses normal acquisition while
the analyzer samples every 250 ms. Wi-Fi and OTA work runs in a temporary worker and reports through
the UI-owned event queue. The UI log is written from the UI
task (and setup before UI creation), using errors delivered in results or measurement snapshots;
sensor code only writes serial diagnostics. The user declined the bounded-log/active-fault redesign
on 2026-09-17: existing diagnostics and reset-to-clear behavior are sufficient for short sessions.

## Settings and application

`AnalyzerSettings` is one small RAM value with validation and measurement-semantic comparison.
`SettingsStore` has only initialization/load/save/readiness operations;
legacy UI-callable field setters were removed. The analyzer is its only runtime user.

Both headers live under `src/settings`: `Settings.h` contains the RAM data/defaults/validation;
`SettingsStore.h` persists those values using the SDK's `Preferences`. The NVS namespace remains
`config` for existing stored data; this naming cleanup does not change its keys or format.

Startup loads the individual Preferences keys once. Missing keys use defaults. The sleep-timeout
field retains its separate quiet fallback. Step 4a repairs other invalid fields individually and
reports LoadedDefaults, preserving unrelated valid fields and skipping startup calibration for that
boot. Validation and repair share the same numeric predicates. Invalid brightness, out-of-range or
non-finite limits, and invalid calibration values use defaults; invalid pure O2 is cleared. Replacing
invalid air calibration also clears its dependent pure-O2 point. An invalid bottom pO2 uses the lower
of its default and a valid deco limit. For an in-range but conflicting pair, bottom is preserved and
deco is raised only to match it. Other invalid deco values use the default. No repair writes NVS
during load; the existing dirty-save path reconciles values on the next accepted save.
Calibration validity is cached independently before repairs. Missing or NaN coefficient values use
RAM defaults without marking the load corrupt; NaN is the persisted unaccepted marker. Other invalid
numeric coefficients still trigger visible repair. An optional pure-O2 point without accepted air is
discarded in RAM and explicitly cleared in storage when air is first accepted, preventing resurrection.
Normal cold boot still respects a valid calibrate-on-start preference. Confirmed application wake
skips it. A general repaired-load warning still suppresses automatic calibration for that boot;
intentional NaN markers alone do not suppress it. Failed sampling/persistence keeps the prior acceptance.

Apply-settings commands validate brightness, pO2 ranges/relationship, and calibration relationships.
Draft calibration fields are replaced by current coefficients before validation, so an old settings
screen cannot undo a calibration. Calibration sessions derive a candidate from their final qualified
window, validate and persist it, then update live coefficients. Sampling, validation, or storage
failure leaves the previous effective settings in place.

Writes are best-effort per-key. No blob, migration, schema, CRC, or redundant record. If a later
write fails, earlier keys may already be stored; runtime remains unchanged and failure is reported.
After a reported failure (or invalid load), one dirty flag forces the complete candidate to be
written on the next accepted save, preventing an unchanged retry from claiming an unreconciled
store is current. A power interruption before that retry may still leave mixed valid stored values.
No persistent rollback or power-loss transaction guarantee is made.

Save receives explicit O2/He acceptance only from calibration or reset commands. Ordinary saves and
forced retries serialize NaN for unaccepted channels rather than their valid-looking RAM fallbacks.
The cached acceptance flags and live generation change only after all writes report success. On a
failed partial calibration write, an ordinary save retry reconciles the old unaccepted state. A power
loss before that retry can still leave a valid coefficient prefix accepted at the next boot; existing
per-key storage cannot provide a stronger guarantee. Older firmware's already-stored valid-looking
defaults cannot be distinguished from accepted values retroactively. No calibration lineage is claimed.

Reset and optional-clear use the same acknowledged path and now apply immediately. Clearing stores
NaN under the existing optional pure-O2 key. Resetting air also clears a pure-O2 point that no longer
exceeds it. Startup and manual calibration use the same owner validation/persistence path.

## Measurements and freshness

Sensor classes retain the existing conversion formulas in `src/sensors/conversions.cpp`. Normal
gas reads use one conversion, not 20-sample averaging. Manual and enabled cold-boot calibration use
individual fresh conversions in the same fixed rolling window. Stability requires both bounded range
and bounded drift between the older and newer halves. Final candidates must be 5-20 mV for O2 air,
30-100 mV for pure O2, and at least 400 mV for helium; rejected candidates do not change live or
stored calibration. `RunningAverage` is no longer a dependency.

`adc_read.h` is one shared function using unmodified Adafruit start/poll/result calls. The elapsed
deadline is 25 ms at 128 SPS; polls yield with `delay(1)`, and Wire has a 10 ms timeout. Completion
or fetching at/after the deadline is conservatively rejected. Synchronous library calls cannot be
interrupted by this check and may overrun it. Individual I2C failure status is hidden by Adafruit:
a failed exchange may return a plausible value. No register/transport driver is reimplemented.

SensorManager tracks readiness and retry time per ADC. Both initialize independently. Failed/timeout
devices retry no more than once per second when needed; explicit calibration may retry too. Timeouts
skip remaining channels on that ADC, not the other ADC. Invalid numerical data does not cause device
reinitialization. The analyzer owns all ADC work and power GPIO writes; retries do not cycle power.
GPIO10 controls the shared 5 V rail for the CO sensor and the LM35 helium-temperature sensor. The
analyzer keeps this rail on when either CO or He is enabled. It records the power-start time only
when the rail changes from off to on, including startup, true re-enable, and resume after preparation.
During the first 14,000 ms, an enabled valid CO sample is classified Warming only when ppm exceeds
zero; at zero ppm it is Valid. CO is sampled throughout this interval, so raw mV remains available
while the primary value shows Warming. Disabling CO while He remains enabled does not power-cycle the
rail or restart the timer. Constants are centralized with calibration policy in
`src/app/AnalyzerPolicy.h`.

He is classified Warming when both its derived reading and temperature are valid and temperature is
below the policy threshold. Invalid/unavailable temperature does not overwrite an independent He
failure state. Raw He mV remains visible while warming. The threshold is in
`src/app/AnalyzerPolicy.h`.

Every snapshot carries cycle-start timestamp, generation, initialized numeric fields, and channel
states for O2/CO/He/temperature: Disabled, Valid, Invalid, Unavailable, and Warming (CO and He).
UI derives Stale at 1,800 ms
using unsigned elapsed arithmetic. Disabled remains distinct. NaN represents unusable numeric fields;
CO stays float until checked integer display. MOD validates inputs and integer bounds. Raw He mV
is preserved separately from its corrected value. He applies the O2 correction when O2 is enabled
and valid; when O2 is disabled, it uses the uncorrected baseline. Temperature is sampled only when
He is enabled. Existing finite O2 clamping/CO formula behavior remains unchanged.

The generation increments only for applied measurement-semantic changes, skipping zero on wrap.
UI accepts a snapshot only for its acknowledged generation; a future or old snapshot shows unavailable
until a matching result/sample arrives. No measurement numbers are presented before Startup readiness.
Result handling invalidates cached old-generation readings immediately, including MOD and raw mV.
Freshness is recomputed even with no new sample, so a stopped analyzer cannot keep old readings current
while the UI is progressing. Network work runs outside the UI task, so scan, connection, and download
waits do not delay visible updates.

`UiAdapter` publishes formatted text through exported subjects. Invalid, Unavailable, Warming,
and Stale replace primary numeric readings; disabled sensor sections are hidden on Main and Large.
Large O2 and He fonts shrink for status text and restore for valid values. Raw mV remains visible for Warming but stale
snapshots suppress it. Main groups Bottom/Deco MOD with O2 and places battery in the header. A
danger-coloured warning button appears when the UI log has Warning/Error severity and opens the
generated Diagnostics screen. A valid displayed CO value above zero uses the danger colour and
returns to inherited theme colour at zero. The obsolete main status subject is removed.

`src/display/UiFeedback.cpp` translates analyzer results into UI log entries and message boxes.
`UiLog` keeps its fixed-capacity circular buffer behind a mutex and copies diagnostics into a
caller-owned fixed-size snapshot while locked; it does not return pointers into mutable shared storage.
Invalid measurement entries identify each affected channel and include its raw reading when available.
`src/display/Battery.cpp` owns the cached ADC characterization and voltage sampling used by the UI task.

Large O2/He values use the exported 60 px H1 font; non-valid state text falls back to the body font.

Calibration, Calibration Run, Firmware Update, and Diagnostics are generated Editor screens owned
by `UiAdapter`; generated C is never edited manually. Transient screens use application-bound
callbacks and are deleted on exit. Cancel/Done visibility is managed directly on transient objects,
avoiding global subject observers that could outlive a deleted screen in LVGL 9.5.

## Firmware updates

The update screen shows the application-owned compile-time version, using `dev` locally and the
release CalVer in published builds. OTA downloads the latest GitHub Release `firmware.bin` in a
temporary worker in `src/network/FirmwareUpdate.cpp` and reports fixed-size progress events to the
UI task. `UiAdapter` alone reads update widgets and writes exported update subjects.

Before HTTPS begins, the worker synchronizes UTC with a ten-second bound. `WiFiClientSecure`
authenticates GitHub and its release-asset redirect using the roots in
`src/network/TrustedRoots.h`; time or TLS failure cancels the update. Release automation and
version injection are detailed in improvement-plan step 8.

## Verification boundary

The native suite uses real conversions, sensors, SettingsStore, Analyzer, and UiState with small library/queue
stand-ins. It checks sampling/validity/freshness, independent recovery, candidate application and
failure retention, changed-key writes, startup admission, matching IDs, queue-full refusal, result
backpressure with continued measurements, generation filtering, sleep-ID matching, late acknowledgements,
resume retries, queue clearing, power restoration, CO warm-up boundaries/recovery, consumed markers,
confirmed-wake calibration handling, and ten simulated wake cycles. It does not execute actual UI
callbacks or FreeRTOS scheduling. The separate `native-ui` suite uses real LVGL and exported assets
to test the adapter's navigation, settings, presentation, rendering, and dynamic-screen lifetimes.
Target debug/release builds use real libraries and generated assets.

Current measured test counts and binary sizes live in progress. The related firmware behavior and UI
have passed physical validation.

## Planned next

- Steps 3-6 are implemented and physically validated. Revalidate affected behavior after future
  hardware, calibration-policy, or Editor changes.
- Step 7: one-time battery ADC characterization is complete. No additional efficiency cleanup is
  planned without a concrete device issue; retain existing diagnostics and reset-to-clear behavior.