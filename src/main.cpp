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

typedef struct _objects_t
{
  lv_obj_t *screen;
  lv_obj_t *label;
  lv_obj_t *label_rpm;
  lv_obj_t *led0;
  lv_obj_t *led1;
  lv_obj_t *led2;
  lv_obj_t *led3;
  lv_obj_t *led4;
  lv_obj_t *led5;
  lv_obj_t *led6;
  lv_obj_t *led7;
  lv_obj_t *led8;
  lv_obj_t *led9;
  lv_obj_t *led10;
} objects_t;

void setupLVGL();
void requestData();
void disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_buf);
void lv_tick_task(void *arg);
void data_request_timer_task(void *pvParameters);
void display_update_task(void *pvParameters);
void test_task(void *pvParameters);
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
    1, 12000000L /* DCLK */
);

Arduino_RGB_Display *gfx = new Arduino_RGB_Display(
    SCREEN_WIDTH /* width */, SCREEN_HEIGHT /* height */, rgbpanel, 0 /* rotation */, true /* auto_flush */,
    expander, GFX_NOT_DEFINED /* RST */, hd40015c40_init_operations, sizeof(hd40015c40_init_operations));

lv_display_t *disp;
objects_t objects;
uint8_t *disp_draw_buf;
uint8_t *disp_draw_buf2;
uint32_t screen_size = SCREEN_WIDTH * SCREEN_HEIGHT;
uint32_t bufSize = screen_size * 2; // size of the display buffer in pixels

/*******************************************************************************
 * End of Arduino_GFX and LVGL setting
 ******************************************************************************/

#define CENTER_X 360
#define CENTER_Y 360

#define TESTING

TaskHandle_t displayTaskHandle;
TaskHandle_t serialTaskHandle;
#define DATA_REQUEST_INTERVAL 1000 // ms
bool pending_response = false;
bool start_recvd = false;
const int response_size = 123;
int data_recived = 0;
uint8_t buffer[response_size];
unsigned long last_request_time = 0;

// UI data variables
char label1Text[32];
uint16_t rpm = 0;


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

  delay(100);

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

  Serial.println("LV init.");
  setupLVGL();

  printf("LVGL stride = %d\n", lv_display_get_horizontal_resolution(disp));
  printf("LVGL height = %d\n", lv_display_get_vertical_resolution(disp));
  printf("Buf size = %d\n", bufSize);

  // Create Screen task
  xTaskCreatePinnedToCore(display_update_task, "displayTask", 4096, NULL, 4, &displayTaskHandle, 1);
  Serial.println("Created Screen update task.");

// Setup data request task
#ifdef TESTING
  xTaskCreatePinnedToCore(test_task, "testTask", 2048, NULL, 1, &serialTaskHandle, 0);
#else
  xTaskCreatePinnedToCore(data_request_timer_task, "serialTask", 2048, NULL, 1, &serialTaskHandle, 0);
#endif
  Serial.println("Created data request task.");

  Serial.println("Setup done.");
}

void loop()
{
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
  disp_draw_buf = (uint8_t *)heap_caps_malloc(bufSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  // disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(bufSize * 2, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  if (!disp_draw_buf)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf = (uint8_t *)heap_caps_malloc(bufSize, MALLOC_CAP_8BIT);
  }
  if (!disp_draw_buf)
  {
    Serial.println("LVGL disp_draw_buf allocate failed!");
    return;
  }
  /*
  if (!disp_draw_buf2)
  {
    // remove MALLOC_CAP_INTERNAL flag try again
    disp_draw_buf2 = (lv_color_t *)heap_caps_malloc(bufSize * sizeof(lv_color_t), MALLOC_CAP_8BIT);
  }
  if (!disp_draw_buf2)
  {
    Serial.println("LVGL disp_draw_buf2 allocate failed!");
    return;
  }
  */
  Serial.println("Display buffers allocated.");
  // Set the display buffers
  lv_display_set_buffers(disp, disp_draw_buf, NULL, bufSize, LV_DISPLAY_RENDER_MODE_FULL);
  Serial.println("Display buffers set.");

  ///////////////////////////
  // Setup Screen Elements //
  ///////////////////////////
  objects.screen = lv_screen_active();
  // Set background color
  lv_obj_set_style_bg_color(objects.screen, lv_color_hex(0xff171b18), LV_PART_MAIN);

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
  objects.label = lv_label_create(objects.screen);
  lv_label_set_text(objects.label, "Hello world");
  lv_obj_set_style_text_color(objects.screen, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_align(objects.label, LV_ALIGN_CENTER, 0, 0);

    /*Create a white label, set its text and align it to the center*/
  objects.label_rpm = lv_label_create(objects.screen);
  lv_label_set_text(objects.label, "rpm");
  lv_obj_set_style_text_color(objects.screen, lv_color_hex(0xffffff), LV_PART_MAIN);
  lv_obj_align(objects.label, LV_ALIGN_CENTER, 0, -100);

  // Create LED ring
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 126, 113);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xff2aff00));
    lv_led_set_brightness(obj, 80);
    objects.led0 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 555, 113);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xff2aff00));
    lv_led_set_brightness(obj, 80);
    objects.led1 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 158, 82);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xff2aff00));
    lv_led_set_brightness(obj, 80);
    objects.led2 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 523, 82);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xff2aff00));
    lv_led_set_brightness(obj, 80);
    objects.led3 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 244, 34);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffffe400));
    lv_led_set_brightness(obj, 80);
    objects.led4 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 437, 34);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffffe400));
    lv_led_set_brightness(obj, 80);
    objects.led5 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 481, 55);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffffe400));
    lv_led_set_brightness(obj, 80);
    objects.led6 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 198, 55);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffffe400));
    lv_led_set_brightness(obj, 80);
    objects.led7 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 341, 16);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffff0000));
    lv_led_set_brightness(obj, 80);
    objects.led8 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 389, 23);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffff0000));
    lv_led_set_brightness(obj, 80);
    objects.led9 = obj;
  }
  {
    lv_obj_t *obj = lv_led_create(objects.screen);
    lv_obj_set_pos(obj, 292, 23);
    lv_obj_set_size(obj, 32, 32);
    lv_led_set_color(obj, lv_color_hex(0xffff0000));
    lv_led_set_brightness(obj, 80);
    objects.led10 = obj;
  }
