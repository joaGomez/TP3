/***************************************************************************//**
  @file     DMA.c
  @brief    Driver del controlador eDMA para K64F

  Nota (erratum e4588): NO se usa el "periodic trigger" del DMAMUX (TRIG=1)
  con fuente "Always Enabled" y major loop > 1, porque la peticion no se
  desasigna correctamente. Por eso:
    - TX (RAM->DAC): el ritmo lo da un canal de FTM (fuente real del DMAMUX),
      no el PIT+AlwaysEnabled.
    - RX (ADC->RAM): el ADC es la fuente real (una peticion por COCO), disparado
      por el PIT via SIM_SOPT7. Tampoco usa el periodic trigger del DMAMUX.
 ******************************************************************************/

#include "DMA.h"
#include "MK64F12.h"

void DMA_Init(void)
{
    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;
    SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;
    DMA0->CR = 0;   // prioridades fijas
}

void DMA_Configure_TxToDAC(uint8_t channel, uint8_t source_request,
                           uint32_t src_addr, uint32_t dest_addr,
                           uint16_t buffer_size)
{
    DMAMUX->CHCFG[channel] = 0;

    DMA0->TCD[channel].SADDR = src_addr;
    DMA0->TCD[channel].DADDR = dest_addr;
    DMA0->TCD[channel].SOFF  = 2;      // avanzar 2 bytes (uint16_t) en RAM
    DMA0->TCD[channel].DOFF  = 0;      // siempre el mismo registro del DAC
    DMA0->TCD[channel].ATTR  = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1); // 16/16
    DMA0->TCD[channel].NBYTES_MLNO = 2;                 // 1 muestra por request
    DMA0->TCD[channel].SLAST = -(int32_t)(buffer_size * 2);  // wrap al inicio
    DMA0->TCD[channel].DLAST_SGA = 0;
    DMA0->TCD[channel].CITER_ELINKNO = buffer_size;
    DMA0->TCD[channel].BITER_ELINKNO = buffer_size;
    // INTMAJOR: interrumpe al terminar el major loop (== fin de un bit) para
    // rellenar el buffer del proximo bit. Sin DREQ: sigue auto-recargando.
    DMA0->TCD[channel].CSR = DMA_CSR_INTMAJOR_MASK;

    DMAMUX->CHCFG[channel] = DMAMUX_CHCFG_SOURCE(source_request) | DMAMUX_CHCFG_ENBL_MASK;
}

void DMA_Configure_RxFromADC(uint8_t channel, uint8_t source_request,
                             uint32_t src_addr, uint32_t dest_addr,
                             uint16_t ring_len)
{
    DMAMUX->CHCFG[channel] = 0;

    DMA0->TCD[channel].SADDR = src_addr;       // &ADC0->R[0] (fijo)
    DMA0->TCD[channel].DADDR = dest_addr;       // ring en RAM
    DMA0->TCD[channel].SOFF  = 0;               // siempre lee R[0]
    DMA0->TCD[channel].DOFF  = 2;               // avanza por el ring (uint16_t)
    DMA0->TCD[channel].ATTR  = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1);
    DMA0->TCD[channel].NBYTES_MLNO = 2;
    DMA0->TCD[channel].SLAST = 0;               // la fuente no se mueve
    DMA0->TCD[channel].DLAST_SGA = -(int32_t)(ring_len * 2); // wrap del ring
    DMA0->TCD[channel].CITER_ELINKNO = ring_len;
    DMA0->TCD[channel].BITER_ELINKNO = ring_len;
    DMA0->TCD[channel].CSR = 0;                 // circular puro, sin IRQ

    DMAMUX->CHCFG[channel] = DMAMUX_CHCFG_SOURCE(source_request) | DMAMUX_CHCFG_ENBL_MASK;
}

void DMA_EnableRequest(uint8_t channel)  { DMA0->SERQ = channel; }
void DMA_DisableRequest(uint8_t channel) { DMA0->CERQ = channel; }

uint16_t DMA_RxWriteIndex(uint8_t channel, uint32_t ring_base, uint16_t ring_len)
{
    (void)ring_len;
    uint32_t daddr = DMA0->TCD[channel].DADDR;
    return (uint16_t)(((daddr - ring_base) / 2));
}
