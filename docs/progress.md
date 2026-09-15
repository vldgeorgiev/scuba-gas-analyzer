# Implementation progress and decisions

Current scope: [improvement plan](improvement-plan.md).
Technical reference: [architecture and messaging](architecture.md).
Historical documents describe the discarded redesign, not acceptance evidence for this work.

## Status

| Step | Status |
| --- | --- |
| 1. Build/test baseline | Implemented and locally verified; Claude review resolved; hosted CI, device checks, and Editor export pending |
| 2. Measurement handling and averaging removal | Software increments 2a-2e implemented and locally verified; on-device acceptance and temporary status-adapter performance checks pending |
| 3. Sleep/wake, five-minute default | Not started |
| 4. Ownership and live settings | Not started |
| 5. Stability-gated calibration | Not started |
| 6. Replacement UI | Not started; sample Editor export required |
| 7. Diagnostics and measured cleanup | Not started |

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

### Next increment

Step 3 is configurable sleep with five-minute default and GPIO14 wake. First implement only the
single-owner stop/resume and sensor-power coordination needed for safe sleep; the current
`configOpen` flag still does not wait for an in-flight read. Keep the larger settings/command
refactor deferred unless it is necessary for that coordination. Step-2 hardware checks remain open.