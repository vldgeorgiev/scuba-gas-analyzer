#include <esp_adc_cal.h>
#include "pin_config.h"
#include <esp32-hal-adc.h>

float getBatteryVoltage() {
  // The voltage detected is about 0.1v lower than the real one
  esp_adc_cal_characteristics_t adc_chars;
  esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_12, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  uint32_t raw = analogRead(PIN_BAT_VOLT);
  uint32_t voltage = esp_adc_cal_raw_to_voltage(raw, &adc_chars) * 2; //The partial pressure is one-half
  return round(voltage / 10) / 100.0;
}