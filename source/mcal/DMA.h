/***************************************************************************//**
  @file     DMA.h
  @brief    Driver del controlador eDMA (Enhanced Direct Memory Access)
 ******************************************************************************/

#ifndef MCAL_DMA_H_
#define MCAL_DMA_H_

#include <stdint.h>
#include <stdbool.h>

// Definición de canales de eDMA que usaremos
#define DMA_CH_DAC0         0   // Canal 0 dedicado al streaming del DAC

/**
 * @brief Inicializa el módulo eDMA y el DMAMUX (Multiplexor de DMA)
 */
void DMA_Init(void);

/**
 * @brief Configura un canal de DMA para transferencia de memoria a periférico (16 bits)
 * @param channel Número de canal eDMA (0 a 15)
 * @param source_request ID de la fuente en el DMAMUX (ej: 45 para DAC0)
 * @param src_addr Dirección del buffer de origen en RAM (Tabla de Senos)
 * @param dest_addr Dirección del registro de destino (DAC0->DAT[0])
 * @param buffer_size Cantidad total de muestras a transferir antes de reiniciar
 */
void DMA_Configure_MemToPeripheral16(uint8_t channel, uint8_t source_request,
                                     uint32_t src_addr, uint32_t dest_addr,
                                     uint16_t buffer_size);

/**
 * @brief Habilita las peticiones de hardware para un canal específico
 */
void DMA_EnableRequest(uint8_t channel);

/**
 * @brief Deshabilita las peticiones de hardware para un canal específico
 */
void DMA_DisableRequest(uint8_t channel);

#endif /* MCAL_DMA_H_ */
