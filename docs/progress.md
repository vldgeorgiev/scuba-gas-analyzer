# Implementation progress and decisions

Current scope: [improvement plan](improvement-plan.md).
Technical reference: [architecture and messaging](architecture.md).
Historical documents describe the discarded redesign, not acceptance evidence for this work.

## Status

| Step | Status |
| --- | --- |
| 1. Build/test baseline | Implemented and locally verified; Claude review resolved; hosted CI, device checks, and Editor export pending |
| 2. Measurement handling and averaging removal | Software increments 2a-2e implemented and locally verified; on-device acceptance and temporary status-adapter performance checks pending |
| 3. Task ownership and sleep/wake | Software implemented through 3e: automatic S3 sleep/wake enabled; physical acceptance pending |
| 4. Remaining settings behavior | Field-specific fallback, independent calibration acceptance, and save-on-exit implemented; device acceptance pending |
| 5. Stability-gated calibration | Manual session, progress, cancellation, persistence-gated Saved state, and graph implemented; cold-boot unification, threshold tuning, and device acceptance pending |
| 6. Replacement UI | Generated Main/Large/Settings/Calibration/Run/Update/Diagnostics screens active; host/build verification passes; device acceptance pending |
| 7. Measured cleanup | Not started; diagnostics redesign declined on 2026-09-17 |

## 2026-09-17: Scope narrowed after calibration and diagnostics discussion

- User confirmed drift detection is worth adding to calibration's rolling-range check. Extra
  sample-gap qualification and stable-dwell changes are not required for now; preserve the
  five-second earliest completion and ten-second limit. Implementation and trace tuning remain open.
- User declined diagnostics improvements: short operating sessions and reset-to-clear errors are
  sufficient. Keep the existing Diagnostics screen/logging; remove the proposed new log buffer,
  separate active-fault state, recovery history, and duplicate suppression from planned work.
- Measured efficiency cleanup remains separate. These are planning changes only; firmware unchanged.

## 2026-09-17: Calibration, generated screens, and warm-up presentation

- Replaced handwritten Calibration, Firmware Update, and Diagnostics dialogs with generated Editor
  screens owned by `UiAdapter`. Calibration starts immediately and opens a generated run screen with
  a 41-point raw-mV chart, current value, elapsed time, stability phase, Cancel, and Done.
- Analyzer now owns a 250 ms manual calibration session. A 2 s rolling range may save no earlier than 5 s;
  an unstable session fails at 10 s. Cancellation, invalid samples, timeout, and persistence failure
  preserve the prior calibration. Saved is published only after persistence succeeds, and terminal
  UI state freezes the graph. Stability constants remain code values pending measured trace tuning.
- Enabled cold-boot calibration still uses the legacy synchronous 100-sample path. Routing it through
  the timed session and removing the remaining `RunningAverage` dependency are still open.
- Main uses larger primary readings, groups Bottom/Deco MOD under O2, places battery in the header,
  and exposes a danger-coloured warning button that opens Diagnostics. Valid displayed CO above zero
  is highlighted and returns to inherited colour at zero. The obsolete main status subject and
  Calibration result label were removed.
- Large O2/He values use the exported 60 px H1 font. Diagnostics navigation is available from Main
  and Calibration. Transient Cancel/Done flags are application-owned to avoid stale LVGL 9.5 observers.
- CO is sampled during its first 12 s after power-on. It reports Warming only when ppm is above 10,
  while retaining raw mV. He reports Warming below 30 C while retaining raw mV. Thresholds live with
  analyzer timing and sensor classification respectively.
- Latest verification: 111 native tests and four real-LVGL tests pass; `t-display-s3` builds with
  LVGL 9.5.0. No device flash or physical acceptance was performed by the assistant.

## 2026-09-16: Exported UI activated

- User explicitly approved replacing EEZ now, without waiting for complete Editor screen parity.
- Both S3 environments build the `lvgl-ui-project` export against LVGL 9.5.0. The retired EEZ project,
  generated library/assets, unused widget/font fakes, and `lib_ignore = ui` exclusions are removed.
  Earlier dated entries below describe the previous implementation, not active dependencies. The native sensor tests
  remain isolated from LVGL; `native-ui` runs the actual generated UI and adapter with LVGL 9.5.0.
- `UiAdapter` initializes assets/subjects and both permanent reading screens, replaces generated
  navigation callbacks after initialization, opens/saves/deletes Settings, and publishes live text.
  It preserves unavailable/stale/disabled/warming states, raw mV diagnostics, He temperature, guarded
  MOD, checked CO integer conversion, battery, and explicit CO warnings. Large fault text uses a
  smaller font instead of truncating large-digit status messages.
- Settings still use the existing application session and analyzer command/result path. Drafts
  start from effective settings; index validation preserves calibration coefficients. Queue rejection
  and storage failure restore effective values and report an error. No retained rejected drafts.
- Calibration/reset/startup preference, logs, and OTA use handwritten action dialogs for now.
  Diagnostics is in the Calibration menu; taps elsewhere still switch Main/Large. No new analyzer
  task, graph, cancellation, or stability algorithm was added. Wi-Fi/OTA callbacks remain blocking;
  transport security is unchanged, but passwords are no longer logged.
- RGB565 draw buffers are explicitly sized and aligned to `LV_DRAW_BUF_ALIGN`. A real host run
  exposed an alignment assertion that compile-only checks missed.
- The initial export contained a logo variable shadowing its asset name. The user re-exported the
  renamed XML object, resolving the collision. PlatformIO now consumes the export directly through
  `lvgl-ui-project/library.json` and a local symlink dependency. The Python script, staging copies,
  and generated-source workaround are removed. Generated files are not manually edited.

Verification:

- Direct-library integration: both S3 profiles build and all four real-LVGL host tests pass with
  the fresh Editor export. Static RAM is 144,920 bytes; release flash is 1,434,817 bytes.
- EEZ cleanup verification: all 109 tests passed after deleting the legacy files and test fakes;
  both S3 profiles also passed a clean rebuild without the former library exclusions.

```sh
pio test -e native -e native-ui
pio run -e t-display-s3 -e t-display-s3-release
```

- 109 tests passed: 105 portable production tests and four real-LVGL integration tests.
- Host checks cover screen creation, image source, nonblank RGB565 rendering, bubbled screen taps,
  action isolation, settings index conversion/rejection, stale/raw reading handling, font restoration,
  and 20 dynamic Settings cycles without heap loss. They invoke LVGL events directly, not physical touch.
- Both S3 builds passed. Static RAM: 144,932 bytes (44.2%). Release flash: 1,436,529 bytes (21.9%).
  These are linker totals, not measured runtime free heap or stack headroom.
- No flashing, upload, OTA, or device tests performed. Action dialogs/keyboard, on-device layout,
  warning visibility, persistence across reboot, and sleep/wake remain acceptance gates.

## 2026-09-15: Step 1

