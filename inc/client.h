#include "sapi.h"

bool_t CLIENT_init(uartMap_t uartForEsp, uartMap_t uartForDebug, uint32_t baudRate );

bool_t CLIENT_register();

bool_t CLIENT_unregister();

void CLIENT_send(uint8_t byte);

bool_t CLIENT_prepareSend(uint16_t size);

bool_t CLIENT_awaitResponse(void);
