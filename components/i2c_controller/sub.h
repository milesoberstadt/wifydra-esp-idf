void subSetup(void);

int randInRange(int min, int max);

void generateIdentity(char *output);

void subReinitTask(void *pvParameters);

void subReinit(void);

static IRAM_ATTR bool i2c_sub_rx_done_callback(i2c_slave_dev_handle_t channel, const i2c_slave_rx_done_event_data_t *edata, void *user_data);