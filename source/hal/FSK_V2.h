/***************************************************************************//**
  @file     FSK_V2.h
  @brief    Módem FSK Bell 202 (Capa HAL) - Versión 2 (PIT + CMP + FTM)
 ******************************************************************************/

#ifndef HAL_FSK_V2_H_
#define HAL_FSK_V2_H_

#include <stdint.h>
#include <stdbool.h>

/* Callback para avisarle a la aplicación que se recibió un byte correctamente */
typedef void (*fsk_v2_rx_callback_t)(uint8_t data);

/**
 * @brief Inicializa el sistema de modulación y demodulación FSK (Versión 2)
 * Configura internamente los drivers MCAL de FTM y CMP, y establece los timers
 * PIT para el control estricto de los tiempos de bit.
 * @param rx_cb Callback invocado al recibir un byte válido
 */
void FSK_V2_Init(fsk_v2_rx_callback_t rx_cb);

/**
 * @brief Envía un byte a través del módem FSK por conmutación digital (Versión 2)
 * Transforma el byte en una trama de 11 bits y arranca el PIT0 para cambiar la
 * frecuencia del FTM0 de manera síncrona cada 833us.
 * @param data Byte a transmitir
 * @return true si se pudo encolar el byte, false si el transmisor está ocupado
 */
bool FSK_V2_TransmitByte(uint8_t data);

/**
 * @brief Verifica si el transmisor FSK V2 está ocupado enviando una trama por hardware
 * @return true si está ocupado, false si está libre (IDLE)
 */
bool FSK_V2_IsTransmitBusy(void);

/**
 * @brief Función periódica de polling para la máquina de estados de la Versión 2
 * (Vacía si todo el sistema corre guiado al 100% por interrupciones).
 */
void FSK_V2_PeriodicTask(void);

#endif /* HAL_FSK_V2_H_ */
