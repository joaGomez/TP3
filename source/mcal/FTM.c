/***************************************************************************//**
  @file     FTM.c
  @brief    Implementación del Driver FlexTimer (FTM) para K64F
 ******************************************************************************/

#include "FTM.h"
#include "MK64F12.h"  // Archivo de registro del microcontrolador K64F

/*******************************************************************************
 * CONFIGURACIONES INTERNAS Y CONSTANTES
 ******************************************************************************/

#define FTM_CLOCK_HZ        50000000U // Frecuencia del reloj del bus (50 MHz)
#define FTM_PRESCALER_DIV   1         // Divisor del prescaler (PS = 0 -> /1)

/* Cálculos de MOD para generar las frecuencias deseadas mediante Toggle (Output Compare)
 * Nota: En modo Output Compare con Toggle, la señal cambia de estado cada vez que el contador
 * alcanza el valor de MOD. Por ende, el período total de la señal es 2 * MOD.
 * Frecuencia_onda = FTM_CLOCK / (2 * MOD)  =>  MOD = FTM_CLOCK / (2 * Frecuencia_onda)
 */
#define MOD_VAL_1200HZ   ((FTM_CLOCK_HZ / (2 * FSK_FREQ_MARK * FTM_PRESCALER_DIV)) - 1)
#define MOD_VAL_2200HZ   ((FTM_CLOCK_HZ / (2 * FSK_FREQ_SPACE * FTM_PRESCALER_DIV)) - 1)

// Punteros globales para almacenar los callbacks de interrupción
static ftm_callback_t tx_callback = 0;
static ftm_callback_t rx_callback = 0;


/*******************************************************************************
 * IMPLEMENTACIÓN DE FUNCIONES
 ******************************************************************************/

void FTM_TX_Init(ftm_module_t module, ftm_channel_t channel, ftm_callback_t callback)
{
	FTM_Type *ftm = FTM0;
	tx_callback = callback;

	/* 1. Habilitar el Clock Gating para FTM0 */
	SIM->SCGC6 |= SIM_SCGC6_FTM0_MASK;
    /* 2. Configuración inicial del FTM */
    ftm->MODE |= FTM_MODE_WPDIS_MASK; // Deshabilitar protección contra escritura para configurar
    ftm->CNTIN = 0;                   // El contador arranca en cero

    /* 3. Configurar para que arranque en un estado inicial (Mark - 1200 Hz) */
    ftm->MOD = MOD_VAL_1200HZ;

    /* 4. Configurar el Canal en modo Output Compare - Toggle on Match */
    // MSnB:MSnA = 0:1 , ELSnB:ELSnA = 0:1  --> Output Compare, Toggle output en match
    ftm->CONTROLS[channel].CnSC = FTM_CnSC_MSA_MASK | FTM_CnSC_ELSA_MASK;

    /* 5. Habilitar la interrupción por desborde del Timer (TOF) para sincronismo de bits */
    ftm->SC = FTM_SC_TOIE_MASK | FTM_SC_PS(0); // Prescaler = 1 (PS=0)

    /* 6. Seleccionar Clock Source para encender el contador (System Clock) */
    ftm->SC |= FTM_SC_CLKS(1);

    /* 7. Habilitar interrupciones en el NVIC */
    if(module == FTM_MODULE_0)      NVIC_EnableIRQ(FTM0_IRQn);
    else if(module == FTM_MODULE_3) NVIC_EnableIRQ(FTM3_IRQn);
}

void FTM_TX_SetFrequency(ftm_module_t module, uint32_t frequency)
{
    FTM_Type *ftm = FTM0;

    // Al escribir sobre MOD, gracias al buffering nativo del FTM,
    // el cambio se efectivizará en el próximo desborde garantizando fase continua.
    if (frequency == FSK_FREQ_MARK) {
        ftm->MOD = MOD_VAL_1200HZ;
    } else if (frequency == FSK_FREQ_SPACE) {
        ftm->MOD = MOD_VAL_2200HZ;
    }
}