- Started from restored `main` at `ecde0b9`, with no tracked implementation changes.
- Pinned the resolved platform, direct libraries, BusIO, and TouchLib commit without upgrading.
- Added a release profile differing only in diagnostic level (debug 4, release 1).
- Added one isolated native environment, one conversion source pair, and seven tests over production
  arithmetic. Existing gas formulas, O2 clamp, temperature rounding, sampling, calibration, tasks,
  and queue behavior remain unchanged. The CO constants moved out of each sensor instance.
- Added one CI job for native tests and both firmware builds. No upload, flash, or OTA steps.
- Do not implement the old OpenSpec drafts. Use the current plan and small behavior-focused changes.
- Claude review is requested only as bounded independent feedback, not a redesign instruction.

### Build and test commands

PlatformIO Core 6.2.0 was used locally. If `pio` is not on PATH, use
`~/.platformio/penv/bin/platformio` in its place.

```sh
pio test -e native
pio run -e t-display-s3
pio run -e t-display-s3-release
pio pkg list -e t-display-s3
```

Default `pio run` still selects `t-display-s3`. `esp32dev` remains experimental and is not part
of acceptance. No device is needed for native tests. Generated UI files are not edited.

### Dependency record

| Dependency | Resolved version |
| --- | --- |
| Espressif32 platform | 7.1.3 |
| Arduino framework package | 4.20017.260907+sha.dcc1105b |
| Xtensa ESP32-S3 / RISC-V toolchains | 8.4.0+2021r2-patch5 |
| esptool package | 2.41100.260830 |
| TFT_eSPI | 2.5.43 |
| Adafruit ADS1X15 | 2.6.2 |
| Adafruit BusIO | 1.17.4 |
| RunningAverage | 0.4.9; retained until its uses are removed |
| LVGL | 9.1.0 |
| TouchLib | ccaedcd9155ef8a6560ae9f594ea92e32c68020f |
| Native platform | 1.2.1 |
| Unity | 2.6.1; installed by PlatformIO test runner using a range, not pinned |

The platform pins its framework/toolchain requirements, which may themselves contain ranges;
this table records actual resolution, not a claim of a fully locked transitive toolchain.
CI pins PlatformIO Core but uses hosted Ubuntu, Python 3.11, and version-tagged GitHub actions.
No vendoring or cold-cache reproducibility claim is added; review resolved versions on rebuild.

### Verification

- Native: seven tests passed, including O2 one/two-point conversion, clamp, He correction thresholds,
  He polynomial/scaling, current missing-O2 behavior, CO endpoints, and temperature rounding.
- Pinned original firmware: build passed, static RAM 159124 bytes, flash 1517389 bytes.
- After extraction: debug RAM 159108 bytes, flash 1517445 bytes; release RAM 159108 bytes,
  flash 1500069 bytes. Debug delta against pinned original: RAM -16 bytes, flash +56 bytes.
  Constants no longer occupy each CO sensor instance; extracted functions now have separate code.
- Static sizes exclude runtime allocation and task-stack demand. No runtime improvement is claimed.
- Pending: hosted CI execution, hardware USB/touch behavior, all sensor/device checks, and a compatible
  user-provided Editor export. The export is independent of the local baseline and does not block step 2.

### Independent review and disposition

Consulted the installed Claude CLI in a fresh, non-persistent read-only session, with hooks disabled,
no MCP servers, and only Read/Grep/Glob tools. Review was limited to step 1, not a new design.

- Rejected the reported temperature regression: installed ESP32 `Arduino.h` declares
  `using std::round` and calls its integer macro `_round`, not `round`. Original and extracted
  expressions both use floating-point rounding/division. Do not apply AVR macro assumptions here.
- Completed the missing post-extraction size record above.
- Explicitly excluded `test_*` suites from embedded environments; the native test environment is
  independent. Normal firmware builds were already unaffected by host test sources.
- Added the outer He correction gate cases (40, 40.1, 44 percent) to the existing boundary test.
- Deferred optional correction-table and parameter-renaming suggestions: neither fixes a current
  defect, and step 1 is deliberately a mechanical extraction. No formula refactor is needed.
- Tests remain characterization, not proof of safe invalid-input handling or sensor accuracy.

## 2026-09-15: Step 2a, normal-reading averaging removal

- Step 1 and its documentation were committed locally as `286cc46`. Local commits are authorized;
  no push is authorized or performed.
- Removed the three normal-reading average buffers and 20-conversion loops. Each normal gas read
  uses one new Adafruit conversion, with unchanged channel selection, gain, formulas, and polarity.
- Left O2/He calibration at five batches of 20 conversions plus five 100 ms pauses. Renamed its
  sample-count constant to make the remaining calibration-only use clear. The pinned
  `RunningAverage` dependency stays until calibration no longer needs it.
- Added seven host tests over the real sensor headers with native-only library/logging stand-ins.
  The tests exercise read counts and changed inputs, not a second implementation of sensor logic.
- No task, UI, queue, sleep, data-rate, or persistence change. Blocking Adafruit helper calls remain
  until the timeout increment. Initialization and invalid-result handling are not fixed by this change.
- No additional Claude review was needed for this small, directly tested loop removal. Use bounded
  independent review for the more consequential validity, sleep, and calibration changes.

### Verification

- All 14 native tests pass: seven conversion characterizations and seven sensor integration checks.
- Both real-library S3 builds pass. Debug: RAM 159036 bytes, flash 1516789 bytes. Release:
  RAM 159036 bytes, flash 1499317 bytes. Against step 1: static RAM -72 bytes, debug flash
  -656 bytes, release flash -752 bytes. Removed heap-backed normal average buffers are not included
  in the static RAM delta.
- Conversion count falls from 61 to 4 with all channels enabled, nominally 477 ms to 31 ms at
  128 SPS before bus/scheduling overhead. This is an estimate, not a measured response improvement.
- Still pending: hardware noise/step-response comparison and timing. The acquisition period remains
  500 ms; no claim of faster display refresh or equivalent noise rejection is made.
- Native include-order issue resolved by using a quoted-include path for the logging stand-in;
  embedded builds never use these stand-ins.

## 2026-09-15: Step 2b, initialized readings and conversion validity

- Started from local step-2a commit `55bb3bd`. No pushes, uploads, or flashes.
- Default-initialized all reading fields to NaN and enable flags to false. CO ppm is now float
  until checked integer presentation; invalid/disabled CO cannot originate an uninitialized integer.
- Guarded O2/He inputs, denominators and results; added finite/range-checked integer conversion and
  guarded MOD. Preserved finite valid formulas, existing O2 clamping, finite unclamped CO results,
  and temperature rounding. NaN remains the existing optional pure-O2 calibration sentinel.
- Kept raw He millivolts even when derivation fails; high-O2 diagnostics now show raw rather than
  corrected mV. He requires usable O2 and no longer silently uses an uncorrected result when O2 is off.
- Reset errors at the start of each cycle, detect invalid derived values, treat all-disabled as
  normal, and sample temperature only for enabled He. Invalid readings no longer trigger the old
  global ADC reinitialization loop. Task-level duplicate UI warnings are suppressed.
