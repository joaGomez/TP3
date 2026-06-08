#ifndef ACCEL_H
#define ACCEL_H

#include <stdint.h>
#include <stdbool.h>


typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} AccelData_t;

typedef struct {
    float roll;
    float pitch;
} AccelAngles_t;


void Accel_Init(void);
bool Accel_StartCapture(void);
bool Accel_IsDataReady(void);
void Accel_GetProcessedData(AccelData_t *data);
void Accel_CalculateAngles(AccelData_t *raw_data, AccelAngles_t *angles);

#endif	// ACCEL_H
