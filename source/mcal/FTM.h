/***************************************************************************//**
  @file     FTM.h
  @brief    Driver para el periférico FlexTimer (FTM) - K64F
 ******************************************************************************/

#ifndef MCAL_FTM_H_
#define MCAL_FTM_H_

#include <stdint.h>
#include <stdbool.h>

/*******************************************************************************
 * ENUMERACIONES Y DEFINICIONES
 ******************************************************************************/

typedef enum {
    FTM_MODULE_0 = 0,
    FTM_MODULE_1,
    FTM_MODULE_2,
    FTM_MODULE_3
} ftm_module_t;

typedef enum {
    FTM_CHANNEL_0 = 0,
    FTM_CHANNEL_1,
    FTM_CHANNEL_2,
    FTM_CHANNEL_3,
    FTM_CHANNEL_4,
    FTM_CHANNEL_5,
    FTM_CHANNEL_6,
    FTM_CHANNEL_7
} ftm_channel_t;

/* Frecuencias requeridas para Bell 202 */
#define FSK_FREQ_MARK   1200  // Hz (Bit '1')
#define FSK_FREQ_SPACE  2200  // Hz (Bit '0')

/* Tipo de callback para las interrupciones del FTM */
typedef void (*ftm_callback_t)(void);

/*******************************************************************************
 * FUNCIONES DE CONFIGURACIÓN Y CONTROL
 ******************************************************************************/

/**
 * @brief Inicializa un módulo FTM en modo Output Compare / PWM para Transmisión (TX)
 * @param module Módulo FTM a utilizar (ej: FTM0 o FTM3)
 * @param channel Canal FTM asociado al pin de salida
 * @param callback Función que se llamará en cada desborde/interrupción de canal (para cambiar de estado)
 */
void FTM_TX_Init(ftm_module_t module, ftm_channel_t channel, ftm_callback_t callback);

/**
 * @brief Modifica dinámicamente la frecuencia de salida manteniendo la fase continua
 * @param module Módulo FTM de TX
 * @param frequency Frecuencia deseada (FSK_FREQ_MARK o FSK_FREQ_SPACE)
 */
void FTM_TX_SetFrequency(ftm_module_t module, uint32_t frequency);

/**
 * @brief Inicializa un módulo FTM en modo Input Capture para Recepción (RX)
 * @param module Módulo FTM a utilizar (ej: FTM1 o FTM2)
 * @param channel Canal FTM mapeado a la salida del CMP (usando SIM_SOPT4)
 * @param callback Función que se llamará al capturar un flanco
 */
void FTM_RX_Init(ftm_module_t module, ftm_channel_t channel, ftm_callback_t callback);

/**
 * @brief Lee el valor capturado en el registro del canal (para cálculo de períodos)
 * @param module Módulo FTM de RX
 * @param channel Canal FTM de RX
 * @return Valor del contador capturado en el flanco (16 bits)
 */
uint16_t FTM_RX_GetCaptureValue(ftm_module_t module, ftm_channel_t channel);

/**
 * @brief Limpia la bandera de interrupción del canal
 * @param module Módulo FTM
 * @param channel Canal FTM
 */
void FTM_ClearInterruptFlag(ftm_module_t module, ftm_channel_t channel);

#endif /* MCAL_FTM_H_ */
