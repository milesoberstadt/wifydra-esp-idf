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

// TODO: Try to actually use this and the enums again once things are working
char *sub_connection_error_tToString(sub_connection_error_t error) {
    // these need to align with the order in the sub_connection_error_t
    switch (error) {
    case Unhandled:
        return "-0";
    case NoDevice:
        return "-1";
    case Collision:
        return "-2";
    case NoResponse:
        return "-3";
    case InvalidIdentity:
        return "-4";
    default:
        return "00";
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