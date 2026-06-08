/***************************************************************************/ /**
   @file     App.c
   @brief    Application functions
   @author   Nicolás Magliola
  ******************************************************************************/

/*******************************************************************************
 * INCLUDE HEADER FILES
 ******************************************************************************/

// FOR INIT
#include "hal/board.h"
#include "hal/leds.h"
#include "hal/system.h"
#include "hal/timers.h"
#include "hal/accel.h"
#include <string.h>
#include "hal/USBComunications.h"
#include "hal/CANComunications.h"
#include "hardware.h"




// #define DEBUG
/*******************************************************************************
 * CONSTANT AND MACRO DEFINITIONS USING #DEFINE
 ******************************************************************************/

#define MAXCHAR 5

#define MIN_TIME_ELAPSED 50 // ms

#define MAX_TIME_ELAPSED 2000 // ms

/*******************************************************************************
 * FUNCTION PROTOTYPES FOR PRIVATE FUNCTIONS WITH FILE LEVEL SCOPE
 ******************************************************************************/
void int_to_ascii(int n, char *s);
int ascii_to_int(char * s);
/*******************************************************************************
 *******************************************************************************
                        GLOBAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/
AccelData_t   rawValues;   // Para X, Y, Z en g's
AccelAngles_t angles;      // Para roll y pitch en grados


char val_ascii[MAXCHAR];        // Buffer para conversión numérica

timer_t timerR_50ms = {.startMillis = 0, .started = false};
timer_t timerR_2000ms = {.startMillis = 0, .started = false};
timer_t timerC_50ms = {.startMillis = 0, .started = false};
timer_t timerC_2000ms = {.startMillis = 0, .started = false};



typedef struct 
{
	char pitch[MAXCHAR];
	char roll[MAXCHAR];
	char orientation[MAXCHAR];
} position_t;

position_t posGrupos[4] = {
	{{'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}},
	{{'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}},
	{{'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}},
	{{'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}, {'0','0','0','0', '\0'}}
}; // Nombre de grupo - 1

void App_Init(void) {
	Accel_Init();
	USBCom_Init();
	initTimers();
	initSystem(20);		// 20Hz == 50ms por cada interrupcion
	CANCom_Init();

	if(ledsInit(RED)) {
		return;
	}
	if(ledsInit(GREEN)) {
		return;
	}
	if(ledsInit(BLUE)) {
		return;
	}

	ledOn(LEDOFF);

	timerStart(&timerR_50ms);
	timerStart(&timerR_2000ms);
	timerStart(&timerC_50ms);
	timerStart(&timerC_2000ms);
}


void App_Run(void) {

	if (Accel_StartCapture()) 
	{
		// Mientras esto ocurre, las interrupciones llenan el buffer en background
		while (!Accel_IsDataReady()) 
		{
			uint8_t rx_led_byte;
			if (USBCom_GetByte(&rx_led_byte)) {
				// Validar estructura binaria (1JKL 0RGB)
				if ((rx_led_byte & 0x80) && !(rx_led_byte & 0x08)) {
					uint8_t target_group = (rx_led_byte >> 4) & 0x07;

					if (target_group == 4) {
						// Controlar hardware local
						uint8_t r = (rx_led_byte >> 2) & 0x01;
						uint8_t g = (rx_led_byte >> 1) & 0x01;
						uint8_t b = rx_led_byte & 0x01;

						uint8_t color_seteado = (r << 2) | (g << 1) | b;

						ledOn(color_seteado);

					}
					else {
						SendCAN(&rx_led_byte, 'L', 1);
					}
				}
			}


			if (CANReceived())
			{
				if (isMsgPosition())
				{
					char CANData[MAXCHAR];
					uint8_t group;
					char CANDataType;
					group = (USBUpdate(CANData, &CANDataType)); // En posGrupos[G-1]
					switch (CANDataType)
					{
						case 'R': // Roll
							for (int i = 0; i < MAXCHAR; i++)
							{
								posGrupos[group].roll[i] = CANData[i];
							}
							break;
						case 'C': // Pitch
							for (int i = 0; i < MAXCHAR; i++)
							{
								posGrupos[group].pitch[i] = CANData[i];
							}
							break;
						case 'O': // Orientación
							for (int i = 0; i < MAXCHAR; i++)
							{
								posGrupos[group].orientation[i] = CANData[i];
							}
							break;
						default:
							break;
					}
					switch(group)
					{
						case 0:
							USBCom_SendString(">S0,A0,V"); // Rolido: Estación 0, Ángulo 1
							USBCom_SendString(posGrupos[0].roll);
							USBCom_SendString("\n"); // Fin de trama
							USBCom_SendString(">S0,A1,V"); // Pitch: Estación 0, Ángulo 0
							USBCom_SendString(posGrupos[0].pitch);
							USBCom_SendString("\n"); // Fin de trama
							break;
						case 1:
							USBCom_SendString(">S1,A0,V"); // Rolido: Estación 1, Ángulo 1
							USBCom_SendString(posGrupos[1].roll);
							USBCom_SendString("\n"); // Fin de trama
							USBCom_SendString(">S1,A1,V"); // Pitch: Estación 1, Ángulo 0
							USBCom_SendString(posGrupos[1].pitch);
							USBCom_SendString("\n"); // Fin de trama
							break;
						case 2:
							USBCom_SendString(">S2,A0,V"); // Rolido: Estación 2, Ángulo 1
							USBCom_SendString(posGrupos[2].roll);
							USBCom_SendString("\n"); // Fin de trama
							USBCom_SendString(">S2,A1,V"); // Pitch: Estación 2, Ángulo 0
							USBCom_SendString(posGrupos[2].pitch);
							USBCom_SendString("\n"); // Fin de trama
							break;
						case 3:
							USBCom_SendString(">S3,A0,V"); // Rolido: Estación 3, Ángulo 1
							USBCom_SendString(posGrupos[3].roll);
							USBCom_SendString("\n"); // Fin de trama
							USBCom_SendString(">S3,A1,V"); // Pitch: Estación 3, Ángulo 0
							USBCom_SendString(posGrupos[3].pitch);
							USBCom_SendString("\n"); // Fin de trama
							break;
						case 5:
							USBCom_SendString(">S5,A0,V"); // Rolido: Estación 3, Ángulo 1
							USBCom_SendString(posGrupos[5].roll);
							USBCom_SendString("\n"); // Fin de trama
							USBCom_SendString(">S5,A1,V"); // Pitch: Estación 3, Ángulo 0
							USBCom_SendString(posGrupos[5].pitch);
							USBCom_SendString("\n"); // Fin de trama
							break;
						case 6:
							USBCom_SendString(">S6,A0,V"); // Rolido: Estación 3, Ángulo 1
							USBCom_SendString(posGrupos[6].roll);
							USBCom_SendString("\n"); // Fin de trama
							USBCom_SendString(">S6,A1,V"); // Pitch: Estación 3, Ángulo 0
							USBCom_SendString(posGrupos[6].pitch);
							USBCom_SendString("\n"); // Fin de trama
							break;
						default:
							break;
					}
				}
			}
		}
	}



	Accel_GetProcessedData(&rawValues);
	Accel_CalculateAngles(&rawValues, &angles);



	bool timerR_50ms_finished = timerCheck(&timerR_50ms) >= MIN_TIME_ELAPSED / 50;
	bool timerR_2000ms_finished = timerCheck(&timerR_2000ms) >= MAX_TIME_ELAPSED / 50;
	bool timerC_50ms_finished = timerCheck(&timerC_50ms) >= MIN_TIME_ELAPSED / 50;
	bool timerC_2000ms_finished = timerCheck(&timerC_2000ms) >= MAX_TIME_ELAPSED / 50;


	if ( (((angles.roll > (ascii_to_int(posGrupos[4].roll) + 5) || angles.roll < (ascii_to_int(posGrupos[4].roll) - 5)) && timerR_50ms_finished) || timerR_2000ms_finished) )
	{
		timerReset(&timerR_50ms);
		timerReset(&timerR_2000ms);

		USBCom_SendString(">S4,A0,V"); // Rolido: Estación 4, Ángulo 0
		int_to_ascii((int)angles.roll, posGrupos[4].roll);
		USBCom_SendString(posGrupos[4].roll);
		USBCom_SendString("\n"); // Fin de trama
		SendCAN(posGrupos[4].roll, 'R', strlen(posGrupos[4].roll));
	}
	if ( (((angles.pitch > (ascii_to_int(posGrupos[4].pitch) + 5) || angles.pitch < (ascii_to_int(posGrupos[4].pitch) - 5)) && timerC_50ms_finished) || timerC_2000ms_finished) ) // falta chequear el tiempo
	{
		timerReset(&timerC_50ms);
		timerReset(&timerC_2000ms);

		USBCom_SendString(">S4,A1,V"); // Pitch: Estación 4, Ángulo 1
		int_to_ascii((int)angles.pitch, posGrupos[4].pitch);
		USBCom_SendString(posGrupos[4].pitch);
		USBCom_SendString("\n"); // Fin de trama
		SendCAN(posGrupos[4].pitch, 'C', strlen(posGrupos[4].pitch));
	}
}


/*******************************************************************************
 *******************************************************************************
                        LOCAL FUNCTION DEFINITIONS
 *******************************************************************************
 ******************************************************************************/

void int_to_ascii(int n, char *s) {
    int i = 0, sign;

    if ((sign = n) < 0) n = -n; // Guardar signo y hacer positivo

    // Extraer dígitos en orden inverso
    do {
        s[i++] = n % 10 + '0';
    } while ((n /= 10) > 0);

    if (sign < 0) s[i++] = '-';
    s[i] = '\0';

    // Invertir el string (porque los dígitos se sacaron al revés)
    for (int j = 0, k = i-1; j < k; j++, k--) {
        char temp = s[j];
        s[j] = s[k];
        s[k] = temp;
    }
}


int ascii_to_int(char * s) 
{
	int result = 1;
	uint32_t aux = 0;
	uint32_t j = 1;
	uint32_t i = 0; 
	if ( *(s) == '-')
	{
		result *= -1;
		++s;
	}
	while (s[i+1] != '\0')
	{
	  j=j*10;
	  i++;
	}
	while (*s != '\0')
	{
		aux += (int)((*s) - '0')*j;
		j /= 10;
		s++;
	}
	return result*aux;
}
/*******************************************************************************
 ******************************************************************************/

