# Current app improvement plan

Updated 2026-09-15. Implementation has started, one small step at a time, from restored `main`
at `ecde0b9`. See [progress and verification](progress.md) for completed work and pending checks.
Earlier redesign build/test results describe discarded work, not completion of this plan.

This document is the current scope and decision source. The old OpenSpec change drafts have been
removed. The earlier architecture is historical reference only. Create a small specification for
the next change when requested; do not recreate the discarded framework.

## Document guide

- Current scope and decisions: this document.
- Current delivery sequence: the numbered steps below.
- Architecture, ownership, and messaging: [architecture](architecture.md).
- Implementation status, decisions, and verification: [progress](progress.md).
- Historical only: [Earlier architecture](historical-simplified-architecture.md),
  [discarded implementation build evidence](historical-dependency-baseline.md), and
  [earlier review](historical-claude-review.md). These are not current requirements or validation.

## Scope and decisions

- Keep PlatformIO and the ESP32-S3 board. Leave `esp32dev` explicitly experimental.
- Use unmodified Adafruit ADS1X15. No driver fork, custom register driver, or transport hierarchy.
  Use its non-blocking conversion API with an application polling deadline and a Wire timeout.
  Accept that individual I2C transfer failures are not exposed and may produce plausible values.
- Preserve current gas formulas, gains, sample rate, polarity, and calibration targets initially.
  Fix invalid inputs and numerical defects separately with focused tests. No gas-model refitting,
  new temperature compensation, sensor/electrical redesign, or accuracy-certification work.
- Remove `RunningAverage` and the application-level 20-conversion batches for normal gas readings.
  Use one fresh ADC conversion per enabled channel per acquisition cycle, keeping the ADC data rate
  unchanged. The ADS1115 hardware provides internal digital filtering; the Adafruit library reads
  its conversion result, rather than performing an equivalent 20-sample software average.
  Removing the extra averaging is a deliberate latency/noise tradeoff, not numerical equivalence.
- The user designs the replacement UI in LVGL Editor. Establish compatibility with LVGL 9.1.0
  using a sample export before integration. Any version change requires an explicit decision.
- Remove EEZ and EEZ Flow only after replacement UI feature parity. Never hand-edit generated
  output; keep application bindings in a small handwritten adapter.
- Calibration must have a live millivolt graph, cancellation, and automatic stability-gated
  completion. These are required improvements, not an indefinite backlog item.
- Add configurable inactivity sleep and button wake. CO has a fixed 5,000 ms startup interval
  (user-adjusted from three seconds before step 3e).
- Keep individual Preferences keys, defaults, and one validated RAM settings value. No settings
  blobs, migration, schema versions, CRCs, redundant records, or atomic multi-key guarantees.
- Preserve existing OTA access. OTA security/protocol redesign and additional product features
  remain deferred. Do not flash, upload, or run OTA during planning.
- Keep the current README unchanged during planning; update user documentation after implementation.

## Design limits

- Two application execution contexts: UI and analyzer. No extra manager, logger, battery, or
  monitoring task. Reuse or block the Arduino loop task as appropriate.
- UI owns widgets and draft settings. Analyzer owns ADCs, sensor power, calibration, effective
  settings, and persistence. Callbacks submit work and return promptly.
- Use fixed-size messages: latest measurement, a small command queue, reliable completion,
  and coalesced calibration progress. No general event framework or multi-operation scheduler.
- Allow one state-changing user operation at a time. Cancellation targets that operation; startup
  calibration uses the same busy state. Define queue-full and late-cancel behavior explicitly.
- Keep one measurement-generation counter to reject cached readings from before settings changes.
  Ordinary settings drafts must not overwrite calibration coefficients.
- Prefer concrete components and free functions. Keep only test boundaries that provide useful
  failure coverage. Do not prescribe a new directory tree or rename working files for appearance.
- Use fixed-capacity application buffers. Do not promise allocation-free internals in third-party
  libraries. Measure actual heap/stack demand before optimizing.

## 1. Establish a small verification baseline

Record and pin the currently working dependency set without upgrading it. Keep debug/release
profiles, a native test command for portable production logic, and one CI workflow for tests and
target builds. Add tests as behavior is changed, not a complete fake infrastructure upfront.
Validate a small Editor export independently so it does not delay urgent correctness fixes.

Acceptance: documented build/test commands, unchanged USB/touch configuration, and an explicit
distinction between local builds, hosted CI execution, and device validation.

## 2. Make readings and failures predictable

Progress: normal-reading batch averaging is removed (step 2a); initialized readings, conversion
guards, checked integer presentation, and cycle error reset are implemented (step 2b).
Latest-only measurement publication and UI-clock stale suppression are implemented (step 2c).
ADC polling deadlines and independent readiness/retry are implemented (step 2d). Explicit channel
state and a temporary status-label adapter are implemented (step 2e). Hardware acceptance, including
sustained status-rendering performance, remains pending; see [verification details](progress.md).

