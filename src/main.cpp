// SPDX-FileCopyrightText: 2023 Limor Fried for Adafruit Industries
//
// SPDX-License-Identifier: MIT

/*
GFX 1.4.7 there is no support for Arduino_ESP32RGBPanel in "GFX Library for Arduino"
for Boards from 3.0.0 on so
be sure to revert to 2.0.17 of boards.
It will take a couple itterations through bootsel/port to get the coms stabalized.
*/
/*******************************************************************************
 * Start of Arduino_GFX setting
 ******************************************************************************/

#include <Arduino_GFX_Library.h>
#include <Adafruit_GFX.h>
#include <lvgl.h>
#include <Fonts/FreeMono24pt7b.h>
#include <Fonts/FreeMono18pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSans24pt7b.h>
#include <Fonts/FreeSans18pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSansBold24pt7b.h>
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>
#include <Fonts/FreeSansBold9pt7b.h>
#include <Fonts/FreeSerif24pt7b.h>
#include <Fonts/FreeSerif18pt7b.h>
#include <Fonts/FreeSerif12pt7b.h>
#include <Fonts/FreeSerif9pt7b.h>
#include <Fonts/FreeSerifBold24pt7b.h>
#include <Fonts/FreeSerifBold18pt7b.h>
#include <Fonts/FreeSerifBold12pt7b.h>
#include <Fonts/FreeSerifBold9pt7b.h>

void generateColorWheel();
void setupLVGL();
unsigned long testFillScreenOnce(int);
unsigned long testText(uint8_t, uint8_t);
unsigned long testLines(uint16_t);
unsigned long testFastLines(uint16_t, uint16_t);
unsigned long testFilledRects(uint16_t, uint16_t);
unsigned long testCircles(uint8_t, uint16_t);
unsigned long testTriangles();
unsigned long testFilledTriangles();
unsigned long testFillScreen();
unsigned long testRects(uint16_t);
unsigned long testFilledCircles(uint8_t, uint16_t);
unsigned long testRoundRects();
unsigned long testFilledRoundRects();
void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf);
unsigned long testColorWheel();
int16_t testTextMakeX();

#define SCREEN_WIDTH 720
#define SCREEN_HEIGHT 720

Arduino_XCA9554SWSPI *expander = new Arduino_XCA9554SWSPI(
    PCA_TFT_RESET, PCA_TFT_CS, PCA_TFT_SCK, PCA_TFT_MOSI,
    &Wire, 0x3F);

// 4.0" 720x720 round display
Arduino_ESP32RGBPanel *rgbpanel = new Arduino_ESP32RGBPanel(
    TFT_DE, TFT_VSYNC, TFT_HSYNC, TFT_PCLK,
    TFT_R1, TFT_R2, TFT_R3, TFT_R4, TFT_R5,
    TFT_G0, TFT_G1, TFT_G2, TFT_G3, TFT_G4, TFT_G5,
    TFT_B1, TFT_B2, TFT_B3, TFT_B4, TFT_B5,
    1 /* hync_polarity */, 46 /* hsync_front_porch */, 2 /* hsync_pulse_width */, 44 /* hsync_back_porch */,
    1 /* vsync_polarity */, 50 /* vsync_front_porch */, 16 /* vsync_pulse_width */, 16 /* vsync_back_porch */,
    1, 35000000L /* DCLK */
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    expander, GFX_NOT_DEFINED /* RST */, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));

lv_display_t *disp;
lv_color_t *disp_draw_buf;
lv_color_t *disp_draw_buf2;
uint32_t bufSize;

/*******************************************************************************
 * End of Arduino_GFX setting
 ******************************************************************************/

#define TEST_DELAY 500
#define FILL_WAIT 400
#define TEXT_X 200
#define TEXT_Y 150

uint16_t *colorWheel;
int width;
int height;
int half_width;
int half_height;
uint8_t rotation;

// LVGL logging callback with correct signature
extern "C" void my_lvgl_log_cb(lv_log_level_t level, const char *buf)
{
  Serial.print("[LVGL] ");
  Serial.println(buf);
}

void lv_tick_task(void *arg)
{
  lv_tick_inc(1);
}

void setup(void)
{
  Serial.begin(115200);

  delay(500);
  rotation = 0;
#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

  Serial.println("Beginning");
  // Init Display

  Wire.setClock(1000000); // speed up I2C
  Serial.println("Initialize gfx->");
  if (!gfx->begin())
  {
    Serial.println("gfx->begin() failed!");
  }
  if (gfx->enableRoundMode())
  {
    Serial.println("RoundMode enabled.");
  }
  // Set global reused values once to not to make so many function calls
  width = gfx->width();
  height = gfx->height();
  half_width = width / 2;
  half_height = height / 2;
  bufSize = width * height / 2;

  Serial.println("LV init.");
  setupLVGL();

  Serial.println("Setup done.");
}

