/***************************************************************************//**
  @file     DAC.c
  @brief    Driver del Conversor Digital-Analógico (DAC) para streaming por DMA
 ******************************************************************************/

#include "DAC.h"
#include "MK64F12.h"

#define DAC_DATL_DATA0_WIDTH 8

/**
 * @brief Inicializa el DAC0 configurado específicamente para trabajar con DMA
 */
void DAC_Init(void)
{
    // 1. Habilitar el Clock Gating para el periférico DAC0
    SIM->SCGC2 |= SIM_SCGC2_DAC0_MASK;

    // 2. Inicializar el primer elemento del buffer en el punto medio (1.65V)
    // para evitar un transitorio brusco al encender el sistema (Offset 2048)
    DAC0->DAT[0].DATL = DAC_DATL_DATA0(2048);
    DAC0->DAT[0].DATH = DAC_DATH_DATA1(2048 >> DAC_DATL_DATA0_WIDTH);

    /* 3. Configuración del Registro de Control 0 (C0):
     * - DACEN: Habilita el módulo DAC.
     * - DACRFS: Selecciona la referencia de tensión de la placa (VDDA = 3.3V).
     * - DACTRGSEL: 1 = TRIGGER POR HARDWARE (Esencial para que el Timer/DMA dicten el ritmo).
     */
    DAC0->C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK | DAC_C0_DACTRGSEL_MASK;

    /* 4. Configuración del Registro de Control 1 (C1):
     * - DACDMAEN: Habilita los requerimientos de DMA. Cada vez que el DAC reciba
     * un trigger de hardware, le pedirá un nuevo dato al módulo eDMA.
     */
    DAC0->C1 = DAC_C1_DMAEN_MASK;

    /* 5. Configuración del Registro de Control 2 (C2):
     * - Coloca el puntero del buffer en 0. Como no usamos el buffer interno
     * del DAC en modo circular propio, lo dejamos apuntando siempre al registro 0.
     */
    DAC0->C2 = 0;
}

/**
 * @brief Permite una escritura manual por software (Útil para debug o calibración)
 */
void DAC_SetData(DAC_t dac, DACData_t data)
{
    // Aseguramos que el dato no sature los 12 bits de resolución (0 a 4095)
    if (data > 4095) {
        data = 4095;
    }

    dac->DAT[0].DATL = DAC_DATL_DATA0(data);
    dac->DAT[0].DATH = DAC_DATH_DATA1(data >> DAC_DATL_DATA0_WIDTH);
}