Initialize every measurement field. Distinguish disabled, warming, valid, invalid, and unavailable;
compute staleness in the UI even when acquisition stops. Never replace unknown values with zero
or show the last good value as current. Guard MOD, denominators, non-finite values, and integer
conversions. Require usable O2 for the existing He correction; preserve raw diagnostics.

Use stock Adafruit conversion start/poll/result calls with a timeout. Initialize ADCs independently,
retry failed initialization or observed timeouts with simple backoff, and read temperature only
when needed. Preserve ADC gains and data rate, but replace each 20-conversion gas batch with one
fresh conversion. Neither range checks nor the polling
deadline prove that an individual I2C exchange succeeded.

Remove the normal-reading average buffers and do not replace them with another smoothing filter.
With all channels enabled at 128 SPS, this reduces the conversion count from 61 to 4 per cycle:
approximately 477 ms to 31 ms of nominal conversion time, excluding bus and scheduling overhead.
This reduces acquisition latency but does not automatically increase UI refresh frequency; measure
end-to-end response and review the task wait interval so a fixed delay does not hide the benefit.
Compare steady-input noise and step response on hardware before claiming an improvement in displayed
readings. Do not automatically reintroduce smoothing if noise rises; review the measured tradeoff.

Retire the `RunningAverage` dependency once its calibration uses are also replaced by the bounded
stability window in step 5. That window is evidence for calibration, not normal-reading smoothing.

Acceptance: focused tests for invalid/disabled values, stopped producer, independent readiness,
timeout/recovery, clock wrap, and exactly one fresh conversion per enabled channel (temperature only
when needed). Verify readings are derived from that conversion without a batch average. Record
before/after acquisition latency, displayed step response, and steady-input noise on hardware.
Target stale indication within two seconds of the last usable
sample, including UI refresh latency. Measure disconnect behavior without promising every bus
failure can be detected with the stock library.

## 3. Establish task ownership and add sleep

Decision updated 2026-09-15: bring the full analyzer/UI task restructuring forward from step 4
to avoid accumulating temporary synchronization. Step 3a's interim gate is replaced by step 3b:
one analyzer task, one UI task, and a small fixed command/result path. The gate, `configOpen`,
GUI mutex, separate screen-update task, and arbitrary startup delay are removed.
Step 3c implements analyzer-owned preparation/resume and the fixed CO startup interval. Step 3d
adds the persisted timeout, configuration dropdown, and tested activity/debounce policy. Step 3e
enables S3 automatic entry, display/touch shutdown, abort/resume, and GPIO14 wake with confirmed-wake
calibration preservation. Physical wake/current, UI restoration, and repeated-cycle acceptance remain pending.

The analyzer owns ADCs, sensor-enable GPIOs, calibration, effective RAM settings, and Preferences.
UI callbacks submit one operation at a time and receive an outcome with effective values and a
measurement generation. Acquisition continues while settings are open. Calibration currently runs
its fixed sampling sequence on the analyzer, not the UI; incremental cancellation/progress is step 5.
The UI initializes and updates all graphics objects in one task. Block the unused Arduino loop.

Default timeout: 5 minutes. Options: Off, 1, 2, 5, 10, and 30 minutes; wake on GPIO14/button 2.
Persist the selected timeout; use 5 minutes when the stored value is missing or invalid.
Measure touch/button inactivity, not redraws. Inhibit sleep during calibration, pending settings,
Wi-Fi scans, and OTA using explicit activity flags, not a nested inhibitor registry. Start a fresh
idle interval after operations finish.
Also inhibit sleep while the settings editor contains a draft; do not discard unsaved changes.

Deliver ownership and sleep against the existing UI before the replacement UI. Reuse the analyzer
command path for sleep preparation/resume and owner-driven sensor power timing. No more temporary
shared-access gates. Do not wait for the Editor export or hand-edit generated UI output.

Ask the analyzer to prepare for sleep with an ID and timeout. Reject preparation while busy.
After acknowledgement, power down the display/touch/backlight and radio as supported, then enter
deep sleep. An aborted preparation must restore usability and resume acquisition; ignore late
acknowledgements. Keep this a small explicit sequence, not a general rollback framework.

Validate the expected wake cause plus a simple consumed application-sleep marker. Load persistent
settings and skip automatic startup calibration only on confirmed application wake. Restart the
CO interval. Report missing/invalid calibration; do not present it as an accepted calibration just
because a numerical default exists. Wait for button release before arming and consume/debounce
the wake press. Verify GPIO output hold/pull requirements on the board rather than assuming them.

