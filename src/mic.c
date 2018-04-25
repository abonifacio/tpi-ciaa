#include "sapi.h"
#include <string.h>

#define N_SAMPLES   1

static uint8_t dma_ch_adc;  /* There are 8 DMA channels available */
static uint8_t dma_tc_adc;  /* TC: terminal count                 */

static uint16_t dma_buffer[N_SAMPLES];

static uint8_t buffer_promedio[8];

static uint16_t aux=0,ind=0,indice=0;
static char byte;

static delay_t mi_delay;


static void startDMATransfer(void);
void DMA_IRQHandler(void);


/* Hardware initialization */
void MIC_init(void)
{

    ADC_CLOCK_SETUP_T adc_setup;

    //Board_Init();
//    uartConfig( UART_USB, 460800 );

    /* ADC configuration */
    // Setup ADC0: 10-bit, 100kHz
    Chip_ADC_Init(LPC_ADC0, &adc_setup);
    Chip_ADC_SetResolution(LPC_ADC0, &adc_setup,ADC_8BITS);
    Chip_ADC_SetSampleRate(LPC_ADC0, &adc_setup, 8000);

    // Enable ADC0 CH1 and its interrupt
    Chip_ADC_EnableChannel(LPC_ADC0, ADC_CH1, ENABLE);
    Chip_ADC_Int_SetChannelCmd(LPC_ADC0, ADC_CH1, ENABLE);

    // Enable ADC0 burst mode
    Chip_ADC_SetBurstCmd(LPC_ADC0, ENABLE);

    /* DMA controller configuration */
    Chip_GPDMA_Init(LPC_GPDMA);

    // Get a free channel for a ADC->Memory DMA transfer
    dma_ch_adc = Chip_GPDMA_GetFreeChannel(LPC_GPDMA, GPDMA_CONN_ADC_0);

    // Enable the interrupt in the NVIC
    NVIC_ClearPendingIRQ(DMA_IRQn);
    NVIC_EnableIRQ(DMA_IRQn);

   /* Inicializar Retardo no bloqueante con tiempo en ms */
	delayConfig(&mi_delay,1);
	startDMATransfer();
}

/* In this example there is only one ADC->Memory DMA transfer */
static void startDMATransfer(void)
{

    Chip_GPDMA_Transfer(LPC_GPDMA, dma_ch_adc,
            GPDMA_CONN_ADC_0, (uint32_t)&dma_buffer,
            GPDMA_TRANSFERTYPE_P2M_CONTROLLER_DMA, N_SAMPLES);
}

/* DMA interrupt triggered after N_SAMPLES conversions of ADC0_1 */
//LIMPIA EL FLAG DE TRANSFERENCIA
void DMA_IRQHandler(void)
{

    if (Chip_GPDMA_Interrupt(LPC_GPDMA, dma_ch_adc) == SUCCESS) {
        Chip_GPDMA_ClearIntPending(LPC_GPDMA, GPDMA_STATCLR_INTTC, dma_ch_adc);
        dma_tc_adc = 1;
    }

}

bool_t MIC_sampleReady(void)
{
	return delayRead( &mi_delay );
}

uint8_t MIC_loadSample(char* buffer)
{
	uint8_t j, retVal = 0;
	byte = ADC_DR_RESULT(dma_buffer[0]);
	//promedio movil
	buffer_promedio[ind]=byte;
	aux = 0;
	for(j=0; j<10;j++){
	   aux=aux+buffer_promedio[j];
	}
	aux=aux/10;
	buffer[indice]=aux;
	ind++;
	if(ind == 10){
	   ind = 0;
	}

	indice++;
	if(indice == 100){
	   indice = 0;
	   retVal = 100;
	}
	startDMATransfer();
	return retVal;
}

