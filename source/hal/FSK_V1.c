/***************************************************************************//**
  @file     FSK_V1.c
  @brief    Módem FSK Bell 202 (Capa HAL) - Versión 1 (TX por DMA + DAC)
 ******************************************************************************/

#include "FSK_V1.h"
#include "MK64F12.h"
#include "mcal/DAC.h"
#include "mcal/DMA.h"
#include <math.h>
#include <stddef.h>

/*******************************************************************************
 * CONSTANTES Y CONFIGURACIONES
 ******************************************************************************/
#define FSAMPLING               22000U      // Frecuencia de muestreo (22 kHz)
#define PI                      3.14159265358979323846f

#define BAUDRATE                1200U
#define BIT_TIME_US             833U        // 1 / 1200 Baudios

// Cantidad de muestras necesarias por bit para cada frecuencia a 22kHz
#define SAMPLES_PER_BIT_MARK    18     // 22000 / 1200 ≈ 18 muestras
#define SAMPLES_PER_BIT_SPACE   10     // 22000 / 2200 = 10 muestras

#define DMAMUX_SOURCE_DAC0      45     // ID de la fuente de hardware DAC0 en K64F

/*******************************************************************************
 * TABLES DE BÚSQUEDA EN RAM (Look-Up Tables)
 ******************************************************************************/
// Almacenamos buffers precargados con un bit completo de duración para cada frecuencia
static uint16_t lut_mark_bit[SAMPLES_PER_BIT_MARK];
static uint16_t lut_space_bit[SAMPLES_PER_BIT_SPACE];

/*******************************************************************************
 * VARIABLES DE ESTADO LOCALES
 ******************************************************************************/
static fsk_v1_rx_callback_t rx_callback = NULL;

static volatile bool tx_busy = false;
static uint8_t tx_bit_buffer[11];
static volatile int8_t current_bit_idx = -1;

/*******************************************************************************
 * PROTOTIPOS LOCALES
 ******************************************************************************/
static void FSK_V1_GenerateLUTs(void);
static void PIT_Baudrate_Init(void);

/*******************************************************************************
 * IMPLEMENTACIÓN DE LA INTERFAZ PÚBLICA V1
 ******************************************************************************/

void FSK_V1_Init(fsk_v1_rx_callback_t rx_cb)
{
    rx_callback = rx_cb;
    tx_busy = false;
    current_bit_idx = -1;

    // 1. Calcular de forma estática las formas de onda en memoria
    FSK_V1_GenerateLUTs();

    // 2. Inicializar los periféricos de MCAL
    DAC_Init();
    DMA_Init();
    PIT_Baudrate_Init();
}

bool FSK_V1_TransmitByte(uint8_t data)
{
    if (tx_busy) {
        return false;
    }

    tx_busy = true;

    // Armar la trama estándar UART de 11 bits (Start, 8 bits, Paridad Impar, Stop)
    tx_bit_buffer[0] = 0; // Start bit (Space = 2200 Hz)

    uint8_t parity_count = 0;
    for (uint8_t i = 0; i < 8; i++) {
        uint8_t bit = (data >> i) & 0x01;
        tx_bit_buffer[i + 1] = bit;
        if (bit) parity_count++;
    }

    tx_bit_buffer[9] = (parity_count % 2 == 0) ? 1 : 0; // Paridad impar
    tx_bit_buffer[10] = 1; // Stop bit (Mark = 1200 Hz)

    // Comenzar apuntando al bit 0 (Start)
    current_bit_idx = 0;

    // Forzar el primer disparo del DMA con el buffer del bit de Space (2200Hz)
    DMA_Configure_MemToPeripheral16(
        DMA_CH_DAC0,
        DMAMUX_SOURCE_DAC0,
        (uint32_t)lut_space_bit,
        (uint32_t)&(DAC0->DAT[0]),
        SAMPLES_PER_BIT_SPACE
    );

    DMA_EnableRequest(DMA_CH_DAC0);

    // Arrancar el PIT para que cuente 833us (tiempo de un bit completo)
    PIT->CHANNEL[0].TCTRL |= PIT_TCTRL_TEN_MASK;

    return true;
}

bool FSK_V1_IsTransmitBusy(void)
{
    return tx_busy;
}

