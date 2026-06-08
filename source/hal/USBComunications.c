#include "mcal/uart.h"
#include "hal/USBComunications.h"

void USBCom_Init(void)
{
    UART_Init();
}

bool USBCom_GetByte(uint8_t *byte) {
    return UART_Receive_Data(byte);
}

void USBCom_SendString(char* str)
{
    UART_SendString(str);
}