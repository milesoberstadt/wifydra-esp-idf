#include "utils.h"

void device_type_str(device_type_t type, char *str)
{
    switch (type)
    {
    case DEVICE_M_NODE:
        strcpy(str, "M-Node");
        break;
    case DEVICE_A_NODE:
        strcpy(str, "A-Node");
        break;
    case DEVICE_SLEEPER:
        strcpy(str, "S-Node");
        break;
    default:
        strcpy(str, "------");
        break;
    }
}

void device_state_str(device_state_t state, char *str)
{
    switch (state)
    {
    case dev_state_error:
        strcpy(str, "Error        ");
        break;
    case dev_state_connected:
        strcpy(str, "Connected    ");
        break;
    case dev_state_pairing:
        strcpy(str, "Pairing      ");
        break;
    case dev_state_disconnecting:
        strcpy(str, "Disconnecting");
        break;
    case dev_state_connecting:
        strcpy(str, "Connecting   ");
        break;
    default:
        strcpy(str, "Disconnected ");
        break;
    }
}