#endif // LOCATE
}

// Make a request for data, response time is 25 ms currently
void data_request_timer_task(void *pvParameters)
{
  while (1)
  {
    if (!pending_response)
    {
      Serial2.write('n'); // Send data request command to speeduino
      last_request_time = millis();
      pending_response = true;
      vTaskDelay(pdMS_TO_TICKS(10)); // Delay for 10 ms to allow response to start coming in
    }
    else if (millis() - last_request_time > 500)
    {
      pending_response = false;
      data_recived = 0;
      start_recvd = false;
      last_request_time = 0;
      vTaskDelay(pdMS_TO_TICKS(DATA_REQUEST_INTERVAL)); // Delay for 1000 ms
      continue;
    }

    int bytes_available = Serial2.available();
    // Recieve as much data as possible, but only a small chunk per iteration to avoid blocking
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
      // Read only a small chunk per iteration to avoid blocking
      int chunk_size = 15;
      int bytes_to_read = bytes_available < chunk_size ? bytes_available : chunk_size;
      for (int i = 0; i < bytes_to_read; i++)
      {
        buffer[data_recived++] = Serial2.read();
      }
      // All Data has been received
      if (data_recived >= response_size)
      {
        data_recived = 0;
        pending_response = false;
        start_recvd = false;
        snprintf(label1Text, sizeof(label1Text), "Time: %d", buffer[0]);
        // Notify the display task to update the GUI
        xTaskNotify(displayTaskHandle, 1, eSetValueWithOverwrite);
        vTaskDelay(pdMS_TO_TICKS(DATA_REQUEST_INTERVAL)); // Delay for 1000 ms
        continue;
      }
    }
    else
    {
    }
    vTaskDelay(pdMS_TO_TICKS(1)); // Delay for 1 ms to wait for more data
  }
}

void display_update_task(void *pvParameters)
{
  uint32_t notifiedValue = 0;
  while (1)
  {
    lv_timer_handler(); /* let the GUI do its work */
    // Check for notification
    if (xTaskNotifyWait(0, ULONG_MAX, &notifiedValue, 0) == pdTRUE)
    {
      lv_label_set_text_fmt(objects.label_rpm, "%d", rpm);
      lv_label_set_text_static(objects.label, label1Text);
      vTaskDelay(pdMS_TO_TICKS(4));
      continue;
    }

    vTaskDelay(pdMS_TO_TICKS(5)); // Delay for 5 ms
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

#ifdef TESTING
void test_task(void *pvParameters)
{
  uint8_t time = 0;
  bool rpmDir = true;
  while (1)
  {
    if (rpm >= 7000)
    {
      rpmDir = false;
    }
    else if (rpm <= 0)
    {
      rpmDir = true;
    }
    rpm = rpmDir ? rpm + 100 : rpm - 100;
    snprintf(label1Text, sizeof(label1Text), "Time: %d", time++);
    xTaskNotify(displayTaskHandle, 1, eSetValueWithOverwrite);
    vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1 second
  }
}
#endif