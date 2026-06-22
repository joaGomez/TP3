/***************************************************************************//**
  @file     fsk_config.h
  @brief    Seleccion de version del modem, COMPARTIDA por todas las unidades
            de compilacion.

  ANTES: USE_VERSION_1/2 se definia solo en App.c, por lo que los #ifdef de
  timers.c (y de cualquier otro .c) quedaban siempre en falso -> sus ISRs no
  se compilaban y la V2 no funcionaba. Esta cabecera centraliza el switch para
  que TODOS los .c vean la misma definicion.

  Descomentar UNA sola de las dos lineas.
 ******************************************************************************/
#ifndef HAL_FSK_CONFIG_H_
#define HAL_FSK_CONFIG_H_

#define USE_VERSION_2   /* V2: PIT + CMP + FTM (input capture / output compare) */
//#define USE_VERSION_1      /* V1: ADC + DAC + DMA                                  */

#if defined(USE_VERSION_1) && defined(USE_VERSION_2)
#  error "Definir solo UNA version (USE_VERSION_1 o USE_VERSION_2)"
#endif
#if !defined(USE_VERSION_1) && !defined(USE_VERSION_2)
#  error "Definir al menos UNA version (USE_VERSION_1 o USE_VERSION_2)"
#endif

#endif /* HAL_FSK_CONFIG_H_ */
