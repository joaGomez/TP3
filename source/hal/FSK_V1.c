/***************************************************************************//**
  @file     FSK_V1.c
  @brief    Modem FSK Bell 202 - Version 1 (ADC + DAC + DMA).

  TX (DMA -> DAC):
    - Generacion por DDS (acumulador de fase) => fase CONTINUA y duracion de bit
      CONSTANTE (833 us), sin los huecos del esquema anterior (que usaba una LUT
      de "un ciclo == un bit", rompiendo el timing para 2200 Hz).
    - El eDMA transmite SPB muestras por bit a DAC0->DAT[0]. El ritmo (Fs) lo da
      un canal de FTM0 en free-run (fuente real del DMAMUX) -> evita el erratum
      e4588 del periodic-trigger del PIT y el hecho de que, con el buffer del DAC
      deshabilitado, el DAC nunca pide DMA por si mismo.
    - Doble buffer (ping-pong): al terminar el major loop (== un bit), la ISR del
      DMA rellena el siguiente bit continuando la fase.
    - La linea transmite MARK continuo (1200 Hz) en reposo, para que el RX tenga
      un reposo "mark" valido y pueda detectar el flanco de start.

  RX (DMA <- ADC):
    - PIT0 dispara por HW el ADC0 a Fs; el ADC pide DMA por cada COCO; el eDMA
      copia R[0] a un ring circular. La DSP (fsk_demod, validada en host) consume
      el ring en FSK_V1_PeriodicTask.

  PUNTOS A VERIFICAR EN HARDWARE (no se pudieron probar aqui):
    - Reloj de FTM0 (asumido 50 MHz) -> FTM0_MOD para Fs (osciloscopio).
    - Numero de fuente DMAMUX de FTM0_CH0 (asumido 20) y bit DMA en CnSC.
    - Relacion SADDR-swap vs. primer request del nuevo major loop (margen ~41 us).
 ******************************************************************************/

#include "fsk_config.h"
#include "FSK_V1.h"
#include "timers.h"
#include "MK64F12.h"
#include "mcal/DAC.h"
#include "mcal/DMA.h"
#include "mcal/ADC.h"
#include "fsk_demod.h"
#include <math.h>
#include <stddef.h>

/*******************************************************************************
 * PARAMETROS
 ******************************************************************************/
#define FS_HZ            24000U          /* Fs de TX y RX (SPB entero)          */
#define BAUDRATE         1200U
#define SPB              (FS_HZ/BAUDRATE)/* 20 muestras por bit                 */
#define FL_HZ            1200U           /* mark = 1                            */
#define FH_HZ            2200U           /* space = 0                           */

#define PIT_CLOCK_HZ     50000000U
#define FTM0_CLOCK_HZ    50000000U       /* VERIFICAR reloj real del FTM        */

#define SINE_LUT_SIZE    256U
#define DAC_CENTER       2048
#define DAC_AMPL         1900            /* amplitud (deja headroom 12 bits)    */

/* incrementos de fase (Q32): inc = f * 2^32 / Fs */
#define PHASE_INC_MARK   ((uint32_t)(((uint64_t)FL_HZ << 32) / FS_HZ))
#define PHASE_INC_SPACE  ((uint32_t)(((uint64_t)FH_HZ << 32) / FS_HZ))

#define ADC_CH_A0        12U             /* PTB2 = ADC0_SE12 = A0               */
#define RX_RING_LEN      256U

/*******************************************************************************
 * ESTADO
 ******************************************************************************/
static fsk_v1_rx_callback_t rx_callback = NULL;

/* --- TX --- */
static int16_t  sine_lut[SINE_LUT_SIZE];
static uint16_t tx_buf[2][SPB];          /* doble buffer (ping-pong)            */
static volatile uint8_t  tx_active_buf;  /* buffer que el DMA esta leyendo      */
static volatile uint32_t tx_phase;       /* acumulador de fase DDS (continuo)   */

static volatile uint8_t  tx_busy = false;
static volatile uint8_t  tx_frame[11];   /* 8O1: start,8 datos,paridad,stop     */
static volatile int8_t   tx_idx;         /* bit actual de la trama; -1 = mark   */

/* --- RX --- */
static uint16_t rx_ring[RX_RING_LEN];
static uint16_t rx_rd;

/*******************************************************************************
 * UTILIDADES
 ******************************************************************************/
static void build_sine_lut(void)
{
    for (uint16_t i = 0; i < SINE_LUT_SIZE; i++) {
        float a = (2.0f * (float)M_PI * (float)i) / (float)SINE_LUT_SIZE;
        sine_lut[i] = (int16_t)(DAC_CENTER + (int)(DAC_AMPL * sinf(a)));
    }
}

/* Genera SPB muestras del 'bit' indicado en buf[], avanzando la fase global. */
static void render_bit(uint16_t *buf, uint8_t bit)
{
    uint32_t inc = (bit) ? PHASE_INC_MARK : PHASE_INC_SPACE;  /* 1=mark, 0=space */
    uint32_t ph  = tx_phase;
    for (uint16_t i = 0; i < SPB; i++) {
        buf[i] = (uint16_t)sine_lut[ph >> 24];   /* top 8 bits -> indice LUT */
        ph += inc;
    }
    tx_phase = ph;                                /* fase continua entre bits */
}

