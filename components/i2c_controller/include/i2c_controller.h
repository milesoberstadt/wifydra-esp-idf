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

void domSetup(void);

char *domCheckChannelForSub(int channel_id, int next_avail_reserved);

char *domDemandResponse(int channel, int responseLength, char *dataInput);

void domInitSubs(void);

char *domReadWire(int channel, int bytesToRead);

int domWriteWire(int channel, char *dataInput);

char* concatenateStrings(int count, ...);

bool is_alpha_numeric(const char *str);

char *sub_connection_error_tToString(sub_connection_error_t error);
