# Current architecture and intended changes

Updated 2026-09-15. Companion to [the improvement plan](improvement-plan.md).
Implementation progress and verification are recorded in [progress](progress.md).
Sections labelled **planned** are contracts for later work, not descriptions of working firmware.
This document replaces the historical architecture as the active technical reference.

## Implemented today

The application still uses the original EEZ UI, sensor classes, and Preferences-backed `Config`.
Step 1 pins dependencies, adds build/test support, and extracts conversion arithmetic.
Step 2a removes normal-reading batch averaging; calibration still uses its original sampling.
Step 2b initializes readings, guards conversions and integer presentation, and resets cycle errors.
Step 2c adds latest-only publication and UI-clock freshness checks without changing task ownership.
Step 2d uses deadline-polled Adafruit conversions and independent per-device initialization/retry.

| Responsibility | Current implementation |
| --- | --- |
| Startup | `setup()` initializes configuration, sensors, GPIOs, queue, and tasks |
| UI handler | `Task_LVGL`, core 0, configured 10 KB stack |
| Reading presentation | `Task_Screen_Update`, core 0, configured 3 KB stack; shared GUI mutex and startup delay |
| Acquisition | `Task_Sensors`, core 1, configured 3 KB stack; nominal 500 ms schedule |
| Measurement transport | One-entry overwrite `sensorsData` queue; UI polls without blocking and caches the latest sample |
| Settings/calibration | UI callbacks can access persistence and calibration; `configOpen` is not exclusive ownership |
| ADC access | Unmodified Adafruit start/poll/result API, 25 ms elapsed deadline, one conversion per normal gas reading |
| Pure arithmetic | `src/sensors/conversions.h` and `.cpp`; used by sensors and native tests |

The framework `loop()` is empty. A diagnostic task exists behind `DEBUG`, which is not enabled by
the baseline profiles. Stack sizes above are allocations, not measured high-water marks.
There is no command/result protocol, automatic sleep, CO startup deadline, or stability detector yet.
Shared hardware access, synchronous library transfers, and blocking UI callbacks remain known limitations, not reusable APIs
that later changes must preserve. Queue or GUI-mutex allocation failure halts application startup
before tasks start and is reported through serial logging; no on-screen fault is available then.

Normal O2, He, and CO readings no longer allocate average buffers or take 20-conversion batches.
Temperature is one conversion only while He is enabled. Existing O2 calibration validation can skip a reading entirely.
With all channels enabled, a cycle now requests four conversions rather than 61; the theoretical
conversion time at 128 SPS falls from about 477 ms to 31 ms. Bus overhead, task waits, and UI
latency are not included. The 500 ms acquisition schedule and UI refresh logic remain unchanged.
Actual displayed response and increased noise require hardware comparison. O2/He calibration still
collects five batches of 20 samples and pauses 100 ms after each; `RunningAverage` remains only
for these local calibration buffers until step 5 replaces them.

### ADC deadlines and recovery implemented in step 2d

`src/sensors/adc_read.h` contains one inline `readCounts` function shared by normal readings and
calibration. It calls unmodified Adafruit `startADCReading`, `conversionComplete`, and
`getLastConversionResults`. The library still owns register configuration, channel constants,
gain, data rate, and volts scaling. No custom ADC class, register driver, or transport hierarchy.

The deadline is 25 ms elapsed from before conversion start at the explicitly configured 128 SPS.
Polls yield with Arduino `delay(1)`. Unsigned subtraction handles clock wrap. A read is accepted
only if completion and result fetching finish before the deadline; completion exactly at 25 ms or
a late result is conservatively rejected. Output counts are untouched on failure. A timeout
returns NaN from the sensor, not a partial or previous reading. This deadline must be reconsidered
if the ADC data rate is changed; slower rates are not supported by this policy as configured.

Wire1 uses a 10 ms bus timeout. The elapsed deadline cannot interrupt an Adafruit/Wire call already
in progress: the whole read or initialization can take longer than 25 ms. The stock driver hides
individual transfer errors, so a disconnected ADC can still return a plausible result and need not
trigger timeout/recovery. Neither host tests nor range validation remove that accepted limitation.

`SensorManager` keeps just readiness and a last-attempt timestamp per ADC. Startup initializes both
independently. Failure leaves the device unready; normal cycles retry it at most once per second,
only when an enabled channel needs it. Each attempt reapplies the original gain and 128 SPS rate.
No exponential retry state or power cycling is added. Retrying never changes sensor-enable GPIOs.