- Extended the native source allowlist to include the real SensorManager cycle with minimal queue
  and ADC stand-ins. Corrected constructor initialization order exposed by host compiler warnings.

### Verification and review

- All 26 native tests pass: 12 conversion and 14 sensor/cycle tests. Added NaN/infinity, invalid
  calibration, overflow, integer boundaries, raw-data preservation, complete snapshot defaults,
  invalid-to-valid recovery, all-disabled/He-only, and temperature-gating coverage.
- Both target builds pass with real libraries: debug RAM 159036 bytes, flash 1517557 bytes;
  release RAM 159036 bytes, flash 1500197 bytes. Relative to step 2a: unchanged static RAM,
  debug flash +768 bytes, release flash +880 bytes.
- Consulted installed Claude in a fresh read-only session. Accepted the missing cycle-test finding,
  prevented invalid data from causing recovery loops, added numerical-boundary tests and explicit
  math includes. Kept the agreed He/O2 validity requirement instead of restoring the unsafe fallback.
- Inspected EEZ source: global assignment retains the Value type; NaN float formatting emits empty
  text. The CO-positive colour condition is false for NaN, so no colour should be interpreted as
  proof of safe CO. Generated UI and LVGL APIs were not changed. Hardware view checks remain pending.
- Runtime correctness is not fully established: stale producer handling, separate disabled/fault
  statuses, bounded ADC reads, independent device recovery, active-fault tracking, and calibration
  transactions remain later work. The existing blocking driver and shared hardware access remain.

## 2026-09-15: Step 2c, latest measurement and UI freshness

- Started from `ba40efb`. Replaced the five-entry blocking measurement queue with one overwrite
  slot. Both normal and all-disabled cycles publish without waiting for the UI.
- Timestamped cycle start with `::millis()`; explicit qualification avoids EEZ's competing clock
  name. Added wrap-safe freshness evaluation to the existing snapshot type, without a new transport
  class or presentation framework.
- UI caches the latest measurement and checks freshness without waiting for new samples. At an age
  of 1,800 ms, all measurement numbers and raw mV become NaN; MOD follows its invalid input.
  The 100 ms UI polling interval and bounded 20 ms mutex attempt leave margin toward two seconds.
- Numeric globals update only for new/pending samples or freshness transitions. Pending updates
  survive mutex contention. Battery sampling is not increased by freshness polling.
- Queue/mutex allocation failures now stop startup before tasks can use null handles. Failure is
  reported over serial; an uninitialized display cannot show that message. No automatic reboot loop.

### Verification and review

- All 31 native tests pass: 12 conversion tests and 19 sensor/cycle tests. Five new tests cover a
  slow consumer receiving only the latest sample, stale blanking without new data, wrap and recovery,
  timestamps on disabled/invalid paths, and all-NaN initial presentation.
- Both S3 profiles build with real libraries: debug RAM 159036 bytes, flash 1518017 bytes;
  release RAM 159036 bytes, flash 1500721 bytes. Compared with step 2b, static RAM is unchanged,
  debug flash increases 460 bytes, release flash 524 bytes. Runtime queue allocation is smaller;
  runtime heap and task-stack use have not been measured.
- Claude reviewed the transport/freshness changes read-only. Addressed misleading indentation and
  excessive numeric republishing; expanded boot and restored-value checks. Kept serial-only startup
  failure handling rather than rebooting or starting a UI task with a missing mutex.
- Deferred a separate stale status, battery freshness, and a cache framework to avoid enlarging this
  increment. Tests exercise production publication and freshness rules with a one-slot queue fake;
  they do not prove FreeRTOS scheduling or EEZ rendering latency.
- Known limitations: GUI callbacks can still block rendering and delay visible stale indication.
  `configOpen` intentionally stops acquisition, so extended settings/calibration pauses now blank
  readings. Verify every affected screen on hardware. ADC helpers remain blocking; stale blanking
  is not a replacement for the next timeout/recovery increment.
- No generated UI files or LVGL APIs changed. No push, flash, or OTA was performed.

## 2026-09-15: Step 2d, ADC deadlines and independent recovery

- Started from `0aa2986`. Added one shared inline `readCounts` helper using the unmodified Adafruit
  public API. All normal and calibration reads now use start/poll/result rather than blocking
  `readADC_*` helpers; no custom registers, scaling, driver fork, or transport classes were added.
- Polling uses a 25 ms elapsed deadline and `delay(1)`; Wire1 timeout is 10 ms. Configuration remains
  the original gains at 128 SPS. Result fetching that reaches the deadline is rejected too; this is
  a conservative acceptance rule, not a hard bound on synchronous library call duration.
- Added two small readiness/timestamp records to SensorManager. Both ADCs initialize independently;
  only needed devices retry, with one second between failed attempts. A timeout marks only the
  affected device unready, and its remaining channels are skipped until recovery. Removed the
  task's global reinitialize-both error loop. No sensor power changes.
- Calibration uses the same bounded sample helper and rate-limited retry on explicit requests.
  A partial run that times out returns NaN without replacing live coefficients. Existing callbacks
  reject that result before persistence. Full calibration transactions remain later work.

### Verification and review

- All 43 native tests pass: 12 conversion tests and 31 sensor/cycle tests. Added timeout, exact
  deadline, late-result rejection, wrap, independent initialization, retry timing, channel skipping,
  disabled retry gating, partial calibration rollback, calibration retry, and error precedence tests.
- Both real-library builds pass: debug RAM 159052 bytes, flash 1518069 bytes; release RAM
  159052 bytes, flash 1501081 bytes. Against step 2c: static RAM +16 bytes, debug flash +52 bytes,
  release flash +360 bytes. No runtime heap/stack or latency measurements were made.
- Real target build caught a host-stand-in API mismatch: Adafruit's `MUX_BY_CHANNEL` is global,
  not a class member. Corrected both production calls and the stand-in, then rebuilt successfully.
- Claude review: fixed calibration requests having no retry opportunity while settings pauses
  acquisition, clarified the unavailable error text, and added boundary/error-precedence tests.
  Kept deadline rejection intentionally, including late results; the 25 ms budget is tied to 128 SPS.
- Rejected the reported new NaN persistence bug after inspecting `action_calibrate_he`: it already
  checks `!isnan(value)` before calling the setter. This does not claim the full settings/calibration
  path is transactional or otherwise validated.
- The host ADC stand-in no longer exposes `readADC_*`; accidentally restoring a blocking helper now
  fails host compilation. Source search confirms no raw register driver or blocking helpers remain.
- Pending hardware checks: normal readings/noise, slow or disconnected ADC behavior, retry latency,
  calibration timeout feedback, and UI responsiveness. The user has an earlier successful upload
  in the terminal context; that is not runtime evidence for this new increment. No upload, flash,
  OTA, or push was performed by the assistant.
