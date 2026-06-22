/***************************************************************************//**
  @file     App.c
  @brief    Application functions - FSK Modem Bridge (Dual Version Configuration)
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include "hal/board.h"
#include "hal/leds.h"
#include "hal/system.h"
#include "hal/timers.h"
#include "hal/FSK_V1.h"
#include "hal/FSK_V2.h"
#include "hal/USBComunications.h"
#include "hardware.h"
#include <string.h>

// ============================================================================
// CONFIGURACIÓN DE SELECCIÓN DE ENTREGA
// ============================================================================
// #define USE_VERSION_2   // <-- Descomentar para compilar la V2 (PIT + CMP + FTM)
#define USE_VERSION_1   // <-- Descomentar para compilar la V1 (ADC + DAC + DMA)



/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/
// Cola o buffer circular simple de recepción para pasar los bytes decodificados
#define RX_BUFFER_SIZE  128

/*******************************************************************************
 * VARIABLES LOCALES CON ALCANCE DE ARCHIVO
 ******************************************************************************/
static volatile uint8_t fsk_rx_fifo[RX_BUFFER_SIZE];
static volatile uint16_t fsk_rx_head = 0;
static volatile uint16_t fsk_rx_tail = 0;

/*******************************************************************************
 * PROTOTIPOS DE FUNCIONES PRIVADAS
 ******************************************************************************/
static void App_FSK_Receive_Callback(uint8_t incoming_byte);
static bool App_PushRxByte(uint8_t byte);
static bool App_PopRxByte(uint8_t *byte);

/*******************************************************************************
 * CONFIGURACIÓN E INICIALIZACIÓN
 ******************************************************************************/

void App_Init(void) {
    USBCom_Init();
    initSystem(20);

    if(ledsInit(RED)) {
    		return;
    	}
    	if(ledsInit(GREEN)) {
    		return;
    	}
    	if(ledsInit(BLUE)) {
    		return;
    	}

    	ledOn(LEDOFF);

#ifdef USE_VERSION_2
    // ==========================================
    // INICIALIZACIÓN - VERSÍON 2 (PIT + CMP + FTM)
    // ==========================================
    FSK_V2_Init(App_FSK_Receive_Callback);
    ledOn(BLUE); // Azul indica que corre la V2 por hardware
#else
    // ==========================================
    // INICIALIZACIÓN - VERSÍON 1 (ADC + DAC + DMA)
    // ==========================================
    FSK_V1_Init((fsk_v1_rx_callback_t)App_FSK_Receive_Callback);
    ledOn(RED);  // Rojo indica que corre la V1 por streaming
#endif

    __enable_irq();
}

void App_Run(void) {
    uint8_t pc_tx_byte;
    uint8_t fsk_rx_byte;

    // TX: Leer desde la PC a través de USB y transmitir por el módem activo
    if (USBCom_GetByte(&pc_tx_byte)) {
#ifdef USE_VERSION_2
        if (!FSK_V2_IsTransmitBusy()) {
            FSK_V2_TransmitByte(pc_tx_byte);
        }
#else
        if (!FSK_V1_IsTransmitBusy()) {
            FSK_V1_TransmitByte(pc_tx_byte);
        }
#endif
    }

    // RX: Leer desde el buffer circular de la aplicación y mandarlo a la PC
    if (App_PopRxByte(&fsk_rx_byte)) {
        ledOn(GREEN); // Destello verde al procesar un byte recibido
        char tx_str[2] = {(char)fsk_rx_byte, '\0'};
        USBCom_SendString(tx_str);
    }

#ifdef USE_VERSION_2
    FSK_V2_PeriodicTask();
#else
    FSK_V1_PeriodicTask();
#endif
}

/*******************************************************************************
 * FUNCIONES AUXILIARES Y CALLBACKS (Guiados por Interrupción)
 ******************************************************************************/

/**
 * @brief Callback disparado en background por el driver del módem activo
 * al decodificar un byte con éxito y verificar su paridad impar.
 */
static void App_FSK_Receive_Callback(uint8_t incoming_byte)
{
    // Empujamos el byte a la cola interna para liberarle la CPU a las interrupciones
    App_PushRxByte(incoming_byte);
}

static bool App_PushRxByte(uint8_t byte)
{
    uint16_t next_head = (fsk_rx_head + 1) % RX_BUFFER_SIZE;
    if (next_head == fsk_rx_tail) {
        return false; // Buffer lleno (Overflow de aplicación)
    }
    fsk_rx_fifo[fsk_rx_head] = byte;
    fsk_rx_head = next_head;
    return true;
}

static bool App_PopRxByte(uint8_t *byte)
{
    if (fsk_rx_head == fsk_rx_tail) {
        return false; // Buffer vacío
    }
    *byte = fsk_rx_fifo[fsk_rx_tail];
    fsk_rx_tail = (fsk_rx_tail + 1) % RX_BUFFER_SIZE;
    return true;
}
