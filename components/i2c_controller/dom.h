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
#include "constants.h"
#include "i2c_master.h"
#include "i2c_messages.h"

void domSetup(void);

char *domCheckChannelForSub(int channel_id, int next_avail_reserved);

char *domDemandResponse(int channel, int responseLength, char *dataInput);

void domInitSubs(void);

char *domReadWire(int channel, int bytesToRead);

int domWriteWire(int channel, char *dataInput);

int *probeChannels(int *num_channels);

void domSetChannel(int channel);