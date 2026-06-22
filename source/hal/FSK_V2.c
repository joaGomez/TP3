/***************************************************************************//**
  @file     FSK_V2.c
  @brief    Implementación del Módem FSK Bell 202 (Capa HAL) con Base de Tiempo PIT
  ******************************************************************************/

#include "FSK.h"
#include "FSK_V2.h"
#include "MK64F12.h"
#include "timers.h"
#include "mcal/FTM.h"
#include "mcal/CMP.h"
#include <stddef.h>


// --- Configuración de Tiempos del FTM (Reloj a 50MHz, Prescaler /1) ---
#define FTM_CLOCK_TICKS_PER_US   50U
#define HALF_PERIOD_1200HZ_TICKS (416U * FTM_CLOCK_TICKS_PER_US)
#define HALF_PERIOD_2200HZ_TICKS (227U * FTM_CLOCK_TICKS_PER_US)
#define THRESHOLD_TICKS          ((HALF_PERIOD_1200HZ_TICKS + HALF_PERIOD_2200HZ_TICKS) / 2)

// --- Constantes del PIT para Baudrate 1200 ---
#define BIT_TIME_US              833U
#define HALF_BIT_TIME_US         416U

// --- Variables de Transmisión (TX) ---
static volatile uint16_t tx_frame = 0;
static volatile int8_t   tx_bit_index = -1;
static bool tx_busy = false;

// --- Variables de Recepción (RX) ---
static fsk_v2_rx_callback_t app_rx_callback = 0;

typedef enum {
    RX_STATE_IDLE,
    RX_STATE_START,
    RX_STATE_DATA,
    RX_STATE_PARITY,
    RX_STATE_STOP
} rx_state_t;

static volatile rx_state_t rx_state = RX_STATE_IDLE;
static volatile bool latest_fsk_bit = 1;
static volatile uint8_t rx_data_buffer = 0;
static volatile uint8_t rx_bit_count = 0;

// [CAMBIO 2] Prototipos corregidos para que coincidan exactamente con la implementación de abajo
static void FSK_V2_PIT_TX_Callback(void);
static void FSK_V2_PIT_RX_Sampling_Callback(void);
static void FSK_V2_RX_InputCapture_Callback(void);
static bool CalculateOddParity(uint8_t byte);

void FSK_V2_Init(fsk_v2_rx_callback_t rx_cb)
{
    app_rx_callback = rx_cb;
    tx_bit_index = -1;
    tx_busy = false;
    rx_state = RX_STATE_IDLE;
    latest_fsk_bit = 1;

    /* 1. Inicializar Hardware Base (MCAL) */
    PIT_Init();
    CMP_Init(0);

    // Pasamos las funciones corregidas con el prefijo _V2_
    FTM_TX_Init(FTM_MODULE_0, FTM_CHANNEL_0, NULL);
    FTM_TX_SetFrequency(FTM_MODULE_0, FSK_FREQ_MARK);

    FTM_RX_Init(FTM_MODULE_1, FTM_CHANNEL_0, FSK_V2_RX_InputCapture_Callback);
}

bool FSK_V2_IsTransmitBusy(void)
{
    return tx_busy;
}

bool FSK_V2_TransmitByte(uint8_t data)
{
    if (tx_busy) return false;

    bool parity = CalculateOddParity(data);

    tx_frame = 0;
    tx_frame |= (0 << 0);
    tx_frame |= ((uint16_t)data << 1);
    tx_frame |= ((uint16_t)parity << 9);
    tx_frame |= (1 << 10);

    tx_bit_index = 0;
    tx_busy = true;

    FTM_TX_SetFrequency(FTM_MODULE_0, FSK_FREQ_SPACE);

    // Pasamos la función corregida con el prefijo _V2_
    PIT_ConfigureChannel(PIT_CH0, BIT_TIME_US, FSK_V2_PIT_TX_Callback);

    return true;
}

/*******************************************************************************
 * CALLBACKS DE INTERRUPCIÓN
 ******************************************************************************/

static void FSK_V2_PIT_TX_Callback(void)
{
    if (tx_busy) {
        tx_bit_index++;

        if (tx_bit_index < 11) {
            uint8_t current_bit = (tx_frame >> tx_bit_index) & 0x01;
            if (current_bit == 1) {
                FTM_TX_SetFrequency(FTM_MODULE_0, FSK_FREQ_MARK);
            } else {
                FTM_TX_SetFrequency(FTM_MODULE_0, FSK_FREQ_SPACE);
            }
        } else {
            PIT_StopChannel(PIT_CH0);
            tx_bit_index = -1;
            tx_busy = false;
            FTM_TX_SetFrequency(FTM_MODULE_0, FSK_FREQ_MARK);
        }
    }
}

static void FSK_V2_RX_InputCapture_Callback(void)
{
    static uint16_t last_capture = 0;
    uint16_t current_capture = FTM_RX_GetCaptureValue(FTM_MODULE_1, FTM_CHANNEL_0);

    uint16_t delta_ticks = current_capture - last_capture;
    last_capture = current_capture;

    bool previous_bit = latest_fsk_bit;
    latest_fsk_bit = (delta_ticks > THRESHOLD_TICKS) ? 1 : 0;

    if (rx_state == RX_STATE_IDLE && previous_bit == 1 && latest_fsk_bit == 0) {
        rx_state = RX_STATE_START;

        // Pasamos la función corregida con el prefijo _V2_
        PIT_ConfigureChannel(PIT_CH1, HALF_BIT_TIME_US, FSK_V2_PIT_RX_Sampling_Callback);
    }
}

static void FSK_V2_PIT_RX_Sampling_Callback(void)
{
    if (rx_state == RX_STATE_START) {
        PIT_ConfigureChannel(PIT_CH1, BIT_TIME_US, FSK_V2_PIT_RX_Sampling_Callback);
    }

    bool sampled_bit = latest_fsk_bit;

    switch (rx_state) {
        case RX_STATE_START:
            if (sampled_bit == 0) {
                rx_data_buffer = 0;
                rx_bit_count = 0;
                rx_state = RX_STATE_DATA;
            } else {
                PIT_StopChannel(PIT_CH1);
                rx_state = RX_STATE_IDLE;
            }
            break;

        case RX_STATE_DATA:
            if (sampled_bit == 1) {
                rx_data_buffer |= (1 << rx_bit_count);
            }
            rx_bit_count++;

            if (rx_bit_count >= 8) {
                rx_state = RX_STATE_PARITY;
            }
            break;

        case RX_STATE_PARITY:
            if (sampled_bit == CalculateOddParity(rx_data_buffer)) {
                rx_state = RX_STATE_STOP;
            } else {
                PIT_StopChannel(PIT_CH1);
                rx_state = RX_STATE_IDLE;
            }
            break;

        case RX_STATE_STOP:
            if (sampled_bit == 1) {
                if (app_rx_callback) {
                    app_rx_callback(rx_data_buffer);
                }
            }
            PIT_StopChannel(PIT_CH1);
            rx_state = RX_STATE_IDLE;
            break;

        default:
            PIT_StopChannel(PIT_CH1);
            rx_state = RX_STATE_IDLE;
            break;
    }
}

static bool CalculateOddParity(uint8_t byte)
{
    uint8_t count = 0;
    for (uint8_t i = 0; i < 8; i++) {
        if ((byte >> i) & 0x01) count++;
    }
    return (count % 2 == 0);
}

void FSK_V2_PeriodicTask(void) {
    // Guiado 100% por interrupciones de hardware
}


