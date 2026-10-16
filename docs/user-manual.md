# Scuba gas analyzer user manual

This analyzer is a DIY device and while well-tested, it is not certified. Use at your own risk.

## Reading gas

Power on the analyzer and allow every enabled sensor to settle. The main screen shows oxygen, helium, carbon monoxide, battery state, and raw sensor voltage. Tap anywhere on the readings screen to switch between the compact and large-number views.

![Main readings screen](images/user-manual/readings.png)

![Large readings screen](images/user-manual/large-readings.png)

`Warming` means that the sensor is not ready yet. `Stale` means the analyzer has not received a recent valid measurement; wait for a fresh reading before relying on it. `Unavailable` means that a required sensor or its supporting measurement cannot currently provide a value. A warning indicator opens Diagnostics, where recent warnings and errors are shown.

### MOD, CO, and battery

The `B` and `D` values under the oxygen reading are the maximum operating depths (MOD). They are calculated from the measured oxygen percentage and the bottom and deco ppO2 limits selected in **Settings**. `B` uses the bottom ppO2 limit; `D` uses the deco ppO2 limit. Recheck the selected limits and gas analysis before relying on either depth.

The CO value turns red when the analyzer detects CO above 0 ppm. Investigate the gas and do not use it until it has been checked.

A fully charged cell is about 4.2 V. Recharge before it drops to about 3.0 V. Use the **Sleep** setting to put the analyzer into low-power sleep after several minutes of inactivity; select `Off` only when continuous operation is needed. Sleep greatly reduces battery consumption while the analyzer is not in use. Keep the analyzer powered during an active firmware update.

## External O2 sensor

Connect the external O2 sensor to the coaxial jack. The analyzer detects it and reads its input;
He and CO are powered off. Their saved enable settings are unchanged. Unplugging
restores the internal O2 sensor and the current He/CO settings; wait for enabled sensors to settle
again. O2 must still be enabled in **Settings**.

Internal and external O2 have separate saved air and optional pure-O2 calibrations. Calibrate the external sensor before its first use.
Without an accepted air calibration for the selected sensor, O2 shows `Invalid` but raw voltage is
still available. One external calibration set is stored, not a set for each interchangeable sensor:
recalibrate whenever you replace the external sensor.

## Calibration

From the main screen, tap the circular-arrow **Calibration** button. Choose the calibration that matches the reference gas being used:

- **Air calibration** calibrate O2 sensor with air.
- **Pure O2 calibration** calibrate O2 sensor with 100% O2 (optional).
- **Helium calibration** calibrate He sensor with 100% helium.

O2 calibration automatically uses the connected sensor: unplug the external sensor to calibrate the
internal one, or plug it in to calibrate the external one. Air calibration must be accepted before
optional pure-O2 calibration for that same sensor. O2 reset and pure-O2 clear also affect only the
selected sensor. Keep the connection unchanged throughout calibration. Disconnect the external O2
sensor before calibrating helium.

![Calibration choices](images/user-manual/calibration.png)

Start calibration only after supplying stable reference gas to the sensors. The analyzer samples the sensor value and waits for a stable reading. It saves after at least five stable seconds. The graph freezes and the screen shows `Saved` when it is complete; use **Done** to return to the calibration screen. Use **Cancel** to stop a calibration in progress.

![Completed air calibration](images/user-manual/calibration-complete.png)

If calibration reports an invalid range, unstable signal, or save failure, do not use the attempted value. Check the reference gas, sensor connection, and sensor warm-up, then try again.

**Calibrate on startup** is off by default. When enabled, it calibrates the selected O2 sensor with air after a cold boot; waking from automatic sleep retains calibration. Before starting the unit with this option enabled, flush it with fresh air so no stale gas remains inside. Residual gas with a different oxygen concentration can produce an incorrect calibration.

## Settings

Open **Settings** from the main screen to enable or disable sensors and adjust the CO correction, oxygen limits, brightness, and sleep timing. Changes save when leaving the screen. If the analyzer is busy or a value is invalid, it keeps the previous effective setting and reports an error.

**CO correction** compensates for a CO sensor that reads a constant few ppm in clean air. Select a value from `+2` to `-5` ppm; the analyzer adds it to every measured CO reading. For example, if clean air reads `3 ppm` after warm-up, select `-3`. The default is `0`. Readings may show below zero when the correction is too negative.

## Firmware update

Open **Settings**, then **Firmware Update**. The screen shows the installed firmware version.

1. Select **Scan Wi-Fi**.
2. Choose your Wi-Fi network when the scan finishes.
3. Enter the Wi-Fi password.
4. Select **Install firmware** and keep the analyzer powered until the operation finishes.

![Firmware update](images/user-manual/firmware-update.png)

The update status indicates the current network or installation step. Do not start another network operation while a scan or update is already in progress.
