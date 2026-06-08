#include "gpio.h"
#include "hardware.h"



static PORT_Type* const pPorts[5] = PORT_BASE_PTRS;
static GPIO_Type* const pGpios[5] = GPIO_BASE_PTRS;

/**
 * @brief Configures the specified pin as GPIO and activates it's clock
 * @param pint the pint whose GPIO you wish to initialize
 * @return ERROR if the initialization was not successful
 */
bool gpioInit(pin_t pin, PORTMux_t mux) {
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);
	switch(port)
	{ // Clock Gating with the corresponding PORT Mask
	case PA:
		SIM->SCGC5 |= SIM_SCGC5_PORTA_MASK;
		break;
	case PB:
		SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
		break;
	case PC:
		SIM->SCGC5 |= SIM_SCGC5_PORTC_MASK;
		break;
	case PD:
		SIM->SCGC5 |= SIM_SCGC5_PORTD_MASK;
		break;
	case PE:
		SIM->SCGC5 |= SIM_SCGC5_PORTE_MASK;
		break;
	default:
		return false;
	}

	pPorts[port]->PCR[number] = PORT_PCR_MUX(mux);		// GPIO

	return true;
}

 /*
 * @brief Configures the specified pin to behave either as an input or an output
 * @param pin the pin whose mode you wish to set (according PORTNUM2PIN)
 * @param mode INPUT, OUTPUT, INPUT_PULLUP or INPUT_PULLDOWN.
 */

void gpioMode (pin_t pin, uint8_t mode) {
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);

	pPorts[port]->PCR[number] = 0x0; //Clear all bits
	pPorts[port]->PCR[number] |= PORT_PCR_MUX(PORT_mGPIO); 		//Set MUX to GPIO

	pGpios[port]->PDDR = (pGpios[port]->PDDR & ~(1 << number)) | (mode << number);
}

/**
 * @brief Write a HIGH or a LOW value to a digital pin
 * @param pin the pin to write (according PORTNUM2PIN)
 * @param val Desired value (HIGH or LOW)
 */

void gpioWrite (pin_t pin, bool value) {
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);


	if(value) {
		pGpios[port]->PSOR = (uint32_t)(1 << number);
	}
	else {
		pGpios[port]->PCOR = (uint32_t)(1 << number);
	}
}

/**
 * @brief Toggle the value of a digital pin (HIGH<->LOW)
 * @param pin the pin to toggle (according PORTNUM2PIN)
 */

void gpioToggle (pin_t pin) {
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);

	pGpios[port]->PTOR = (uint32_t)(1 << number);
}

/*
 * @brief Reads the value from a specified digital pin, either HIGH or LOW.
 * @param pin the pin to read (according PORTNUM2PIN)
 * @return HIGH or LOW
 */

bool gpioRead (pin_t pin) {
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);

	bool result = (pGpios[port]->PDIR >> number) & 1;

	return result;
}




void fallingEdgeIRQC(pin_t pin) {
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);

	pPorts[port]->PCR[number] &= ~PORT_PCR_IRQC_MASK;   // limpiar IRQC previo
	pPorts[port]->PCR[number] |= PORT_PCR_IRQC(0b1010); // falling edge
}

void risingEdgeIRQC(pin_t pin)
{
    uint8_t port = PIN2PORT(pin);
    uint8_t number = PIN2NUM(pin);

    // Limpiar campo IRQC
    pPorts[port]->PCR[number] &= ~PORT_PCR_IRQC_MASK;

    // Configurar rising edge
    pPorts[port]->PCR[number] |= PORT_PCR_IRQC(0b1001);
}

void gpioPullUp(pin_t pin)
{
    uint8_t port = PIN2PORT(pin);
    uint8_t number = PIN2NUM(pin);

    pPorts[port]->PCR[number] |= PORT_PCR_PE_MASK | PORT_PCR_PS_MASK;
}


void cleanFlags(pin_t pin)
{
	uint8_t port = PIN2PORT(pin);
	uint8_t number = PIN2NUM(pin);

	switch(port)
	{
	case PA:
			// Limpiar flags pendientes antes de habilitar NVIC
			PORTA->ISFR |= (1 << number);
			break;
	case PB:
			// Limpiar flags pendientes antes de habilitar NVIC
		    PORTB->ISFR |= (1 << number);
		    break;
	default:
	case PC:
			// Limpiar flags pendientes antes de habilitar NVIC
		    PORTC->ISFR |= (1 << number);
		    break;
	case PD:
			// Limpiar flags pendientes antes de habilitar NVIC
		    PORTD->ISFR |= (1 << number);
		    break;
	case PE:
			// Limpiar flags pendientes antes de habilitar NVIC
		    PORTE->ISFR |= (1 << number);
		    break;

	}
}
