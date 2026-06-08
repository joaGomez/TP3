#include "accel.h"
#include "hardware.h"
#include "board.h"

#include "mcal/i2c.h"

#include "math.h"

#define PI 3.1415926535f

// Sensibilidad de lectura de los datos del acelerómetro
#define SENS_2G				0x00
#define SENS_4G				0x01
#define SENS_8G				0x02


static uint8_t g_accel_raw_buffer[6];

void Accel_Init(void) {
    I2C_Init();

    // Sensor en Standby para poder configurar registros
    I2C_WriteByte(ACCEL_ADDR, REG_CTRL_REG1, 0x00);


    I2C_WriteByte(ACCEL_ADDR, REG_XYZ_DATA_CFG, SENS_8G);

    // Activar el sensor (Modo Active, ODR 800Hz)
    I2C_WriteByte(ACCEL_ADDR, REG_CTRL_REG1, 0x01);
}

/**
 * @brief Inicia la captura de datos (No bloqueante)
 * Devuelve true si pudo empezar, false si el bus estaba ocupado
 */
bool Accel_StartCapture(void) {
    return I2C_ReadRegisters(ACCEL_ADDR, REG_OUT_X_MSB, g_accel_raw_buffer, 6);
}

/**
 * @brief Indica si la lectura actual ha terminado
 */
bool Accel_IsDataReady(void) {
    return !I2C_IsBusy();
}

/**
 * @brief Procesa los datos crudos del buffer y los guarda en la estructura.
 * Solo debe llamarse cuando Accel_IsDataReady() retorne true.
 */
void Accel_GetProcessedData(AccelData_t *data) {
    // Procesamos el formato de 14 bits (Big Endian)
    // El desplazamiento >> 2 es porque el sensor FXOS8700 tiene 14 bits alineados a la izquierda
    data->x = (int16_t)((g_accel_raw_buffer[0] << 8) | g_accel_raw_buffer[1]) >> 2;
    data->y = (int16_t)((g_accel_raw_buffer[2] << 8) | g_accel_raw_buffer[3]) >> 2;
    data->z = (int16_t)((g_accel_raw_buffer[4] << 8) | g_accel_raw_buffer[5]) >> 2;
}

void Accel_CalculateAngles(AccelData_t *raw_data, AccelAngles_t *angles) {
    // Convertimos los ejes a float para la matemática
    float ax = (float)raw_data->x;
    float ay = (float)raw_data->y;
    float az = (float)raw_data->z;

    angles->pitch = atan2f(ay, az) * (-180.0f / PI);

    float denominator = sqrtf(ay * ay + az * az);
    angles->roll = atan2f(-ax, denominator) * (-180.0f / PI);
}
