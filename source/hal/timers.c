/***************************************************************************//**
  @file     timers.c
  @brief    Implementacion del PIT para la base de tiempos del Modem FSK
 ******************************************************************************/

#include "fsk_config.h"      /* [FIX] version compartida por TODAS las TU      */
#include "FSK.h"
#include "timers.h"
#include "MK64F12.h"

#define PIT_CLOCK_HZ    50000000U

static pit_callback_t pit_callbacks[4] = {0};

void PIT_Init(void)
{
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    PIT->MCR = PIT_MCR_FRZ_MASK;            /* MDIS=0 habilita, FRZ=1 frena en debug */

    NVIC_EnableIRQ(PIT0_IRQn);
    NVIC_EnableIRQ(PIT1_IRQn);
    NVIC_EnableIRQ(PIT2_IRQn);
    NVIC_EnableIRQ(PIT3_IRQn);
}

void PIT_ConfigureChannel(pit_channel_t channel, uint32_t us, pit_callback_t callback)
{
    PIT->CHANNEL[channel].TCTRL = 0;
    pit_callbacks[channel] = callback;

    uint32_t ticks = (uint32_t)(((uint64_t)us * PIT_CLOCK_HZ) / 1000000U) - 1;
    PIT->CHANNEL[channel].LDVAL = ticks;

    PIT->CHANNEL[channel].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;
}

void PIT_ConfigureTrigger(pit_channel_t channel, uint32_t ldval_ticks)
{
    /* [NUEVO] PIT como FUENTE DE TRIGGER (ej: disparo HW del ADC via SOPT7).
       TEN=1 (corre y genera el trigger periodico) pero TIE=0 (sin IRQ de CPU). */
    PIT->CHANNEL[channel].TCTRL = 0;
    pit_callbacks[channel] = 0;
    PIT->CHANNEL[channel].LDVAL = ldval_ticks;
    PIT->CHANNEL[channel].TCTRL = PIT_TCTRL_TEN_MASK;
}

void PIT_StopChannel(pit_channel_t channel)
{
    PIT->CHANNEL[channel].TCTRL &= ~PIT_TCTRL_TEN_MASK;
}

void PIT_StartChannel(pit_channel_t channel)
{
    PIT->CHANNEL[channel].TCTRL |= PIT_TCTRL_TEN_MASK;
}

/*******************************************************************************
 * ISRs DEL PIT
 * [FIX] Antes PIT0_IRQHandler/PIT1_IRQHandler estaban bajo #ifdef USE_VERSION_x,
 * macro que solo existia en App.c -> en esta TU no se definian y los handlers
 * NO se compilaban (V2 quedaba sin base de tiempos). Ahora son despachadores
 * genericos: limpian el flag y llaman al callback registrado (o nada).
 ******************************************************************************/
static inline void pit_dispatch(pit_channel_t ch)
{
    PIT->CHANNEL[ch].TFLG = PIT_TFLG_TIF_MASK;   /* limpiar TIF escribiendo 1 */
    if (pit_callbacks[ch]) {
        pit_callbacks[ch]();
    }
}

void PIT0_IRQHandler(void) { pit_dispatch(PIT_CH0); }
void PIT1_IRQHandler(void) { pit_dispatch(PIT_CH1); }
void PIT2_IRQHandler(void) { pit_dispatch(PIT_CH2); }
void PIT3_IRQHandler(void) { pit_dispatch(PIT_CH3); }
