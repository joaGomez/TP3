
#include "mcal/gpio.h"
#include "hardware.h"
#include "hal/IRQN_Ports.h"
#include "hal/board.h"

#define CANT_PUERTOS 5
#define MAX_INTERRUPT 8

pCallBack_t pCallBackList[CANT_PUERTOS][MAX_INTERRUPT] = {{{},{},{},{},{},{},{},{}},{{},{},{},{},{},{},{},{}},{{},{},{},{},{},{},{},{}},{{},{},{},{},{},{},{},{}},{{},{},{},{},{},{},{},{}}};

void toggleInterruptFlag(bool fte) {
	gpioWrite(PIN_INTERRUPTION_FLAG, fte);
}


void PORTC_IRQHandler(void)
{
	int i = 0;
	for (i = 0; i < MAX_INTERRUPT; i++)
	{
		if (pCallBackList[PC][i])
		{
			pCallBackList[PC][i]();
		}
	}

}

void setCallbacks(uint8_t port, pCallBack_t pCallBack, uint8_t interruptNum)
{
	pCallBackList[port][interruptNum] = pCallBack;
}


void init_nvic(uint8_t port) {
	if(!gpioInit(PIN_INTERRUPTION_FLAG, PORT_mGPIO)) {
		return;
	}
	gpioMode(PIN_INTERRUPTION_FLAG, OUTPUT);

	switch(port)
	{
	case PA:
		NVIC_ClearPendingIRQ(PORTA_IRQn);
		NVIC_SetPriority(PORTA_IRQn, 1);
		NVIC_EnableIRQ(PORTA_IRQn);
		break;
	case PB:
		NVIC_ClearPendingIRQ(PORTB_IRQn);
		NVIC_SetPriority(PORTB_IRQn, 2);
		NVIC_EnableIRQ(PORTB_IRQn);
		break;
	default:
	case PC:
		NVIC_ClearPendingIRQ(PORTC_IRQn);
		NVIC_SetPriority(PORTC_IRQn, 3);
		NVIC_EnableIRQ(PORTC_IRQn);
		break;
	case PD:
		NVIC_ClearPendingIRQ(PORTD_IRQn);
		NVIC_SetPriority(PORTD_IRQn, 4);
		NVIC_EnableIRQ(PORTD_IRQn);
		break;
	case PE:
		NVIC_ClearPendingIRQ(PORTD_IRQn);
		NVIC_SetPriority(PORTD_IRQn, 5);
		NVIC_EnableIRQ(PORTD_IRQn);
		break;
	}

}

/* Función para testear las interrupciones de GPIO*/

