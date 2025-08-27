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

void setupLVGL();
void requestData();
void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf);
void lv_tick_task(void *arg);
void data_request_timer_task(void *pvParameters);
void display_update_task(void *pvParameters);
void data_recieve_task(void *pvParameters);
void my_lvgl_log_cb(lv_log_level_t level, const char *buf);

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

#define CENTER_X 360
#define CENTER_Y 360

#define DATA_REQUEST_INTERVAL 1000 // ms
bool pending_response = false;
bool start_recvd = false;
const int response_size = 123;
int data_recived = 0;
uint8_t buffer[response_size];
unsigned long last_request_time = 0;

lv_obj_t *label1;
char label1Text[32];
uint16_t *colorWheel;
int width;
int height;
int half_width;
int half_height;

void setup(void)
{
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, A0, A1);
  Serial2.setTimeout(5);
  Wire.setClock(1000000); // speed up I2C

  while (!Serial)
  {
    delay(10);
  }

  Serial.println("Hello");
  // Setup data request task
  xTaskCreatePinnedToCore(data_request_timer_task, "serialTask", 512, NULL, 1, NULL, 0);
  Serial.println("Created task 1");

  // Create Screen task
  //xTaskCreatePinnedToCore(display_update_task, "displayTask", 4096, NULL, 2, NULL, 1);
  Serial.println("Created task 2");
  delay(500);

#ifdef GFX_EXTRA_PRE_INIT
  GFX_EXTRA_PRE_INIT();
#endif

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

}

void requestData()
{
  int timeout = 20; // ms
  uint8_t buffer[256];

  // flush input buffer
  uint32_t sendTime = millis();
  Serial2.write('n');

  // wait for data or timeout
  uint32_t start = millis();
  uint32_t end = start;
  while (Serial2.available() < 3 && (end - start) < timeout)
  {
    end = millis();
  }

  // if within timeout, read data
  if (end - start < timeout)
  {
    Serial.print("Recieved in: ");
    Serial.println(end - sendTime);
    Serial.print("Recieved in without write: ");
    Serial.println(end - start);
    // skip first two bytes
    Serial2.read(); // 'n'
    Serial2.read(); // 0x32
    uint8_t dataLen = Serial2.read();
    Serial.println("Data length: " + String(dataLen));
    Serial2.readBytes(buffer, dataLen);

    // Print data
    Serial.print("secL: ");
    Serial.println(buffer[0]);
    Serial.print("Baro: ");
    Serial.println(buffer[40]);
  }
  else
  {
    Serial.print("Timeout exceeded. Took: ");
    Serial.println(end - start);
  }
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
  // Set background color
  lv_obj_set_style_bg_color(lv_screen_active(), lv_palette_main(LV_PALETTE_AMBER), LV_PART_MAIN);

  // #define LOCATE

#ifdef LOCATE

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
  lv_scale_set_major_tick_every(h_scale, 1);  // every tick is major
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
  /*Create a white label, set its text and align it to the center*/
  label1 = lv_label_create(lv_screen_active());
  lv_label_set_text(label1, "Hello world");
  lv_obj_set_style_text_color(lv_screen_active(), lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_align(label1, LV_ALIGN_CENTER, 0, 0);
#endif // LOCATE
}

void data_request_timer_task(void *pvParameters)
{
  while (1)
  {
    if (!pending_response)
    {
      Serial2.write('n'); // Send data request command to speeduino
      last_request_time = millis();
      pending_response = true;
      vTaskDelay(pdMS_TO_TICKS(2)); // Delay for 2 ms to allow response to start coming in
    }
    int bytes_available = Serial2.available();
    if (bytes_available >= 3)
    {
      // skip first two bytes and data length
      if (!start_recvd)
      {
        Serial2.read(); // 'n'
        Serial2.read(); // 0x32
        Serial2.read(); // dataLen
        bytes_available -= 3;
        start_recvd = true;
      }
      for (int i = 0; i < bytes_available; i++)
      {
        buffer[data_recived + i] = Serial2.read();
      }
      data_recived += bytes_available;
      // Data has been received
      if (data_recived >= response_size)
      {
        data_recived = 0;
        pending_response = false;
        start_recvd = false;
        snprintf(label1Text, sizeof(label1Text), "Time: %d", buffer[0]);
        lv_label_set_text(label1, label1Text);
        vTaskDelay(pdMS_TO_TICKS(DATA_REQUEST_INTERVAL)); // Delay for 1000 ms
      }
      else {
        vTaskDelay(pdMS_TO_TICKS(2)); // Delay for 2 ms to wait for more data
      }
    }
    else {
      vTaskDelay(pdMS_TO_TICKS(2)); // Delay for 2 ms to wait for more data
    }
  }
}

void data_recieve_task(void *pvParameters)
{
  while (1)
  {
    if (pending_response)
    {
      int bytes_available = Serial2.available();
      if (bytes_available >= 3)
      {
        // skip first two bytes and data length
        if (!start_recvd)
        {
          Serial2.read(); // 'n'
          Serial2.read(); // 0x32
          Serial2.read(); // dataLen
          bytes_available -= 3;
          start_recvd = true;
        }
        for (int i = 0; i < bytes_available; i++)
        {
          buffer[data_recived + i] = Serial2.read();
        }
        data_recived += bytes_available;
        // Data has been received
        if (data_recived >= response_size)
        {
          data_recived = 0;
          pending_response = false;
          start_recvd = false;
          snprintf(label1Text, sizeof(label1Text), "Time: %d", buffer[0]);
          lv_label_set_text(label1, label1Text);
        }
      }
    }
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay for 5 ms to yield to other tasks
  }
}

void display_update_task(void *pvParameters)
{
  while (1)
  {
    lv_timer_handler();           /* let the GUI do its work */
    vTaskDelay(pdMS_TO_TICKS(5)); // Delay for 1000 ms
  }
}

void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf)
{

  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  gfx->draw16bitRGBBitmap(area->x1, area->y1, (uint16_t *)px_buf, w, h);

  lv_disp_flush_ready(disp);
}

void lv_tick_task(void *arg)
{
  lv_tick_inc(1);
}

// LVGL logging callback with correct signature
void my_lvgl_log_cb(lv_log_level_t level, const char *buf)
{
  Serial.print("[LVGL] ");
  Serial.println(buf);
}