/***************************************************************************//**
  @file     timers.h
  @brief    Driver para el Timer de Interrupción Periódica (PIT) - K64F
  ******************************************************************************/

#ifndef HAL_TIMERS_H_
#define HAL_TIMERS_H_

#include <stdint.h>

typedef enum {
    PIT_CH0 = 0,
    PIT_CH1,
    PIT_CH2,
    PIT_CH3
} pit_channel_t;

typedef void (*pit_callback_t)(void);

/**
 * @brief Inicializa el módulo PIT global
 */
void PIT_Init(void);

/**
 * @brief Configura y enciende un canal del PIT con un tiempo en microsegundos
 * @param channel Canal a configurar (0 a 3)
 * @param us Tiempo en microsegundos
 * @param callback Función a llamar al vencer el temporizador
 */
void PIT_ConfigureChannel(pit_channel_t channel, uint32_t us, pit_callback_t callback);

/**
 * @brief Detiene un canal del PIT
 */
void PIT_StopChannel(pit_channel_t channel);

/**
 * @brief Inicia o reinicia un canal del PIT (asume ya configurado)
 */
void PIT_StartChannel(pit_channel_t channel);

#endif /* HAL_TIMERS_H_ */
