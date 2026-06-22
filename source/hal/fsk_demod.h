/***************************************************************************//**
  @file     fsk_demod.h
  @brief    Demodulador FSK Bell 202 por software (cadena de streaming, V1).
            Validado en host (BER=0 hasta ~6 dB SNR, ver design_fir.py/ber.py).
 ******************************************************************************/
#ifndef HAL_FSK_DEMOD_H_
#define HAL_FSK_DEMOD_H_

#include <stdint.h>

#define FSK_DEMOD_FS   24000U   /* frecuencia de muestreo del ADC esperada    */

typedef void (*fsk_demod_byte_cb_t)(uint8_t byte);

/** Inicializa el estado del demodulador. cb se llama por cada byte 8O1 valido. */
void fsk_demod_init(fsk_demod_byte_cb_t cb);

/** Procesa UNA muestra del ADC (codigo de 12 bits, 0..4095, centro 2048). */
void fsk_demod_process_sample(uint16_t adc_code);

#endif /* HAL_FSK_DEMOD_H_ */
