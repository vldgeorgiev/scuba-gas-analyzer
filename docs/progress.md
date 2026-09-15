# Implementation progress and decisions

Current scope: [improvement plan](improvement-plan.md).
Technical reference: [architecture and messaging](architecture.md).
Historical documents describe the discarded redesign, not acceptance evidence for this work.

## Status

| Step | Status |
| --- | --- |
| 1. Build/test baseline | Implemented and locally verified; Claude review resolved; hosted CI, device checks, and Editor export pending |
| 2. Measurement handling and averaging removal | In progress: normal-reading averaging removed and locally verified; initialization, guards, timeout/recovery, and freshness still pending |
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

### Next increment

Continue step 2 with initialized fields, explicit usable/unusable readings, and guarded derived
values; then bounded stock-Adafruit acquisition, independent readiness, and latest/stale publication.
Keep each slice tested and locally committed, and preserve the current simple sensor structure.