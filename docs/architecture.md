# Current architecture and intended changes

Updated 2026-09-15. Companion to [the improvement plan](improvement-plan.md).
Implementation progress and verification are recorded in [progress](progress.md).
Sections labelled **planned** are contracts for later work, not descriptions of working firmware.
This document replaces the historical architecture as the active technical reference.

## Implemented today

The application still uses the original EEZ UI, sensor classes, and Preferences-backed `Config`.
Step 1 pins dependencies, adds build/test support, and extracts conversion arithmetic.
Step 2a removes normal-reading batch averaging; calibration still uses its original sampling.

| Responsibility | Current implementation |
| --- | --- |
| Startup | `setup()` initializes configuration, sensors, GPIOs, queue, and tasks |
| UI handler | `Task_LVGL`, core 0, configured 10 KB stack |
| Reading presentation | `Task_Screen_Update`, core 0, configured 3 KB stack; shared GUI mutex and startup delay |
| Acquisition | `Task_Sensors`, core 1, configured 3 KB stack; nominal 500 ms schedule |
| Measurement transport | Five-entry `sensorsData` queue; producer and consumer can block indefinitely |
| Settings/calibration | UI callbacks can access persistence and calibration; `configOpen` is not exclusive ownership |
| ADC access | Unmodified Adafruit ADS1X15, blocking conversion helpers, one conversion per normal gas reading |
| Pure arithmetic | `src/sensors/conversions.h` and `.cpp`; used by sensors and native tests |

The framework `loop()` is empty. A diagnostic task exists behind `DEBUG`, which is not enabled by
the baseline profiles. Stack sizes above are allocations, not measured high-water marks.
There is no command/result protocol, automatic sleep, CO startup deadline, or stability detector yet.
The current queue and shared hardware access are known limitations to address, not reusable APIs
that later changes must preserve.

Normal O2, He, and CO readings no longer allocate average buffers or take 20-conversion batches.
Temperature remains one conversion. Existing O2 calibration validation can skip a reading entirely.
With all channels enabled, a cycle now requests four conversions rather than 61; the theoretical
conversion time at 128 SPS falls from about 477 ms to 31 ms. Bus overhead, task waits, and UI
latency are not included. The 500 ms acquisition schedule and UI refresh logic remain unchanged.
Actual displayed response and increased noise require hardware comparison. O2/He calibration still
collects five batches of 20 samples and pauses 100 ms after each; `RunningAverage` remains only
for these local calibration buffers until step 5 replaces them.

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
fresh conversion per channel. Replace the current blocking helpers with stock Adafruit start/poll/result calls with an elapsed
deadline and the ESP32 Wire timeout. Keep gains and 128 SPS initially. The library's hidden I2C
failures remain an accepted limitation: a returned number is not proof that its transfer succeeded.
Do not introduce custom register access or another ADC library to hide this tradeoff.

## Planned messaging

Use plain fixed-size values, without pointers to widgets, mutable settings, or sample arrays.
No futures, generic event bus, result-reservation service, or multi-operation scheduler.

| Path | Contract |
| --- | --- |
| Measurements | Length-one overwrite queue; UI can skip old samples; acquisition never waits for UI |
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

Each measurement carries a timestamp and settings generation. Increment the generation only when
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

Native tests compile production conversion arithmetic and the actual sensor headers. Seven characterization tests
preserve existing outputs, including the current He/NaN fallback and temperature rounding. They do
not certify those behaviors as correct; step 2 will add explicit tests for intentional fixes.
Tolerance is 0.0001 in each function's output units for floating arithmetic, not sensor accuracy.

Seven sensor-read tests check one conversion per normal read, channel selection, response to the
next input, existing polarity/correction behavior, and unchanged calibration counts/pauses.
Small public-API stand-ins for Adafruit, logging, and calibration averaging live under `test/fakes`.
The averaging stand-in only supports filling a fixed batch, not library ring-buffer behavior.
Both `-I` and `-iquote` paths are native-only; the latter selects the logging stand-in before the
real ESP32 header. Target builds use the real libraries. Host tests do not simulate conversion
timing, prove bus reliability, or measure sensor noise.

Add tests per changed behavior; do not recreate the discarded infrastructure tests. Target builds
validate real library integration and sizes only. Device checks cover noise, displayed response,
calibration traces, sleep current, and wake. LVGL MCP and a user-provided export are required before
new UI/API integration; neither is validated by this document. No LVGL API change is part of step 1.

Claude is available for bounded read-only review of meaningful changes, particularly sleep and
calibration. Record actionable findings and their disposition in progress, not another competing
plan. Do not delegate architectural scope decisions or treat a reviewer opinion as validation.