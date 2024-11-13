#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/random.h>
#include "driver/i2c_slave.h"
#include "esp_log.h"
// Include FreeRTOS for threading
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
bool i2c_bus_initialized = false;
QueueHandle_t s_receive_queue;
TaskHandle_t i2c_handle = NULL;

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
    ESP_LOGI(TAG, "New identity: %s", secretMessage);

    subReinit();
    // xTaskCreate(subReinitTask, "subReinitTask", 2048, NULL, 5, &i2c_handle);
    ESP_LOGI(TAG, "joined i2c channel %d", i2cChannel);

    // Wire.begin(i2cChannel);
    // Wire.onReceive(receiveEvent);
    // Wire.onRequest(requestEvent);
}

void subRespond(char *message)
{
    int len = strlen(message);
    uint8_t *response = malloc(len);
    memcpy(response, message, len);
    
    esp_err_t ret = i2c_slave_transmit(i2c_bus_handle, response, len, 10000);
    free(response);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Response sent: %s", message);
    } else {
        ESP_LOGE(TAG, "Failed to send response");
    }
}

void receiveMessage(char *message) {
    ESP_LOGI(TAG, "Received message: %s", message);
    if (strcmp(message, "identify") == 0) {
        char *validateIdentityMsg = concatenateStrings(2, "Hello:", secretMessage);
        subRespond(validateIdentityMsg);
    }
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
    output[2] = '\0'; // Null-terminate the string
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

static IRAM_ATTR bool i2c_sub_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data) {
    ESP_LOGI(TAG, "RX Done Callback triggered");
    BaseType_t high_task_wakeup = pdFALSE;
    QueueHandle_t receive_queue = (QueueHandle_t)user_data;
    xQueueSendFromISR(receive_queue, edata, &high_task_wakeup);
    return high_task_wakeup == pdTRUE;
}

void subReinit()
{
    i2c_slave_config_t i2c_sub_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,  // set the clock source
        .i2c_port = I2C_NUM_0,              // set I2C port number
        .scl_io_num = 5,                    // SCL GPIO number
        .sda_io_num = 4,                    // SDA GPIO number
        .addr_bit_len = I2C_ADDR_BIT_LEN_7, // 7-bit address
        .send_buf_depth = 256,              // set TX buffer length
        .slave_addr = i2cChannel,           // slave address
    };
    if (i2c_bus_initialized == true)
    {
        if(i2c_del_slave_device(i2c_bus_handle) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to delete I2C slave device");
            i2c_bus_initialized = false;
            return;
        }
    }
    if (i2c_new_slave_device(&i2c_sub_config, &i2c_bus_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C slave device");
        return;
    }
    ESP_LOGD(TAG, "I2C slave device created");
    i2c_bus_initialized = true;
    vTaskDelay(pdMS_TO_TICKS(500)); // Add delay to ensure the I2C device is ready

    int DATA_LENGTH = 8;
    uint8_t *data_rd = (uint8_t *)malloc(DATA_LENGTH);
    uint32_t size_rd = 0;
    ESP_LOGD(TAG, "Initial contents of data_rd:");
    for (int i = 0; i < DATA_LENGTH; i++)
    {
        ESP_LOGD(TAG, "data_rd[%d] = 0x%02x", i, data_rd[i]);
    }

    s_receive_queue = xQueueCreate(5, sizeof(i2c_slave_rx_done_event_data_t));

    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = i2c_sub_rx_done_callback,
    };
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(i2c_bus_handle, &cbs, s_receive_queue));

    i2c_slave_rx_done_event_data_t rx_data;
    ESP_ERROR_CHECK(i2c_slave_receive(i2c_bus_handle, data_rd, DATA_LENGTH));
    ESP_LOGD(TAG, "before xQueueReceive");
    if (xQueueReceive(s_receive_queue, &rx_data, pdMS_TO_TICKS(5000)) == pdTRUE)
    {
        char receivedString[DATA_LENGTH + 1];
        memcpy(receivedString, data_rd, DATA_LENGTH);
        receivedString[DATA_LENGTH] = '\0'; // Null-terminate the string
        receiveMessage(receivedString);
    }
    else
    {
        ESP_LOGW(TAG, "Receive timeout");
        char receivedString[DATA_LENGTH + 1];
        memcpy(receivedString, data_rd, DATA_LENGTH);
        receivedString[DATA_LENGTH] = '\0'; // Null-terminate the string
        ESP_LOGD(TAG, "Contents of data_rd: %s", receivedString);
        for (int i = 0; i < DATA_LENGTH; i++)
        {
            ESP_LOGD(TAG, "data_rd[%d] = 0x%02x", i, data_rd[i]);
        }
    }
    free(data_rd);
}

void subReinitTask(void *pvParameters) {
    i2c_slave_config_t i2c_sub_config = {
        .addr_bit_len = I2C_ADDR_BIT_LEN_7, // 7-bit address
        .clk_source = I2C_CLK_SRC_DEFAULT,  // set the clock source
        .i2c_port = I2C_NUM_0,              // set I2C port number
        .send_buf_depth = 256,              // set TX buffer length
        .scl_io_num = 5,                    // SCL GPIO number
        .sda_io_num = 4,                    // SDA GPIO number
        .slave_addr = i2cChannel,           // slave address
    };
    if (i2c_bus_handle != NULL)
    {
        if (i2c_del_slave_device(i2c_bus_handle) != ESP_OK)
        {
            ESP_LOGE(TAG, "Failed to delete I2C slave device");
            vTaskDelete(NULL);
            return;
        }
    }
    // FIXME: seems like the following line is throwing some kind of error which makes exec stop
    if (i2c_new_slave_device(&i2c_sub_config, &i2c_bus_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C slave device");
        vTaskDelete(NULL);
        return;
    }
    int DATA_LENGTH = 8;
    uint8_t *data_rd = (uint8_t *)malloc(DATA_LENGTH);
    uint32_t size_rd = 0;

    s_receive_queue = xQueueCreate(1, sizeof(i2c_slave_rx_done_event_data_t));

    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = i2c_sub_rx_done_callback,
    };
    ESP_ERROR_CHECK(i2c_slave_register_event_callbacks(i2c_bus_handle, &cbs, s_receive_queue));

    i2c_slave_rx_done_event_data_t rx_data;

    // Continuously receive data
    while (true)
    {
        ESP_ERROR_CHECK(i2c_slave_receive(i2c_bus_handle, data_rd, DATA_LENGTH));
        if (xQueueReceive(s_receive_queue, &rx_data, pdMS_TO_TICKS(10000)) == pdPASS)
        {
            // Process received data
            ESP_LOGI(TAG, "Received data: %02x %02x", data_rd[0], data_rd[1]);
            char receivedString[DATA_LENGTH + 1];
            memcpy(receivedString, data_rd, DATA_LENGTH);
            receiveMessage(receivedString);
        }
        else
        {
            ESP_LOGW(TAG, "Receive timeout");
        }
    }

    // Free allocated memory (this will never be reached in this example)
    free(data_rd);
    vTaskDelete(NULL);
}