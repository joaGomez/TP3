#include "leds.h"
#include "mcal/gpio.h"
#include "hal/board.h"

// returns NULL if correct
bool ledsInit(uint8_t color)
{
	if(gpioInit(PIN_LED_RED, PORT_mGPIO))
	{
		gpioMode(PIN_LED_RED, OUTPUT);
		gpioWrite(PIN_LED_RED, HIGH);
	}
	else
	{
		return 1;
	}
	if (gpioInit(PIN_LED_BLUE, PORT_mGPIO))
	{
		gpioMode(PIN_LED_BLUE, OUTPUT);
		gpioWrite(PIN_LED_BLUE, HIGH);
	}
	else
	{
		return 1;
	}
	if (gpioInit(PIN_LED_GREEN, PORT_mGPIO))
	{
		gpioMode(PIN_LED_GREEN, OUTPUT);
		gpioWrite(PIN_LED_GREEN, HIGH);
	}
	else
	{
		return 1;
	}
	return 0;
}
// returns NULL if correct
bool ledOn(uint8_t color)
{
	switch(color)
	{
	case RED:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_RED, LOW);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	case GREEN:
		gpioWrite(PIN_LED_GREEN, LOW);
		gpioWrite(PIN_LED_RED, HIGH);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	case BLUE:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_RED, HIGH);
		gpioWrite(PIN_LED_BLUE, LOW);
		break;
	case CYAN:
		gpioWrite(PIN_LED_GREEN, LOW);
		gpioWrite(PIN_LED_RED, HIGH);
		gpioWrite(PIN_LED_BLUE, LOW);
		break;
	case PINK:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_RED, LOW);
		gpioWrite(PIN_LED_BLUE, LOW);
		break;
	case YELLOW:
		gpioWrite(PIN_LED_GREEN, LOW);
		gpioWrite(PIN_LED_RED, LOW);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	case WHITE:
		gpioWrite(PIN_LED_GREEN, LOW);
		gpioWrite(PIN_LED_RED, LOW);
		gpioWrite(PIN_LED_BLUE, LOW);
	default:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_RED, HIGH);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	}
	return 0;
}
// returns NULL if correct
bool ledOff(uint8_t color)
{
	switch(color)
	{
	case RED:
		gpioWrite(PIN_LED_RED, HIGH);
		break;
	case GREEN:
		gpioWrite(PIN_LED_GREEN, HIGH);
		break;
	case BLUE:
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	case CYAN:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	case PINK:
		gpioWrite(PIN_LED_RED, HIGH);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	case YELLOW:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_RED, HIGH);
		break;
	case WHITE:
	default:
		gpioWrite(PIN_LED_GREEN, HIGH);
		gpioWrite(PIN_LED_RED, HIGH);
		gpioWrite(PIN_LED_BLUE, HIGH);
		break;
	}
	return 0;

}
// returns NULL if correct
bool ledToggle(uint8_t color)
{
	switch(color)
	{
	case RED:
		gpioToggle(PIN_LED_RED);
		break;
	case GREEN:
		gpioToggle(PIN_LED_GREEN);
		break;
	case BLUE:
		gpioToggle(PIN_LED_BLUE);
		break;
	case CYAN:
		gpioToggle(PIN_LED_GREEN);
		gpioToggle(PIN_LED_BLUE);
		break;
	case PINK:
		gpioToggle(PIN_LED_RED);
		gpioToggle(PIN_LED_BLUE);
		break;
	case YELLOW:
		gpioToggle(PIN_LED_GREEN);
		gpioToggle(PIN_LED_RED);
		break;
	case WHITE:
	default:
		gpioToggle(PIN_LED_GREEN);
		gpioToggle(PIN_LED_RED);
		gpioToggle(PIN_LED_BLUE);
		break;
	}
	return 0;
}