An observed conversion timeout marks only that ADC unready, starts its retry interval, and skips
its remaining channels for the cycle. The other ADC continues. Invalid numerical data does not
request initialization. The old global error-count/reinitialize-both loop is removed. The existing
summary error prioritizes an observed timeout over an unavailable device over invalid data;
individual channel reason codes remain pending. `ADC_Init_Failed` is presented as "ADC unavailable"
because it also represents waiting for retry after a runtime timeout.

Calibration requests may perform the same rate-limited device retry even while `configOpen` pauses
ordinary acquisition. They fail immediately if still unready. Every calibration sample uses the
deadline helper; any timeout abandons the batch and returns NaN. The manager keeps previous live
coefficients on non-finite calibration results. Existing UI callbacks reject NaN before writing
Preferences. Successful sampling still follows the existing non-transactional application path;
whole-candidate validation, cancellation, persistence failure handling, and stability gating remain
planned. A failed cold-boot calibration is not automatically restarted after later ADC recovery.

The existing shared-access race between calibration callbacks and in-flight acquisition is not
solved by polling deadlines. Readiness is not a replacement for single-owner coordination. That
coordination is required before sleep and the later ownership milestone; no thread-safety claim is
made here. Adafruit `begin()` may allocate internally during retry, as in the original library.

### Reading validity implemented in step 2b

Every reading field, including CO ppm and temperature, defaults to `NaN`. Enable flags default to
false. CO ppm stays a float in the snapshot; only presentation converts a finite in-range result
to an integer. MOD validates positive finite inputs and the final integer range before conversion.
The integer range check uses double comparisons to avoid rounding `INT_MAX` up to an unsafe float.
The existing EEZ globals accept `FloatValue(NAN)` without an implicit cast; their float-to-text
formatter emits empty text. Generated UI files are untouched.

O2 rejects non-finite inputs, raw values outside its existing 0..100 mV range, air calibration below
the existing 5 mV minimum, and a present pure-O2 calibration that is non-finite or not above air.
NaN pure-O2 calibration still means absent. Finite O2 outputs retain their existing 0..100 clamp.
He requires finite O2 in 0..100, nonnegative corrected input, a positive finite coefficient, and a
finite representable result. Raw He millivolts are now retained separately from the correction used
to calculate percentage. This intentionally changes the diagnostic millivolt display at high O2.
CO rejects negative/non-finite millivolts but retains its existing finite out-of-range formula
outputs; no new gas-model limits are introduced. Temperature rounding remains unchanged, with
non-finite/overflow protection.

He enabled without usable O2 reports an invalid derived value while retaining raw He and temperature.
It no longer silently uses an uncorrected branch. The cycle checks derived validity, starts each
pass with no error, and treats all-disabled as normal. Invalid readings do not trigger the old
global ADC reinitialization counter; duplicate task-level UI warnings are suppressed until the
error changes. Per-sensor debug logs and the historical log indicator are not redesigned here.

The EEZ `co_value > 0` condition controls CO-positive colouring and is false for NaN. An empty
reading is not a zero-CO result; colour is not an all-clear signal. On-device verification of all
affected views is pending. Explicit disabled/fault/warming presentation and active-fault tracking
are still planned. Stale numeric suppression is implemented in step 2c below.

### Latest measurements and freshness implemented in step 2c

`SensorManager::readSensors()` timestamps the start of each acquisition cycle using Arduino
`::millis()`, including disabled and invalid cycles. A slow cycle cannot make an early channel
appear newly measured merely because publication was late. Both publication paths use
`xQueueOverwrite` on the single-entry queue: a stopped consumer cannot block acquisition or build
a backlog. This is deliberately lossy telemetry, not a command/completion transport.

The presentation task retains one latest snapshot, polls the queue without waiting, and checks
freshness every pass, including when no new snapshot arrives. `sensorsData::forDisplay()` returns
a copy with all numbers set to NaN once unsigned elapsed time reaches 1,800 ms. The original cache
is unchanged; a new fresh snapshot restores readings. Raw diagnostics and derived MOD also blank.
Default snapshots contain no numbers, so the initial timestamp of zero cannot fabricate a reading.

