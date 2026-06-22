/***************************************************************************//**
  @file     CMP.h
  @brief    Driver para el Comparador Analógico (CMP) - K64F
 ******************************************************************************/

#ifndef MCAL_CMP_H_
#define MCAL_CMP_H_

#include <stdint.h>

typedef enum {
    CMP_MODULE_0 = 0,
    CMP_MODULE_1,
    CMP_MODULE_2
} cmp_module_t;

/**
 * @brief Inicializa el módulo CMP con su DAC interno como referencia a VCC/2 (1.65V)
 * y aplica histéresis para el filtrado de ruido.
 * @param module Módulo CMP a utilizar (ej: CMP0)
 * @param input_channel Canal de entrada analógica positivo (donde entra la señal FSK)
 */
void CMP_Init(uint8_t input_channel);

#endif /* MCAL_CMP_H_ */
