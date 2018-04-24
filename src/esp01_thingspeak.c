#include "sapi.h"
#include <string.h>
#include "client.h"
#include "mic.h"

#define UART_DEBUG                 UART_USB
#define UART_ESP01                 UART_232
#define UARTS_BAUD_RATE            115200

char BUFFER[100];

DEBUG_PRINT_ENABLE


void stopProgramError(bool_t registered) {
	if (registered) {
		CLIENT_unregister();
	}
	while ( TRUE) {
		sleepUntilNextInterrupt();
	}
}

int main(void) {

	uint8_t packet_size;

	boardConfig();
	debugPrintConfigUart(UART_DEBUG, UARTS_BAUD_RATE);

	if (!CLIENT_init(UART_ESP01, UART_DEBUG, UARTS_BAUD_RATE)) {
		stopProgramError(FALSE);
	}
	if (!CLIENT_register()) {
		stopProgramError(FALSE);
	}

	MIC_init();

	while ( TRUE) {
//		debugPrintlnString("En el while");
		if (MIC_sampleReady()) {
			packet_size = MIC_loadSample(BUFFER);
			if (packet_size) {
				CLIENT_send(BUFFER, packet_size);
			}
		}
//		delay(20000);
	}

	return 0;
}

