#include <TFT_eSPI.h>
#include <lvgl.h>

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  #include <Wire.h>
  #include <TouchLib.h>
#endif

#include "UiAdapter.h"
#include "DisplayManager.h"
#include "pin_config.h"
#include "app/SleepPolicy.h"

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 170;

enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
alignas(LV_DRAW_BUF_ALIGN) static uint16_t buf [SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft;

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
TouchLib touch(Wire, PIN_TOUCH_SDA, PIN_TOUCH_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);
static bool touchInputEnabled = false;
#endif

void display_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *pixelmap)
{
    uint32_t w = ( area->x2 - area->x1 + 1 );
    uint32_t h = ( area->y2 - area->y1 + 1 );
    tft.startWrite();
    tft.setAddrWindow( area->x1, area->y1, w, h );
    tft.pushColors( (uint16_t*) pixelmap, w * h, true );
    tft.endWrite();
    lv_disp_flush_ready( disp );
}

/*Set tick routine needed for LVGL internal timings*/
static uint32_t display_tick_get_cb (void) { return esp_timer_get_time() / 1000ULL; }

void my_input_read(lv_indev_t * indev, lv_indev_data_t*data)
{
  uint16_t x = 0, y = 0;
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  bool pressed = touchInputEnabled && touch.read();
  if (pressed) {
    TP_Point t = touch.getPoint(0);
    uint16_t temp = t.y;
    t.y = t.x;
    t.x = screenWidth - temp;
    data->point.x = t.x;
    data->point.y = t.y;
#else
  bool pressed = tft.getTouch(&x, &y);
  if(pressed) {
    data->point.x = x;
    data->point.y = y;
#endif
    data->state = LV_INDEV_STATE_PRESSED;
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void DisplayManager::init() {
  tft.init();
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  pinMode(PIN_POWER_ON, OUTPUT);
  digitalWrite(PIN_POWER_ON, HIGH);
  tft.setRotation(3);
  // touch.setRotation(3); // The lib doesn't support rot 3. Do it manually in the input read above
  touchInputEnabled = touch.init();
  if (!touchInputEnabled)
    log_e("Touch IC not found");
#else
  tft.setRotation(1);
  // uint16_t calData[5] = { 256, 3396, 466, 3400, 2 }; //  For rotation 2, 240x320 portrait
  uint16_t calData[5] = { 462, 3361, 252, 3401, 7 };  // For rotation 1, 320x170 landscape Lilygo-S3
  tft.setTouch(calData);
#endif

  lv_init();

  static lv_disp_t* disp;
  // disp = lv_tft_espi_create(screenWidth, screenHeight, buf, sizeof(buf));
  disp = lv_display_create( screenWidth, screenHeight );
  lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
  lv_display_set_buffers( disp, buf, NULL, sizeof(buf), LV_DISPLAY_RENDER_MODE_PARTIAL );
  lv_display_set_flush_cb( disp, display_flush );
  lv_tick_set_cb( display_tick_get_cb );

  static lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_input_read);

  ui::init();
  resetInactivity();
}

void DisplayManager::resetInactivity() { lv_display_trigger_activity(nullptr); }
uint32_t DisplayManager::inactiveTime() const { return lv_display_get_inactive_time(nullptr); }

bool DisplayManager::touchActive() {
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  const bool active = touchInputEnabled && touch.read();
  if (active) resetInactivity();
  return active;
#else
  return false;
#endif
}

bool DisplayManager::resetTouchController() {
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  pinMode(PIN_TOUCH_RES, OUTPUT);
  digitalWrite(PIN_TOUCH_RES, LOW);
  delay(200);
  digitalWrite(PIN_TOUCH_RES, HIGH);
  delay(200);
  Wire.beginTransmission(CTS820_SLAVE_ADDRESS);
  const uint8_t error = Wire.endTransmission();
  log_i("Sleep: touch reset address status=%u", static_cast<unsigned>(error));
  return error == 0;
#else
  return false;
#endif
}

bool DisplayManager::prepareSleep() {
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  _sleepFailure = "Touch not initialized";
  if (!touchInputEnabled) return false;
  _sleepFailure = "Input before panel sleep";
  if (touchActive() || digitalRead(PIN_BUTTON_2) == LOW || digitalRead(PIN_BUTTON_1) == LOW) return false;
  tft.writecommand(TFT_DISPOFF);
  tft.writecommand(TFT_SLPIN);
  _panelSleeping = true;
  ledcWrite(0, 0);
  ledcDetachPin(PIN_LCD_BL);
  pinMode(PIN_LCD_BL, OUTPUT);
  digitalWrite(PIN_LCD_BL, LOW);
  delay(120);
  _sleepFailure = "Input after panel sleep";
  if (touchActive() || digitalRead(PIN_BUTTON_2) == LOW || digitalRead(PIN_BUTTON_1) == LOW) return false;
  _touchSleepAttempted = true;
  touchInputEnabled = false;
  _sleepFailure = "Touch reset probe failed";
  if (!resetTouchController()) return false;
  _sleepFailure = "Input after touch reset";
  if (touch.read() || digitalRead(PIN_BUTTON_2) == LOW || digitalRead(PIN_BUTTON_1) == LOW) return false;
  const bool touchSleepFailed = touch.enableSleep();
  _sleepFailure = "Touch sleep write failed";
  log_i("Sleep: touch sleep failure flag=%d", touchSleepFailed);
  return !touchSleepFailed;
#else
  return false;
#endif
}

bool DisplayManager::restoreAfterSleepAbort(uint8_t brightness) {
  bool restored = true;
#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  if (_touchSleepAttempted) {
    restored = resetTouchController();
    _touchSleepAttempted = !restored;
    touchInputEnabled = restored;
  }
#endif
  if (_panelSleeping) {
    tft.writecommand(TFT_SLPOUT);
    delay(120);
    tft.writecommand(TFT_DISPON);
    _panelSleeping = false;
  }
  setBrightness(brightness);
  resetInactivity();
  return restored;
}

void DisplayManager::tick() {
  lv_timer_handler();
}

void DisplayManager::setBrightness(uint8_t brightness)
{
  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, brightness < 8 ? 8 : brightness);
}