/* Devuelve el proximo bit a transmitir: de la trama si hay, si no MARK(1). */
static uint8_t next_tx_bit(void)
{
    if (tx_idx >= 0 && tx_idx < 11) {
        uint8_t b = tx_frame[tx_idx++];
        if (tx_idx >= 11) { tx_idx = -1; tx_busy = false; }
        return b;
    }
    return 1; /* reposo: mark continuo */
}

/*******************************************************************************
 * FTM0 como base de tiempos del DMA de TX (free-run, DMA por match de canal 0)
 ******************************************************************************/
static void FTM0_DacClock_Init(void)
{
    SIM->SCGC6 |= SIM_SCGC6_FTM0_MASK;

    FTM0->SC = 0;
    FTM0->CNTIN = 0;
    FTM0->CNT = 0;
    FTM0->MOD = (uint16_t)(FTM0_CLOCK_HZ / FS_HZ) - 1;   /* periodo = 1/Fs */

    /* Canal 0 output-compare; peticion de DMA en cada match (MSA + DMA + CHIE). */
    FTM0->CONTROLS[0].CnV  = 0;
    FTM0->CONTROLS[0].CnSC = FTM_CnSC_MSA_MASK | FTM_CnSC_CHIE_MASK | FTM_CnSC_DMA_MASK;

    FTM0->SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);            /* system clock, /1 */
}

/*******************************************************************************
 * INTERFAZ PUBLICA
 ******************************************************************************/
void FSK_V1_Init(fsk_v1_rx_callback_t rx_cb)
{
    rx_callback = rx_cb;
    tx_busy = false;
    tx_idx  = -1;
    tx_phase = 0;
    tx_active_buf = 0;

    build_sine_lut();

    DAC_Init();
    DMA_Init();

    /* ---- TX: precargar ambos buffers con MARK (reposo) y arrancar stream ---- */
    render_bit(tx_buf[0], 1);
    render_bit(tx_buf[1], 1);
    DMA_Configure_TxToDAC(DMA_CH_DAC0, DMAMUX_SRC_FTM0_CH0,
                          (uint32_t)tx_buf[0], (uint32_t)&DAC0->DAT[0], SPB);
    NVIC_EnableIRQ(DMA0_IRQn);
    DMA_EnableRequest(DMA_CH_DAC0);
    FTM0_DacClock_Init();

    /* ---- RX: ADC0 disparado por PIT0 + eDMA a ring circular ---- */
    ADC_Init();
    ADC_ConfigRxHwTriggerDMA(ADC_CH_A0);
    DMA_Configure_RxFromADC(DMA_CH_ADC0, DMAMUX_SRC_ADC0,
                            (uint32_t)&ADC0->R[0], (uint32_t)rx_ring, RX_RING_LEN);
    DMA_EnableRequest(DMA_CH_ADC0);

    rx_rd = 0;
    fsk_demod_init((fsk_demod_byte_cb_t)rx_callback);

    /* PIT0 = reloj de muestreo del ADC (trigger HW a Fs, sin IRQ de CPU). */
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    PIT->MCR = PIT_MCR_FRZ_MASK;
    PIT_ConfigureTrigger(PIT_CH0, (PIT_CLOCK_HZ / FS_HZ) - 1);  /* 50e6/24000-1 */
}

bool FSK_V1_TransmitByte(uint8_t data)
{
    if (tx_busy) {
        return false;
    }

    uint8_t parity_count = 0;
    tx_frame[0] = 0;                         /* start = space (0) */
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (data >> i) & 0x01;    /* LSB primero */
        tx_frame[i + 1] = bit;
        if (bit) parity_count++;
    }
    tx_frame[9]  = (parity_count % 2 == 0) ? 1 : 0;  /* paridad IMPAR */
    tx_frame[10] = 1;                                /* stop = mark (1) */

    tx_idx  = 0;
    tx_busy = true;          /* la ISR del DMA tomara la trama bit a bit */
    return true;
}

bool FSK_V1_IsTransmitBusy(void)
{
    return tx_busy;
}

void FSK_V1_PeriodicTask(void)
{
    /* Consumir muestras nuevas del ring de RX y pasarlas por la DSP. */
    uint16_t wr = DMA_RxWriteIndex(DMA_CH_ADC0, (uint32_t)rx_ring, RX_RING_LEN);
    while (rx_rd != wr) {
        fsk_demod_process_sample(rx_ring[rx_rd]);
        rx_rd = (uint16_t)((rx_rd + 1U) % RX_RING_LEN);
        wr = DMA_RxWriteIndex(DMA_CH_ADC0, (uint32_t)rx_ring, RX_RING_LEN);
    }
}

/*******************************************************************************
 * ISR del eDMA canal 0 (fin de major loop == fin de un bit de TX).
 ******************************************************************************/
#ifdef USE_VERSION_1
void DMA0_IRQHandler(void)
{
    DMA0->CINT = DMA_CH_DAC0;                 /* limpiar flag de interrupcion */

    uint8_t just_done = tx_active_buf;
    uint8_t next      = (uint8_t)(just_done ^ 1u);

    DMA0->TCD[DMA_CH_DAC0].SADDR = (uint32_t)tx_buf[next];  /* poner a leer el listo */
    tx_active_buf = next;

    render_bit(tx_buf[just_done], next_tx_bit());           /* rellenar el otro */
}
#endif
