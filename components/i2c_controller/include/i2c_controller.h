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

void setup(void);

char *check_channel_for_sub(int channel_id, int next_avail_reserved);

void init_subs(void);

char *sub_connection_error_tToString(sub_connection_error_t error);

bool is_alpha_numeric(const char *str);

char *domReadWire(int channel, int bytesToRead);

int domWriteWire(int channel, char *dataInput);

char* concatenateStrings(int count, ...);
