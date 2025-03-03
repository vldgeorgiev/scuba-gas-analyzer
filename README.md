# DIY Scuba gas analyzer

A DIY project for a scuba gas analyzer. The analyzer detects Oxygen, Helium and Carbon monoxide.  
_Disclaimer: This is not a certified device and should be used with caution at your own risk. Always double-check your results with a professional-grade analyzer._

Based on ideas from  
<https://slideplayer.fr/slide/10883088>  
<https://scubaboard.com/community/threads/nitrox-trimix-co-analyzer.595564>  

## Software

The application is developed with [Platformio](https://platformio.org/).  
It uses FreeRTOS tasks for multithread UI processing, data reading, etc.  

Libraries used:

- [TFT eSPI](https://github.com/Bodmer/TFT_eSPI) MCU graphics library and driver
- [LVGL UI library](https://github.com/lvgl/lvgl) lightweight GUI library for ESP32 and Arduino
- [EEZ Studio](https://github.com/eez-open/studio) for generating the LVGL UI and for some UI automation

## Hardware

Some elements are available in different versions, depending on how the board is built. E.g. SMD for a proper PCB, or a perfboard, or a breadboard...

- [Lilygo S3 ESP32](https://www.lilygo.cc/products/t-display-s3) (24$) - An ESP32 controller with a built in touch display, 2-core, LiPo power, LiPo charging, etc.
- [ADS 1115](https://aliexpress.com/item/1005001703504835.html) (2 x 1$) - 16bit analog-to-digital converter for reading sensor values.
- [3V voltage regulator](https://www.ti.com/product/TPS782/part-details/TPS78230DDCT) or similar. The helium sensor requires a 3.0V power. The regulator should have an "enable" pin so it can be turned on/off by a signal from the ESP32 if the sensor is not used.
- [5V voltage regulator](https://eu.mouser.com/ProductDetail/Texas-Instruments/TPS61240DRVR) or similar. The CO2 sensor requires a 5.0V to 12.0V power. The regulator should have an "enable" pin so it can be turned on/off by a signal from the ESP32 if the sensor is not used.
- [LM35DZ temperature sensor](https://aliexpress.com/item/32921896929.html) (2$) for detecting that the He sensor has warmed up.

Gas sensors:

- [O2 (Oxygen cell)](https://www.aliexpress.com/item/1005006077026218.html) (21$ - 50$) - A standard O2 sensor with a 3 pin Molex connector. There are various similar oxygen cell sensors from various suppliers. The cheap Chinese one works equally well. Other connectors can be used too instead of the Molex. **Service life 2-3 years**
- [Winsen MD61 or MD62 helium sensor](https://www.aliexpress.com/item/33019119435.html) (30$) - A thermal conductivity inert gas sensor. Measures the change of conductivity of gases. It can not identify different gases. In our case it depends on the assumption that there are no other gases like hydrogen, CO2, etc. The values are measured with a Wheatstone bridge. It is very sensitive, so the sensor and bridge should be soldered, to avoid the variable resistance of unstable connectors. **Service life 10 years**  
The sensor is thermal and after powered on warms up to about 25C. The time required is 1-2 min, but depends on the starting temperature. Readings are accurate after the warmup. The warming feature uses up to 120mA, which is considerable for a device powered by a battery. It is best to have an option to turn it on/off programatically.  
_The sensor is sensitive to CO2. Readings will be affected by breath while testing._
- [Winsen ZE15-CO or ZE07-CO carbon monoxied sensor](https://www.aliexpress.com/item/4001321584178.html) (10$) - Measures 0-500ppm of CO. For scuba the CO should be 0 and sensors that measure from 10-20ppm and higher are not suitable. The ZE15/ZE07 come factory calibrated and provide both a digital and analog readout. The project uses the analog one (400mV-2000mV range corresponding to 0-500ppm).  
The sensor has an up to 3 min warm up time, but my testing in various conditions and temperatures ranging 10-30C has not shown any difference in the readings. **Service life 3-5 years**

Other elements:

- The device is powered by one 18650 LiIon battery, providing 2.7-4.2V. The Lilygo S3 is designed to be powered and to charge the battery directly. Any other lithium battery can be used as long as the voltage is suitable.
- Wheatstone bridge for reading the MD61 helium sensor. Comprised of 2 x 2KOhm resistors and a 500Ohm precise potentiometer (at least 10-15 turns)
- A few capacitors for the voltage converters.

Power consumption:

The consumption varies depending on the sensors enabled and display brightness. Here are some estimates with medium brightness, based on a 3000mAh battery

- **O2 sensor only**: 90mA, ~33 hours
- **O2 and CO sensors**: 100mA, ~30 hours
- **O2, CO, and He sensors**: 210mA, ~14 hours
- **Display brightness**: varies about 30mA between lowest and highest

## TODO

- [x] 100% O2 calibration. How much does it improve accuracy? Most other analyzers don't do it, including brand ones. Check that linear drift in more detail.
- [ ] Metric/Imperial values - not so important yet
- [ ] Go to sleep after X minutes
- [ ] Screen flip 180 degrees option
- [x] Add large digits screen for easier monitoring. Switch from main with touch anywhere.
- [ ] Remember last screen on startup (where applicable)
- [x] Battery level and charging indicator
- [ ] After initial testing limit the displayed values: e.g. O2 max 100%, CO 0-500, etc.
- [ ] Show a warning if the initial O2 sensor's voltage is much different from the previous startup. E.g. when a sensor is replaced. Reset the 100% O2 calibration and remind user
- [ ] Remove the millivolts from the main screen after testing. Move them to a separate details screen, maybe opened from the Config
- [x] Log screen to show warnings and errors. To be opened from the main or config screens
- [x] Indicator icon on the main screen for warnings. To open the log screen
- [ ] Auto detection of stable levels during calibration. Read for up to 5-10s and wait for minimal deviation
- [ ] OTA updates with an AP mode
- [x] Brightness control
- [ ] Translation in other languages
- [ ] Gas calculator, best mix, etc...
