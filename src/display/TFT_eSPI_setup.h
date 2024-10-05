  // Configuration file for TFT_eSPI displlay library
  // Update the values if the display or pins change

#ifdef ARDUINO_LILYGO_T_DISPLAY_S3
  // ST7789 using 8-bit Parallel

  #define USER_SETUP_ID 206

  #define ST7789_DRIVER
  #define INIT_SEQUENCE_3 // Using this initialisation sequence improves the display image

  #define CGRAM_OFFSET
  #define TFT_RGB_ORDER TFT_RGB  // Colour order Red-Green-Blue
  //#define TFT_RGB_ORDER TFT_BGR // Colour order Blue-Green-Red

  #define TFT_INVERSION_ON
  // #define TFT_INVERSION_OFF

  #define TFT_PARALLEL_8_BIT

  #define TFT_WIDTH 170
  #define TFT_HEIGHT 320

  #define TFT_CS  6
  #define TFT_DC  7
  #define TFT_RST 5

  #define TFT_WR 8
  #define TFT_RD 9

  #define TFT_D0 39
  #define TFT_D1 40
  #define TFT_D2 41
  #define TFT_D3 42
  #define TFT_D4 45
  #define TFT_D5 46
  #define TFT_D6 47
  #define TFT_D7 48

  #define TFT_BL 38
  #define TFT_BACKLIGHT_ON HIGH

  #define LOAD_GLCD
  #define LOAD_FONT2
  #define LOAD_FONT4
  #define LOAD_FONT6
  #define LOAD_FONT7
  #define LOAD_FONT8
  #define LOAD_GFXFF

  #define SMOOTH_FONT

#else
  #define ILI9341_DRIVER

  #define TFT_MISO 19  // (leave TFT SDO disconnected if other SPI devices share MISO)
  #define TFT_MOSI 23
  #define TFT_SCLK 18
  #define TFT_CS   5  // Chip select control pin
  #define TFT_DC    4  // Data Command control pin
  #define TFT_RST   12  // Reset pin (could connect to RST pin)

  // Optional touch screen chip select
  #define TOUCH_CS 16 // Chip select pin (T_CS) of touch screen

  #define LOAD_GLCD    // Font 1. Original Adafruit 8 pixel font needs ~1820 bytes in FLASH
  #define LOAD_FONT2   // Font 2. Small 16 pixel high font, needs ~3534 bytes in FLASH, 96 characters
  #define LOAD_FONT4   // Font 4. Medium 26 pixel high font, needs ~5848 bytes in FLASH, 96 characters
  #define LOAD_GFXFF   // FreeFonts. Include access to the 48 Adafruit_GFX free fonts FF1 to FF48 and custom fonts

  #define SMOOTH_FONT

  // TFT SPI clock frequency
  // #define SPI_FREQUENCY  20000000
  // #define SPI_FREQUENCY  27000000
  #define SPI_FREQUENCY  40000000
  // #define SPI_FREQUENCY  80000000

  // Optional reduced SPI frequency for reading TFT
  #define SPI_READ_FREQUENCY  16000000
#endif