void FSK_V1_PeriodicTask(void)
{
    // Reservado para procesamiento de bloques del ADC (Recepción)
}

/*******************************************************************************
 * GENERACIÓN DE SEÑAL ANALÓGICA & INFRAESTRUCTURA
 ******************************************************************************/

/**
 * @brief Pre-calcula un bit completo de Mark y Space centrados a 1.65V (12 bits)
 */
static void FSK_V1_GenerateLUTs(void)
{
    // Llenar tabla de Mark (1200 Hz -> 18 muestras)
    for (uint16_t i = 0; i < SAMPLES_PER_BIT_MARK; i++) {
        float angle = (2.0f * PI * (float)i) / (float)SAMPLES_PER_BIT_MARK;
        lut_mark_bit[i] = (uint16_t)((sinf(angle) * 2047.0f) + 2048.0f);
    }

    // Llenar tabla de Space (2200 Hz -> 10 muestras)
    for (uint16_t i = 0; i < SAMPLES_PER_BIT_SPACE; i++) {
        float angle = (2.0f * PI * (float)i) / (float)SAMPLES_PER_BIT_SPACE;
        lut_space_bit[i] = (uint16_t)((sinf(angle) * 2047.0f) + 2048.0f);
    }
}

static void PIT_Baudrate_Init(void)
{
    SIM->SCGC6 |= SIM_SCGC6_PIT_MASK;
    PIT->MCR = 0x00;

    // Configurar PIT Canal 0 para interrumpir a la tasa de bits (1200 Hz -> 833 us)
    // 50 MHz / 1200 Hz = 41666 ticks
    PIT->CHANNEL[0].LDVAL = (50000000U / BAUDRATE) - 1;
    PIT->CHANNEL[0].TCTRL = PIT_TCTRL_TIE_MASK; // Habilitar ISR pero arranca apagado (TEN=0)

    NVIC_EnableIRQ(PIT0_IRQn);
}

/**
 * @brief ISR del PIT0 - Se ejecuta SOLO al final de cada bit (Cada 833 us)
 */
void PIT0_IRQHandler(void)
{
    PIT->CHANNEL[0].TFLG = PIT_TFLG_TIF_MASK;

    if (tx_busy) {
        current_bit_idx++;

        if (current_bit_idx < 11) {
            // Determinar el próximo bit a transmitir de la trama
            uint8_t next_bit = tx_bit_buffer[current_bit_idx];

            // Apagar momentáneamente el requerimiento para reconfigurar el puntero de RAM
            DMA_DisableRequest(DMA_CH_DAC0);

            if (next_bit == 1) {
                // Modificar el TCD del DMA al vuelo para que apunte a las muestras de Mark
                DMA0->TCD[DMA_CH_DAC0].SADDR = (uint32_t)lut_mark_bit;
                DMA0->TCD[DMA_CH_DAC0].CITER_ELINKNO = SAMPLES_PER_BIT_MARK;
                DMA0->TCD[DMA_CH_DAC0].BITER_ELINKNO = SAMPLES_PER_BIT_MARK;
                DMA0->TCD[DMA_CH_DAC0].SLAST = -(SAMPLES_PER_BIT_MARK * 2);
            } else {
                // Modificar el TCD del DMA al vuelo para que apunte a las muestras de Space
                DMA0->TCD[DMA_CH_DAC0].SADDR = (uint32_t)lut_space_bit;
                DMA0->TCD[DMA_CH_DAC0].CITER_ELINKNO = SAMPLES_PER_BIT_SPACE;
                DMA0->TCD[DMA_CH_DAC0].BITER_ELINKNO = SAMPLES_PER_BIT_SPACE;
                DMA0->TCD[DMA_CH_DAC0].SLAST = -(SAMPLES_PER_BIT_SPACE * 2);
            }

            // Volver a encender el canal
            DMA_EnableRequest(DMA_CH_DAC0);
        } else {
            // Se transmitieron los 11 bits, apagar transmisión
            DMA_DisableRequest(DMA_CH_DAC0);
            PIT->CHANNEL[0].TCTRL &= ~PIT_TCTRL_TEN_MASK; // Apagar Timer
            current_bit_idx = -1;
            tx_busy = false;

            // Dejar el DAC fijo en tensión media (IDLE)
            DAC_SetData(DAC_0, 2048);
        }
    }
}

