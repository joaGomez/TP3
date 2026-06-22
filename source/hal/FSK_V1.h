/***************************************************************************//**
  @file     FSK_V1.h
  @brief    Módem FSK Bell 202 (Capa HAL) - Versión 1 (ADC + DAC + DMA)
 ******************************************************************************/

#ifndef HAL_FSK_V1_H_
#define HAL_FSK_V1_H_

#include <stdint.h>
#include <stdbool.h>

/* Callback específico para la recepción de la Versión 1 */
typedef void (*fsk_v1_rx_callback_t)(uint8_t data);

/**
 * @brief Inicializa el sistema de modulación y demodulación FSK V1 (Streaming)
 * @param rx_cb Callback invocado al recibir un byte válido
 */
void FSK_V1_Init(fsk_v1_rx_callback_t rx_cb);

/**
 * @brief Envía un byte a través del módem FSK V1 (Carga buffer de streaming)
 * @param data Byte a transmitir
 * @return true si se pudo iniciar, false si el transmisor está ocupado
 */
bool FSK_V1_TransmitByte(uint8_t data);

/**
 * @brief Verifica si el transmisor FSK V1 está ocupado
 * @return true si está ocupado, false si está libre (IDLE)
 */
bool FSK_V1_IsTransmitBusy(void);

/**
 * @brief Tarea periódica de procesamiento para RX en la Versión 1
 */
void FSK_V1_PeriodicTask(void);

#endif /* HAL_FSK_V1_H_ */
