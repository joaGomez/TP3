/***************************************************************************//**
  @file     DAC.c
  @brief    Driver del Conversor Digital-Analogico (DAC) - streaming por DMA.

  Nota de diseno (corregida):
   - Con el buffer del DAC DESHABILITADO (DACBFEN=0, reset), DAT[0] es la salida
     "viva": cada escritura a DAT[0] aparece inmediatamente en la salida. No hace
     falta trigger ni se generan peticiones de DMA desde el DAC.
   - Por eso el ritmo de muestreo (Fs) lo impone una fuente EXTERNA que dispara el
     eDMA (en la V1: un canal de FTM en free-run -> DMA -> DAT[0]). Ver FSK_V1.c.
   - [FIX] El codigo anterior ponia DACTRGSEL=1 (que es trigger por SOFTWARE, no
     por hardware como decia el comentario) y DACDMAEN=1 esperando que el DAC
     pidiera DMA; con el buffer deshabilitado eso nunca ocurre, asi que la cadena
     de TX no transmitia. Aqui se deja el DAC en modo "registro vivo".
 ******************************************************************************/

#include "DAC.h"
#include "MK64F12.h"

#define DAC_DATL_DATA0_WIDTH 8

void DAC_Init(void)
{
    // 1. Clock gating del DAC0
    SIM->SCGC2 |= SIM_SCGC2_DAC0_MASK;

    // 2. Valor inicial = media escala (1.65 V) para evitar transitorio al arrancar
    DAC0->DAT[0].DATL = DAC_DATL_DATA0(2048);
    DAC0->DAT[0].DATH = DAC_DATH_DATA1(2048 >> DAC_DATL_DATA0_WIDTH);

    // 3. C0: habilitar DAC y referencia = VDDA (3.3 V). Buffer deshabilitado
    //    (DACBFEN=0) => DAT[0] es la salida viva. DACTRGSEL/DMAEN no se usan.
    DAC0->C0 = DAC_C0_DACEN_MASK | DAC_C0_DACRFS_MASK;

    // 4. C1 / C2 en reset (sin buffer, sin DMA propio del DAC)
    DAC0->C1 = 0;
    DAC0->C2 = 0;
}

void DAC_SetData(DAC_t dac, DACData_t data)
{
    if (data > 4095) {
        data = 4095;
    }
    dac->DAT[0].DATL = DAC_DATL_DATA0(data);
    dac->DAT[0].DATH = DAC_DATH_DATA1(data >> DAC_DATL_DATA0_WIDTH);
}
