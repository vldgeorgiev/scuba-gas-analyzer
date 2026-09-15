#ifndef APP_DEVICE_SLEEP_H
#define APP_DEVICE_SLEEP_H

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
#include <esp_sleep.h>
#include <esp_system.h>
#include <driver/gpio.h>
#include <driver/rtc_io.h>
#include <WiFi.h>
#include "pin_config.h"
#include "display/DisplayManager.h"
#include "SleepPolicy.h"

namespace app {

extern uint32_t retainedSleepMarker;
constexpr gpio_num_t sleepHeldPins[] = {
  static_cast<gpio_num_t>(PIN_HE_ENABLE), static_cast<gpio_num_t>(PIN_CO_ENABLE),
  static_cast<gpio_num_t>(PIN_LCD_BL), static_cast<gpio_num_t>(PIN_POWER_ON)
};

inline void releaseSleepHolds() {
  for (gpio_num_t pin : sleepHeldPins) gpio_hold_dis(pin);
  gpio_deep_sleep_hold_dis();
}

inline bool consumeDeviceWake() {
  const bool expected = esp_reset_reason() == ESP_RST_DEEPSLEEP &&
      esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1 &&
      esp_sleep_get_ext1_wakeup_status() == (1ULL << PIN_BUTTON_2);
  const bool confirmed = consumeSleepMarker(retainedSleepMarker, expected);
  pinMode(PIN_HE_ENABLE, OUTPUT);
  pinMode(PIN_CO_ENABLE, OUTPUT);
  pinMode(PIN_LCD_BL, OUTPUT);
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_HE_ENABLE, LOW);
  digitalWrite(PIN_CO_ENABLE, LOW);
  digitalWrite(PIN_LCD_BL, LOW);
  digitalWrite(PIN_POWER_ON, HIGH);
  releaseSleepHolds();
  digitalWrite(PIN_HE_ENABLE, LOW);
  digitalWrite(PIN_CO_ENABLE, LOW);
  digitalWrite(PIN_LCD_BL, LOW);
  digitalWrite(PIN_POWER_ON, HIGH);
  rtc_gpio_deinit(static_cast<gpio_num_t>(PIN_BUTTON_2));
  return confirmed;
}

inline const char* enterDeviceSleep(DisplayManager& display) {
  const auto wakePin = static_cast<gpio_num_t>(PIN_BUTTON_2);
  if (digitalRead(PIN_BUTTON_2) == LOW || digitalRead(PIN_BUTTON_1) == LOW || display.touchActive()) return "Input before wake setup";
  esp_err_t error = esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);
  const char* failure = "RTC power setup failed";
  if (error == ESP_OK) { failure = "Wake pull-up failed"; error = rtc_gpio_pullup_en(wakePin); }
  if (error == ESP_OK) { failure = "Wake pull-down failed"; error = rtc_gpio_pulldown_dis(wakePin); }
  if (error == ESP_OK) {
    failure = "EXT1 wake setup failed";
    error = esp_sleep_enable_ext1_wakeup(1ULL << PIN_BUTTON_2, ESP_EXT1_WAKEUP_ANY_LOW);
  }
  if (error != ESP_OK) {
    log_e("Sleep: %s (%s)", failure, esp_err_to_name(error));
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
    return failure;
  }
  const wifi_mode_t previousWifi = WiFi.getMode();
  log_i("Sleep: Wi-Fi mode before entry=%d", static_cast<int>(previousWifi));
  bool ready = display.prepareSleep();
  failure = display.sleepFailure();
  if (ready && previousWifi != WIFI_OFF) {
    failure = "Wi-Fi shutdown failed";
    ready = WiFi.mode(WIFI_OFF);
  }
  if (ready) failure = "Button during shutdown";
  if (ready && digitalRead(PIN_BUTTON_2) != LOW && digitalRead(PIN_BUTTON_1) != LOW) {
    for (gpio_num_t pin : sleepHeldPins) {
      error = gpio_hold_en(pin);
      if (error != ESP_OK) {
        log_e("Sleep: hold GPIO %d failed (%s)", static_cast<int>(pin), esp_err_to_name(error));
        failure = "GPIO hold failed";
        ready = false;
        break;
      }
    }
    if (ready && digitalRead(PIN_BUTTON_2) != LOW && digitalRead(PIN_BUTTON_1) != LOW) {
      gpio_deep_sleep_hold_en();
      retainedSleepMarker = APPLICATION_SLEEP_MARKER;
      esp_deep_sleep_start();
    }
  }
  retainedSleepMarker = 0;
  releaseSleepHolds();
  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_EXT1);
  esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_AUTO);
  if (previousWifi != WIFI_OFF && WiFi.getMode() != previousWifi) WiFi.mode(previousWifi);
  return failure;
}

}
#endif

#endif