#include <stdio.h>
#include "driver/i2c_master.h"
#include "PCA9554A.h"

/*
    This code configures an the ESP32 as the master controller and to communicate with a PCA9554A as setup on the Adafuit Qualia DevBoard
*/

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t expander_handle;

void IO_Expander_Start()
{
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = 0,
        .scl_io_num = 18,
        .sda_io_num = 8,
        .glitch_ignore_cnt = 7
    };

    
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus_handle));

    i2c_device_config_t OB_Expander_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = 0x3F,
        .scl_speed_hz = 400000,
    };

    
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &OB_Expander_cfg, &expander_handle));
}

void write(uint8_t* data_wr, size_t DATA_LENGTH){
    ESP_ERROR_CHECK(i2c_master_transmit(expander_handle, data_wr, DATA_LENGTH, -1));
}

void read(uint8_t* data_rd, size_t DATA_LENGTH){
    ESP_ERROR_CHECK(i2c_master_receive(expander_handle, data_rd, DATA_LENGTH, -1));
}