The task sleeps 100 ms between passes and waits at most 20 ms per GUI mutex attempt. The 1,800 ms
cutoff leaves scheduling/rendering margin toward the two-second stale-display target. Freshness
is evaluated after acquiring the mutex. Numeric globals update only for a pending sample or a
freshness transition, not on every poll; failed lock attempts leave the update pending. Battery
sampling stays tied to pending readings, not freshness-only updates. Existing battery values and
historical log state are not sensor-freshness indicators.

This does not guarantee a two-second visible update if a long callback holds the GUI mutex or
rendering stops. It prevents waiting for the producer, not waiting for the whole graphics system.
With the current `configOpen` pause, readings now blank during extended settings/calibration pauses;
they are not live readings during that pause. Hardware checks must exercise pause/resume and stalled
producer behavior. The later single-UI-owner change removes the remaining shared-mutex dependency.
There is no separate stale reason field or settings-generation filter yet.

## Planned ownership

Use two application contexts, not a general application-manager framework:

```text
UI task                       Analyzer task
widgets and draft settings    ADCs and sensor power
input and rendering           acquisition and calibration
sleep initiation              effective settings and Preferences writes
             commands --->
             <--- latest measurement, completion, calibration progress
```

Keep generated UI names behind handwritten bindings. One UI context will initialize and update
widgets; remove the second UI task and mutex only after moving all callers. Reuse or block the
Arduino loop task. No separate logger, battery, or permanent monitoring task is planned.
Choose stack sizes and scheduling from measured behavior; no core-affinity change is required.

The analyzer will own state changes between bounded units of work. Normal readings already use one
fresh conversion per channel through stock Adafruit start/poll/result calls with an elapsed
deadline and the ESP32 Wire timeout. Keep gains and 128 SPS initially. The library's hidden I2C
failures remain an accepted limitation: a returned number is not proof that its transfer succeeded.
Do not introduce custom register access or another ADC library to hide this tradeoff.

## Messaging target and current subset

Use plain fixed-size values, without pointers to widgets, mutable settings, or sample arrays.
No futures, generic event bus, result-reservation service, or multi-operation scheduler.

| Path | Contract |
| --- | --- |
| Measurements (implemented) | Length-one overwrite queue; UI can skip old samples; acquisition never waits for UI |
| Commands | Small FIFO; zero-wait submission; full means not submitted, visibly reported |
| Completion | Separate bounded path; retain an undelivered result and retry without stopping ordinary measurements |
| Calibration progress | Latest fixed-capacity snapshot; may be overwritten, never displaces completion |

Only one user state-changing operation is outstanding until its terminal result is consumed.
UI disables conflicting actions; the analyzer still checks its own state. Startup calibration
must publish its busy state and be accounted for even though it was not requested by the UI.
Choose exact queue capacities when implementing the protocol and test the startup/request race;
do not assume a large queue guarantees delivery.

Commands are a closed set: apply editable settings, start calibration for a supported target,
reset/clear calibration, prepare sleep, and resume an aborted sleep. Cancellation addresses the
active calibration ID; it is not a new calibration operation. Repeated cancellation must not create
an unbounded stream of results. A request ID identifies an operation, not a heap-owned handle.

One terminal outcome reports completed, rejected, failed, or cancelled, with a compact reason and
the effective values needed by the UI. Enqueue success is not application success. A late cancel
cannot undo a calibration already applied: keep its actual completion and effective coefficient.
Do not discard a committed result merely because the UI has since requested cancellation.

Timestamps and UI-clock freshness are implemented. A settings generation is planned. Increment it only when
measurement meaning changes, and include it in completion. UI checks freshness using its own clock
even if no new sample arrives; cached old-generation values must not survive acknowledgement.
Use wrap-safe elapsed-time comparisons. Disabled/warming/invalid/unavailable are explicit states,
not numbers inferred from zero. Ordinary settings drafts cannot change calibration coefficients.

## Planned settings and calibration

Keep one validated RAM settings value and individual Preferences keys. Load once, validate related
fields together, write changed keys, then apply the candidate only after reported write success.
On a reported failure, preserve previous runtime settings and show failure. Earlier writes may
already be persistent; validate the resulting set at reboot. No storage rollback, migration,
schema, blob, CRC, or redundant record machinery is required.

