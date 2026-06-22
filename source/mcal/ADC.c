
#include "ADC.h"

#define TWO_POW_NUM_OF_CAL (1 << 4)

bool ADC_interrupt[2] = {false, false};

void ADC_Init (void)
{
    // 1. Pin fisico PTB2 (A0 = ADC0_SE12) en modo analogico (MUX = ALT0)
    SIM->SCGC5 |= SIM_SCGC5_PORTB_MASK;
    PORTB->PCR[2] = PORT_PCR_MUX(0);

    // 2. Clock gating SOLO de ADC0 (la V1 usa unicamente ADC0).
    //    [FIX] Antes se habilitaba ADC1 (SCGC3) y su IRQ del NVIC sin configurarlo
    //    ni tener handler -> se elimina para no dejar perifericos/vectores a medias.
    SIM->SCGC6 |= SIM_SCGC6_ADC0_MASK;

    // ADC clock = bus/1, sin divisor
    ADC0->CFG1 = ADC_CFG1_ADIV(0x00);

    ADC_SetResolution(ADC0, ADC_b12); // 12 bits (0..4095) -> alinea con DAC0->DAT0
    ADC_SetCycles(ADC0, ADC_c4);
    ADC_Calibrate(ADC0);

    // [FIX] No se dispara ninguna conversion aqui. Antes se escribia
    //   ADC0->SC1[0] = ADC_SC1_ADCH(12), lo que lanzaba UNA conversion que
    //   nadie leia ni rearmaba. El armado real (HW trigger + DMA) lo hace
    //   ADC_ConfigRxHwTriggerDMA(), llamado por la cadena de RX.
}


void ADC_SetResolution (ADC_t adc, ADCBits_t bits)
{
	adc->CFG1 = (adc->CFG1 & ~ADC_CFG1_MODE_MASK) | ADC_CFG1_MODE(bits);
}

void ADC_SetCycles (ADC_t adc, ADCCycles_t cycles)
{
	if (cycles & ~ADC_CFG2_ADLSTS_MASK)
	{
		adc->CFG1 &= ~ADC_CFG1_ADLSMP_MASK;
	}
	else
	{
		adc->CFG1 |= ADC_CFG1_ADLSMP_MASK;
		adc->CFG2 = (adc->CFG2 & ~ADC_CFG2_ADLSTS_MASK) | ADC_CFG2_ADLSTS(cycles);
	}
}



void ADC_SetInterruptMode (ADC_t adc, bool mode)
{
	if (adc == ADC0)
		ADC_interrupt[0] = mode;
	else if (adc == ADC1)
		ADC_interrupt[1] = mode;
}

bool ADC_IsInterruptPending (ADC_t adc)
{
	return adc->SC1[0] & ADC_SC1_COCO_MASK;
}

void ADC_ClearInterruptFlag (ADC_t adc)
{
	// [FIX] COCO se limpia leyendo R[0] (o escribiendo SC1 con ADCH=0b11111).
	// Antes se hacia SC1[0]=0, y como ADCH=0 es un canal valido, eso LANZABA
	// una conversion espuria en el canal 0 en vez de limpiar el flag.
	(void)adc->R[0];
}



ADCBits_t ADC_GetResolution (ADC_t adc)
{
	// [FIX] devolver el valor del enum, no el campo "in-place".
	return (ADCBits_t)((adc->CFG1 & ADC_CFG1_MODE_MASK) >> ADC_CFG1_MODE_SHIFT);
}



ADCCycles_t ADC_GetCycles (ADC_t adc)   // [FIX] nombre alineado con el header
{
	if (adc->CFG1 & ADC_CFG1_ADLSMP_MASK)
		return ADC_c4;
	else
		return (ADCCycles_t)(adc->CFG2 & ADC_CFG2_ADLSTS_MASK);
}

