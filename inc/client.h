#include "sapi.h"

bool_t CLIENT_init(void);

bool_t CLIENT_register(void);

bool_t CLIENT_unregister(void);

void CLIENT_send(uint8_t byte);

bool_t CLIENT_prepareSend(uint16_t size);

bool_t CLIENT_awaitResponse(void);