- Accepted limitations: hidden I2C transfer failures may still produce plausible values; synchronous
  calls can exceed the deadline before returning. Calibration callbacks can still race acquisition
  under the old ownership model. These are not fixed by the helper or retry records.

## 2026-09-15: Step 2e, channel state and reading-status presentation

- Started from `c49455a`. Added four fixed-size state fields to the existing measurement snapshot.
  Disabled channels remain distinct from ADC unavailability and invalid derived inputs. Stale copies
  preserve Disabled while marking other channel states Stale and suppressing their numbers.
- Added a small handwritten adapter to the existing main/large reading labels, applied after EEZ
  ticks under the existing GUI mutex. No generated files or design assets changed. Invalid numeric
  globals remain NaN, not strings; the adapter only changes label text and temporary status fonts.
- Kept disabled panels hidden according to existing EEZ bindings. Enabled channels show Invalid,
  Unavailable, or Stale. A temperature-only failure preserves valid He percentage with a T status.
- State text uses existing Geneva 16; valid values regain their normal font. The adapter assumes
  the current lifetime-stable generated screens and will be replaced when the new UI is integrated.

### Verification and review

- All 49 native tests pass: 12 conversion and 37 sensor/cycle/adapter tests. Added mixed channel
  state, stale/disabled, zero-CO validity, temperature-only timeout, text mapping, generated overwrite,
  and font-restoration coverage. Label/font stand-ins do not prove actual layout or rendering.
- Native library discovery initially pulled in generated UI; resolved with native-only
  `lib_ignore = ui`. Embedded targets continue to compile the actual UI and LVGL.
- Both S3 profiles pass: debug RAM 159116 bytes, flash 1528349 bytes; release RAM 159116 bytes,
  flash 1511329 bytes. Against step 2d: static RAM +64 bytes, debug flash +10280 bytes,
  release flash +10248 bytes. Runtime label allocation and screen fit are not measured.
- Consulted LVGL MCP and verified the used label/font APIs in installed LVGL 9.1 headers.
- Claude read-only review claimed unsynchronized snapshot copying and missing valid-text restore.
  Rejected both against source: `displayedReadings` is copied inside `gui_mutex`, and generated
  `tick_screen_main/large` compare evaluated text against `lv_label_get_text` every tick. Tests
  model that generated update before restoring font/normal presentation.
- Accepted the cache-lifetime/temporary-binding caveat. EEZ and the adapter repeatedly update text
  while a channel is non-valid; sustained device testing must check heap, responsiveness, flicker,
  and layout. Do not claim the adapter is allocation-free or a permanent UI architecture.
- Pending hardware acceptance: startup, main/large navigation, disabled channels, invalid O2/He,
  ADC disconnect/recovery (subject to hidden-I2C-error limitations), stale pause/resume, font recovery,
  and long-running invalid-state display. No assistant upload, flash, OTA, or push performed.

## 2026-09-15: Step 3a, interim exclusive sensor access

- Started from `09b5054`. Added a small atomic gate for existing sensor access, without a new task,
  transport hierarchy, or general command framework. This is an interim prerequisite, not sleep.
- Startup calibration/configuration and each acquisition cycle acquire the same gate used by UI
  calibration/reset and close-settings operations. A failed UI attempt waits at most 200 ms before
  reporting busy; it does not release another operation's access. Acquisition skips busy cycles.
- `configOpen` remains a scheduling hint, not evidence of idle hardware. Settings close now applies
  accepted configuration to live sensors as well as GPIO power before resuming acquisition.
- Fixed the review-discovered failure path: if close-settings admission times out, clear the pause
  without applying the draft so navigation does not strand acquisition. Show settings-not-applied.
- No deep sleep, timeout preference, wake marker, wake GPIO configuration, or three-second CO interval
  added yet. Calibration still runs synchronously in UI callbacks. Full owner commands and persistence
  transactions remain later work; the temporary gate must not be mistaken for those guarantees.

### Verification and review

- All 54 native tests pass, including five access tests: exclusion, rejection retaining ownership,
  zero-timeout, 200 ms expiration across wrap, and two concurrent host threads updating protected data.
- Both S3 profiles build: debug RAM 159116 bytes, flash 1529265 bytes; release RAM 159116 bytes,
  flash 1512337 bytes. Against step 2e: unchanged static RAM, debug flash +916 bytes, release +1008.
- Claude read-only review found the close-settings pause leak on timeout; repaired it. Retained its
  cautions about UI mutex blocking and resets reaching live state only on close. The gate is not
  copyable (`std::atomic` deletes copying); no owner-handle framework was added for that false alarm.
- Moved the host millis/delay implementation to `test/fakes/Arduino.h` so sensor and gate tests use
  the same clock stand-in. Native-only `-pthread` enables the concurrency test, not firmware threads.
- Source review confirms setup hardware initialization happens before task creation, sensor task
  releases before its UI-log reporting, and protected operations do not require GUI-mutex acquisition
  from the sensor task. The actual callback/FreeRTOS interleavings are not simulated by host tests.
- Pending hardware: rapid settings open/close during reads, calibration during startup, busy-action
  recovery, live enable changes, and GUI responsiveness. No push, upload, flash, or OTA performed.

## 2026-09-15: Step 3b, task ownership moved forward

- User approved bringing the complete task restructure into step 3, rather than keeping temporary
  access fixes until step 4. Started from `5418b77`. Removed `SensorAccess` and its five tests,
  `configOpen`, GUI mutex, separate screen-update task, one-second startup delay, and monitor task.
- Added one concrete Analyzer plus fixed command/result definitions, and one AnalyzerSettings value.
  Analyzer task owns local Config/SensorManager instances and sensor-power GPIOs. UI task owns all
  LVGL/EEZ access, status adapter, brightness, and effective settings copy. Arduino loop blocks.
- Added depth-one command and result queues beside the existing overwrite measurement queue.
  Startup readiness and one outstanding user operation bound the protocol. Outcomes are retained
  and retried without blocking ordinary measurements; no reservation or multi-operation framework.
- UI callbacks enqueue settings/calibration/reset commands and return. Startup/manual calibration
  uses the analyzer, not the UI task. Calibration still occupies that owner for its fixed sampling
  sequence; cancellation/progress/stability remain step 5. Wi-Fi/OTA callbacks remain synchronous.
- Preferences values are loaded once; old public setters were replaced with validated changed-key
  saves. Failed writes leave effective RAM unchanged. A dirty flag rewrites the full next candidate
  after a partial failure, but mixed persistent values on power loss remain an accepted limitation.
- Reset/clear now persist/apply/acknowledge; ordinary drafts cannot overwrite calibration coefficients.
  Measurement generations prevent cached old coefficients from appearing current after acknowledgement.
- Invalid loaded settings currently fall back visibly as a whole; field-specific recovery and
  calibration-required state remain step 4. No migration, schema, or redundant storage introduced.

### Verification and review

