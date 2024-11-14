#ifndef __I2C_CONTROLLER_H__
#define __I2C_CONTROLLER_H__

typedef enum {
    Unhandled,
    NoDevice,
    Collision,
    NoResponse,
    InvalidIdentity,
} sub_connection_error_t;

typedef struct {
    const char *subId;
    int channel;
} sub_config_t;

#include "../dom.h"

void subSetup(void);

char* concatenateStrings(int count, ...);

bool is_alpha_numeric(const char *str);

char *sub_connection_error_tToString(sub_connection_error_t error);

#endif // __I2C_CONTROLLER_H__