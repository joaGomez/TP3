#ifndef _USBCOMUNICATIONS_H_
#define _USBCOMUNICATIONS_H_

void USBCom_Init(void);

/**
 * @brief Wrapper de comunicaciones para recibir datos sin bloquear el programa.
 */
bool USBCom_GetByte(uint8_t *byte);

void USBCom_SendString(char* str);

#endif /* _USBCOMUNICATIONS_H_ */