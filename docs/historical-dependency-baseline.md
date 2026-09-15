# Historical: dependency baseline and build procedure

Status: historical evidence from the discarded redesign. Code, build profiles, tests, and CI were
reverted on 2026-09-15. Commands and sizes below may not apply to the restored firmware; rerun and
record a baseline when implementing [the current plan](improvement-plan.md).

What this document is: a record of exactly which package versions this firmware builds against,
which build macros are actually in effect, the firmware sizes those builds produce, and the
commands that reproduce all of it.

What it is not: evidence that the firmware works. A successful build and a byte count are
compile-time facts. Nothing here has been flashed, run, or measured on a device.

PlatformIO executable used for every command below:
`/Users/A24B525/.platformio/penv/bin/platformio` (PlatformIO Core 6.2.0). Substitute `pio` if it
is on your path.

## Environments

| Environment | Purpose |
| --- | --- |
| `t-display-s3` | Primary target, debug profile, `CORE_DEBUG_LEVEL=4`. Default environment. |
| `t-display-s3-release` | Same sources and flags, `CORE_DEBUG_LEVEL=1`. |
| `esp32dev` | Pre-existing experimental target, unchanged in scope and behavior. |
| `native` | Host tests. Compiles only hardware-free sources from `src/` plus `test/`. |

## Pinned versions

Resolved for `t-display-s3` (`pio pkg list -e t-display-s3`):

| Package | Version |
| --- | --- |
| platform espressif32 | 7.1.3 |
| framework-arduinoespressif32 | 4.20017.260907+sha.dcc1105b |
| toolchain-xtensa-esp32s3 | 8.4.0+2021r2-patch5 |
| toolchain-riscv32-esp | 8.4.0+2021r2-patch5 |
| tool-esptoolpy | 2.41100.260830 |
| TFT_eSPI | 2.5.43 |
| Adafruit ADS1X15 | 2.6.2 |
| Adafruit BusIO | 1.17.4 (transitive) |
| RunningAverage | 0.4.9 |
| lvgl | 9.1.0 |
| TouchLib | 0.0.2+sha.ccaedcd (`ccaedcd9155ef8a6560ae9f594ea92e32c68020f`) |

Host side (`native`): platform native 1.2.1, Unity 2.6.1.

Unity is **resolved and recorded, not pinned**. The `native` environment does declare an exact
`throwtheswitch/Unity@2.6.1` in `lib_deps`, and the host suite passes with it, but that entry only
documents the intended version - it is not an effective pin. The `native` platform injects its own
`throwtheswitch/Unity @ ^2.6.1` after `lib_deps` is resolved, and that spec wins. Verified
directly: declaring `throwtheswitch/Unity@2.6.0` installed 2.6.0 and the platform then reinstalled
2.6.1 over it. A `#v2.6.1` git URI fails outright because the upstream repository carries no
PlatformIO manifest. So 2.6.1 is what resolves today with `platform = native@1.2.1`, and a future
2.x Unity release would be picked up silently. If that becomes a problem the fix is vendoring
Unity into the repository, not another `lib_deps` line.

Adafruit BusIO is a transitive dependency of Adafruit ADS1X15 and is not pinned directly. If the
manifest check ever shows it moving, pin it explicitly at that point.

## Effective build macros

Taken from the actual compiler command line for the primary target, in order:

```
-DARDUINO_USB_MODE=1          (from the board definition)
-DCORE_DEBUG_LEVEL=4
-DUSER_SETUP_LOADED=1
-DLV_CONF_SKIP
-DLV_LVGL_H_INCLUDE_SIMPLE
-DTOUCH_MODULES_CST_SELF
-DDISABLE_ALL_LIBRARY_WARNINGS
-UARDUINO_USB_MODE            (later on the command line, so USB mode ends up undefined)
```

`ARDUINO_USB_MODE` is defined by the board and then undefined by our flag; the ordering, and
therefore the effective result, is unchanged by the pinning. `t-display-s3-release` is identical
except for `CORE_DEBUG_LEVEL=1`.

Regenerate with:

```
pio run -e t-display-s3 -t clean
pio run -e t-display-s3 -v | grep -m1 conversions.cpp | tr ' ' '\n' | grep -nE '^-[DU]'
```

## Sizes

| Build | Static RAM | Flash |
| --- | --- | --- |
| `t-display-s3` before this change (supervisor-measured) | 159124 | 1517389 |
| `t-display-s3` after this change | 159124 | 1517461 |
| `t-display-s3-release` after this change | 159124 | 1500085 |

The 72-byte flash increase comes from the conversion extraction: the arithmetic now lives in its
own translation unit as out-of-line functions instead of being inlined into each sensor header.
Static RAM is unchanged. Release is smaller because the lower debug level drops log strings.

### After `stabilize-measurement-handling` (change 2)

| Build | Static RAM | Flash |
| --- | --- | --- |
| `t-display-s3` | 160988 | 1519153 |
| `t-display-s3-release` | 160988 | 1502189 |

Against the change 1 row above: static RAM +1864 bytes, debug flash +1692 bytes, release flash
+2104 bytes. The RAM comes almost entirely from the log rewrite - the fixed-record store plus the
render buffer it copies into replace a single 500-byte character ring - offset by the four
`RunningAverage` instances the retired sensor classes used to hold. The flash covers the bounded
ADC transport, the per-device recovery policy, and the guarded conversions.

### After `refactor-acquisition-ownership` (change 3)

| Build | Static RAM | Flash |
| --- | --- | --- |
| `t-display-s3` | 161284 | 1522653 |
| `t-display-s3-release` | 161284 | 1506301 |

