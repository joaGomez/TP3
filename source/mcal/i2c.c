#include "i2c.h"
#include "hardware.h"
#include "MK64F12.h"
#include "gpio.h"

#define PIN_I2C0_SDA    PORTNUM2PIN(PE, 25)
#define PIN_I2C0_SCL    PORTNUM2PIN(PE, 24)



typedef enum {
    I2C_IDLE,
	I2C_WRITING_DATA,
    I2C_WRITING_REG_ADDR,
    I2C_RESTARTING,
    I2C_DUMMY_READ,
    I2C_READING_DATA
} i2c_state_t;

static volatile i2c_state_t g_state = I2C_IDLE;
static volatile uint8_t g_dataToWrite;
static volatile uint8_t g_slaveAddr;
static volatile uint8_t g_regAddr;
static volatile uint8_t *g_buffer;
static volatile uint8_t g_length;
static volatile uint8_t g_index;
static volatile bool g_busy = false;

static void I2C_Wait_Blocking(void) {
    uint32_t timeout = 100000;
    while (!(I2C0->S & I2C_S_IICIF_MASK) && timeout > 0) timeout--;
    I2C0->S |= I2C_S_IICIF_MASK; 	// Limpiar flag
}

// Esta es la función que usa tu Accel_Init
void I2C_WriteByte(uint8_t slaveAddr, uint8_t regAddr, uint8_t data) {
    g_busy = true; // Bloqueo g_busy

    I2C0->C1 |= I2C_C1_MST_MASK | I2C_C1_TX_MASK; // START
    I2C0->D = (slaveAddr << 1);
    I2C_Wait_Blocking();

    I2C0->D = regAddr;
    I2C_Wait_Blocking();

    I2C0->D = data;
    I2C_Wait_Blocking();

    I2C0->C1 &= ~I2C_C1_MST_MASK; // STOP
    I2C0->C1 &= ~I2C_C1_TX_MASK;

    g_busy = false;		// Actualizo

    // Delay para el sensor
    for(volatile int i = 0; i < 2000; i++);
}




void I2C_Init(void) {

	gpioInit(PIN_I2C0_SDA, PORT_mAlt5);
	gpioInit(PIN_I2C0_SCL, PORT_mAlt5);

    SIM->SCGC4 |= SIM_SCGC4_I2C0_MASK;
    I2C0->F = 0x1F; 						// Frecuencia de I2C

    I2C0->C1 = I2C_C1_IICEN_MASK | I2C_C1_IICIE_MASK;	// Módulo I2C + Interrupciones

    NVIC_EnableIRQ(I2C0_IRQn);
}


bool I2C_ReadRegisters(uint8_t slaveAddr, uint8_t startReg, uint8_t *buffer, uint8_t length) {
    if (g_busy) return false; // El bus está ocupado con otra tarea

    // Actualizo las variables globales del protocolo
    g_slaveAddr = slaveAddr;
    g_regAddr = startReg;
    g_buffer = buffer;
    g_length = length;
    g_index = 0;
    g_busy = true;
    g_state = I2C_WRITING_REG_ADDR;

    // Inicio bus
    I2C0->C1 |= I2C_C1_MST_MASK | I2C_C1_TX_MASK;
    I2C0->D = (slaveAddr << 1); 	// Envía Dirección + W

    return true;
}

bool I2C_IsBusy(void) {
    return g_busy;
}

void I2C0_IRQHandler(void) {
    I2C0->S |= I2C_S_IICIF_MASK;		// Limpio el flag

    switch (g_state) {
        case I2C_WRITING_REG_ADDR:
            I2C0->D = g_regAddr;           // Enviar dirección del registro
            g_state = I2C_RESTARTING;
            break;

        case I2C_RESTARTING:
            I2C0->C1 |= I2C_C1_RSTA_MASK;  				// Repeated Start
            I2C0->D = (g_slaveAddr << 1) | 1; 			// Dirección + Read
            g_state = I2C_DUMMY_READ;
            break;

        case I2C_DUMMY_READ:
            I2C0->C1 &= ~I2C_C1_TX_MASK;   				// Cambiar a modo Recepción

            // Si solo pedimos 1 byte, preparar NACK ya mismo
            if (g_length == 1) I2C0->C1 |= I2C_C1_TXAK_MASK;
            else I2C0->C1 &= ~I2C_C1_TXAK_MASK;

            volatile uint8_t dummy = I2C0->D; // Trigger del primer byte real
            g_state = I2C_READING_DATA;
            break;

        case I2C_READING_DATA:
            // Penúltimo byte
            if (g_index == g_length - 2) {
                I2C0->C1 |= I2C_C1_TXAK_MASK; // Próximo byte será NACK
            }

            // Último byte
            if (g_index == g_length - 1) {
                I2C0->C1 &= ~I2C_C1_MST_MASK; // STOP
                I2C0->C1 &= ~I2C_C1_TX_MASK;
                g_buffer[g_index] = I2C0->D;  // Guardar último dato
                g_busy = false;               // Liberar driver
                g_state = I2C_IDLE;
            } else {
                g_buffer[g_index++] = I2C0->D; // Guardar y disparar siguiente reloj
            }
            break;

        default:
            g_busy = false;
            break;
    }
}