void ADC_SetHardwareAverage (ADC_t adc, ADCTaps_t taps)
{
	if (taps & ~ADC_SC3_AVGS_MASK)
	{
		adc->SC3 &= ~ADC_SC3_AVGE_MASK;
	}
	else
	{
		adc->SC3 |= ADC_SC3_AVGE_MASK;
		adc->SC3 = (adc->SC3 & ~ADC_SC3_AVGS_MASK) | ADC_SC3_AVGS(taps);
	}
}
ADCTaps_t ADC_GetHardwareAverage (ADC_t adc)
{
	if (adc->SC3 & ADC_SC3_AVGE_MASK)
		return ADC_t1;
	else
		return (ADCTaps_t)(adc->SC3 & ADC_SC3_AVGS_MASK);
}

bool ADC_Calibrate (ADC_t adc)
{
	int32_t  Offset		= 0;
	uint32_t Minus	[7] = {0,0,0,0,0,0,0};
	uint32_t Plus	[7] = {0,0,0,0,0,0,0};
	uint8_t  i;
	uint32_t scr3;

	/// SETUP
	adc->SC1[0] = 0x1F;
	scr3 = adc->SC3;
	// [FIX] Habilitar promediado por hardware 32x DURANTE la calibracion
	// (recomendado por NXP). Antes se usaba '&=', que en la practica DESHABILITA
	// AVGE/AVGS y degrada la calibracion.
	adc->SC3 |= (ADC_SC3_AVGS(0x03) | ADC_SC3_AVGE_MASK);

	/// INITIAL CALIBRATION
	adc->SC3 &= ~ADC_SC3_CAL_MASK;
	adc->SC3 |=  ADC_SC3_CAL_MASK;
	while (!(adc->SC1[0] & ADC_SC1_COCO_MASK));
	if (adc->SC3 & ADC_SC3_CALF_MASK)
	{
		adc->SC3 |= ADC_SC3_CALF_MASK;
		adc->SC3 = scr3;             // [FIX] restaurar SC3 antes de salir
		return false;
	}
	adc->PG  = (0x8000 | ((adc->CLP0+adc->CLP1+adc->CLP2+adc->CLP3+adc->CLP4+adc->CLPS) >> (1 + TWO_POW_NUM_OF_CAL)));
	adc->MG  = (0x8000 | ((adc->CLM0+adc->CLM1+adc->CLM2+adc->CLM3+adc->CLM4+adc->CLMS) >> (1 + TWO_POW_NUM_OF_CAL)));

	// FURTHER CALIBRATIONS
	for (i = 0; i < TWO_POW_NUM_OF_CAL; i++)
	{
		adc->SC3 &= ~ADC_SC3_CAL_MASK;
		adc->SC3 |=  ADC_SC3_CAL_MASK;
		while (!(adc->SC1[0] & ADC_SC1_COCO_MASK));
		if (adc->SC3 & ADC_SC3_CALF_MASK)
		{
			adc->SC3 |= ADC_SC3_CALF_MASK;
			adc->SC3 = scr3;         // [FIX] restaurar SC3
			return false;            // [FIX] antes devolvia 1 (=true=exito) ante un FALLO
		}
		Offset += (short)adc->OFS;
		Plus[0] += (unsigned long)adc->CLP0;
		Plus[1] += (unsigned long)adc->CLP1;
		Plus[2] += (unsigned long)adc->CLP2;
		Plus[3] += (unsigned long)adc->CLP3;
		Plus[4] += (unsigned long)adc->CLP4;
		Plus[5] += (unsigned long)adc->CLPS;
		Plus[6] += (unsigned long)adc->CLPD;
		Minus[0] += (unsigned long)adc->CLM0;
		Minus[1] += (unsigned long)adc->CLM1;
		Minus[2] += (unsigned long)adc->CLM2;
		Minus[3] += (unsigned long)adc->CLM3;
		Minus[4] += (unsigned long)adc->CLM4;
		Minus[5] += (unsigned long)adc->CLMS;
		Minus[6] += (unsigned long)adc->CLMD;
	}
	adc->OFS = (Offset >> TWO_POW_NUM_OF_CAL);
	adc->PG  = (0x8000 | ((Plus[0] +Plus[1] +Plus[2] +Plus[3] +Plus[4] +Plus[5] ) >> (1 + TWO_POW_NUM_OF_CAL)));
	adc->MG  = (0x8000 | ((Minus[0]+Minus[1]+Minus[2]+Minus[3]+Minus[4]+Minus[5]) >> (1 + TWO_POW_NUM_OF_CAL)));
	adc->CLP0 = (Plus[0] >> TWO_POW_NUM_OF_CAL);
	adc->CLP1 = (Plus[1] >> TWO_POW_NUM_OF_CAL);
	adc->CLP2 = (Plus[2] >> TWO_POW_NUM_OF_CAL);
	adc->CLP3 = (Plus[3] >> TWO_POW_NUM_OF_CAL);
	adc->CLP4 = (Plus[4] >> TWO_POW_NUM_OF_CAL);
	adc->CLPS = (Plus[5] >> TWO_POW_NUM_OF_CAL);
	adc->CLPD = (Plus[6] >> TWO_POW_NUM_OF_CAL);
	adc->CLM0 = (Minus[0] >> TWO_POW_NUM_OF_CAL);
	adc->CLM1 = (Minus[1] >> TWO_POW_NUM_OF_CAL);
	adc->CLM2 = (Minus[2] >> TWO_POW_NUM_OF_CAL);
	adc->CLM3 = (Minus[3] >> TWO_POW_NUM_OF_CAL);
	adc->CLM4 = (Minus[4] >> TWO_POW_NUM_OF_CAL);
	adc->CLMS = (Minus[5] >> TWO_POW_NUM_OF_CAL);
	adc->CLMD = (Minus[6] >> TWO_POW_NUM_OF_CAL);

	/// UN-SETUP
	adc->SC3 = scr3;

	return true;
}