- All 62 native tests pass: 12 conversions, 37 sensor/cycle/adapter, and 13 analyzer/message tests.
  New cases cover effective-value acknowledgement, generation changes, invalid candidates, storage
  failure, live calibration restoration, reset/clear, startup barrier, mismatched/duplicate IDs,
  command-full refusal, result retention with continued measurement, and partial-write reconciliation.
- Both real S3 profiles pass: debug static RAM 158908 bytes, flash 1525333 bytes; release static
  RAM 158908 bytes, flash 1511501 bytes. Against 3a: static RAM -208 bytes, debug flash -3932,
  release flash -836. Task stacks are now 10 KB UI + 4 KB analyzer, versus 10+3+3 KB previously;
  queue allocations and actual stack/heap high-water marks remain unmeasured.
- LVGL MCP consulted for single-task graphics ownership. Inspected generated EEZ initialization:
  `eez_flow_init()` loads assets, starts flow (including globals), creates screens and selects one
  before returning. The old startup delay worked around concurrent initialization; no arbitrary
  replacement wait was added. Hardware startup acceptance remains pending.
- Claude read-only review: exposed ADC initialization failures in Startup results, retained first
  operation failure, moved UI log writes onto the UI owner, and removed `nan mv` from error dialogs.
  Kept effective settings visible until ack intentionally; previews may revert while the save is
  pending. Active drafts are not overwritten by unrelated completions except startup synchronization.
- Rejected timeout-based busy clearing: forgetting an in-flight command could admit another before
  the first completed. Matching result delivery is tested; permanent task failure remains explicit
  unavailability, not automatic recovery. Partial persistent writes are accepted by scope, so no
  version-key or blob protocol was added in response to that review suggestion.
- Target build found EEZ's own Settings type; renamed the app value AnalyzerSettings. Native discovery
  found the deleted gate test's empty directory; removed it rather than retaining obsolete tests.
- No source references to SensorAccess/configOpen/gui_mutex remain. Generated UI assets are unchanged.
  No push, upload, flash, or OTA performed. The user must verify startup, rapid navigation while busy,
  calibration with live UI, settings/reset persistence, status recovery, and stack/heap behavior.

## 2026-09-15: Settings naming cleanup

- Renamed `Config` to `SettingsStore` and moved `src/config.h` to `src/settings/SettingsStore.h`.
  Updated the analyzer, task-local instance, tests, and current architecture reference.
- `AnalyzerSettings` remains the data/defaults/validation value in `src/settings/Settings.h`.
  `Preferences` remains the ESP32 library (and its native test stand-in), not another app abstraction.
- Naming only: the NVS namespace `config`, individual keys, defaults, and persistence behavior are
  unchanged. Historical entries above keep the names used at those milestones.
- Verification: all 13 analyzer tests pass and both S3 firmware profiles build. No push or upload.

## 2026-09-15: Step 3c, sleep handshake and CO power timing

- Started from `536ce39`. The worktree had no intervening changes. Added PrepareSleep and Resume
  to the existing fixed-size protocol; no extra queues, tasks, access gates, or persistence format.
- Preparation with a nonzero ID stops measurements, deasserts sensor power, advances generation,
  and clears the queued reading before acknowledgement. Ordinary commands are rejected while
  prepared. A matching resume restores effective power enables and a fresh generation; disabled
  sensors remain off. Already-awake resume is idempotent and does not discard a fresh reading.
- UI keeps its operation ID/busy state through Preparing/Prepared/Resuming. A two-second prepare
  timeout requests resume rather than abandoning ownership. Main checks expiry before accepting
  results. Full command queues are retried, and late prepare results cannot authorize sleeping.
- Added the fixed 3,000 ms CO interval at actual power assertion. Warming suppresses both CO mV
  and ppm and skips conversions. Off/on and resume restart it; ADC-only recovery and unrelated
  settings do not. Warming text uses the existing status adapter; no LVGL API or generated UI changes.
- No normal UI path requests PrepareSleep yet. Actual inactivity detection, timeout persistence,
  display/touch shutdown, deep-sleep entry, and GPIO14 wake are deliberately the next increment.

### Verification and review

- All 71 native tests pass: 12 conversions, 37 sensor/cycle/adapter, 22 analyzer/message tests.
  Nine new analyzer tests cover CO expiry/re-enable/wrap/ADC retry, prepared publication stop and
  queue clearing, ID matching, disabled power restoration, generation changes, prepare timeout
  with command backpressure, late results, and resume retry/idempotence. Existing text mapping now
  also checks Warming.
- Both real S3 profiles build: debug RAM 158924 bytes, flash 1526137 bytes; release RAM 158924 bytes,
  flash 1512297 bytes. Relative to the naming-cleanup build: static RAM +16 bytes, debug/release
  flash +788/+780 bytes. No target timing, GPIO behavior, or current measurements were made.
- Claude read-only review found a failed-Resume acknowledgement left resumeQueued latched. Fixed it
  to permit a one-second rate-limited retry while retaining busy, and tested actual re-enqueue
  rather than injecting an unsolicited later success. Also made Resume while awake safely succeed
  for the case where an abandoned Prepare never took effect. Mismatched IDs while prepared still fail.
- Kept Prepared without a watchdog: future power-down must explicitly enter sleep or abort/resume.
  No inactivity trigger is exposed before that path exists. The handshake is not proof of physical
  wake or recovery from a stuck task. Sleep/resume issue no Preferences writes.
- Hardware acceptance pending: three-second CO state after startup/re-enable, first post-warm-up
  sample, power pins during preparation/resume, resumed publication, and UI status recovery.
  No assistant push, upload, flash, or OTA performed.

## 2026-09-15: Step 3d, timeout setting and inactivity policy

- Started from `13daa76`. Added the `sleep_minutes` Preferences key and five-minute default, with
  Off/1/2/5/10/30-minute choices. A shared option/default definition supplies validation, dropdown
  formatting and value mapping, avoiding independently maintained lists.
- Invalid timeout data falls back per-field without resetting other settings. It is serial-logged;
  the next accepted save rewrites the repaired settings through the existing dirty-store mechanism.
  No automatic boot write, schema, or migration. Timeout edits do not invalidate measurements.
- Added a handwritten Sleep dropdown group to the existing configuration screen. Its selection is
  loaded from effective settings and saved through ApplySettings on close; failures retain the old
  effective value. Generated files and the UI design project are untouched.
- Added a pure idle decision and 50 ms wake-button release debounce. UI GPIO14 polling counts held
  presses/transitions/release as activity; touch uses LVGL's existing input activity. Completion
  of startup/settings/calibration/resume resets the LVGL inactivity counter when the app becomes idle.
- Automatic sleep is still disabled: `sleepDue` is tested but does not yet submit PrepareSleep.
  Display/touch shutdown, wake marker/classification, scan/OTA completion resets, and the actual
  deep-sleep call must be completed together in the next increment. The control currently stores
  the intended policy only; it does not yet cause power-down.

### Verification and review

