/***************************************************************************//**
  @file     timers.c
  @brief    Implementación del PIT para la base de tiempos del Módem FSK
  ******************************************************************************/

#include "FSK.h"
#include "timers.h"
#include "MK64F12.h"

// Frecuencia típica del reloj del bus para el PIT en K64F (ej: 50MHz)
#define PIT_CLOCK_HZ    50000000U

static pit_callback_t pit_callbacks[4] = {0};

void PIT_Init(void)
{
    /* 1. Activar el Clock Gating del PIT */
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;

    /* 2. Habilitar el módulo PIT en el registro MCR (MDIS = 0 habilita el timer, FRZ = 1 lo frena en debug) */
    PIT->MCR = PIT_MCR_FRZ_MASK;

    /* 3. Habilitar las interrupciones en el NVIC */
    NVIC_EnableIRQ(PIT0_IRQn);
    NVIC_EnableIRQ(PIT1_IRQn);
    // Habilitar PIT2 y PIT3 si se usaran
}

void PIT_ConfigureChannel(pit_channel_t channel, uint32_t us, pit_callback_t callback)
{
    // Detener por seguridad antes de configurar
    PIT->CHANNEL[channel].TCTRL = 0;

    pit_callbacks[channel] = callback;

    /* Calcular el valor de recarga (LDVAL) */
    // Ticks = (Tiempo_us * Frecuencia_Hz) / 1.000.000 - 1
    uint32_t ticks = (uint32_t)(((uint64_t)us * PIT_CLOCK_HZ) / 1000000U) - 1;

    PIT->CHANNEL[channel].LDVAL = ticks;

    /* Configurar Control del Canal:
       TIE = 1 (Habilitar interrupción)
       TEN = 1 (Habilitar Timer) */
    PIT->CHANNEL[channel].TCTRL = PIT_TCTRL_TIE_MASK | PIT_TCTRL_TEN_MASK;
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
 ******************************************************************************/
#ifdef USE_VERSION_2
void PIT0_IRQHandler(void)
{
    // Limpiar bandera de interrupción escribiendo un 1 en TFLG
    PIT->CHANNEL[PIT_CH0].TFLG = PIT_TFLG_TIF_MASK;
    if (pit_callbacks[PIT_CH0]) {
        pit_callbacks[PIT_CH0]();
    }
}
#endif

#ifdef USE_VERSION_1
void PIT1_IRQHandler(void)
{
    PIT->CHANNEL[PIT_CH1].TFLG = PIT_TFLG_TIF_MASK;
    if (pit_callbacks[PIT_CH1]) {
        pit_callbacks[PIT_CH1]();
    }
}
#endif