Against the change 2 rows: static RAM +296 bytes, debug flash +3500 bytes, release flash +4112
bytes. The RAM is the three protocol queues' static objects and the owner's retained-outcome
array; the queue storage itself is heap, allocated by FreeRTOS at creation. The flash covers the
command and result protocol, the acquisition owner with calibration as a stepped operation, and
the merged interface loop.

### After `make-calibration-transactional` (change 4)

Not measured. The session that wrote change 4 had no way to run `pio`: every invocation of the
build and test commands below was refused by the environment's approval gate, so no binary was
produced and no size could be read. The change 3 rows above are therefore still the newest numbers
in this document, and they do **not** describe the current tree.

Measuring this is task 6.2 and it remains unchecked. The dual-slot implementation has since been
rejected in favour of the existing individual Preferences keys, so the current partial tree is not
the intended change 4 architecture. Measure again only after it is simplified.

## Commands

```
pio run                                  # default environment: t-display-s3
pio run -e t-display-s3-release
pio test -e native                       # host tests, no device needed
pio pkg list -e t-display-s3             # regenerate the version table above
```

Fresh-resolution check, which must never disturb your working cache. One named temporary root with
three subdirectories, so cleanup has exactly one target and that target can be checked before it is
removed:

```
#!/bin/sh
set -eu

PIO_FRESH_ROOT=$(mktemp -d "${TMPDIR:-/tmp}/pio-fresh.XXXXXX")

cleanup() {
  # Only ever remove a path that still looks like the root we created ourselves.
  case "${PIO_FRESH_ROOT}" in
    */pio-fresh.??????) rm -rf -- "${PIO_FRESH_ROOT}" ;;
    *) echo "refusing to remove unexpected path: ${PIO_FRESH_ROOT}" >&2 ;;
  esac
}
trap cleanup EXIT INT TERM

mkdir -p "${PIO_FRESH_ROOT}/core" "${PIO_FRESH_ROOT}/libdeps" "${PIO_FRESH_ROOT}/build"

PLATFORMIO_CORE_DIR="${PIO_FRESH_ROOT}/core" \
PLATFORMIO_LIBDEPS_DIR="${PIO_FRESH_ROOT}/libdeps" \
PLATFORMIO_BUILD_DIR="${PIO_FRESH_ROOT}/build" \
  pio pkg install -e t-display-s3

PLATFORMIO_CORE_DIR="${PIO_FRESH_ROOT}/core" \
PLATFORMIO_LIBDEPS_DIR="${PIO_FRESH_ROOT}/libdeps" \
PLATFORMIO_BUILD_DIR="${PIO_FRESH_ROOT}/build" \
  pio run -e t-display-s3
```

All three variables are required: setting only `PLATFORMIO_CORE_DIR` still reuses the project's
`.pio/libdeps`, which is not a cold resolve. The script downloads everything from scratch and
prints what it resolved; it needs network access and roughly 2.5 GB of temporary space, and the
trap removes the single root on any exit path. It never touches your real PlatformIO cache and
never deletes anything outside the directory it created. If it cannot complete, the correct
conclusion is that the fresh resolution is unverified - not that it passed.

## Verification status

### ADC simplification, 2026-09-15

- The current ADC path uses unmodified Adafruit ADS1X15 2.6.2. The custom register constants,
  transport interface, and Arduino bus adapter were removed; a timing/readiness wrapper remains.
- All 158 native tests pass across ten suites. Two register-transfer failure tests were removed
  because the stock library does not expose that status. ADC tests use a public-API stand-in,
  available only through the native environment's `test/fakes/adafruit` include path.
- Debug and release target builds pass against the real library. Debug: RAM 161628 bytes,
  flash 1525785 bytes. Release: RAM 161628 bytes, flash 1511345 bytes.
- No device was flashed or measured. Individual I2C failure detection is not guaranteed.
- The entries below describe earlier milestones and are retained as historical evidence.

- Host tests: 98 cases across eight suites were run and passed **as of change 3**. Change 4 brings
  the tree to 160 registered cases across ten suites - it adds `test_settings` (18) and
  `test_settings_store` (21) and rewrites `test_owner` to 38 - and none of that has been executed,
  because `pio` could not be run in the session that wrote it. The "passing" claim above covers the
  change 3 tree only.
- Unity: resolved and recorded at 2.6.1; pinning is explicitly unfulfilled, see the note under
  "Pinned versions" for what was tried and why it cannot work here.
- CI pinning: `pip install 'platformio==6.2.0'` matches the Core version used for every number in
  this document, and the three GitHub Actions are referenced by immutable commit SHA with the tag
  in a trailing comment (`actions/checkout` v4.4.0, `actions/setup-python` v5.6.0, `actions/cache`
  v4.3.0). The runner image, the Python 3.11 patch level, and the packages PlatformIO downloads at
  CI time remain outside our control.
- Both target profiles: built successfully at change 3, sizes above. Not rebuilt for change 4, so
  the current tree is not known to compile for the target at all.
- Effective macros: captured from the real compile command line.
- Fresh resolution in an isolated cache: run, and it resolved every version in the table above,
  including the TouchLib commit and lvgl 9.1.0. The cold set also built successfully. The existing
  developer cache was not touched or deleted; the throwaway directories were removed afterwards.
  The cold build reports a flash size 128 bytes smaller than the local one, which comes from the
  shorter dependency paths embedded in diagnostic strings, not from a different package set.
- CI: the workflow in `.github/workflows/ci.yml` has been checked locally only - it parses, it
  contains no upload, flash, or OTA step, and every command it runs succeeds on a development
  host. It has not been executed on GitHub, because no push is authorized. Treat CI as pending
  external verification.
- Device behavior: not verified. Nothing was flashed.
