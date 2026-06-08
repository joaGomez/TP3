/***************************************************************************//**
  @file     board.h
  @brief    Board management
  @author   Nicolás Magliola
 ******************************************************************************/

#ifndef _BOARD_H_
#define _BOARD_H_

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/
#include <stdbool.h>
#include <stdint.h>

#include "mcal/gpio.h"

/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

/***** BOARD defines **********************************************************/

// On Board User LEDs
#define PIN_LED_RED     PORTNUM2PIN(PB,22)// ???
#define PIN_LED_GREEN   PORTNUM2PIN(PE,26)// ???
#define PIN_LED_BLUE    PORTNUM2PIN(PB,21) // PTB21

#define LED_ACTIVE      LOW


// On Board User Switches
#define PIN_SW2         PORTNUM2PIN(PC,6)	// PTC6
#define PIN_SW3         PORTNUM2PIN(PA,4) // PTA4

#define SW_ACTIVE       LOW // 
#define SW_INPUT_TYPE   BOOL // 


/**** Encoder defines **********************************************************/
#define PIN_RCHA		PORTNUM2PIN(PC,3)
#define PIN_RCHB		PORTNUM2PIN(PC,2)
#define PIN_RCHD		PORTNUM2PIN(PA,2)


/***** CARD defines **********************************************************/

#define PIN_CARD_CLOCK    PORTNUM2PIN(PC, 5)   // Setear pinout de la placa
#define PIN_CARD_DATA     PORTNUM2PIN(PC, 1)  // Setear pinout de la placa
#define PIN_ENABLE_DATA   PORTNUM2PIN(PC, 0)  // Setear pinout de la placa

/***** SHIFT defines **********************************************************/
#define PIN_SHIFT PORTNUM2PIN(PB, 23) // DATA - SHIFT REGISTER OUTPUT
#define PIN_CLK PORTNUM2PIN(PB, 9)    // serial clock
#define PIN_SETOUT PORTNUM2PIN(PA, 1) // latch clock


#define PIN_INTERRUPTION_FLAG PORTNUM2PIN(PB,18)

/****** ACCELEROMETER defines **************************************************/
#define ACCEL_ADDR          0x1D 	// Dirección I2C del sensor en la FRDM-K64F

// FXOS8700CQ REGISTERS
#define REG_STATUS          0x00
#define REG_OUT_X_MSB       0x01
#define REG_WHO_AM_I        0x0D
#define REG_XYZ_DATA_CFG    0x0E
#define REG_CTRL_REG1       0x2A


/*******************************************************************************
 ******************************************************************************/

#endif // _BOARD_H_
