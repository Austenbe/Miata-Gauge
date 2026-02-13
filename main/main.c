#include <stdio.h>
#include <esp_lvgl_port.h>
#include <lvgl.h>
#include <lv_demos.h>
#include "IO_EXPANDER/PCA9554A.h"

void app_main(void)
{
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    esp_err_t err = lvgl_port_init(&lvgl_cfg);

    IO_Expander_Start();

}
