/***************************************************************************//**
  @file     DMA.h
  @brief    Driver del controlador eDMA (Enhanced Direct Memory Access) - K64F
 ******************************************************************************/

#ifndef MCAL_DMA_H_
#define MCAL_DMA_H_

#include <stdint.h>
#include <stdbool.h>

/* ---- Canales eDMA usados por la V1 -------------------------------------- */
#define DMA_CH_DAC0   0   /* TX: RAM -> DAC0->DAT[0], disparado por FTM0_CH0  */
#define DMA_CH_ADC0   1   /* RX: ADC0->R[0] -> buffer circular en RAM         */

/* ---- IDs de fuente del DMAMUX (K64F, RM "Chip Configuration") -----------
 * Verificar contra la tabla de DMA request sources del RM de la K64.       */
#define DMAMUX_SRC_ADC0       40   /* 0x28 */
#define DMAMUX_SRC_FTM0_CH0   20   /* FTM0 canal 0 (FTM0 ch0..7 = 20..27)     */

void DMA_Init(void);

/**
 * @brief TX: configura un canal RAM->periferico de 16 bits, con auto-reload
 *        (buffer circular) e interrupcion al fin del major loop (INTMAJOR).
 *        El ritmo lo da la 'source_request' (ej: un canal de FTM en free-run).
 *        El handler de fin de major loop debe rellenar el buffer del proximo bit.
 */
void DMA_Configure_TxToDAC(uint8_t channel, uint8_t source_request,
                           uint32_t src_addr, uint32_t dest_addr,
                           uint16_t buffer_size);

/**
 * @brief RX: configura un canal periferico->RAM de 16 bits como buffer
 *        circular (ADC0->R[0] -> ring). Sin interrupcion (la DSP lo consume
 *        en la tarea periodica). El ADC genera la peticion por cada COCO.
 */
void DMA_Configure_RxFromADC(uint8_t channel, uint8_t source_request,
                             uint32_t src_addr, uint32_t dest_addr,
                             uint16_t ring_len);

void DMA_EnableRequest(uint8_t channel);
void DMA_DisableRequest(uint8_t channel);

/** Indice (0..ring_len-1) de la PROXIMA posicion que el DMA escribira en RX.
 *  Permite a la DSP saber cuantas muestras nuevas hay. */
uint16_t DMA_RxWriteIndex(uint8_t channel, uint32_t ring_base, uint16_t ring_len);

#endif /* MCAL_DMA_H_ */
