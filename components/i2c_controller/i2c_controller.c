#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "i2c_controller.h"
// Include FreeRTOS for delay
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TARGET_SUBS 2
#define I2C_TOOL_TIMEOUT_VALUE_MS (50)
// TODO: Tighten this delay
#define SUB_REBOOT_DELAY_MS 800
static const char *TAG = "i2c:comm";
static uint32_t i2c_frequency = 100 * 1000;

int initializedSubChannels[TARGET_SUBS];
sub_config_t subIds[TARGET_SUBS];
i2c_master_bus_handle_t tool_bus_handle;


char *sub_connection_error_tToString(sub_connection_error_t error) {
    switch (error)
    {
    case Unhandled:
        return "-1";
    case NoDevice:
        return "-2";
    case Collision:
        return "-3";
    case NoResponse:
        return "-4";
    case InvalidIdentity:
        return "-5";
    default:
        return "00";
    }
}

void setup() {
    // delay so I can read serial output in time
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    ESP_LOGD(TAG, "i2c init");
    init_subs();
}

char *check_channel_for_sub(int channel_id, int next_avail_reserved) {
    int error = domWriteWire(channel_id, "identify");
    if (error == 0)
    {
        ESP_LOGD(TAG, "Found device");
    }
    else if (error == 2)
    {
        // NACK / No response
        return sub_connection_error_tToString(NoDevice);
    }
    else if (error == 4)
    {
        ESP_LOGD(TAG, "Unknown error");
        int reinitError = domWriteWire(channel_id, "reinit");
        if (reinitError != 0) {
            ESP_LOGD(TAG, "REINIT ERROR %d", reinitError);
        }
        return sub_connection_error_tToString(Collision);
    }
    else
    {
        ESP_LOGD(TAG, "Error %d", error);
        return sub_connection_error_tToString(Unhandled);
    }

    // TODO: Eval if we should revert these 2 lines
    char *subResult = domReadWire(channel_id, 8);
    if (subResult == NULL) {
        return sub_connection_error_tToString(Unhandled);
    }
    char *topic = strtok(subResult, ":");

    if (strcmp(topic, "Hello") == 0)
    {
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
            if (reassignMessage == NULL) {
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
            if (validateIdentityMsg == NULL) {
                return sub_connection_error_tToString(Unhandled);
            }
            int confirmError = domWriteWire(next_avail_reserved, validateIdentityMsg);
            free(validateIdentityMsg);
            // Identify with the expected sub id (so the sub can verify it's being talked to)
            if (confirmError != 0)
            {
                ESP_LOGD(TAG, "Confirm ERROR %d", confirmError);
                // TODO: Error confirming reassignment
                return sub_connection_error_tToString(Unhandled);
            }
            char *confirmResult = domReadWire(next_avail_reserved, 8);
            if (confirmResult == NULL) {
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

void init_subs() {
    int address;
    int nextAvailableChannel = 1; // next channel to assign to in reserved space

    // reserve the first 20 (extra space in case there's something else on this bus) for subs and scan the rest of the keyspace
    for (address = 21; address < 127; address++)
    {
        char *result = check_channel_for_sub(address, nextAvailableChannel);
        ESP_LOGD(TAG, "RETURN RESPONSE: %s", result);
        int responseError = 0;
        if (result[0] == '-')
        {
            responseError = atoi(result);
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
            const sub_config_t newSub = { subId: result, channel: nextAvailableChannel };
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
        init_subs();
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

bool is_alpha_numeric(const char *str) {
    const char *alphabet = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    int i = 0;
    while (str[i] != '\0')
    {
        bool found = false;
        char currentChar = str[i];
        for (int j = 0; j < 62; j++)
        {
            // char currentAlpha = alphabet[j];
            if (currentChar == alphabet[j])
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            ESP_LOGD(TAG, "Not found char: %c", currentChar);
            return false;
        }
        i++;
    }
    return true;
}

char *domReadWire(int channel, int bytesToRead) {
    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = 0,
    };
    uint8_t *data = malloc(bytesToRead + 1);
    i2c_master_dev_handle_t dev_handle;
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK) {
        return NULL;
    }

    esp_err_t ret = i2c_master_transmit_receive(dev_handle, (uint8_t*)&channel, 1, data, bytesToRead, I2C_TOOL_TIMEOUT_VALUE_MS);

    if (ret == ESP_OK) {
        // Null-terminate the received data to make it a valid string
        data[bytesToRead] = '\0';
        return (char*)data;
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Bus is busy");
    } else {
        ESP_LOGW(TAG, "Read failed");
    }
    free(data);
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK) {
        return NULL;
    }
    return NULL;
}

int domWriteWire(int channel, char *dataInput) {
    /* Check chip address: "-c" option */
    int chip_addr = 0;
    /* Check data: "-d" option */
    int len = strlen(dataInput);

    i2c_device_config_t i2c_dev_conf = {
        .scl_speed_hz = i2c_frequency,
        .device_address = chip_addr,
    };
    i2c_master_dev_handle_t dev_handle;
    if (i2c_master_bus_add_device(tool_bus_handle, &i2c_dev_conf, &dev_handle) != ESP_OK) {
        return 1;
    }

    uint8_t *data = malloc(len + 1);
    if (data == NULL) {
        return -1; // Memory allocation failed
    }
    data[0] = (uint8_t)channel;
    memcpy(&data[1], dataInput, len);
    // Optionally, you can add a null terminator if needed (e.g., for debugging)
    // data[len + 1] = '\0'; // This is optional and only if you want to null-terminate the data

    // Now, you can send 'data' over I2C (replace this with your actual I2C sending code)
    esp_err_t ret = i2c_master_transmit(dev_handle, data, len + 1, I2C_TOOL_TIMEOUT_VALUE_MS);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Write OK");
    } else if (ret == ESP_ERR_TIMEOUT) {
        ESP_LOGW(TAG, "Bus is busy");
    } else {
        ESP_LOGW(TAG, "Write Failed");
    }

    free(data);
    if (i2c_master_bus_rm_device(dev_handle) != ESP_OK) {
        return 1;
    }
    return 0;
}

char* concatenateStrings(int numArgs, ...) {
    // Calculate the total length of the final string
    int totalLength = 0;
    va_list args;

    va_start(args, numArgs);
    for (int i = 0; i < numArgs; i++) {
        totalLength += strlen(va_arg(args, char*));
    }
    va_end(args);

    // Allocate memory for the final concatenated string
    char *result = malloc(totalLength + 1); // +1 for the null terminator
    if (result == NULL) {
        // Handle memory allocation failure
        return NULL;
    }

    // Initialize the result string with an empty string
    result[0] = '\0';

    // Concatenate all the strings
    va_start(args, numArgs);
    for (int i = 0; i < numArgs; i++) {
        strcat(result, va_arg(args, char*));
    }
    va_end(args);

    return result;
}