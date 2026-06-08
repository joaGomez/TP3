#ifndef __IRQN_PORTS_H__
#define __IRQN_PORTS_H__
#include <stdbool.h>
typedef void (*pCallBack_t) (void);

void init_nvic(uint8_t port);

// MAX_INTERRUPT_NUM = 8
void setCallbacks(uint8_t port, pCallBack_t pCallBack, uint8_t interruptNum);

void toggleInterruptFlag(bool fte);

#endif // __IRQN_PORTS_H__
