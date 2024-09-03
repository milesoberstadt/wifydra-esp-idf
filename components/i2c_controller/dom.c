#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
// Include FreeRTOS for delay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "i2c_controller.h"
#include "dom.h"

static uint32_t i2c_frequency = 100 * 1000;

int initializedSubChannels[TARGET_SUBS];
sub_config_t subIds[TARGET_SUBS];
i2c_master_bus_handle_t tool_bus_handle;
i2c_master_dev_handle_t dev_handle;
int i2c_last_channel = 0;

void domSetup()
{
    // delay so I can read serial output in time
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGD(TAG, "i2c init");

    i2c_master_bus_config_t i2c_bus_conf = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = I2C_NUM_0,
        .scl_io_num = 22,
        .sda_io_num = 21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    if (i2c_new_master_bus(&i2c_bus_conf, &tool_bus_handle) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to create I2C bus");
        return;
    }
    domInitSubs();
}

char *domCheckChannelForSub(int channel_id, int next_avail_reserved)
{
    char *subResult = domDemandResponse(channel_id, 8, "identify");
    // int error = domWriteWire(channel_id, "identify");
    // if (error == 0)
    // {
    //     ESP_LOGD(TAG, "Found device");
    // }
    // else if (error == 2)
    // {
    //     // NACK / No response
    //     return sub_connection_error_tToString(NoDevice);
    // }
    // else if (error == 4)
    // {
    //     ESP_LOGD(TAG, "Unknown error");
    //     int reinitError = domWriteWire(channel_id, "reinit");
    //     if (reinitError != 0) {
    //         ESP_LOGD(TAG, "REINIT ERROR %d", reinitError);
    //     }
    //     return sub_connection_error_tToString(Collision);
    // }
    // else
    // {
    //     ESP_LOGD(TAG, "Error %d", error);
    //     return sub_connection_error_tToString(Unhandled);
    // }
    // char *subResult = domReadWire(channel_id, 8);
    if (subResult == NULL)
    {
        return sub_connection_error_tToString(Unhandled);
    }
    char *topic = strtok(subResult, ":");

    if (strcmp(topic, "Hello") == 0)
    {
        ESP_LOGD(TAG, "HELLO DETECTED ON CHANNEL %d", channel_id);
        char *subId = strtok(0, ":");
        if (strlen(subId) == 2 && is_alpha_numeric(subId))
        {
            ESP_LOGD(TAG, "SUB DETECTED ON CHANNEL %d WITH ID %s", channel_id, subId);
            // Reassign to reserved space
            char subBuffer[3];
            strcpy(subBuffer, subId);
            subBuffer[2] = '\0';

            char channelBuffer[3];
            itoa(next_avail_reserved, channelBuffer, 10);
            // write our reassign command, wait 500ms for a reboot, then confirm identity on the new channel
            char *reassignMessage = concatenateStrings(4, "reassign:", subBuffer, ":", channelBuffer);
            if (reassignMessage == NULL)
            {
                return sub_connection_error_tToString(Unhandled);
            }
            int reassignError = domWriteWire(channel_id, reassignMessage);
            free(reassignMessage);

            if (reassignError != 0)
            {
                ESP_LOGD(TAG, "reassign error %d", reassignError);
            }
            // Wait for sub to restart and listen on the reserved channel
            vTaskDelay(SUB_REBOOT_DELAY_MS / portTICK_PERIOD_MS);

            ESP_LOGD(TAG, "Confirming %s on channel %s", subBuffer, channelBuffer);
            char *validateIdentityMsg = concatenateStrings(2, "identify:", subBuffer);
            if (validateIdentityMsg == NULL)
            {
                return sub_connection_error_tToString(Unhandled);
            }
            // int confirmError = domWriteWire(next_avail_reserved, validateIdentityMsg);
            char *confirmResult = domDemandResponse(next_avail_reserved, 8, validateIdentityMsg);
            free(validateIdentityMsg);
            // Identify with the expected sub id (so the sub can verify it's being talked to)
            // if (confirmError != 0)
            // {
            //     ESP_LOGD(TAG, "Confirm ERROR %d", confirmError);
            //     // TODO: Error confirming reassignment
            //     return sub_connection_error_tToString(Unhandled);
            // }
            // char *confirmResult = domReadWire(next_avail_reserved, 8);
            if (confirmResult == NULL)
            {
                return sub_connection_error_tToString(Unhandled);
            }
            char *client_id_confirm = strtok(confirmResult, ":");
            client_id_confirm = strtok(0, ":");
            bool moveConfirmed = strcmp(client_id_confirm, subId) == 0;
            free(confirmResult);
            if (moveConfirmed)
            {
                ESP_LOGD(TAG, "MOVE CONFIRMED: %s", subBuffer);
                // free(subResult);
                return subBuffer;
            }
            else
            {
                free(subResult);
                ESP_LOGD(TAG, "Wrong confirmation: %s", confirmResult);
                // TODO: Wrong device responded? Garbled message?
                return sub_connection_error_tToString(Unhandled);
            }
        }
        else
        {
            free(subResult);
            ESP_LOGW(TAG, "COLLISION DETECTED ON CHANNEL %d", channel_id);
            int reinitError = domWriteWire(channel_id, "reinit");
            if (reinitError != 0)
            {
                ESP_LOGD(TAG, "REINIT ERROR %d", reinitError);
                return sub_connection_error_tToString(Unhandled);
            }
            return sub_connection_error_tToString(Collision);
        }
    }
    else
    {
        free(subResult);
        // Unrelated devices will probably show here
        return sub_connection_error_tToString(Unhandled);
    }
}

