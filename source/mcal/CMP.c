/***************************************************************************//**
  @file     CMP.c
  @brief    Implementación del Driver del Comparador Analógico (CMP)
 ******************************************************************************/

#include "CMP.h"
#include "MK64F12.h"


void CMP_Init(uint8_t input_channel)
{
	CMP_Type *cmp = CMP0;

	SIM->SCGC4 |= SIM_SCGC4_CMP_MASK;
	cmp->CR0 = CMP_CR0_HYSTCTR(3);
    /* 2. Configurar Histéresis y Filtro (Registro CR0) */
    // HYSTCTR: 00=5mV, 01=10mV, 10=20mV, 11=30mV. Ponemos nivel 3 (30mV) para filtrar ruido FSK.
    cmp->CR0 = CMP_CR0_HYSTCTR(3);

    /* 3. Configurar el DAC interno como referencia de tensión (Registro DACCR) */
    // DACEN = 1 (Habilitar DAC)
    // VRSEL = 0 (Seleccionar Vin1 = VDD de 3.3V)
    // VOSEL = 32 (Calculo: Vout = (VDD / 64) * (VOSEL + 1). Para 1.65V queremos la mitad, 32 da exacto ~1.7V o 31 para ~1.65V)
    cmp->DACCR = CMP_DACCR_DACEN_MASK | CMP_DACCR_VRSEL(0) | CMP_DACCR_VOSEL(31);

    /* 4. Configurar el Multiplexor de Entradas (Registro MUXCR) */
    // PSEL: Entrada positiva -> El canal analógico por donde entra la señal (input_channel)
    // MSEL: Entrada negativa -> 7 (Selecciona internamente la salida del DAC que acabamos de configurar)
    cmp->MUXCR = CMP_MUXCR_PSEL(input_channel) | CMP_MUXCR_MSEL(7);

    /* 5. Habilitar el Comparador en modo Low-Power / High-Speed (Registro CR1) */
    // EN = 1 (Habilita el módulo)
    // PMODE = 1 (Modo High-Speed para minimizar el delay en la detección de flancos)
    // OPE = 1 (Permite mapear la salida del comparador a un pin externo si se deseara ver en osciloscopio)
    cmp->CR1 = CMP_CR1_EN_MASK | CMP_CR1_PMODE_MASK | CMP_CR1_OPE_MASK;
}