- All 79 native tests pass: eight new sleep tests cover missing/invalid defaults, options, storage
  repair/roundtrip, boundary/inhibition decisions, held button, release debounce across wrap,
  dropdown mapping/format bounds, analyzer acknowledgement and persistence failure.
- Both S3 profiles build: debug RAM 158924 bytes, flash 1527353 bytes; release RAM 158924 bytes,
  flash 1513473 bytes. Against step 3c: unchanged static RAM, debug flash +1216 bytes, release
  flash +1176 bytes. Additional runtime widget allocation and layout were not measured.
- Consulted LVGL MCP and installed 9.1 headers for inactivity and dropdown APIs. Checked the pinned
  S3 SDK wake/hold declarations and TouchLib sleep/reset implementation surface for the next step.
  No claim of physical wake support or current consumption from these source checks.
- Claude read-only review found no functional defect. Consolidated the duplicated timeout options
  and default and ensured repaired storage is reconciled on the next save. Kept effective-until-ack
  UI semantics deliberately; drafting/rejection UX refinements remain step 4.
- Hardware checks pending: dropdown fit/accessibility in the scrolling configuration screen,
  selection/rejection/restart behavior, touch/button activity, and sustained UI behavior. No push,
  upload, flash, or OTA performed by the assistant.

## 2026-09-15: Step 3e, automatic S3 deep sleep and wake

- Started from `baae3f0` plus the user's uncommitted `CO_STARTUP_MS = 5000` adjustment. Preserved
  that change, updated timing tests to use the constant, and updated current plan/architecture.
  The older three-second records above are historical; five seconds is now effective.
- Connected `sleepDue` after input processing to PrepareSleep on S3 only. Default is five minutes;
  Off and all configured alternatives apply. Startup, pending calibration/settings, network work,
  and an open settings draft inhibit entry. Network callbacks reset activity on every return.
- Input during preparation or a late acknowledgement causes Resume with the same operation ID.
  Wake button must be released/debounced; either button can abort entry. Raw touch checks cover
  preparation and the panel shutdown interval before the touch controller itself sleeps.
- Added primary-board power entry functions, checked RTC GPIO14 wake and GPIO holds, panel sleep,
  full backlight-off, checked touch sleep, and radio shutdown. Kept existing board power-on high
  rather than inventing a rail shutdown policy. UI/render polling stops during hardware entry.
- On returned entry, restore holds/radio/display/touch, report failure, and request analyzer Resume.
  Touch reset is explicit because repeated TouchLib init short-circuits; failed touch restoration
  disables polling and is reported. Wi-Fi mode restore is not a reconnection guarantee.
- Added one RTC-retained marker definition. Confirmed wake requires deep-sleep reset, EXT1 GPIO14
  status and the marker; all other restarts follow cold-boot behavior. Marker is consumed before
  tasks start. Outputs are configured before hold release and reasserted after it.
- Confirmed wake skips automatic calibration, preserves stored coefficients, and starts CO warming.
  Missing/invalid wake calibration suppresses affected derived readings and reports calibration
  required; successful recalibration persists even an unchanged numerical default. Existing reset
  commands retain their deliberate default-storage behavior, not a new provenance framework.

### Verification and review

- All 87 native tests pass: 12 conversions, 37 sensor/cycle/adapter, 22 analyzer/message, 16 sleep.
  Eight new sleep tests cover marker validation/consumption, cold boot vs wake, missing calibration,
  missing-O2 dependency, store failure, activity-driven abort, and ten simulated wake cycles.
  These simulations use host ADC/storage/queue stand-ins, not ESP32 sleep or real peripherals.
- Both S3 profiles pass with real libraries: debug static RAM 159096 bytes, flash 1538985 bytes;
  release static RAM 159096 bytes, flash 1525121 bytes. Against 3d: static RAM +172 bytes,
  debug flash +11632 bytes, release flash +11648 bytes. Runtime stack/heap and sleep current unmeasured.
- Consulted LVGL MCP, pinned 9.1 headers, IDF4.4.7 sleep/hold documentation, TFT panel command
  definitions, and TouchLib reset/sleep implementation. No SDK/library or generated UI edits.
- Claude read-only review: made the RTC marker a single definition, kept failed touch polling
  disabled, explicitly configured touch reset output, and prioritized wake storage failure. Kept
  the SDK-documented known-output-before-hold-release order and added output reassertion afterward.
  Kept backlight hold failure fatal instead of claiming a safe sleep with uncontrolled output.
- Rejected the asserted He dependency leak: He conversion already rejects missing O2; added a
  direct wake test with stored He/missing O2. Explicit reset defaults remain the existing product
  operation; full calibration provenance and field-specific fallback remain step 4 refinements.
- No native test executes actual DeviceSleep/DisplayManager shutdown, RTC pin holds, Wi-Fi mode
  restoration, or real UI scheduling. Panel writes cannot acknowledge success. Final touch input
  after touch sleep cannot cancel entry; GPIO14 remains the configured wake source.
- No push, upload, flash, or OTA performed by the assistant. Automatic sleep will become active
  when this firmware is manually flashed; physical acceptance is still required.

### Device acceptance (partially verified; see results below)

- [x] Select one minute, leave the main screen idle, observe full backlight-off and wake on GPIO14.
- [ ] Confirm Off stays awake and the five-minute default/each offered interval survives restart.
- [ ] Exercise touch/button activity, held wake button for five seconds, bounce/release, and settings drafts.
- [ ] Confirm calibration, settings writes, scans, and OTA do not sleep mid-operation or immediately afterward.
- [ ] Abort preparation via touch/either button; verify visible UI, touch response, resumed readings and CO warm-up.
- [ ] Verify failed touch sleep/wake-arm/hold handling on a debug fault-injected build; no black stranded state.
- [ ] Run ten physical sleep/wake cycles and check accepted coefficients, no automatic recalibration on wake,
      normal cold-boot calibration, and five-second CO warming after each power restoration.
- [ ] Measure sleep current, wake reliability and CO/He/backlight/board-power levels; verify hold behavior on S3.
- [ ] Check task stack/heap and status/dropdown layout during long runs. Step-2 hardware checks remain open.

## 2026-09-15: Fix immediate sleep abort after device trial

- User reported the one-minute timeout began shutdown but immediately displayed sleep-aborted.
- Found the application interpreted pinned TouchLib's `enableSleep()` boolean backwards.
  `TouchLibCommon::writeRegister(uint8_t, uint8_t)` returns 0 for successful I2C transmission and
  -1 for failure; `TouchLibCSTSelf::enableSleep()` directly converts that to bool. Successful touch
  sleep therefore returned false and was always treated as an abort by our implementation.
- Corrected the application check to regard false as success, true as failure. No vendor modification,
  GPIO-policy change, or bypass of actual write failures. The existing abort restoration remains.
- Both S3 profiles build: static RAM 159096 bytes; debug flash 1538989 bytes, release 1525125 bytes.
  This checks real-library compilation, not successful physical sleep. Existing native tests do not
  execute TouchLib's hardware path and did not cover this return-value contract. Device retry pending.