void ADC_Start (ADC_t adc, ADCChannel_t channel, ADCMux_t mux)
{
	adc->CFG2 = (adc->CFG2 & ~ADC_CFG2_MUXSEL_MASK) | ADC_CFG2_MUXSEL(mux);

	if (adc == ADC0)
		adc->SC1[0] = ADC_SC1_AIEN(ADC_interrupt[0]) | ADC_SC1_ADCH(channel);
	else if (adc == ADC1)
		adc->SC1[0] = ADC_SC1_AIEN(ADC_interrupt[1]) | ADC_SC1_ADCH(channel);
}

bool ADC_IsReady (ADC_t adc)
{
	return adc->SC1[0] & ADC_SC1_COCO_MASK;
}

ADCData_t ADC_getData (ADC_t adc)
{
	return adc->R[0];
}

void ADC_ConfigRxHwTriggerDMA (ADCChannel_t channel)
{
	// Disparo por HARDWARE + peticiones de DMA.
	//   SC2[ADTRG]=1 -> conversion iniciada por trigger HW (no por escritura de SC1)
	//   SC2[DMAEN]=1 -> al completar (COCO) genera una peticion de DMA
	ADC0->SC2 = ADC_SC2_ADTRG_MASK | ADC_SC2_DMAEN_MASK;

	// Mux 'A', single-ended, single conversion por trigger
	ADC0->CFG2 &= ~ADC_CFG2_MUXSEL_MASK;          // ADHWTSA -> SC1A/R[0]
	ADC0->SC3  &= ~ADC_SC3_ADCO_MASK;             // NO continuo: 1 conv por trigger

	// Selección de canal en SC1A. AIEN=0 (no usamos IRQ del ADC; el DMA hace el trabajo).
	// Al estar en modo HW-trigger, escribir SC1A solo arma el canal; el PIT dispara.
	ADC0->SC1[0] = ADC_SC1_ADCH(channel);

	// Ruta de trigger: PIT canal 0 -> ADC0 (SIM_SOPT7).
	//   ADC0ALTTRGEN=1 habilita el trigger alternativo; ADC0TRGSEL=0b0100 = PIT trigger 0.
	SIM->SOPT7 = (SIM->SOPT7 & ~(SIM_SOPT7_ADC0TRGSEL_MASK | SIM_SOPT7_ADC0PRETRGSEL_MASK))
	           | SIM_SOPT7_ADC0ALTTRGEN_MASK
	           | SIM_SOPT7_ADC0TRGSEL(0x04);
}
