#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/random.h>
#include "driver/i2c_slave.h"
#include "esp_log.h"
// Include FreeRTOS for delay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "i2c_controller.h"
#include "sub.h"

// fun fact, if two subs generate the same i2cChannel AND the same secretMessage pair, they will behave as the same entity
int i2cChannel = 29;
char secretMessage[] = "C8";
char *lastMessage = "";
bool setupComplete = false;
// bool storeConnectionSettings = false;    // TODO: We could reuse these if the DOM implemented saving and scanning reserved channels
// bool troubleshootConnectionInit = false; // reset after a successful setupComplete
// bool testCollision = false;
unsigned long setupStartMillis = 0;
i2c_slave_dev_handle_t i2c_bus_handle;

void subSetup()
{
    ESP_LOGI(TAG, "sub i2c init");

    // TODO: add a way to test the collision
    // else if (testCollision && !preferences.getBool("tested_collision"))
    // {
    //     reinit();
    //     i2cChannel = 29;
    //     preferences.putBool("tested_collision", true);
    // }
    i2cChannel = randInRange(21, 127);
    generateIdentity(secretMessage);
    ESP_LOGD(TAG, "New identity: %s", secretMessage);

    subReinit();
    ESP_LOGI(TAG, "joined i2c channel %d", i2cChannel);

    // Wire.begin(i2cChannel);
    // Wire.onReceive(receiveEvent);
    // Wire.onRequest(requestEvent);
}

void generateIdentity(char *output)
{
    const char alphanum[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    size_t alphanum_len = sizeof(alphanum) - 1;
    unsigned char random_bytes[2];

    // Generate 2 random bytes
    getrandom(random_bytes, sizeof(random_bytes), 0);

    // Map the random bytes to alphanumeric characters
    output[0] = alphanum[random_bytes[0] % alphanum_len];
    output[1] = alphanum[random_bytes[1] % alphanum_len];
    // output[2] = '\0'; // Null-terminate the string
}

int randInRange(int min, int max)
{
    unsigned char random_byte;

    // Generate a random byte
    getrandom(&random_byte, sizeof(random_byte), 0);

    // Map the random byte to the desired range
    int range = max - min + 1;
    int random_number = (random_byte % range) + min;

    return random_number;
}

void subReinit()
{
    i2c_slave_config_t i2c_slv_config = {
        .addr_bit_len = I2C_ADDR_BIT_LEN_7, // 7-bit address
        .clk_source = I2C_CLK_SRC_DEFAULT,  // set the clock source
        .i2c_port = I2C_NUM_0,              // set I2C port number
        .send_buf_depth = 256,              // set TX buffer length
        .scl_io_num = 5,                    // SCL GPIO number
        .sda_io_num = 4,                    // SDA GPIO number
        .slave_addr = i2cChannel,                 // slave address
    };
    if (i2c_bus_handle != NULL)
    {
        if(i2c_del_slave_device(i2c_bus_handle) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to delete I2C slave device");
            return;
        }
    }
    if (i2c_new_slave_device(&i2c_slv_config, &i2c_bus_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C slave device");
        return;
    }
}