- No assistant upload or push performed. Repeat the one-minute timeout and GPIO14 wake check before
  marking the corresponding acceptance items complete.

## 2026-09-15: First sleep still aborts; investigation open

- User reports the inverted TouchLib-result correction did not resolve the problem. The previous
  firmware also sometimes succeeded on its second attempt. The first fix corrected a verified API
  interpretation error, but physical sleep is not fixed or accepted yet.
- Added uncommitted stage-specific failure reporting to the sleep dialog and serial log: wake
  setup, input before/after panel sleep, touch initialization/write, Wi-Fi shutdown, and GPIO holds.
  GPIO hold failures log the pin and SDK error; touch sleep logs its actual failure flag. Shutdown
  checks are not bypassed, reordered, or retried as part of this diagnostic probe.
- Checked pinned Wi-Fi implementation: an uninitialized radio reports WIFI_OFF, so shutdown is
  skipped. TouchLib ignores read status and can expose stale touch data. Aborting after the touch
  write resets the controller; aborting earlier does not. That difference could explain a later
  successful attempt, but is not confirmed without the first attempt's failure stage.
- Claude independently reviewed first-use paths. Its wake-pin cleanup suggestion is a hardware
  hypothesis, not a demonstrated explanation. No speculative pin-reset change was applied.
- Diagnostic firmware builds in both S3 profiles and all 87 existing native tests pass. Those tests
  do not exercise physical shutdown or establish the root cause. No commit, upload, or push made;
  the user requested no commit until the actual failure is fixed and tested.
- Next device observation: after a fresh boot, capture the new first-abort dialog text and serial
  `Sleep:` / `Sleep aborted:` lines. Compare the second attempt without resetting. Do not mark
  sleep acceptance complete based only on second-attempt success.

## 2026-09-15: Touch write failure isolated; reset-before-sleep pending device test

- Device log confirms Wi-Fi mode 0 and `touch sleep failure flag=1`, followed by
  `Touch sleep write failed`. This isolates the abort to the touch sleep I2C write, not Wi-Fi
  shutdown or GPIO holds. Earlier Wire read errors do not establish which device failed.
- Sleep preparation now resets and probes the touch controller before the sleep write, using
  the same 200 ms low / 200 ms high sequence previously used only during abort recovery.
  Reset/probe code is shared with recovery. Input is checked again after reset; probe or sleep
  write failures still abort. No library fork or ignored write failure was introduced.
- Hypothesis: the reset previously performed by the first abort restores the state needed by
  the next sleep attempt. The write failure is confirmed; its electrical/controller cause and
  whether this preparation fixes it remain unverified on hardware.
- Both S3 debug and release builds pass. No commit, upload, or push made. Next device check:
  from a fresh boot, the first idle sleep should log `touch reset address status=0` and
  `touch sleep failure flag=0`, then enter sleep. Verify button wake, restored touch operation,
  and another idle sleep. Preserve any failure-stage messages if this does not succeed.

## 2026-09-15: First-attempt sleep fix verified on device

- With the reset-before-sleep build, the user captured `touch reset address status=0`, followed
  by USB disconnect during sleep entry, and confirmed the screen stayed off without an abort.
- The user then confirmed button wake, working touch after wake, and another successful idle sleep.
  This passes the basic first-attempt sleep/wake regression check; it is not the ten-cycle endurance
  test, a sleep-current measurement, or full acceptance of all step-3 hardware behavior.
- Retained reset/probe before the touch sleep write, shared with abort recovery, and stage-specific
  failure reporting. The observed write failure is resolved in the tested sequence; the underlying
  controller/electrical cause of the original failed write has not been independently established.
- Both S3 profiles built successfully after the code change. All 87 native tests passed during
  diagnosis, but do not execute the physical reset/sleep path. Device flashing/testing was performed
  by the user. The no-commit-until-fixed-and-tested condition is now met for this regression;
  the fix is ready for the previously authorized local commit, with no push.

## 2026-09-15: Step 4a, field-specific saved-settings recovery

- Started from `8c2d4cb`. Strengthened the existing invalid-load test: a bad saved O2 air coefficient
  must not reset valid brightness, channel enables, or pO2 limits. It failed with brightness 128
  instead of the saved 64, confirming whole-set fallback was the controlling defect.
- Added field repair to AnalyzerSettings using predicates shared with complete-candidate validation.
  SettingsStore records whether non-timeout values needed repair; Analyzer retains the startup
  LoadedDefaults warning and skips automatic calibration for that boot without discarding valid
  neighboring settings. Ordinary ApplySettings still rejects invalid candidates rather than repairing
  user edits. Invalid timeout alone keeps its existing quiet fallback.
- Pair rules: an invalid bottom pO2 preserves a valid low deco limit by using it as the bottom
  fallback; conflicting in-range limits preserve bottom and raise deco only to match. Other invalid
  deco values use the default. Invalid air calibration resets air and clears dependent pure O2;
  invalid pure O2 alone clears only that point. Invalid He affects only its coefficient.
- Loading performs no writes. Repaired loads retain the existing full-candidate rewrite on the next
  accepted save; failed partial writes remain dirty and retryable. No new keys, schema, or migration.
- All 93 native tests pass (28 analyzer, 16 sleep, 37 sensor/cycle, 12 conversion). Added coverage for
  valid no-op repair, numeric/pair boundaries, non-finite inputs, persisted recovery and partial-save
  retry, repaired-wake suppression, and timeout-only warning compatibility.
- Both final S3 profiles build: static RAM 159096 bytes; debug flash 1540717 bytes, release flash
  1526373 bytes. Editor diagnostics and whitespace checks pass. No dependency or generated UI changes.
- Claude's bounded review prompted the narrower conflicting-deco correction and explicit wake and
  timeout-only tests. Kept the defensive analyzer validity fallback. The review's claim that removing
  the load-warning propagation would escape all tests was incorrect: the startup regression checks
  LoadedDefaults directly. No new store framework or calibration provenance was introduced.
- Remaining limitation: repaired loads still conservatively mark both wake calibrations required,
  even for an unrelated UI-field repair. Cold-boot calibration acceptance and persisted calibration
  provenance are not resolved by field recovery; they remain the next increment, not a completion claim.
- Native tests use fake Preferences and do not prove corrupt-key behavior on real NVS. No assistant
  upload or push performed. Device apply/reset/clear/restart checks remain open.

## 2026-09-15: Step 4b, independent calibration acceptance

- Started from `5174618`. Changed the existing repaired-wake test to expect accepted calibration
  after brightness-only repair. It failed with CalibrationRequired instead of LoadedDefaults,
  confirming the prior all-calibrations-required rule was the controlling defect.
- SettingsStore caches O2-air/He validity from raw saved coefficients, not key presence or repaired
  RAM defaults. Both cold boot and wake now suppress unaccepted derived values independently; O2
  remains a dependency for He. Valid calibration survives unrelated repairs. Results report both
  required flags and the existing UI dialog/log identifies O2, He, or both as applicable.
