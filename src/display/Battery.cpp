#include "Battery.h"
#include "pin_config.h"
#include <esp_adc_cal.h>
#include <esp32-hal-adc.h>

namespace {

esp_adc_cal_characteristics_t& batteryAdcCharacteristics() {
  static esp_adc_cal_characteristics_t characteristics;
  static bool initialized = false;
  if (!initialized) {
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &characteristics);
    initialized = true;
  }
  return characteristics;
}

}  // namespace

void initializeBatteryVoltage() {
  batteryAdcCharacteristics();
}

float getBatteryVoltage() {
  auto& adcChars = batteryAdcCharacteristics();
  const uint32_t raw = analogRead(PIN_BAT_VOLT);
  const uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, &adcChars) * 2;
  return round(voltage / 10) / 100.0;
}