#include "hal/system.h"
#include "mcal/SysTick.h"

bool initSystem(uint32_t tick_hz)
{
	return SysTick_Init(tick_hz);
}