void domInitSubs()
{
    int address;
    int nextAvailableChannel = 1; // next channel to assign to in reserved space

    // reserve the first 20 (extra space in case there's something else on this bus) for subs and scan the rest of the keyspace
    for (address = 21; address < 127; address++)
    {
        char *result = domCheckChannelForSub(address, nextAvailableChannel);
        ESP_LOGD(TAG, "RETURN RESPONSE: %s", result);
        int responseError = 0;
        if (result[0] == '-')
        {
            responseError = abs(atoi(result));
        }
        if (responseError == NoDevice)
        {
            // Do nothing
        }
        else if (responseError < 0)
        {
            // TODO: handle more sub_connection_errors if we can
            ESP_LOGE(TAG, "NOT INCREMENTING, Error code: %s", result);
        }
        else
        {
            const int nextIndex = sizeof(subIds) / sizeof(subIds[0]);
            // subIds.insert(std::make_pair(nextSubIndex, result));
            const sub_config_t newSub = {subId : result, channel : nextAvailableChannel};
            subIds[nextIndex] = newSub;
            // initializedSubChannels[nextIndex] = nextAvailableChannel;
            nextAvailableChannel++;
        }
    }
    // Check how many subs are connected and determine if we need to run again
    const int connectedSubs = sizeof(subIds) / sizeof(subIds[0]);
    if (connectedSubs < TARGET_SUBS)
    {
        ESP_LOGD(TAG, "SCANNING FOR MORE SUBS, %d/%d connected", connectedSubs, TARGET_SUBS);
        // subs will take 200ms before rebooting, delay at least 300ms
        vTaskDelay(500 / portTICK_PERIOD_MS);
        domInitSubs();
    }
    else
    {
        ESP_LOGD(TAG, "ALL SUBS CONNECTED, ASSIGNING CHANNELS");
        int subIndex;
        for (subIndex = 0; subIndex < connectedSubs; subIndex++)
        {
            ESP_LOGD(TAG, "sub index: %d, id: %s, i2c channel: %d", subIndex, subIds[subIndex].subId, subIds[subIndex].channel);
        }
    }
}

