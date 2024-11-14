
#if !defined(__I2C_MASTER_H__)
#define __I2C_MASTER_H__

#include <stdio.h>
#include <stdbool.h>

#include "driver/i2c_master.h"
#include "driver/i2c_types.h"
#include "esp_err.h"
#include "esp_log.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "constants.h"

#define I2C_TAG "I2C_Master"

bool i2c_init();

bool i2c_write(uint8_t *data_wr);

#endif // __I2C_MASTER_H__
