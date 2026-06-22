/***************************************************************************//**
  @file     DMA.c
  @brief    Driver del controlador eDMA para K64F
 ******************************************************************************/

#include "DMA.h"
#include "MK64F12.h"

void DMA_Init(void)
{
    // 1. Habilitar Clock Gating para el eDMA y el DMAMUX
    SIM->SCGC7 |= SIM_SCGC7_DMA_MASK;
    SIM->SCGC6 |= SIM_SCGC6_DMAMUX_MASK;

    // 2. Habilitar el modo de prioridades fijas en el controlador de DMA
    DMA0->CR = 0;
}

void DMA_Configure_MemToPeripheral16(uint8_t channel, uint8_t source_request,
                                     uint32_t src_addr, uint32_t dest_addr,
                                     uint16_t buffer_size)
{
    // Asegurar que el canal esté deshabilitado antes de configurar
    DMAMUX->CHCFG[channel] = 0;

    // 1. Configurar el DMAMUX para el canal elegido
    // Setea la fuente de hardware (source_request) y activa el canal (ENBL = 1)
    DMAMUX->CHCFG[channel] = DMAMUX_CHCFG_SOURCE(source_request) | DMAMUX_CHCFG_ENBL_MASK;

    // 2. Configurar el TCD (Transfer Control Descriptor) del eDMA

    // Dirección de origen (Buffer de RAM con las muestras)
    DMA0->TCD[channel].SADDR = src_addr;

    // Dirección de destino (Registro de datos del DAC)
    DMA0->TCD[channel].DADDR = dest_addr;

    // Modificadores de dirección (Offset):
    DMA0->TCD[channel].SOFF = 2;  // Avanza 2 bytes en la RAM tras cada muestra (uint16_t)
    DMA0->TCD[channel].DOFF = 0;  // Siempre escribe en el mismo registro del DAC (0 offset)

    // Tamaño de los datos (Atributos): Entrada de 16 bits, Salida de 16 bits
    // 1 = 16 bits (half-word)
    DMA0->TCD[channel].ATTR = DMA_ATTR_SSIZE(1) | DMA_ATTR_DSIZE(1);

    // Cantidad de bytes por "Minor Loop" (Cada petición de hardware mueve 1 muestra = 2 bytes)
    DMA0->TCD[channel].NBYTES_MLNO = 2;

    // Ajuste de la dirección de origen al terminar el "Major Loop"
    // Al transferir todo el buffer, resta (buffer_size * 2) bytes para volver a apuntar al inicio de la tabla
    DMA0->TCD[channel].SLAST = -(buffer_size * 2);
    DMA0->TCD[channel].DLAST_SGA = 0; // El destino no cambia al final

    // Iteraciones del "Major Loop" (Cuántas muestras totales componen el buffer circular)
    DMA0->TCD[channel].CITER_ELINKNO = buffer_size;
    DMA0->TCD[channel].BITER_ELINKNO = buffer_size;

    // Configuración de Control y Estado (CSR)
    // Dejamos en 0 para que sea un buffer circular puro que se auto-recarga infinitamente sin pedir interrupción
    DMA0->TCD[channel].CSR = 0;
}

void DMA_EnableRequest(uint8_t channel)
{
    DMA0->SERQ = channel;
}

void DMA_DisableRequest(uint8_t channel)
{
    DMA0->CERQ = channel;
}
