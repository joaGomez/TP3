/***************************************************************************//**
  @file     FSK.h
  @brief    Módem FSK Bell 202 (Capa HAL) - Interfaz Unificada (V1 / V2)
 ******************************************************************************/

#ifndef HAL_FSK_H_
#define HAL_FSK_H_

#include <stdint.h>
#include <stdbool.h>

/* Callback para avisarle a la aplicación que se recibió un byte correctamente */
typedef void (*fsk_rx_callback_t)(uint8_t data);

/**
 * @brief Inicializa el sistema de modulación y demodulación FSK
 * Configura internamente los drivers según la versión activa por directivas
 * de compilación (V2: FTM+CMP o V1: ADC+DAC+DMA).
 * @param rx_cb Callback invocado al recibir un byte válido
 */
void FSK_Init(fsk_rx_callback_t rx_cb);

/**
 * @brief Envía un byte a través del módem FSK activo
 * @param data Byte a transmitir
 * @return true si se pudo iniciar la transmisión, false si está ocupado
 */
bool FSK_TransmitByte(uint8_t data);

/**
 * @brief Verifica si el transmisor FSK está ocupado enviando una trama
 * @return true si está ocupado, false si está libre (IDLE)
 */
bool FSK_IsTransmitBusy(void);

/**
 * @brief Función periódica de polling para la máquina de estados si fuera necesario
 */
void FSK_PeriodicTask(void);

#endif /* HAL_FSK_H_ */