void FTM_RX_Init(ftm_module_t module, ftm_channel_t channel, ftm_callback_t callback)
{
    FTM_Type *ftm = FTM0;
    rx_callback = callback;

    /* 1. Habilitar Clock Gating */
    if(module == FTM_MODULE_1)      SIM->SCGC6 |= SIM_SCGC6_FTM1_MASK;
    else if(module == FTM_MODULE_2) SIM->SCGC6 |= SIM_SCGC6_FTM2_MASK;

    ftm->MODE |= FTM_MODE_WPDIS_MASK; // Deshabilitar protección contra escritura
    ftm->CNTIN = 0;
    ftm->MOD = 0xFFFF;                // Dejar que cuente libremente hasta el máximo (Free Running)

    /* 2. Configurar Canal en modo Input Capture */
    // MSnB:MSnA = 0:0, ELSnB:ELSnA = 0:1 (Captura en flanco ascendente) u 1:1 (Cualquier flanco)
    // Para medir semiciclos o ciclos enteros elegimos capturar en cualquier flanco (Either Edge)
    ftm->CONTROLS[channel].CnSC = FTM_CnSC_ELSA_MASK | FTM_CnSC_ELSB_MASK | FTM_CnSC_CHIE_MASK;

    /* 3. Encender el contador (System clock, Prescaler /1) */
    ftm->SC = FTM_SC_CLKS(1) | FTM_SC_PS(0);

    /* 4. Ruteo interno: Conectar la salida del CMP al canal FTM (Esto se hace vía SIM_SOPT4) */
    // Por ejemplo, si usamos FTM1 Canal 0, mapear la salida de CMP0 a FTM1_CH0
    if (module == FTM_MODULE_1 && channel == FTM_CHANNEL_0) {
        SIM->SOPT4 |= SIM_SOPT4_FTM1CH0SRC(1); // 1 selecciona la salida del comparador analógico (CMP0)
    }

    /* 5. Habilitar interrupciones en el NVIC */
    if(module == FTM_MODULE_1)      NVIC_EnableIRQ(FTM1_IRQn);
    else if(module == FTM_MODULE_2) NVIC_EnableIRQ(FTM2_IRQn);
}

uint16_t FTM_RX_GetCaptureValue(ftm_module_t module, ftm_channel_t channel)
{
    return (uint16_t)(FTM0->CONTROLS[channel].CnV);
}

void FTM_ClearInterruptFlag(ftm_module_t module, ftm_channel_t channel)
{
    // Para borrar la bandera de canal CHF hay que leer CnSC y escribir un 0 en el bit CHF
    FTM0->CONTROLS[channel].CnSC &= ~FTM_CnSC_CHF_MASK;
}

/*******************************************************************************
 * INTERRUPCIONES (ISRs)
 ******************************************************************************/

// ISR para FTM0 (Asumido TX en este ejemplo)
void FTM0_IRQHandler(void)
{
    // Si fue por desborde de Timer (TOF)
    if (FTM0->SC & FTM_SC_TOF_MASK) {
        FTM0->SC &= ~FTM_SC_TOF_MASK; // Limpiar bandera TOF
        if (tx_callback) {
            tx_callback(); // Invoca la lógica de la capa HAL para cambiar bits si corresponde
        }
    }
}

// ISR para FTM1 (Asumido RX en este ejemplo)
void FTM1_IRQHandler(void)
{
    // Verificamos si la interrupción fue generada por el canal 0 (Input Capture)
    if (FTM1->CONTROLS[FTM_CHANNEL_0].CnSC & FTM_CnSC_CHF_MASK) {
        if (rx_callback) {
            rx_callback(); // Invoca la lógica de demodulación
        }
        // Limpiamos la bandera del canal
        FTM1->CONTROLS[FTM_CHANNEL_0].CnSC &= ~FTM_CnSC_CHF_MASK;
    }
}
