#include "sapi.h"

bool_t CLIENT_init(uartMap_t uartForEsp, uartMap_t uartForDebug, uint32_t baudRate );

bool_t CLIENT_register();

bool_t CLIENT_send(char * buffer,uint32_t size);

bool_t CLIENT_unregister();
