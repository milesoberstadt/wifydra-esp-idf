#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <sys/random.h>
#include "esp_log.h"
// Include FreeRTOS for threading
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "config.h"
#include "i2c_slave.h"
#include "i2c_controller.h"

void subSetup(void);

void subReinit(void);

int randInRange(int min, int max);

void generateIdentity(char *output);