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
  // Calibration offset: LilyGo battery ADC typically reads ~0.15V lower than actual
  constexpr float BATTERY_VOLTAGE_OFFSET_MV = 150;
  const float calibratedVoltage = (voltage + BATTERY_VOLTAGE_OFFSET_MV) / 1000.0;
  return round(calibratedVoltage * 100) / 100.0;
}