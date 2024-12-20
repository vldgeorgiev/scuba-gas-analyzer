#include <TFT_eSPI.h>
#include <lvgl.h>

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  #include <Wire.h>
  #include <TouchLib.h>
#endif

#include "ui.h"
#include "DisplayManager.h"
#include "pin_config.h"

static const uint16_t screenWidth  = 320;
static const uint16_t screenHeight = 170;

enum { SCREENBUFFER_SIZE_PIXELS = screenWidth * screenHeight / 10 };
static lv_color_t buf [SCREENBUFFER_SIZE_PIXELS];

TFT_eSPI tft;

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
TouchLib touch(Wire, PIN_TOUCH_SDA, PIN_TOUCH_SCL, CTS820_SLAVE_ADDRESS, PIN_TOUCH_RES);
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
  bool pressed = touch.read();
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
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);
  tft.setRotation(3);
  // touch.setRotation(3); // The lib doesn't support rot 3. Do it manually in the input read above
  if (!touch.init())
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
  lv_display_set_buffers( disp, buf, NULL, SCREENBUFFER_SIZE_PIXELS * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL );
  lv_display_set_flush_cb( disp, display_flush );
  lv_tick_set_cb( display_tick_get_cb );

  static lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, my_input_read);

  ui_init();
}

void DisplayManager::tick() {
  lv_task_handler();
  lv_timer_handler();
  lv_tick_inc(5);
  ui_tick();
}

void DisplayManager::setBrightness(uint8_t brightness)
{
  ledcSetup(0, 10000, 8);
  ledcAttachPin(PIN_LCD_BL, 0);
  ledcWrite(0, brightness);
}
