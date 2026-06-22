/***************************************************************************//**
  @file     DAC.h
  @brief    Driver del Conversor Digital-Analógico (DAC) para streaming por DMA
 ******************************************************************************/

#ifndef MCAL_DAC_H_
#define MCAL_DAC_H_

#include <stdint.h>
#include "MK64F12.h"

// Tipo de datos para el valor digital que va al DAC (12 bits: 0 a 4095)
typedef uint16_t DACData_t;

// Redefinimos el tipo para apuntar a la estructura de registros del SDK
typedef DAC_Type* DAC_t;

// Exponemos el puntero del periférico DAC0 de forma amigable para la app
#define DAC_0   DAC0

/**
 * @brief Inicializa el DAC0 configurado específicamente para trabajar con DMA
 * Activa el Clock Gating, setea el offset inicial a la mitad de escala (1.65V),
 * y habilita tanto el Trigger por Hardware como las peticiones eDMA.
 */
void DAC_Init(void);

/**
 * @brief Permite una escritura manual por software
 * Útil para debug, testing o calibración manual de tensiones continuas.
 * @param dac Puntero al módulo (ej: DAC_0)
 * @param data Valor de 12 bits a convertir
 */
void DAC_SetData(DAC_t dac, DACData_t data);

#endif /* MCAL_DAC_H_ */