void loop()
{
  lv_timer_handler(); /* let the GUI do its work */
  delay(5);
}

/* Display flushing */
void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf)
{

  Serial.println("Flushing buffer to display.");
  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_buf, w, h);

  lv_disp_flush_ready(disp);
}

void setupLVGL()
{
  lv_init();

  // Setup tick timer
  const esp_timer_create_args_t lv_timer_args = {
      .callback = &lv_tick_task,
      .name = "lv_tick"};
  esp_timer_handle_t lv_timer;
  esp_timer_create(&lv_timer_args, &lv_timer);
  esp_timer_start_periodic(lv_timer, 1000); // 1000 µs = 1 ms

  // Register LVGL logging callback
  lv_log_register_print_cb(my_lvgl_log_cb);

  disp = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(disp, disp_flush);
  Serial.println("Display Created.");

  // Allocate the display buffer
  disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
    return;
  }
  if (!disp_draw_buf2)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_8BIT);
  }
  if (!disp_draw_buf2)
  {
    Serial.println("LVGL disp_draw_buf2 allocate failed!");
    return;
  }

  Serial.println("Display buffers allocated.");
  // Set the display buffers
  lv_display_set_buffers(disp, disp_draw_buf, disp_draw_buf2, bufSize, LV_DISPLAY_RENDER_MODE_PARTIAL);
  Serial.println("Display buffers set.");

  ///////////////////////////
  // Setup Screen Elements //
  ///////////////////////////
  #define LOCATE

  #ifdef LOCATE
  // Set background color
  lv_obj_set_style_bg_color(lv_screen_active(), lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

  // Cross position variables
  static int cross_x = 360;
  static int cross_y = 360;

  // Horizontal scale
  lv_obj_t *h_scale = lv_scale_create(lv_screen_active());
  lv_obj_set_size(h_scale, 720, 60);
  lv_scale_set_mode(h_scale, LV_SCALE_MODE_HORIZONTAL_BOTTOM);
  lv_obj_set_style_bg_color(h_scale, lv_color_black(), LV_PART_MAIN);
  lv_obj_align(h_scale, LV_ALIGN_TOP_LEFT, 0, cross_y);
  lv_scale_set_label_show(h_scale, true);
  lv_scale_set_total_tick_count(h_scale, 25); // 700/50 = 14 intervals, 15 ticks
  lv_scale_set_major_tick_every(h_scale, 1); // every tick is major
  lv_obj_set_style_length(h_scale, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(h_scale, 10, LV_PART_INDICATOR);
  lv_scale_set_range(h_scale, 0, 720);

  // Vertical scale
  lv_obj_t *v_scale = lv_scale_create(lv_screen_active());
  lv_obj_set_size(v_scale, 60, 720);
  lv_scale_set_mode(v_scale, LV_SCALE_MODE_VERTICAL_RIGHT);
  lv_obj_set_style_bg_color(v_scale, lv_color_black(), LV_PART_MAIN);
  lv_obj_align(v_scale, LV_ALIGN_TOP_LEFT, cross_x, 0);
  lv_scale_set_label_show(v_scale, true);
  lv_scale_set_total_tick_count(v_scale, 25); // 700/50 = 14 intervals, 15 ticks
  lv_scale_set_major_tick_every(v_scale, 1);
  lv_obj_set_style_length(v_scale, 5, LV_PART_ITEMS);
  lv_obj_set_style_length(v_scale, 10, LV_PART_INDICATOR);
  lv_scale_set_range(v_scale, 720, 0);
  
  #else
  

  #endif // LOCATE
}

void runBenchmark(void)
{
  /*
  Serial.println(F("Benchmark                Time (microseconds)"));
  delay(TEST_DELAY);
  Serial.print(F("Screen fill Once RED     "));
  Serial.println(testFillScreenOnce(RED));
  delay(TEST_DELAY * 10);

  Serial.print(F("Screen fill Once BLUE    "));
  Serial.println(testFillScreenOnce(BLUE));
  delay(TEST_DELAY * 10);

  Serial.print(F("Screen fill Once GREEN   "));
  Serial.println(testFillScreenOnce(GREEN));
  delay(TEST_DELAY * 10);


  Serial.print(F("Fill "));
  Serial.println(testFillScreen());
  delay(TEST_DELAY);

  gfx->setFont();
  Serial.print(F("Text default             "));
  Serial.println(testText(0, 3));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMono24pt7b);
  Serial.print(F("Text Mono24              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMono18pt7b);
  Serial.print(F("Text Mono18              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMono12pt7b);
  Serial.print(F("Text Mono12              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMono9pt7b);
  Serial.print(F("Text Mono9               "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMonoBold24pt7b);
  Serial.print(F("Text MonoBold24          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMonoBold18pt7b);
  Serial.print(F("Text MonoBold18          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMonoBold12pt7b);
  Serial.print(F("Text MonoBold12          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeMonoBold9pt7b);
  Serial.print(F("Text MonoBold9           "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSans24pt7b);
  Serial.print(F("Text Sans24              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSans18pt7b);
  Serial.print(F("Text Sans18              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSans12pt7b);
  Serial.print(F("Text Sans12              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSans9pt7b);
  Serial.print(F("Text Sans9               "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSansBold24pt7b);
  Serial.print(F("Text SansBold24          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSansBold18pt7b);
  Serial.print(F("Text SansBold18          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSansBold12pt7b);
  Serial.print(F("Text SansBold12          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSansBold9pt7b);
  Serial.print(F("Text SansBold9           "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerif24pt7b);
  Serial.print(F("Text Serif24             "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerif18pt7b);
  Serial.print(F("Text Serif18             "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerif12pt7b);
  Serial.print(F("Text Serif12             "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerif9pt7b);
  Serial.print(F("Text Serif9              "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerifBold24pt7b);
  Serial.print(F("Text SerifBold24         "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerifBold18pt7b);
  Serial.print(F("Text SerifBold18         "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerifBold12pt7b);
  Serial.print(F("Text SerifBold12         "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);

  gfx->setFont(&FreeSerifBold9pt7b);
  Serial.print(F("Text SerifBold9          "));
  Serial.println(testText(rotation, rotation));
  delay(TEST_DELAY * 4);
  */

  Serial.print(F("Lines                    "));
  Serial.println(testLines(CYAN));
  delay(TEST_DELAY);

  Serial.print(F("Horiz/Vert Lines         "));
  Serial.println(testFastLines(RED, BLUE));
  delay(TEST_DELAY);

  Serial.print(F("Rectangles (outline)     "));
  Serial.println(testRects(GREEN));
  delay(TEST_DELAY);

  Serial.print(F("Rectangles (filled)      "));
  Serial.println(testFilledRects(YELLOW, MAGENTA));
  delay(TEST_DELAY);

  Serial.print(F("Circles (filled)         "));
  Serial.println(testFilledCircles(10, MAGENTA));
  delay(TEST_DELAY);

  Serial.print(F("Circles (outline)        "));
  Serial.println(testCircles(10, WHITE));
  delay(TEST_DELAY);

  Serial.print(F("Triangles (outline)      "));
  Serial.println(testTriangles());
  delay(TEST_DELAY);

  Serial.print(F("Triangles (filled)       "));
  Serial.println(testFilledTriangles());
  delay(TEST_DELAY);

  Serial.print(F("Rounded rects (outline)  "));
  Serial.println(testRoundRects());
  delay(TEST_DELAY);

  Serial.print(F("Rounded rects (filled)   "));
  Serial.println(testFilledRoundRects());
  delay(TEST_DELAY);

  Serial.print(F("Color Wheel              "));
  Serial.println(testColorWheel());
  delay(TEST_DELAY);

  Serial.println(F("Done!"));
}

/* void loop(void) {
  Serial.printf("%s\n", __FILE__);
  Serial.printf("SDK Version       : %s\n", ESP.getSdkVersion());
  Serial.print("Sketch MD5        : ");
  Serial.println(ESP.getSketchMD5());
  Serial.printf("ESP32 Chip model  : %s, Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());

#ifdef USB_VID
  Serial.printf("ESP32 VID, PID    : 0x%x, 0x%x\n", USB_VID, USB_PID);
#endif
#ifdef USB_MANUFACTURER
  Serial.printf("Manufacturer      : %s %s\n", USB_MANUFACTURER, USB_PRODUCT);
#endif
  for (rotation = 0; rotation < 4; rotation++) {
    runBenchmark();
    gfx->setRotation(rotation);
    delay(1000);
  }
}

unsigned long testFillScreenOnce(int color) {
  unsigned long start, t = 0;
  start = micros();
  gfx->fillScreen(color);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  return t;
} */

unsigned long testFillScreen()
{
  unsigned long start, t = 0;
  Serial.print(".");
  start = micros();
  gfx->fillScreen(BLACK);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(NAVY);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(DARKGREEN);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(DARKCYAN);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(MAROON);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(PURPLE);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(OLIVE);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(LIGHTGREY);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(DARKGREY);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(BLUE);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(GREEN);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(CYAN);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(RED);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(MAGENTA);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(YELLOW);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(WHITE);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(ORANGE);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(GREENYELLOW);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(".");
  start = micros();
  gfx->fillScreen(PALERED);
  t += micros() - start;
  yield();
  delay(FILL_WAIT);
  Serial.print(" ");
  return t;
}

unsigned long testText(uint8_t rotS, uint8_t rotE)
{
  gfx->fillScreen(BLACK);
  uint8_t rot = rotS; // starting rotation position
  unsigned long start = micros();
  do
  {
    gfx->setRotation(rot);
    gfx->setCursor(TEXT_X, TEXT_Y);
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->setTextColor(WHITE);
    gfx->setTextSize(1);
    gfx->println("Hello World!");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->setTextColor(YELLOW);
    gfx->setTextSize(2);
    gfx->println(1234.56);
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->setTextColor(RED);
    gfx->setTextSize(3);
    gfx->println(0xDEADBEEF, HEX);
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println();
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->setTextColor(GREEN);
    gfx->setTextSize(5);
    gfx->println("Groop");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->setTextSize(2);
    gfx->println("I implore thee,");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->setTextSize(1);
    gfx->println("my foonting turlingdromes.");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println("And hooptiously drangle me");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println("with crinkly bindlewurdles,");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println("Or I will rend thee");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println("in the gobberwarts");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println("with my blurglecruncheon,");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
    gfx->println("see if I don't!");
    gfx->setCursor(testTextMakeX(), gfx->getCursorY());
  } while (++rot <= rotE);
  unsigned long t = micros() - start;
  // delay(TEST_DELAY * 4);
  gfx->setRotation(rotation); // put the rotation back
  return t;
}

int16_t testTextMakeX()
{
  // Calculate the farthest visible pixels on this row
  int dy = gfx->getCursorY() - half_height;
  int dx = sqrt(sq(half_height) - sq(dy)); // sq() is the square function in Arduino

  return (half_width - dx) + 10;
}

unsigned long testLines(uint16_t color)
{
  unsigned long start, t;
  int x1, y1, x2, y2,
      w = width,
      h = height;

  gfx->fillScreen(BLACK);
  yield();

  x1 = y1 = 0;
  y2 = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  x2 = w - 1;
  for (y2 = 0; y2 < h; y2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  t = micros() - start; // fillScreen doesn't count against timing

  yield();
  gfx->fillScreen(BLACK);
  yield();

  x1 = w - 1;
  y1 = 0;
  y2 = h - 1;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  x2 = 0;
  for (y2 = 0; y2 < h; y2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  t += micros() - start;

  yield();
  gfx->fillScreen(BLACK);
  yield();

  x1 = 0;
  y1 = h - 1;
  y2 = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  x2 = w - 1;
  for (y2 = 0; y2 < h; y2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  t += micros() - start;

  yield();
  gfx->fillScreen(BLACK);
  yield();

  x1 = w - 1;
  y1 = h - 1;
  y2 = 0;
  start = micros();
  for (x2 = 0; x2 < w; x2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);
  x2 = 0;
  for (y2 = 0; y2 < h; y2 += 6)
    gfx->drawLine(x1, y1, x2, y2, color);

  yield();
  return micros() - start;
}

unsigned long testFastLines(uint16_t color1, uint16_t color2)
{
  unsigned long start;
  int x, y, w = width, h = height;

  gfx->fillScreen(BLACK);
  start = micros();
  for (y = 0; y < h; y += 5)
    gfx->drawFastHLine(0, y, w, color1);
  for (x = 0; x < w; x += 5)
    gfx->drawFastVLine(x, 0, h, color2);

  return micros() - start;
}

unsigned long testRects(uint16_t color)
{
  unsigned long start;
  int n, i, i2,
      cx = width / 2,
      cy = height / 2;

  gfx->fillScreen(BLACK);
  n = min(width, height);
  start = micros();
  for (i = 2; i < n; i += 6)
  {
    i2 = i / 2;
    gfx->drawRect(cx - i2, cy - i2, i, i, color);
  }

  return micros() - start;
}

unsigned long testFilledRects(uint16_t color1, uint16_t color2)
{
  unsigned long start, t = 0;
  int n, i, i2,
      cx = width / 2 - 1,
      cy = height / 2 - 1;

  gfx->fillScreen(BLACK);
  n = min(width, height);
  for (i = n; i > 0; i -= 6)
  {
    i2 = i / 2;
    start = micros();
    gfx->fillRect(cx - i2, cy - i2, i, i, color1);
    t += micros() - start;
    // Outlines are not included in timing results
    gfx->drawRect(cx - i2, cy - i2, i, i, color2);
    yield();
  }

  return t;
}

unsigned long testFilledCircles(uint8_t radius, uint16_t color)
{
  unsigned long start;
  int x, y, w = width, h = height, r2 = radius * 2;

  gfx->fillScreen(BLACK);
  start = micros();
  for (x = radius; x < w; x += r2)
  {
    for (y = radius; y < h; y += r2)
    {
      gfx->fillCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testCircles(uint8_t radius, uint16_t color)
{
  unsigned long start;
  int x, y, r2 = radius * 2,
            w = width + radius,
            h = height + radius;

  // Screen is not cleared for this one -- this is
  // intentional and does not affect the reported time.
  start = micros();
  for (x = 0; x < w; x += r2)
  {
    for (y = 0; y < h; y += r2)
    {
      gfx->drawCircle(x, y, radius, color);
    }
  }

  return micros() - start;
}

unsigned long testTriangles()
{
  unsigned long start;
  int n, i, cx = width / 2 - 1,
            cy = height / 2 - 1;

  gfx->fillScreen(BLACK);
  n = min(cx, cy);
  start = micros();
  for (i = 0; i < n; i += 5)
  {
    gfx->drawTriangle(
        cx, cy - i,     // peak
        cx - i, cy + i, // bottom left
        cx + i, cy + i, // bottom right
        gfx->color565(i, i, i));
  }

  return micros() - start;
}

unsigned long testFilledTriangles()
{
  unsigned long start, t = 0;
  int i, cx = width / 2 - 1,
         cy = height / 2 - 1;

  gfx->fillScreen(BLACK);
  start = micros();
  for (i = min(cx, cy); i > 10; i -= 5)
  {
    start = micros();
    gfx->fillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                      gfx->color565(0, i * 10, i * 10));
    t += micros() - start;
    gfx->drawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
                      gfx->color565(i * 10, i * 10, 0));
    yield();
  }

  return t;
}

unsigned long testRoundRects()
{
  unsigned long start;
  int w, i, i2,
      cx = width / 2 - 1,
      cy = height / 2 - 1;

  gfx->fillScreen(BLACK);
  w = min(width, height);
  start = micros();
  for (i = 0; i < w; i += 6)
  {
    i2 = i / 2;
    gfx->drawRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx->color565(i, 0, 0));
  }

  return micros() - start;
}

unsigned long testFilledRoundRects()
{
  unsigned long start;
  int i, i2,
      cx = width / 2 - 1,
      cy = height / 2 - 1;

  gfx->fillScreen(BLACK);
  start = micros();
  for (i = min(width, height); i > 20; i -= 6)
  {
    i2 = i / 2;
    gfx->fillRoundRect(cx - i2, cy - i2, i, i, i / 8, gfx->color565(0, i, 0));
    yield();
  }

  return micros() - start;
}

unsigned long testColorWheel()
{
  unsigned long start;
  gfx->fillScreen(BLACK);
  start = micros();
  gfx->draw16bitRGBBitmap(0, 0, colorWheel, width, height);
  return micros() - start;
}

// https://chat.openai.com/share/8edee522-7875-444f-9fea-ae93a8dfa4ec
void generateColorWheel()
{
  float angle;
  uint8_t r, g, b;
  int index, scaled_index;

  for (int y = 0; y < half_height; y++)
  {
    for (int x = 0; x < half_width; x++)
    {
      index = y * half_width + x;
      angle = atan2(y - half_height / 2, x - half_width / 2);
      r = uint8_t(127.5 * (cos(angle) + 1));
      g = uint8_t(127.5 * (sin(angle) + 1));
      b = uint8_t(255 - (r + g) / 2);
      uint16_t color = RGB565(r, g, b);

      // Scale this pixel into 4 pixels in the full buffer
      for (int dy = 0; dy < 2; dy++)
      {
        for (int dx = 0; dx < 2; dx++)
        {
          scaled_index = (y * 2 + dy) * width + (x * 2 + dx);
          colorWheel[scaled_index] = color;
        }
      }
    }
  }
}
