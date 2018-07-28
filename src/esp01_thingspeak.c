#include "constants.h"
#include "sapi.h"
#include "client.h"
#include "mic.h"
#include "stopwatch.h"

void stopProgramError(bool_t registered) {
	if (registered) {
		CLIENT_unregister();
	}
	while ( TRUE) {
		sleepUntilNextInterrupt();
	}
}

int main(void) {

	uint8_t recByte;
	uint16_t processed = 0;
	uint32_t nbrOfTicks,startTime;

	boardConfig();
	StopWatch_Init();

	if (!CLIENT_init()) {
		stopProgramError(FALSE);
	}
	if (!CLIENT_register()) {
		stopProgramError(FALSE);
	}

	nbrOfTicks = StopWatch_UsToTicks(SAMPLE_DELAY_US);
	MIC_init();
	if(!CLIENT_prepareSend(BUFFER_SIZE)){
		stopProgramError(TRUE);
	}
	startTime = StopWatch_Start();

	while ( TRUE) {
		if(StopWatch_Elapsed(startTime) >= nbrOfTicks){
			recByte = MIC_loadSample();
			startTime = StopWatch_Start();
			CLIENT_send(recByte);
			processed++;
		}
		if(processed==BUFFER_SIZE){
			CLIENT_awaitResponse();
			processed = 0;
			while(!CLIENT_prepareSend(BUFFER_SIZE));
		}
	}

	return 0;
}

