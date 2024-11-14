#if !defined(__TYPES_H__)
#define __TYPES_H__

// #include "driver/gpio.h"
// #include "freertos/FreeRTOS.h"

typedef enum device_state_t
{
    dev_state_error = -1,
    dev_state_disconnected = 0,
    dev_state_connected = 1,
    dev_state_pairing = 2,
    dev_state_disconnecting = 3,
    dev_state_connecting = 4,
} device_state_t;

typedef enum device_type_t
{
    UNKNOWN_DEVICE = -1,
    DEVICE_M_NODE = 0,
    DEVICE_A_NODE = 1,
    DEVICE_SLEEPER = 2,
} device_type_t;

typedef enum message_t
{
    msg_err = 0x00,
    msg_init_start = 0x01,
    msg_init_end = 0x02,
    msg_dev_selected = 0x03,
    msg_dev_type = 0x10,
    msg_dev_state = 0x11,
    msg_dev_data = 0x12,
    msg_dev_error = 0x14,
    msg_dev_battery_level = 0x15,
    msg_screen_toggle = 0x50,
    msg_req_dev = 0x60,
    msg_res_dev = 0x61,
} message_t;

typedef struct device_t
{
    device_type_t type;
    device_state_t state;
    uint8_t battery_level;
    uint8_t *value;
    uint8_t value_size;
} device_t;

#endif // __TYPES_H__