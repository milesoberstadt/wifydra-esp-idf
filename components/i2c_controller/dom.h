void domSetup(void);

char *domCheckChannelForSub(int channel_id, int next_avail_reserved);

char *domDemandResponse(int channel, int responseLength, char *dataInput);

void domInitSubs(void);

char *domReadWire(int channel, int bytesToRead);

int domWriteWire(int channel, char *dataInput);
