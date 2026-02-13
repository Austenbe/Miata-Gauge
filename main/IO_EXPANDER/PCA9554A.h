#include <stdio.h>
#include "driver/i2c_master.h"

void IO_Expander_Start();

void write(uint8_t* data_wr, size_t DATA_LENGTH);
void read(uint8_t* data_rd, size_t DATA_LENGTH);