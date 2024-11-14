
#include "devices.h"

device_t devices[DEVICES_COUNT];
static size_t selected_device = 0;

void on_change(size_t idx)
{
    if (idx == selected_device)
    {
        device_t device = devices[idx];
        // display_device(idx, device);
    }
}

void init_devices()
{
    for (uint8_t i = 0; i < DEVICES_COUNT; i++)
    {
        devices[i].type = UNKNOWN_DEVICE;
        devices[i].state = dev_state_disconnected;
        devices[i].value = NULL;
        devices[i].value_size = 0;
        devices[i].battery_level = 0;
    }
}

void set_device_type(uint8_t idx, device_type_t type)
{
    if (idx < DEVICES_COUNT)
    {
        devices[idx].type = type;
        on_change(idx);
    }
}

void set_device_state(uint8_t idx, device_state_t state)
{
    if (idx < DEVICES_COUNT)
    {
        devices[idx].state = state;
        on_change(idx);
    }
}

void set_device_battery_level(uint8_t idx, uint8_t battery_level)
{
    if (idx < DEVICES_COUNT)
    {
        devices[idx].battery_level = battery_level;
        on_change(idx);
    }
}

void set_device_value(uint8_t idx, uint8_t *value, uint8_t value_size)
{
    if (idx < DEVICES_COUNT)
    {
        devices[idx].value = value;
        devices[idx].value_size = value_size;
        on_change(idx);
    }
}

device_t get_device(uint8_t idx)
{
    if (idx < DEVICES_COUNT)
    {
        return devices[idx];
    }
    device_t empty_device;
    empty_device.type = UNKNOWN_DEVICE;
    empty_device.state = dev_state_disconnected;
    empty_device.value = NULL;
    empty_device.value_size = 0;
    return empty_device;
}

void select_device(uint8_t idx)
{
    if (idx < DEVICES_COUNT)
    {
        selected_device = idx;
        on_change(idx);
    }
}

device_t get_selected_device()
{
    return devices[selected_device];
}