char *domReadWire(int channel, int bytesToRead)
{
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = (uint16_t)channel,
    };
    uint8_t *data = malloc(bytesToRead + 1);
    if (i2c_last_channel != channel)
    {
        if (dev_handle != NULL)
        {
            if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
            {
                return NULL;
            }
        }
        ESP_ERROR_CHECK(i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle));
        i2c_last_channel = channel;
    }

    ESP_ERROR_CHECK(i2c_master_receive(dev_handle, data, bytesToRead, -1));
    // esp_err_t ret = i2c_master_receive(dev_handle, data, bytesToRead + 1, I2C_TOOL_TIMEOUT_VALUE_MS);

    // if (ret == ESP_OK) {
    //     // Null-terminate the received data to make it a valid string
    //     data[bytesToRead] = '\0';
    //     return (char*)data;
    // } else if (ret == ESP_ERR_TIMEOUT) {
    //     ESP_LOGW(TAG, "Bus is busy");
    // } else {
    //     ESP_LOGW(TAG, "Read failed");
    // }
    return (char *)data;
}

int domWriteWire(int channel, char *dataInput)
{
    int len = strlen(dataInput);

    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = (uint16_t)channel,
    };
    if (i2c_last_channel != channel)
    {
        if (dev_handle != NULL)
        {
            if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
            {
                return 1;
            }
        }
        ESP_ERROR_CHECK(i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle));
        i2c_last_channel = channel;
    }
    // if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK) {
    //     return 1;
    // }

    uint8_t *data = malloc(len);
    if (data == NULL)
    {
        return -1; // Memory allocation failed
    }
    memcpy(data, dataInput, len);
    // null terminator not needed here, maybe calling functions are adding it? maybe subs are adding it?
    // data[len + 1] = '\0';

    // ESP_ERROR_CHECK(i2c_master_transmit(dev_handle, data, len, -1));
    esp_err_t ret = i2c_master_transmit(dev_handle, data, len, -1);
    // esp_err_t ret = i2c_master_transmit(dev_handle, data, len + 1, I2C_TOOL_TIMEOUT_VALUE_MS);

    free(data);

    if (ret == ESP_OK)
    {
        ESP_LOGI(TAG, "Write OK");
        return 0;
    }
    else if (ret == ESP_ERR_TIMEOUT)
    {
        ESP_LOGW(TAG, "Bus is busy");
        return 2;
    }
    else
    {
        ESP_LOGW(TAG, "Write Failed on channel %d", channel);
        return 2;
    }
    return 1;
}

char *domDemandResponse(int channel, int responseLength, char *dataInput)
{
    int len = strlen(dataInput);

    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = (uint16_t)channel,
    };
    if (i2c_last_channel != channel)
    {
        if (dev_handle != NULL)
        {
            if (i2c_master_bus_rm_device(dev_handle) != ESP_OK)
            {
                return NULL;
            }
        }
        ESP_ERROR_CHECK(i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle));
        i2c_last_channel = channel;
    }

    uint8_t *command_data = malloc(len);
    if (command_data == NULL)
    {
        return NULL; // Memory allocation failed
    }
    memcpy(command_data, dataInput, len);
    // null terminator not needed here, maybe calling functions are adding it? maybe subs are adding it?
    // data[len + 1] = '\0';

    uint8_t *response_data = malloc(responseLength + 1);

    // ESP_ERROR_CHECK(i2c_master_transmit_receive(dev_handle, command_data, len, response_data, responseLength+1, -1));
    esp_err_t ret = i2c_master_transmit_receive(dev_handle, command_data, len, response_data, responseLength + 1, -1);
    free(command_data);
    if (ret == ESP_OK)
    {
        // Null-terminate the received data to make it a valid string
        ESP_LOGI(TAG, "Demand sent, untreated %s", response_data);
        response_data[responseLength] = '\0';
        ESP_LOGI(TAG, "Demand sent, response %s", response_data);
    }
    else if (ret == ESP_ERR_TIMEOUT)
    {
        ESP_LOGW(TAG, "Bus is busy");
        return NULL;
    }
    else
    {
        ESP_LOGW(TAG, "Demand failed on channel %d", channel);
        return NULL;
    }
    return (char *)response_data;
}