One incremental calibration session handles manual and enabled cold-boot calibration. It consumes
individual fresh timestamped conversions, checks spread/drift/coverage/gaps/dwell, and times out
instead of force-accepting. Store its history in a fixed-capacity window, independent of UI refresh.
Estimate the coefficient from the final qualified window, validate, persist, then apply. This window
is calibration evidence, not a replacement smoothing filter for normal readings. Calibration
failure or cancellation before persistence must not mutate live coefficients.

The three-second CO interval starts at actual power assertion, including re-enable and wake.
ADC reinitialization alone does not restart it. The first usable CO conversion starts after the
deadline. Sensor-specific calibration/stability thresholds require recorded traces.

## Planned early sleep integration

Sleep is step 3, before the full ownership refactor and new UI. Implement just the coordinated
stop/resume and sensor-power ownership needed to avoid racing in-flight reads; a shared pause flag
alone is insufficient. Reuse that integration in step 4, rather than creating a temporary framework.
Expose the timeout through a small handwritten binding to the existing UI until the new export exists.

Default: five minutes; options Off, 1, 2, 5, 10, 30. Missing/invalid timeout uses five minutes.
User input resets inactivity; measurement updates do not. Calibration, pending settings, scan,
and OTA inhibit sleep via named activity flags. Restart the idle interval after completion.

Use a short state sequence: awake, awaiting preparation, entering sleep. Only an acknowledgement
for the active sleep ID permits power-down. Busy preparation is rejected. Timeout or new input
aborts the attempt; resume acquisition, restore any affected peripherals, and ignore late sleep
acknowledgements. Reserve a way to submit resume while ordinary commands are blocked. Confirm
acquisition resumed before calling the abort recovered; do not claim recovery from a permanently
stuck task. An abort that cycled CO power restarts its warm-up and invalidates pre-sleep readings.

GPIO14/button 2 wakes from deep sleep. Require release before arming and consume/debounce the wake
press. Classify application wake with the expected wake cause and a consumed retained marker;
skip automatic startup calibration only for that wake. Load calibration from Preferences, not RTC
copies. Missing/invalid calibration must be reported instead of presented as accepted calibration.
Board power-down, GPIO hold/pulls, wake reliability, and actual current require device verification.

## Verification and boundaries

Native tests compile production conversion arithmetic, actual sensor headers, and `sensors.cpp`.
Twelve conversion tests cover valid-output characterization plus NaN/infinity, invalid denominators,
overflow, integer boundaries, and MOD. The former He/NaN fallback test now requires an unavailable
result. Temperature rounding and valid gas formulas remain characterized.
Tolerance is 0.0001 in each function's output units for floating arithmetic, not sensor accuracy.

Thirty-one sensor/cycle tests check one conversion per normal read, channel selection, response to the
next input, raw diagnostics, default snapshots, invalid-to-valid recovery, all-disabled and He-only
configurations, temperature gating, and unchanged calibration counts/pauses.
They also exercise latest-only queue consumption, timestamped disabled/invalid cycles, stopped
producer blanking, exact freshness boundaries, clock wrap, empty boot state, and restored values.
ADC cases cover conversion deadlines, late results, initialization failure isolation, one-second
retry across clock wrap, disabled-device gating, timeout skipping of sibling channels, invalid
data without reinitialization, calibration retries, and partial-calibration failure retention.
Small public-API stand-ins for Adafruit, a copying queue, logging, and calibration averaging live under `test/fakes`.
The averaging stand-in only supports filling a fixed batch, not library ring-buffer behavior.
Both `-I` and `-iquote` paths are native-only; the latter selects the logging stand-in before the
real ESP32 header. Target builds use the real libraries. Host tests do not simulate conversion
real bus timing, prove bus reliability, or measure sensor noise. The fake clock advances during
polling and can model delayed completion/result fetching. The queue stand-in models one copied slot;
target builds verify the real FreeRTOS calls. Tests do not execute the actual UI task or establish
display/mutex scheduling latency.

Add tests per changed behavior; do not recreate the discarded infrastructure tests. Target builds
validate real library integration and sizes only. Device checks cover noise, displayed response,
calibration traces, sleep current, and wake. LVGL MCP and a user-provided export are required before
new UI/API integration; neither is validated by this document. No LVGL API change is part of step 1.

Claude is available for bounded read-only review of meaningful changes, particularly sleep and
calibration. Record actionable findings and their disposition in progress, not another competing
plan. Do not delegate architectural scope decisions or treat a reviewer opinion as validation.