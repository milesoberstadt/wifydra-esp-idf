#include "i2c_messages.h"

#define HEADER_LEN 3

void i2c_send_message_master(message_t msg, size_t dev_idx)
{
    i2c_send_message_data_master(msg, dev_idx, NULL, 0);
}

void i2c_send_message_data_master(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len)
{

    if (data_len > I2C_DATA_LEN - HEADER_LEN)
    {
        ESP_LOGE(I2C_MSG_TAG, "Data length exceeds maximum allowed length");
        return;
    }

    uint8_t *msg_data = (uint8_t *)malloc(I2C_DATA_LEN);

    if (msg_data == NULL)
    {
        ESP_LOGE(I2C_MSG_TAG, "Failed to allocate memory for message data");
        return;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;
    msg_data[2] = data_len;

    memcpy(msg_data + HEADER_LEN, data, data_len);

    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++)
    {
        msg_data[i] = 0xFF;
    }

    i2c_write(msg_data);

#if LOG_I2C == 1
    ESP_LOGI(I2C_MSG_TAG, "Message sent: %d", msg);
    esp_log_buffer_hex(I2C_MSG_TAG, msg_data, I2C_DATA_LEN);
#endif

    free(msg_data);
}

void process_message_master(uint8_t *data, size_t length)
{

    esp_log_buffer_hex(I2C_MSG_TAG, data, length);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[3];

    ESP_LOGI(I2C_MSG_TAG, "Message type: %d\n", msg_type);

    // Handle devices message
    switch (msg_type)
    {
    case msg_req_dev:

        // size_t dev_idx = data[4];
        // device_t dev;
        // get_cache_device(&dev, dev_idx);
        ESP_LOGI(I2C_MSG_TAG, "Request device %d\n", dev_idx);

        // uint8_t res[3];
        // res[0] = dev.type;
        // res[1] = dev.state;
        // res[2] = dev.battery_level;

        // i2c_send_message_data(msg_res_dev, dev_idx, res, 3);

    default:
        break;
    }
}

void process_message_slave(uint8_t* data, size_t length) {

    esp_log_buffer_hex(I2C_MESSAGES, data, length);

    uint8_t msg_type = data[0];
    uint8_t dev_idx = data[1];
    uint8_t msg_len = data[2];

    ESP_LOGI(I2C_MESSAGES, "Message type: %d\n", msg_type);

    // Handle devices message
    switch (msg_type) {
        case msg_err:
            ESP_LOGI(I2C_MESSAGES, "Error received\n");
            break;
        case msg_init_start:
            ESP_LOGI(I2C_MESSAGES, "Initialisation started\n");
            break;
        case msg_init_end:
            ESP_LOGI(I2C_MESSAGES, "Initialisation end\n");
            break;
        case msg_dev_type:
            {
                device_type_t dev_type = data[3];

                set_device_type(dev_idx, dev_type);

                #if LOG_I2C_MESSAGES == 1
                char* type_str = malloc(DEV_TYPE_STR_LEN*sizeof(char));
                device_type_str(dev_type, type_str);
                ESP_LOGI(I2C_MESSAGES, "Device %d type: %s\n", dev_idx, type_str);
                free(type_str);
                #endif

            }
            break;
        case msg_dev_state:
            {
                device_state_t dev_state = data[3];

                set_device_state(dev_idx, dev_state);

                #if LOG_I2C_MESSAGES == 1
                char* state_str = malloc(DEV_STATE_STR_LEN*sizeof(char));
                device_state_str(dev_state, state_str);
                ESP_LOGI(I2C_MESSAGES, "Device %d state: %s\n", dev_idx, state_str);
                free(state_str);
                #endif

            }
            break;
        case msg_dev_data:
            ESP_LOGI(I2C_MESSAGES, "Device data received\n");
            // set_device_value(dev_idx, data+HEADER_LEN, msg_len);
            break;

        case msg_dev_battery_level:
            ESP_LOGI(I2C_MESSAGES, "Device battery level received\n");
            // set_device_battery_level(dev_idx, data[3]);
            break;
        case msg_dev_selected:
            ESP_LOGI(I2C_MESSAGES, "Device selected\n");
            // select_device(dev_idx);

            #if LOG_I2C_MESSAGES == 1
            ESP_LOGI(I2C_MESSAGES, "Device %d selected\n", dev_idx);
            #endif
            break;

        case msg_dev_error:
            
            // #if LOG_I2C_MESSAGES == 1
            ESP_LOGI(I2C_MESSAGES, "Device %d error\n", dev_idx);
            // #endif

            break;
        case msg_screen_toggle:

            // #if LOG_I2C_MESSAGES == 1
            ESP_LOGI(I2C_MESSAGES, "Screen toggle\n");
            // #endif

            // start_sleeping();

            break;
        case msg_res_dev:

            // device_t dev;
            // dev.type = data[3];
            // dev.state = data[4];
            // dev.battery_level = data[5];

            // set_device_state(dev_idx, dev.state);
            // set_device_type(dev_idx, dev.type);
            // set_device_battery_level(dev_idx, dev.battery_level);
            ESP_LOGI(I2C_MESSAGES, "Device %d response received\n", dev_idx);
            break;
        default:
            ESP_LOGI(I2C_MESSAGES,"Unknown message received\n");
            break;
    }
}

void i2c_send_message_data_slave(message_t msg, size_t dev_idx, uint8_t *data, size_t data_len) {

    if (data_len > I2C_DATA_LEN - HEADER_LEN) {
        ESP_LOGE(I2C_MESSAGES, "Data length exceeds maximum allowed length");
        return;
    }

    uint8_t *msg_data = (uint8_t *)malloc(I2C_DATA_LEN);

    if (msg_data == NULL) {
        ESP_LOGE(I2C_MESSAGES, "Failed to allocate memory for message data");
        return;
    }

    msg_data[0] = msg;
    msg_data[1] = dev_idx;
    msg_data[2] = data_len;

    memcpy(msg_data + HEADER_LEN, data, data_len);

    for (size_t i = HEADER_LEN + data_len; i < I2C_DATA_LEN; i++) {
        msg_data[i] = 0xFF;
    }

    i2c_slave_send(msg_data, I2C_DATA_LEN);

    #if LOG_I2C_MESSAGES == 1
    ESP_LOGI(I2C_MESSAGES, "Message sent: %d", msg);
    esp_log_buffer_hex(I2C_MESSAGES, msg_data, I2C_DATA_LEN);
    #endif

    free(msg_data);
}

void i2c_send_message_slave(message_t msg, size_t dev_idx) {
    i2c_send_message_data_slave(msg, dev_idx, NULL, 0);
}