/**
 * @file main.c
 * @author risinek (risinek@gmail.com)
 * @date 2021-04-03
 * @copyright Copyright (c) 2021
 * 
 * @brief Main file used to setup ESP32 into initial state
 * 
 * Starts management AP and webserver  
 */

#include <stdio.h>

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#define DOM_MODE true

#include "esp_log.h"
#include "esp_event.h"

// #include "attack.h"
// #include "wifi_controller.h"
// #include "webserver.h"

#include "i2c_controller.h"

static const char* TAG = "main";

void app_main(void)
{
    if (DOM_MODE) {
        ESP_LOGI(TAG, "DOM MODE START");
        domSetup();
    } else {
        ESP_LOGI(TAG, "SUB MODE START");
        subSetup();
    }
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // wifictl_mgmt_ap_start();
    // attack_init();
    // webserver_run();
}