Acceptance: five-minute default, missing/invalid timeout fallback, Off and all timeout options,
busy inhibition, abort/resume, held button, retained settings, and ten sleep/wake cycles. Actual
wake reliability and current consumption require hardware.

## 4. Finish settings behavior and acceptance

Ownership, RAM settings, complete-candidate validation, checked changed-key writes, acknowledged
reset/clear, and generation filtering have moved into step 3b because the task handoff needs them.
Do not repeat that restructuring. Keep individual Preferences keys and explicit best-effort
persistence: a failed multi-key write may have stored a prefix even though runtime state is retained.
An in-session retry rewrites the complete candidate before reporting success after such a failure.

Finish field-specific invalid-load fallback, calibration-required status, draft/rejection UX, and
device checks for apply/reset/clear across reboot. Currently an invalid loaded set is visibly
replaced with defaults as a whole; refine that without adding versioning or migration machinery.
Check that calibration outcomes cannot overwrite in-progress editable settings drafts, and that
old-generation readings never accompany new coefficients. Keep tests focused on product behavior.

Acceptance: retained settings and reset/clear behavior verified on the device, write failures
reported accurately, and no repeated NVS reads during display updates. No new store framework.

## 5. Complete stability-gated calibration

Use one incremental calibration session for manual and enabled cold-boot calibration. Collect fresh
timestamped millivolt samples into a fixed rolling window. Require minimum sample count/coverage,
low spread, low drift, and a continuous stable dwell. Invalid samples, gaps, or instability reset
qualification. Use an overall timeout; never force acceptance merely because time elapsed.

Feed the window individual fresh conversions, not pre-averaged batches. Replace calibration's
`RunningAverage` objects with this fixed-capacity history and remove the library dependency once
unused. A mean of the final qualified window may still be used to estimate the coefficient; this
does not restore continuous smoothing to normal measurements.

Compute the candidate from the final stable window, validate, persist, then apply. Cancellation,
sampling failure, invalid input, timeout, and reported write failure retain the previous live
calibration. Define late cancellation after successful application as already completed, not undone.
Publish bounded history/progress independently of screen refresh. Thresholds are code constants
chosen from recorded traces, not new user-facing settings. No CO calibration workflow is added.

Acceptance: tests for steady input, noise, slow drift, settling, spikes, duplicate/stale samples,
gaps, timeout, cancellation, and clock wrap. A hidden or slow graph must not change the result.
Keep UI response near 200 ms as a provisional target; measure analyzer response separately.
The former roughly 477 ms batch-acquisition estimate no longer applies after step 2 removes the
20-conversion batches. Measure the single-conversion path, including timeouts and task waits,
before adding scheduling machinery. Use bounded acquisition steps only if needed, without changing
ADC rate or adding another task.

## 6. Integrate the replacement UI

Bind the user's actual Editor export through a thin adapter. Preserve normal/large readings, MOD,
channel settings, calibration/reset, startup calibration preference, brightness, battery, logs,
fault status, OTA access, and sleep configuration. Add the calibration graph, elapsed time,
stability status, Cancel, and a retained final result. Do not invent generated filenames in advance.

Keep interim EEZ changes limited to bindings needed for correctness. Avoid building new temporary
screens. Remove EEZ/Flow and unused assets only after parity and regeneration checks.

Acceptance: compatible export compiles, regeneration preserves handwritten code, navigation stays
responsive, and invalid/disabled/warming states are understandable. Consult the LVGL MCP server
and verify APIs against the selected version before implementation; compatibility is not yet proven.

## 7. Finish diagnostics and measured cleanup

Keep one bounded chronological log and separate active-fault flags. Copy log text safely and
rate-limit duplicates. No logging framework. Sample battery less frequently, initialize persistent
peripheral configuration once, and fix rounding/truncation issues with focused tests.

Measure acquisition duration, UI latency, free heap, stack high-water marks, and sleep current.
Use measurements to decide whether draw-buffer sizing or other optimizations are worthwhile.
Update build, UI generation, calibration, sleep, and troubleshooting documentation after delivery.

## Deferred

- OTA authentication/TLS/rollback/protocol redesign; preserve access and document existing limitations.
- Rotation, remembered screen, translations, units, calculators, and startup-calibration change warnings.
- ADC rate changes/interleaving, polynomial optimization, DMA, and display-driver replacement.
- New gas models, chemistry/hardware research, and additional power modes or wake sources.

## Implementation order

Deliver the baseline and reading fixes (1-2), then task ownership and sleep/wake (3), remaining settings
behavior (4), calibration (5), the replacement UI (6), and measured cleanup (7). Editor compatibility and calibration trace collection can proceed
independently. Keep one current checklist per active change. Do not resume the discarded redesign.
Update architecture and progress alongside meaningful changes. Use Claude for bounded independent
review when useful, particularly sleep and calibration; no OpenSpec scaffolding is required for
small changes.