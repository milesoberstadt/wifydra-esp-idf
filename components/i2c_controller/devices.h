
#if !defined(__DEVICES_H__)
#define __DEVICES_H__

#include <stdio.h>

#include "esp_log.h"

// #include "display.h"

#include "types.h"
#include "utils.h"
#include "constants.h"

#define DEVICES "Devices"

void init_devices();

void set_device_type(uint8_t idx, device_type_t type);
void set_device_state(uint8_t idx, device_state_t state);
void set_device_value(uint8_t idx, uint8_t *value, uint8_t value_size);
void set_device_battery_level(uint8_t idx, uint8_t battery_level);

device_t get_device(uint8_t idx);

void select_device(uint8_t idx);

device_t get_selected_device();

#endif // __DEVICES_H__