- Ordinary saves cannot promote fallback coefficients. Unaccepted calibration uses NaN under the
  existing float keys. Missing/NaN values load RAM defaults without a repair warning or forced rewrite;
  malformed numeric coefficients still trigger recovery. No new keys, schema, migration or version.
- Calibration and explicit reset pass acceptance for only their channel; store cache, live flags,
  and generation change only after full save success. Explicit reset intentionally accepts the
  documented numerical default, not a measured value. Same-default acceptance still writes NVS.
- Pure-O2 calibration is refused without accepted air. An orphan optional pure point is discarded,
  then explicitly cleared on first air acceptance, including when RAM already contains NaN.
- Removed O2's calibration-before-read early exit: raw millivolts remain available, while guarded
  conversion returns NaN for the percentage. Updated the old skip-read test to require one raw
  conversion and unavailable percentage. No ADC gain/rate, conversion formula, or library changes.
- All 101 native tests pass: 36 analyzer, 16 sleep, 37 sensor/cycle, 12 conversion. New coverage
  includes independent invalid/missing calibration on both boot paths, dirty-save/reboot behavior,
  per-channel reset acceptance, failed/partial writes and retry, orphan pure points, enabled-channel
  messages, raw readings, and startup sampling with persisted NaN markers.
- Both final S3 profiles build: static RAM 159096 bytes; debug flash 1540449 bytes, release flash
  1526085 bytes. Editor diagnostics and whitespace checks pass. Hardware acceptance is not implied.
- Claude review exposed a NaN-marker reload loop that would suppress startup calibration and force
  writes each boot; fixed and added explicit regression checks. Also ordered required warnings before
  primary operation errors so the legacy log does not downgrade their indicator. Existing generic
  Invalid reading labels remain; result messages identify required calibration. No LVGL API changes.
- Power loss during multi-key writes remains non-atomic, including a valid calibration prefix before
  a reported failure is retried. Old valid-looking default values cannot retroactively be distinguished
  from calibration. No stronger persistence/provenance guarantee is claimed.
- Device checks pending: missing/invalid O2 vs He, unrelated field repair with valid calibrations,
  settings save/reboot while unaccepted, accepted calibration/reset and pure clear across reboot,
  cold-start preference, visible messages, and raw diagnostics. No assistant upload or push performed.

## 2026-09-15: Step 4c, retained settings drafts and rejection UX

- Superseded by the save-on-exit simplification below at the user's request.
- Started from `4db91c9`. Confirmed the generated callbacks use SCREEN_LOADED/SCREEN_UNLOADED;
  closing happens after navigation and cannot assume a synchronous result. Added a regression for
  rejected edits surviving reopening, then implemented the small UiState-owned draft lifecycle.
- One retained RAM settings value, editing/retained flags, settings request ID and reopened flag
  preserve edits across busy/queue/storage/validation rejection. Results still update effective
  settings independently. Success clears only the acknowledged non-reopened draft; newer editing
  sessions survive earlier results. Coefficient rebasing prevents stale drafts undoing calibration.
- Existing callbacks capture controls on close, restore effective globals and brightness, and reopen
  with the retained draft. Async results do not touch editing controls except the existing startup
  synchronization. Duplicate unload calls outside an editing session are ignored deliberately.
- Rejection modal adds Edits retained and Discard edits. Closing the modal retains edits; correction
  and resubmission happen on reopening settings. Discard restores effective controls and restarts
  inactivity. It cannot cancel a queued write and updates the same modal when a write is pending.
- Open or retained drafts inhibit sleep until saved or explicitly discarded, including after a
  rejection modal is dismissed. Preparation/prepared checks now request Resume for settings activity
  as well as physical input. RAM drafts do not survive power loss/reset. No auto-retry or timeout.
- All 110 native tests pass: 45 analyzer, 16 sleep, 37 sensor/cycle, 12 conversion. Nine draft tests
  cover rejection/reopening, queue-full/discard, calibration rebasing, earlier success vs newer edits,
  pending-write discard refusal, mismatched results/duplicate events, startup seeding, preparation
  interruption/resume, and fresh editing after discard in an open screen.
- Both final S3 profiles build: static RAM 159128 bytes; debug flash 1541437 bytes, release flash
  1527085 bytes. Relative to 4b: RAM +32 bytes, debug flash +988, release flash +1000.
- Consulted LVGL MCP for lifecycle/modal behavior, verified footer and async-close APIs against
  installed 9.1 headers, and inspected generated callback bindings. No generated UI edits.
- Claude review prompted explicit settings checks during preparation and avoiding pending-dialog
  stacking. Retained-draft sleep inhibition is intentional, not permanent: saving or discarding
  resolves it. Kept the editor active after discard to allow fresh edits; unchanged closes may
  intentionally reconcile a dirty store. Did not add a timeout that forgets an in-flight write.
- Device checks pending: edit/close/reopen while another operation is busy, invalid pO2 rejection,
  correction/resubmission, discard and brightness restoration, result delivery while editing, dialog
  fit on 320x170, and inactivity behavior with retained/discarded drafts. Native tests do not execute
  real EEZ/LVGL events. No assistant upload or push performed.

## 2026-09-15: Simplify settings to save on exit

- User rejected retained drafts as disproportionate for this device. Removed the retained value,
  retention/reopened flags, extra settings request ID, coefficient rebasing of retained drafts,
  discard API/button, and retention-based sleep inhibition introduced in `85212f5`.
- Settings save on exit through the existing analyzer command. Until acknowledgement the UI restores
  effective values/brightness. Failure or busy refusal shows an ordinary error; edits are not retained
  and reopening uses the latest effective settings. Close starts a fresh inactivity interval.
- Kept one editing flag and a tested result-sync predicate so asynchronous save/calibration results
  cannot overwrite an open editor. Startup still seeds controls. Analyzer validation, per-channel
  calibration acceptance, persistence checks, and generation filtering are unchanged.
- An open editor still inhibits/interrupts sleep preparation. Once closed, failed edits no longer
  block sleep; only an actual pending operation or another existing inhibitor does.
- Replaced nine retention tests with three save-on-exit tests: success/rejection acknowledgements,
  queue refusal/duplicate close, and protection of open controls from delayed results.
  All 104 native tests pass (39 analyzer, 16 sleep, 37 sensor/cycle, 12 conversion).
- Both S3 builds pass: RAM 159096 bytes; debug flash 1540573 bytes, release flash 1526217 bytes.
  LVGL MCP consulted; only existing message-box APIs remain, with no generated UI changes.
- Device checks still needed: save/reopen, busy/invalid rejection and brightness restoration,
  results during editing, and sleep after rejected saves. No assistant upload or push performed.

### Next increment

Step-4 implementation is in place; device settings/save-on-exit/apply/reset/clear/restart acceptance remains
open, alongside earlier sleep/current and sensor checks. Step 5 is incremental stability-gated
calibration with cancellation, bounded progress/history, and the required live graph.